#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdint.h>

// ---- 函数声明 ----
void sm4_key_expand(const uint8_t key[16], uint32_t rk[32]);
void sm4_encrypt_block(const uint8_t in[16], uint8_t out[16], const uint32_t rk[32]);
void sm4_ttable_encrypt_block(const uint8_t in[16], uint8_t out[16], const uint32_t rk[32]);
void sm4_shuffle_encrypt_block(const uint8_t in[16], uint8_t out[16], const uint32_t rk[32]);

void aes_key_expand(const uint8_t key[16], uint8_t round_keys[11][16]);
void aes_encrypt_block(const uint8_t in[16], uint8_t out[16], const uint8_t round_keys[11][16]);
void aes_ni_encrypt_block(const uint8_t in[16], uint8_t out[16], const uint8_t round_keys[11][16]);

void gift_key_schedule(const uint8_t key[16], uint32_t round_keys[40]);
void gift_encrypt_block(const uint8_t in[8], uint8_t out[8], const uint32_t round_keys[40]);

void twine_key_schedule(const uint8_t key[10], uint32_t round_keys[36]);
void twine_encrypt_block(const uint8_t in[8], uint8_t out[8], const uint32_t round_keys[36]);

void ctr_crypt(const uint8_t *key, const uint8_t *iv, const uint8_t *in, uint8_t *out, size_t len,
               void (*block_enc)(const uint8_t[16], uint8_t[16], const void*), const void *ctx);

// ---- 通用基准 ----
// 对于 16 字节块加密函数
double bench_16(void (*enc)(const uint8_t[16], uint8_t[16], const void*),
                const void *ctx, int iterations) {
    uint8_t plain[16] = {0}, cipher[16];
    clock_t start = clock();
    for (int i = 0; i < iterations; i++) {
        enc(plain, cipher, ctx);
        memcpy(plain, cipher, 16);
    }
    clock_t end = clock();
    return (double)(end - start) / CLOCKS_PER_SEC;
}

// 对于 8 字节块加密函数（GIFT/TWINE）
double bench_8(void (*enc)(const uint8_t[8], uint8_t[8], const void*),
               const void *ctx, int iterations) {
    uint8_t plain[8] = {0}, cipher[8];
    clock_t start = clock();
    for (int i = 0; i < iterations; i++) {
        enc(plain, cipher, ctx);
        memcpy(plain, cipher, 8);
    }
    clock_t end = clock();
    return (double)(end - start) / CLOCKS_PER_SEC;
}

// ---- SM4 测试 ----
void test_sm4() {
    uint8_t key[16] = {0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF,0xFE,0xDC,0xBA,0x98,0x76,0x54,0x32,0x10};
    uint32_t rk[32];
    sm4_key_expand(key, rk);
    int iterations = 1000000;
    double mb = (iterations * 16.0) / (1024*1024);

    printf("\n=== SM4 性能 (%d 块) ===\n", iterations);
    double t = bench_16((void*)sm4_encrypt_block, rk, iterations);
    printf("基本实现: %.3f s, %.2f MB/s\n", t, mb/t);
    t = bench_16((void*)sm4_ttable_encrypt_block, rk, iterations);
    printf("T-table : %.3f s, %.2f MB/s\n", t, mb/t);
    t = bench_16((void*)sm4_shuffle_encrypt_block, rk, iterations);
    printf("Shuffle  : %.3f s, %.2f MB/s\n", t, mb/t);
}

// ---- AES 测试 ----
void test_aes() {
    uint8_t key[16] = {0};
    uint8_t rk[11][16];
    aes_key_expand(key, rk);
    int iterations = 1000000;
    double mb = (iterations * 16.0) / (1024*1024);

    printf("\n=== AES 性能 (%d 块) ===\n", iterations);
    double t = bench_16((void*)aes_encrypt_block, rk, iterations);
    printf("基本实现: %.3f s, %.2f MB/s\n", t, mb/t);
    t = bench_16((void*)aes_ni_encrypt_block, rk, iterations);
    printf("AES-NI  : %.3f s, %.2f MB/s\n", t, mb/t);
}

// ---- GIFT 测试 ----
void test_gift() {
    uint8_t key[16] = {0};
    uint32_t rk[40];
    gift_key_schedule(key, rk);
    int iterations = 2000000;  // 2M 块（8字节）
    double mb = (iterations * 8.0) / (1024*1024);

    printf("\n=== GIFT-64 性能 (%d 块) ===\n", iterations);
    double t = bench_8((void*)gift_encrypt_block, rk, iterations);
    printf("基本实现: %.3f s, %.2f MB/s\n", t, mb/t);
}

// ---- TWINE 测试 ----
void test_twine() {
    uint8_t key[10] = {0};
    uint32_t rk[36];
    twine_key_schedule(key, rk);
    int iterations = 2000000;
    double mb = (iterations * 8.0) / (1024*1024);

    printf("\n=== TWINE-80 性能 (%d 块) ===\n", iterations);
    double t = bench_8((void*)twine_encrypt_block, rk, iterations);
    printf("基本实现: %.3f s, %.2f MB/s\n", t, mb/t);
}

// ---- CTR 模式测试 ----
void test_ctr() {
    uint8_t key[16] = {0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF,0xFE,0xDC,0xBA,0x98,0x76,0x54,0x32,0x10};
    uint32_t rk[32];
    sm4_key_expand(key, rk);
    uint8_t iv[16] = {0};
    uint8_t plain[1024*1024] = {0};
    uint8_t cipher[1024*1024];
    size_t len = 1024*1024;

    printf("\n=== SM4-CTR 模式 (1MB 数据) ===\n");
    clock_t start = clock();
    ctr_crypt((const uint8_t*)&rk, iv, plain, cipher, len,
              (void(*)(const uint8_t[16], uint8_t[16], const void*))sm4_encrypt_block, rk);
    clock_t end = clock();
    double t = (double)(end - start) / CLOCKS_PER_SEC;
    printf("加密 1MB: %.3f s, 吞吐量 %.2f MB/s\n", t, 1.0/t);
}

int main() {
    test_sm4();
    test_aes();
    test_gift();
    test_twine();
    test_ctr();
    return 0;
}
