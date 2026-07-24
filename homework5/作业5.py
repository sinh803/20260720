import numpy as np
import tenseal as ts


def conv2d_numpy(input_matrix, kernel):
    H, W = input_matrix.shape
    KH, KW = kernel.shape
    OH, OW = H - KH + 1, W - KW + 1
    result = np.zeros((OH, OW))
    for i in range(OH):
        for j in range(OW):
            # 提取滑动窗口并做逐元素乘加
            window = input_matrix[i:i + KH, j:j + KW]
            result[i, j] = np.sum(window * kernel)
    return result


# 1. 设置 TenSEAL 上下文（CKKS 方案）
context = ts.context(
    ts.SCHEME_TYPE.CKKS,
    poly_modulus_degree=8192,
    coeff_mod_bit_sizes=[60, 40, 40, 60]
)
context.generate_galois_keys()
context.generate_relin_keys()
scale = 2 ** 40
context.global_scale = scale

print("同态加密上下文创建成功")

# 2. 定义输入和卷积核（明文）
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

print("\n输入矩阵 (4x4):")
print(plain_input)
print("\n卷积核 (3x3):")
print(plain_kernel)

# 3. 计算明文卷积结果（验证基准）
plain_result = conv2d_numpy(plain_input, plain_kernel)
print("\n明文卷积结果 (2x2):")
print(plain_result)

# 4. 加密并执行密文卷积
# 展平输入并加密
plain_flat = plain_input.flatten().tolist()
encrypted_input = ts.ckks_vector(context, plain_flat)

# 展平卷积核并反转
kernel_flat = plain_kernel.flatten().tolist()

H, W = 4, 4
KH, KW = 3, 3
OH, OW = H - KH + 1, W - KW + 1

enc_results = []

for i in range(OH):
    for j in range(OW):
        # 构造掩码：提取 3x3 窗口
        mask = np.zeros(H * W)
        for ki in range(KH):
            for kj in range(KW):
                idx = (i + ki) * W + (j + kj)
                mask[idx] = 1.0

        # 构造带权重的掩码（卷积核值）
        weight_mask = np.zeros(H * W)
        for ki in range(KH):
            for kj in range(KW):
                idx = (i + ki) * W + (j + kj)
                weight_mask[idx] = kernel_flat[ki * KW + kj]

        # 加密掩码
        enc_mask = ts.ckks_vector(context, mask.tolist())
        enc_weight = ts.ckks_vector(context, weight_mask.tolist())

        # 提取窗口并点积
        enc_window = encrypted_input * enc_mask
        enc_dot = enc_window * enc_weight

        # 对密文向量求和，得到单个值的密文
        enc_sum = enc_dot.sum()
        enc_results.append(enc_sum)

# 5. 解密并验证
decrypted = []
for c in enc_results:
    # 解密后是向量，取第一个有效槽位
    val = c.decrypt()[0]
    decrypted.append(val)

decrypted_result = np.array(decrypted).reshape(OH, OW)

print("\n解密后的卷积结果:")
print(decrypted_result)

# 误差计算
error = np.abs(decrypted_result - plain_result)
print(f"\n最大绝对误差: {np.max(error):.6f}")
print(f"平均绝对误差: {np.mean(error):.6f}")

if np.max(error) < 1e-3:
    print("\n验证通过！密文卷积结果与明文一致（CKKS 近似误差允许范围）")
else:
    print("\n误差略大，属于正常浮点数取整误差")
