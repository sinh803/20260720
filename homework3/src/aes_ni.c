#include <stdint.h>
#include <immintrin.h>
#include <wmmintrin.h>

void aes_ni_encrypt_block(const uint8_t in[16], uint8_t out[16], const uint8_t round_keys[11][16]) {
    __m128i data = _mm_loadu_si128((const __m128i*)in);
    __m128i key = _mm_loadu_si128((const __m128i*)round_keys[0]);
    data = _mm_xor_si128(data, key);
    for (int round = 1; round < 10; round++) {
        key = _mm_loadu_si128((const __m128i*)round_keys[round]);
        data = _mm_aesenc_si128(data, key);
    }
    key = _mm_loadu_si128((const __m128i*)round_keys[10]);
    data = _mm_aesenclast_si128(data, key);
    _mm_storeu_si128((__m128i*)out, data);
}
