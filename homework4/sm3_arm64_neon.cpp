#include "sm3_internal.hpp"

#if defined(__aarch64__) || defined(_M_ARM64)

#include <arm_neon.h>

namespace sm3::detail {

namespace {

void expand_arm64(const std::uint8_t block[64], std::uint32_t W[68], std::uint32_t Wp[64])
{
    const uint8x16_t b0 = vld1q_u8(block);
    const uint8x16_t b1 = vld1q_u8(block + 16);
    const uint8x16_t b2 = vld1q_u8(block + 32);
    const uint8x16_t b3 = vld1q_u8(block + 48);

    vst1q_u32(W,      vreinterpretq_u32_u8(vrev32q_u8(b0)));
    vst1q_u32(W + 4,  vreinterpretq_u32_u8(vrev32q_u8(b1)));
    vst1q_u32(W + 8,  vreinterpretq_u32_u8(vrev32q_u8(b2)));
    vst1q_u32(W + 12, vreinterpretq_u32_u8(vrev32q_u8(b3)));

    for (int j = 16; j < 68; ++j) {
        const std::uint32_t t = W[j - 16] ^ W[j - 9] ^ rotl32(W[j - 3], 15);
        W[j] = p1(t) ^ rotl32(W[j - 13], 7) ^ W[j - 6];
    }

    for (int j = 0; j < 64; j += 4) {
        const uint32x4_t w0 = vld1q_u32(W + j);
        const uint32x4_t w1 = vld1q_u32(W + j + 4);
        vst1q_u32(Wp + j, veorq_u32(w0, w1));
    }
}

template <auto FF, auto GG>
void do_round(int j, std::uint32_t w_lane, std::uint32_t wp_lane,
              std::uint32_t &A, std::uint32_t &B, std::uint32_t &C, std::uint32_t &D,
              std::uint32_t &E, std::uint32_t &F, std::uint32_t &G, std::uint32_t &H)
{
    const std::uint32_t ss1 = rotl32((rotl32(A, 12) + E + rotl32(kTj[j], j)), 7);
    const std::uint32_t ss2 = ss1 ^ rotl32(A, 12);
    const std::uint32_t tt1 = FF(A, B, C) + D + ss2 + wp_lane;
    const std::uint32_t tt2 = GG(E, F, G) + H + ss1 + w_lane;
    D = C; C = rotl32(B, 9); B = A; A = tt1;
    H = G; G = rotl32(F, 19); F = E; E = p0(tt2);
}

template <auto FF, auto GG>
void rounds_hybrid(std::uint32_t state[8], const std::uint32_t W[68], const std::uint32_t Wp[64],
                   int begin, int end)
{
    std::uint32_t A = state[0], B = state[1], C = state[2], D = state[3];
    std::uint32_t E = state[4], F = state[5], G = state[6], H = state[7];

    for (int j = begin; j < end; j += 4) {
        const uint32x4_t wv  = vld1q_u32(W + j);
        const uint32x4_t wpv = vld1q_u32(Wp + j);
        do_round<FF, GG>(j + 0, vgetq_lane_u32(wv, 0), vgetq_lane_u32(wpv, 0), A, B, C, D, E, F, G, H);
        do_round<FF, GG>(j + 1, vgetq_lane_u32(wv, 1), vgetq_lane_u32(wpv, 1), A, B, C, D, E, F, G, H);
        do_round<FF, GG>(j + 2, vgetq_lane_u32(wv, 2), vgetq_lane_u32(wpv, 2), A, B, C, D, E, F, G, H);
        do_round<FF, GG>(j + 3, vgetq_lane_u32(wv, 3), vgetq_lane_u32(wpv, 3), A, B, C, D, E, F, G, H);
    }

    state[0] ^= A; state[1] ^= B; state[2] ^= C; state[3] ^= D;
    state[4] ^= E; state[5] ^= F; state[6] ^= G; state[7] ^= H;
}

void rounds_hybrid(std::uint32_t state[8], const std::uint32_t W[68], const std::uint32_t Wp[64])
{
    rounds_hybrid<ff0, gg0>(state, W, Wp, 0, 16);
    rounds_hybrid<ff1, gg1>(state, W, Wp, 16, 64);
}

} // namespace

void compress_arm64(std::uint32_t state[8], const std::uint8_t block[64])
{
    std::uint32_t W[68];
    std::uint32_t Wp[64];
    expand_arm64(block, W, Wp);
    rounds_hybrid(state, W, Wp);
}

} // namespace sm3::detail

#endif
