"""
ECDSA 签名伪造攻击演示
原理：利用验证时只检查哈希值，不检查原始消息的漏洞
"""

from ecdsa import SECP256k1, VerifyingKey, SigningKey
from ecdsa.util import sigencode_der, sigdecode_der
import hashlib
import random
import sys

def print_separator(title=""):
    """打印分隔线"""
    print("\n" + "="*60)
    if title:
        print(f"  {title}")
        print("="*60)

def int_to_bytes_32(num):
    """将整数转为 32 字节"""
    return num.to_bytes(32, byteorder='big')

def main():
    print_separator("ECDSA 签名伪造攻击实验")
    
    # ========== 步骤1: 生成合法密钥 ==========
    print_separator("步骤1: 生成合法的 ECDSA 密钥对")
    
    sk = SigningKey.generate(curve=SECP256k1)
    vk = sk.get_verifying_key()
    
    d = sk.to_string()
    P = vk.to_string()
    
    print(f"私钥 d (hex): {d.hex()}")
    print(f"公钥 P (hex): {P.hex()}")
    print(f"私钥长度: {len(d)} 字节")
    print(f"公钥长度: {len(P)} 字节")
    
    # ========== 步骤2: 攻击者选择随机数 ==========
    print_separator("步骤2: 攻击者随机选择 u, v")
    
    n = SECP256k1.order
    print(f"曲线阶 n = {n}")
    print(f"n 的位长: {n.bit_length()} 位")
    
    u = random.randint(1, n-1)
    v = random.randint(1, n-1)
    
    print(f"随机选择 u = {u}")
    print(f"随机选择 v = {v}")
    
    # ========== 步骤3: 计算 R' = uG + vP ==========
    print_separator("步骤3: 计算 R' = u*G + v*P")
    
    G = SECP256k1.generator
    P_point = vk.pubkey.point
    
    print(f"生成元 G: ({G.x()}, {G.y()})")
    print(f"公钥点 P: ({P_point.x()}, {P_point.y()})")
    
    # 计算椭圆曲线点乘法
    R = u * G + v * P_point
    r = R.x() % n
    
    print(f"u*G + v*P 结果点 R': ({R.x()}, {R.y()})")
    print(f"r' = R'.x mod n = {r}")
    
    # ========== 步骤4: 计算伪造签名 ==========
    print_separator("步骤4: 构造伪造签名 (r', s')")
    
    # 计算 v 的模逆
    v_inv = pow(v, -1, n)
    s_fake = (r * v_inv) % n
    e_fake = (r * u * v_inv) % n
    
    print(f"v^-1 mod n = {v_inv}")
    print(f"s' = r' * v^-1 mod n = {s_fake}")
    print(f"e' = r' * u * v^-1 mod n = {e_fake}")
    
    # 转为字节用于验证
    e_bytes = int_to_bytes_32(e_fake)
    print(f"e' (hex): {e_bytes.hex()}")
    
    # ========== 步骤5: 验证伪造签名 ==========
    print_separator("步骤5: 验证伪造签名")
    
    print(f"伪造的消息哈希 (32字节): {e_bytes.hex()}")
    
    # 编码为 DER 格式
    sig_der = sigencode_der(r, s_fake, n)
    print(f"DER 编码签名: {sig_der.hex()}")
    
    try:
        # 使用 verify_digest (只验哈希，不验原始消息)
        verified = vk.verify_digest(sig_der, e_bytes, sigdecode=sigdecode_der)
        print_separator("✅ 攻击成功！")
        print(f"伪造签名被验证通过！")
        print(f"伪造的签名 (r', s') = ({r}, {s_fake})")
        print(f"伪造的消息哈希 e' = {e_bytes.hex()}")
        print("\n⚠️  注意：这个签名对原始消息无效，只对这个哈希值有效")
        return True
        
    except Exception as e:
        print_separator("❌ 攻击失败")
        print(f"错误信息: {e}")
        return False

if __name__ == "__main__":
    print(f"\nPython 版本: {sys.version}")
    print(f"ECDSA 库: ecdsa")
    print(f"椭圆曲线: SECP256k1")
    
    success = main()
    
