#include <stdint.h>
#include <string.h>

static const uint8_t Sbox[16] = {0x1,0xA,0x4,0xC,0x6,0xF,0x3,0x9,0x2,0xD,0xB,0x7,0x5,0x0,0x8,0xE};
static const uint8_t InvSbox[16] = {0xD,0x0,0x8,0x6,0x2,0xC,0x4,0xB,0xE,0x7,0x1,0xA,0x3,0x9,0xF,0x5};

// 置换（位排列），这里用查表法
static const uint8_t P[64] = {
    0,17,34,51,48,1,18,35,32,49,2,19,16,33,50,3,
    4,21,38,55,52,5,22,39,36,53,6,23,20,37,54,7,
    8,25,42,59,56,9,26,43,40,57,10,27,24,41,58,11,
    12,29,46,63,60,13,30,47,44,61,14,31,28,45,62,15
};

static uint64_t permute(uint64_t x) {
    uint64_t y = 0;
    for (int i = 0; i < 64; i++) {
        y |= ((x >> i) & 1ULL) << P[i];
    }
    return y;
}

// 密钥编排（简化版，实际GIFT有专门的密钥调度）
void gift_key_schedule(const uint8_t key[16], uint32_t round_keys[40]) {
    // 此处简化：直接将密钥分成5个32-bit轮密钥（每轮使用）
    // 真实实现应遵循GIFT规范，这里仅作演示
    for (int i = 0; i < 4; i++) {
        round_keys[i] = (key[4*i] << 24) | (key[4*i+1] << 16) | (key[4*i+2] << 8) | key[4*i+3];
    }
    for (int i = 4; i < 40; i++) {
        round_keys[i] = round_keys[i-4] ^ 0x9e3779b9; // 简单常数
    }
}

void gift_encrypt_block(const uint8_t in[8], uint8_t out[8], const uint32_t round_keys[40]) {
    uint64_t state = 0;
    for (int i = 0; i < 8; i++) state = (state << 8) | in[i];
    
    for (int round = 0; round < 40; round++) {
        // SubCells (4-bit S-box)
        uint64_t ns = 0;
        for (int i = 0; i < 16; i++) {
            int nibble = (state >> (4*i)) & 0xF;
            ns |= (uint64_t)Sbox[nibble] << (4*i);
        }
        state = ns;
        // PermBits
        state = permute(state);
        // AddRoundKey (异或轮密钥)
        state ^= (uint64_t)round_keys[round] << 32;
    }
    
    for (int i = 7; i >= 0; i--) {
        out[i] = state & 0xFF;
        state >>= 8;
    }
}
