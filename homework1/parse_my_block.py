import requests

print("正在通过交易ID获取区块哈希...")

txid = "52b43d185a8a1df79f5575d1c515da013812a47d470d658557d3cb413890137e"

# 1. 获取交易信息，得到区块哈希
url_tx = f"https://mempool.space/testnet/api/tx/{txid}"
resp = requests.get(url_tx)
data = resp.json()
block_hash = data['status']['block_hash']
print(f"区块哈希: {block_hash}")
print()

# 2. 获取区块信息
url_block = f"https://mempool.space/testnet/api/block/{block_hash}"
resp = requests.get(url_block)
block = resp.json()

print("="*70)
print("解析我的交易所在区块")
print("="*70)

print("\n--- 区块信息 ---")
print(f"  区块哈希: {block['id']}")
print(f"  区块高度: {block['height']}")
print(f"  版本号: {block['version']}")
print(f"  时间戳: {block['timestamp']} (UTC)")
print(f"  交易数量: {block['tx_count']}")
print(f"  区块大小: {block['size']} 字节")
print(f"  难度: {block['difficulty']}")
print(f"  Merkle根: {block['merkle_root']}")
JIUSH
# 3. 获取区块内的所有交易ID
print("\n--- 区块内的交易 ---")
url_txs = f"https://mempool.space/testnet/api/block/{block_hash}/txids"
resp = requests.get(url_txs)
txids = resp.json()

my_txid = "52b43d185a8a1df79f5575d1c515da013812a47d470d658557d3cb413890137e"

for i, tx in enumerate(txids):
    is_my = " [我的交易]" if tx == my_txid else ""
    print(f"  交易 #{i+1}: {tx}{is_my}")

print("-"*70)
print(f"区块总交易数: {len(txids)}")
print("="*70)

print("\n我的交易详细信息:")
print(f"  交易ID: {my_txid}")
print(f"  查看: https://mempool.space/testnet/tx/{my_txid}")