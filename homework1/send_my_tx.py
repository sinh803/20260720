from bit import PrivateKeyTestnet
import requests
import hashlib

MY_WIF = 'cVaMKEck6qKkndxZBD8uUebrC3Jw4YHMJuDVbw8qVQAr6pvum6Cf'
TO_ADDRESS = 'mkH41dfD4S8DEoSfcVSvEfpyZ9siogWWtr'
AMOUNT = 0.00001

key = PrivateKeyTestnet(MY_WIF)

print("=" * 50)
print("准备发送交易")
print("=" * 50)
print(f"发送方地址: {key.address}")
print(f"接收方地址: {TO_ADDRESS}")
print(f"发送金额: {AMOUNT} tBTC")
print("-" * 50)

# 创建交易
print("正在创建交易...")
raw_hex = key.create_transaction([(TO_ADDRESS, AMOUNT, 'btc')])
print(f"交易创建成功，大小: {len(raw_hex) // 2} 字节")

# 计算交易ID
tx_bytes = bytes.fromhex(raw_hex)
txid_bytes = hashlib.sha256(hashlib.sha256(tx_bytes).digest()).digest()
txid = txid_bytes[::-1].hex()
print(f"交易ID: {txid}")

# 尝试多个广播API
print("正在广播交易...")

# API列表
apis = [
    ("https://mempool.space/testnet/api/tx", "text/plain"),
    ("https://blockstream.info/testnet/api/tx", "text/plain"),
    ("https://api.blockcypher.com/v1/btc/test3/txs/push", "application/json"),
]

success = False
for url, content_type in apis:
    try:
        if content_type == "application/json":
            response = requests.post(url, json={"tx": raw_hex}, timeout=10)
        else:
            response = requests.post(url, data=raw_hex,
                                     headers={"Content-Type": "text/plain"},
                                     timeout=10)

        if response.status_code == 200 or response.status_code == 201:
            print(f"\n广播成功！使用API: {url}")
            success = True
            break
        else:
            print(f"API {url} 返回: {response.status_code}")
    except Exception as e:
        print(f"API {url} 出错: {e}")

if success:
    print(f"\n交易已广播！")
    print(f"交易ID (TxID): {txid}")
    print(f"查看交易: https://mempool.space/testnet/tx/{txid}")

    print(f"\n原始十六进制数据 (raw hex):")
    print(raw_hex)

    with open('my_tx_hex.txt', 'w') as f:
        f.write(raw_hex)
    print(f"\n原始数据已保存到 my_tx_hex.txt")
else:
    print("\n所有API都失败了，但交易已创建。")
    print(f"请手动访问以下网址广播：")
    print(f"https://blockstream.info/testnet/tx/push")
    print(f"粘贴以下raw_hex:")
    print(raw_hex)

    # 保存raw_hex备用
    with open('my_tx_hex.txt', 'w') as f:
        f.write(raw_hex)
    print(f"\nraw_hex已保存到 my_tx_hex.txt")

print("=" * 50)