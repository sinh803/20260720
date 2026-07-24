#include <stdint.h>

// 外部引用 SM4 S 盒（已在 sm4_basic.c 中定义）
extern const uint8_t Sbox[256];

// 预计算 T 表：T[x] = L(S(x))
static uint32_t SM4_T[256];
static int ttable_initialized = 0;

static uint32_t rotl32(uint32_t x, int n) {
    return (x << n) | (x >> (32 - n));
}

void sm4_init_ttable(void) {
    if (ttable_initialized) return;
    for (int i = 0; i < 256; i++) {
        uint32_t s = Sbox[i];
        SM4_T[i] = s ^ rotl32(s, 2) ^ rotl32(s, 10) ^ rotl32(s, 18) ^ rotl32(s, 24);
    }
    ttable_initialized = 1;
}

// T-table 加密一个块
void sm4_ttable_encrypt_block(const uint8_t in[16], uint8_t out[16], const uint32_t rk[32]) {
    sm4_init_ttable();
    uint32_t X[4];
    X[0] = (in[0]<<24)|(in[1]<<16)|(in[2]<<8)|in[3];
    X[1] = (in[4]<<24)|(in[5]<<16)|(in[6]<<8)|in[7];
    X[2] = (in[8]<<24)|(in[9]<<16)|(in[10]<<8)|in[11];
    X[3] = (in[12]<<24)|(in[13]<<16)|(in[14]<<8)|in[15];

    for (int i = 0; i < 32; i++) {
        uint32_t t = X[1] ^ X[2] ^ X[3] ^ rk[i];
        uint32_t tmp = X[0] ^ SM4_T[(t >> 24) & 0xFF]
                        ^ SM4_T[(t >> 16) & 0xFF]
                        ^ SM4_T[(t >> 8) & 0xFF]
                        ^ SM4_T[t & 0xFF];
        X[0] = X[1]; X[1] = X[2]; X[2] = X[3]; X[3] = tmp;
    }

    out[0] = (X[3]>>24)&0xFF; out[1] = (X[3]>>16)&0xFF; out[2] = (X[3]>>8)&0xFF; out[3] = X[3]&0xFF;
    out[4] = (X[2]>>24)&0xFF; out[5] = (X[2]>>16)&0xFF; out[6] = (X[2]>>8)&0xFF; out[7] = X[2]&0xFF;
    out[8] = (X[1]>>24)&0xFF; out[9] = (X[1]>>16)&0xFF; out[10] = (X[1]>>8)&0xFF; out[11] = X[1]&0xFF;
    out[12] = (X[0]>>24)&0xFF; out[13] = (X[0]>>16)&0xFF; out[14] = (X[0]>>8)&0xFF; out[15] = X[0]&0xFF;
}
