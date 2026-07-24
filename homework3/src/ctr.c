#include <stdint.h>
#include <string.h>

static void inc_counter(uint8_t *ctr) {
    for (int i = 15; i >= 0; i--) {
        if (++ctr[i]) break;
    }
}

void ctr_crypt(const uint8_t *key, const uint8_t *iv, const uint8_t *in, uint8_t *out, size_t len,
               void (*block_enc)(const uint8_t[16], uint8_t[16], const void*), const void *ctx) {
    uint8_t counter[16], keystream[16];
    memcpy(counter, iv, 16);
    size_t processed = 0;
    while (processed < len) {
        block_enc(counter, keystream, ctx);
        inc_counter(counter);
        size_t chunk = (len - processed >= 16) ? 16 : (len - processed);
        for (size_t i = 0; i < chunk; i++) {
            out[processed + i] = in[processed + i] ^ keystream[i];
        }
        processed += chunk;
    }
}
