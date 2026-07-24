"""
正确使用 ECDSA - 验证原始消息
"""

from ecdsa import SECP256k1, SigningKey, VerifyingKey
import hashlib

def main():
    print("="*60)
    print("  正确使用 ECDSA 验证")
    print("="*60)
    
    # 生成密钥
    sk = SigningKey.generate(curve=SECP256k1)
    vk = sk.get_verifying_key()
    
    # 原始消息
    message = b"Hello, Bitcoin! This is a real transaction."
    print(f"\n原始消息: {message}")
    
    # 签名（自动哈希）
    signature = sk.sign(message)
    print(f"签名 (DER): {signature.hex()[:60]}...")
    
    # 正确验证
    try:
        is_valid = vk.verify(signature, message)
        print(f"\n验证通过: {is_valid}")
        print("   (因为使用了正确的原始消息)")
    except Exception as e:
        print(f"\n验证失败: {e}")
    
    # 尝试篡改消息
    tampered = b"Hello, Bitcoin! This is a FAKE transaction."
    print(f"\n篡改消息: {tampered}")
    
    try:
        is_valid = vk.verify(signature, tampered)
        print(f"\n异常：篡改消息居然通过验证？")
    except Exception as e:
        print(f"\n正确：篡改消息验证失败")
        print(f"   原因: {e}")
    
    # 演示只验哈希的危险
    print("\n" + "="*60)
    print("  危险用法：只验证哈希")
    print("="*60)
    
    msg_hash = hashlib.sha256(message).digest()
    print(f"\n消息哈希: {msg_hash.hex()}")
    
    try:
        # 只验哈希的接口
        vk.verify_digest(signature, msg_hash)
        print("\n通过验证 (但这是危险的！)")
        print("   如果攻击者伪造哈希值，这个接口无法发现")
    except Exception as e:
        print(f"\n验证失败: {e}")

if __name__ == "__main__":
    main()