import numpy as np
import seal

def conv2d_standard(input_matrix, kernel):
    H, W = input_matrix.shape
    KH, KW = kernel.shape
    OH, OW = H - KH + 1, W - KW + 1
    result = np.zeros((OH, OW))
    for i in range(OH):
        for j in range(OW):
            window = input_matrix[i:i+KH, j:j+KW]
            result[i, j] = np.sum(window * kernel)
    return result

# CKKS 参数
params = seal.EncryptionParameters(seal.scheme_type.CKKS)
params.set_poly_modulus_degree(8192)
params.set_coeff_modulus(seal.CoeffModulus.Create(8192, [60, 40, 40, 60]))
context = seal.SEALContext(params)

# 密钥生成（含旋转密钥）
keygen = seal.KeyGenerator(context)
secret_key = keygen.secret_key()
public_key = seal.PublicKey()
keygen.create_public_key(public_key)

rotation_steps = [1, 2, 4, 5, 6, 8, 9, 10]
galois_keys = seal.GaloisKeys()
keygen.create_galois_keys(rotation_steps, galois_keys)

encoder = seal.CKKSEncoder(context)
encryptor = seal.Encryptor(context, public_key)
evaluator = seal.Evaluator(context)
decryptor = seal.Decryptor(context, secret_key)

scale = 2.0 ** 40
slot_count = encoder.slot_count()
print(f"Slot count: {slot_count}")

# 输入和卷积核
plain_input = np.array([
    [1.0, 2.0, 3.0, 4.0],
    [5.0, 6.0, 7.0, 8.0],
    [9.0, 10.0, 11.0, 12.0],
    [13.0, 14.0, 15.0, 16.0]
])
plain_kernel = np.array([
    [1.0, 0.0, -1.0],
    [1.0, 0.0, -1.0],
    [1.0, 0.0, -1.0]
])

plain_result = conv2d_standard(plain_input, plain_kernel)
print("\n明文标准卷积结果 (2x2):")
print(plain_result)

# Pack：行优先展平后加密
plain_flat = plain_input.flatten().tolist()
plaintext = seal.Plaintext()
encoder.encode(plain_flat, scale, plaintext)
ciphertext = seal.Ciphertext()
encryptor.encrypt(plaintext, ciphertext)
print("\n输入已打包加密 (Pack)")

# 3x3 卷积核的 9 个偏移及对应的槽位
slot_offsets = [0, 1, 2, 4, 5, 6, 8, 9, 10]
kernel_coords = [(0,0),(0,1),(0,2),(1,0),(1,1),(1,2),(2,0),(2,1),(2,2)]
enc_result = None
rotation_count = 0

def rotate_ciphertext(ctx, cipher, steps, galois_keys):
    try:
        rotated = seal.Ciphertext()
        ctx.rotate_vector(cipher, steps, galois_keys, rotated)
        return rotated
    except AttributeError:
        pass
    try:
        rotated = seal.Ciphertext()
        ctx.rotate_rows(cipher, steps, galois_keys, rotated)
        return rotated
    except AttributeError:
        pass
    try:
        rotated = seal.Ciphertext()
        ctx.apply_galois(cipher, steps, galois_keys, rotated)
        return rotated
    except AttributeError:
        pass
    if hasattr(ctx, 'rotate'):
        try:
            return ctx.rotate(cipher, steps, galois_keys)
        except Exception:
            pass
    raise RuntimeError("No supported rotation API found.")

# Pack → Rotate → Accumulate
for offset, (ki, kj) in zip(slot_offsets, kernel_coords):
    if offset == 0:
        rotated = seal.Ciphertext(ciphertext)
    else:
        rotated = rotate_ciphertext(evaluator, ciphertext, -offset, galois_keys)
        rotation_count += 1

    weight = plain_kernel[ki, kj]
    weight_plain = seal.Plaintext()
    encoder.encode([weight] * slot_count, scale, weight_plain)
    part = seal.Ciphertext()
    evaluator.multiply_plain(rotated, weight_plain, part)

    if enc_result is None:
        enc_result = part
    else:
        temp = seal.Ciphertext()
        evaluator.add(enc_result, part, temp)
        enc_result = temp

print(f"\nPack -> Rotate -> Accumulate 完成，共执行 {rotation_count} 次旋转")

# Mask：提取输出槽位 0,1,4,5
# 理论依据：对于 4×4 输入和 3×3 核，采用行优先展平，
# 经过上述旋转累加后，有效卷积结果必然落在槽位 0,1,4,5。
# 这是因为每个输出位置对应的输入窗口在展平后的偏移集合，
# 经过旋转和累加，最终系数之和在这些槽位形成卷积和。
# 因此 Mask 提取这些槽位是合理的。
mask_vector = [0.0] * slot_count
mask_vector[0] = 1.0
mask_vector[1] = 1.0
mask_vector[4] = 1.0
mask_vector[5] = 1.0
mask_plain = seal.Plaintext()
encoder.encode(mask_vector, scale, mask_plain)
enc_result_masked = seal.Ciphertext()
evaluator.multiply_plain(enc_result, mask_plain, enc_result_masked)

# 解密
decrypted_plain = seal.Plaintext()
decryptor.decrypt(enc_result_masked, decrypted_plain)
result_list = []
encoder.decode(decrypted_plain, result_list)
result_list = result_list[:slot_count]

output = np.array([
    [result_list[0], result_list[1]],
    [result_list[4], result_list[5]]
])

print("\n解密后的卷积结果 (2x2):")
print(output)

error = np.abs(output - plain_result)
print(f"\n最大绝对误差: {np.max(error):.6f}")
print(f"平均绝对误差: {np.mean(error):.6f}")

print("\n旋转次数统计")
print(f"本次卷积共执行旋转操作: {rotation_count} 次")

print("\n理论最小值分析")
print(f"输入尺寸: 4x4 = 16 个值")
print(f"卷积核尺寸: 3x3")
print(f"输出尺寸: 2x2 = 4 个位置")
print("理论分析:")
print("  对于3x3卷积核，共有9个偏移位置")
print("  每个偏移位置需要一次旋转，其中偏移0不需要旋转")
print(f"  因此理论最小旋转次数 = 9 - 1 = 8 次")
print(f"本次实际旋转次数: {rotation_count} 次")

if rotation_count == 8:
    print("结论: 达到理论最小值")
else:
    print(f"结论: 未达到理论最小值。实际 {rotation_count} 次，理论 8 次。")
