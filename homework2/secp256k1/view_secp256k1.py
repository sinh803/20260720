"""
辅助脚本：在 VSCode 中方便地查看 secp256k1 源码
"""

import os
import subprocess

def clone_and_show():
    """克隆 secp256k1 并在 VSCode 中打开"""
    
    if not os.path.exists("secp256k1"):
        print("正在克隆 secp256k1 仓库...")
        subprocess.run([
            "git", "clone", 
            "https://github.com/bitcoin-core/secp256k1.git"
        ])
    else:
        print("secp256k1 已存在")
    
    # 显示关键文件
    files_to_show = [
        "secp256k1/src/ecdsa_verify.c",
        "secp256k1/src/ecmult_impl.h",
        "secp256k1/src/ecmult_gen_impl.h",
    ]
    
    for f in files_to_show:
        if os.path.exists(f):
            print(f"\n{'='*60}")
            print(f"文件: {f}")
            print(f"{'='*60}")
            with open(f, 'r') as file:
                lines = file.readlines()
                # 只显示前30行
                print(''.join(lines[:30]))
                if len(lines) > 30:
                    print("... (更多内容在完整文件中)")
        else:
            print(f"文件不存在: {f}")

if __name__ == "__main__":
    clone_and_show()