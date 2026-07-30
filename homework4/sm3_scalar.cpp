#include "sm3_internal.hpp"

namespace sm3::detail {

namespace {

void expand_scalar(const std::uint8_t block[64], std::uint32_t W[68], std::uint32_t Wp[64])
{
    for (int j = 0; j < 16; ++j) {
        W[j] = load_be32(block + j * 4);
    }
    for (int j = 16; j < 68; ++j) {
        W[j] = p1(W[j - 16] ^ W[j - 9] ^ rotl32(W[j - 3], 15))
             ^ rotl32(W[j - 13], 7) ^ W[j - 6];
    }
    for (int j = 0; j < 64; ++j) {
        Wp[j] = W[j] ^ W[j + 4];
    }
}

template <auto FF, auto GG>
void rounds_gpr(std::uint32_t state[8], const std::uint32_t W[68], const std::uint32_t Wp[64],
                int begin, int end)
{
    std::uint32_t A = state[0], B = state[1], C = state[2], D = state[3];
    std::uint32_t E = state[4], F = state[5], G = state[6], H = state[7];

    for (int j = begin; j < end; ++j) {
        const std::uint32_t ss1 = rotl32((rotl32(A, 12) + E + rotl32(kTj[j], j)), 7);
        const std::uint32_t ss2 = ss1 ^ rotl32(A, 12);
        const std::uint32_t tt1 = FF(A, B, C) + D + ss2 + Wp[j];
        const std::uint32_t tt2 = GG(E, F, G) + H + ss1 + W[j];
        D = C; C = rotl32(B, 9); B = A; A = tt1;
        H = G; G = rotl32(F, 19); F = E; E = p0(tt2);
    }

    state[0] ^= A; state[1] ^= B; state[2] ^= C; state[3] ^= D;
    state[4] ^= E; state[5] ^= F; state[6] ^= G; state[7] ^= H;
}

void rounds_gpr(std::uint32_t state[8], const std::uint32_t W[68], const std::uint32_t Wp[64])
{
    rounds_gpr<ff0, gg0>(state, W, Wp, 0, 16);
    rounds_gpr<ff1, gg1>(state, W, Wp, 16, 64);
}

} // namespace

void compress_scalar(std::uint32_t state[8], const std::uint8_t block[64])
{
    std::uint32_t W[68];
    std::uint32_t Wp[64];
    expand_scalar(block, W, Wp);
    rounds_gpr(state, W, Wp);
}

} // namespace sm3::detail
