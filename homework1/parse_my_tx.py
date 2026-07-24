import struct

def parse_varint(data, offset):
    first = data[offset]
    if first < 0xfd:
        return first, offset + 1
    elif first == 0xfd:
        return struct.unpack("<H", data[offset+1:offset+3])[0], offset + 3
    elif first == 0xfe:
        return struct.unpack("<I", data[offset+1:offset+5])[0], offset + 5
    else:
        return struct.unpack("<Q", data[offset+1:offset+9])[0], offset + 9

def parse_tx(raw_hex):
    tx_bytes = bytes.fromhex(raw_hex)
    offset = 0
    print("="*70)
    print("解析我的交易 (逐字节拆解)")
    print("="*70)
    print(f"原始十六进制: {raw_hex[:64]}... (总长度 {len(raw_hex)} 字符)")
    print("-"*70)

    # 1. 版本号 (4字节)
    version = struct.unpack("<I", tx_bytes[offset:offset+4])[0]
    print(f"[偏移 {offset:3d}] 版本号 (Version): {version} (0x{version:08x}) [4字节]")
    offset += 4

    # 2. 输入数量 (VarInt)
    input_count, offset = parse_varint(tx_bytes, offset)
    print(f"[偏移 {offset:3d}] 输入数量 (Input Count): {input_count} [VarInt]")
    print()

    for i in range(input_count):
        print(f"  --- 输入 #{i+1} ---")
        prev_hash = tx_bytes[offset:offset+32][::-1].hex()
        print(f"  [偏移 {offset:3d}] 前交易哈希: {prev_hash} [32字节]")
        offset += 32
        prev_index = struct.unpack("<I", tx_bytes[offset:offset+4])[0]
        print(f"  [偏移 {offset:3d}] 前输出索引: {prev_index} [4字节]")
        offset += 4
        script_len, offset = parse_varint(tx_bytes, offset)
        script_sig = tx_bytes[offset:offset+script_len].hex()
        print(f"  [偏移 {offset:3d}] 解锁脚本长度: {script_len} 字节 [VarInt]")
        print(f"  [偏移 {offset:3d}] 解锁脚本: {script_sig} [{script_len}字节]")
        offset += script_len
        sequence = struct.unpack("<I", tx_bytes[offset:offset+4])[0]
        print(f"  [偏移 {offset:3d}] 序列号: {sequence} (0x{sequence:08x}) [4字节]")
        offset += 4
        print()

    # 3. 输出数量 (VarInt)
    output_count, offset = parse_varint(tx_bytes, offset)
    print(f"[偏移 {offset:3d}] 输出数量 (Output Count): {output_count} [VarInt]")
    print()

    for i in range(output_count):
        print(f"  --- 输出 #{i+1} ---")
        amount = struct.unpack("<Q", tx_bytes[offset:offset+8])[0]
        print(f"  [偏移 {offset:3d}] 金额: {amount} 聪 ({amount/1e8} tBTC) [8字节]")
        offset += 8
        script_len, offset = parse_varint(tx_bytes, offset)
        script_pubkey = tx_bytes[offset:offset+script_len].hex()
        print(f"  [偏移 {offset:3d}] 锁定脚本长度: {script_len} 字节 [VarInt]")
        print(f"  [偏移 {offset:3d}] 锁定脚本: {script_pubkey} [{script_len}字节]")
        offset += script_len
        print()

    # 4. 锁定时间 (4字节)
    locktime = struct.unpack("<I", tx_bytes[offset:offset+4])[0]
    print(f"[偏移 {offset:3d}] 锁定时间: {locktime} (0x{locktime:08x}) [4字节]")
    print("-"*70)
    print(f"解析完成！交易总大小: {len(tx_bytes)} 字节")
    print("="*70)

# 读取你的交易原始数据
with open('my_tx_hex.txt', 'r') as f:
    raw_hex = f.read().strip()
parse_tx(raw_hex)