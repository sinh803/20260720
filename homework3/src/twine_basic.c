#include <stdint.h>
#include <string.h>

static const uint8_t Sbox[16] = {0xC,0x0,0xF,0xA,0x2,0xB,0x9,0x5,0x8,0x3,0xD,0x7,0x1,0xE,0x6,0x4};

// 置换P (基于Feistel)
static const uint8_t P[16] = {5,0,1,4,7,12,3,8,13,6,9,2,15,10,11,14};

void twine_key_schedule(const uint8_t key[10], uint32_t round_keys[36]) {
    // 简化版：直接使用前36个半字节
    for (int i = 0; i < 36; i++) {
        uint8_t index = (i * 5) % 10;
        round_keys[i] = (key[index] << 24) | (key[(index+1)%10] << 16) |
                        (key[(index+2)%10] << 8) | key[(index+3)%10];
    }
}

void twine_encrypt_block(const uint8_t in[8], uint8_t out[8], const uint32_t round_keys[36]) {
    uint64_t state = 0;
    for (int i = 0; i < 8; i++) state = (state << 8) | in[i];
    
    for (int round = 0; round < 36; round++) {
        // S-box替换
        uint64_t ns = 0;
        for (int i = 0; i < 16; i++) {
            int nibble = (state >> (4*i)) & 0xF;
            ns |= (uint64_t)Sbox[nibble] << (4*i);
        }
        state = ns;
        // 置换P
        uint64_t ps = 0;
        for (int i = 0; i < 16; i++) {
            int src = P[i];
            ps |= ((state >> (4*src)) & 0xF) << (4*i);
        }
        state = ps;
        // 异或轮密钥
        state ^= (uint64_t)round_keys[round] << 32;
    }
    
    for (int i = 7; i >= 0; i--) {
        out[i] = state & 0xFF;
        state >>= 8;
    }
}
