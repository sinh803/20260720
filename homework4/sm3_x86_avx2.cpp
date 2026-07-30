#include "sm3_internal.hpp"

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)

#include <immintrin.h>

#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <cpuid.h>
#endif

namespace sm3::detail {

namespace {

void expand_avx2(const std::uint8_t block[64], std::uint32_t W[68], std::uint32_t Wp[64])
{
    alignas(32) static const std::uint8_t be_mask_bytes[32] = {
        3, 2, 1, 0, 7, 6, 5, 4, 11, 10, 9, 8, 15, 14, 13, 12,
        3, 2, 1, 0, 7, 6, 5, 4, 11, 10, 9, 8, 15, 14, 13, 12
    };
    const __m256i be_mask = _mm256_load_si256(reinterpret_cast<const __m256i *>(be_mask_bytes));

    _mm256_storeu_si256(reinterpret_cast<__m256i *>(W),
        _mm256_shuffle_epi8(_mm256_loadu_si256(reinterpret_cast<const __m256i *>(block)), be_mask));
    _mm256_storeu_si256(reinterpret_cast<__m256i *>(W + 8),
        _mm256_shuffle_epi8(_mm256_loadu_si256(reinterpret_cast<const __m256i *>(block + 32)), be_mask));

    for (int j = 16; j < 68; ++j) {
        const std::uint32_t t = W[j - 16] ^ W[j - 9] ^ rotl32(W[j - 3], 15);
        W[j] = p1(t) ^ rotl32(W[j - 13], 7) ^ W[j - 6];
    }

    for (int j = 0; j < 64; j += 8) {
        const __m256i a = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(W + j));
        const __m256i b = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(W + j + 4));
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(Wp + j), _mm256_xor_si256(a, b));
    }
}

std::uint32_t extract_epi32(__m256i v, int lane)
{
    alignas(32) std::uint32_t tmp[8];
    _mm256_storeu_si256(reinterpret_cast<__m256i *>(tmp), v);
    return tmp[lane];
}

template <auto FF, auto GG>
void do_round(int j, std::uint32_t wv, std::uint32_t wpv,
              std::uint32_t &A, std::uint32_t &B, std::uint32_t &C, std::uint32_t &D,
              std::uint32_t &E, std::uint32_t &F, std::uint32_t &G, std::uint32_t &H)
{
    const std::uint32_t ss1 = rotl32((rotl32(A, 12) + E + rotl32(kTj[j], j)), 7);
    const std::uint32_t ss2 = ss1 ^ rotl32(A, 12);
    const std::uint32_t tt1 = FF(A, B, C) + D + ss2 + wpv;
    const std::uint32_t tt2 = GG(E, F, G) + H + ss1 + wv;
    D = C; C = rotl32(B, 9); B = A; A = tt1;
    H = G; G = rotl32(F, 19); F = E; E = p0(tt2);
}

template <auto FF, auto GG>
void rounds_hybrid(std::uint32_t state[8], const std::uint32_t W[68], const std::uint32_t Wp[64],
                   int begin, int end)
{
    std::uint32_t A = state[0], B = state[1], C = state[2], D = state[3];
    std::uint32_t E = state[4], F = state[5], G = state[6], H = state[7];

    for (int j = begin; j < end; j += 8) {
        const __m256i wv  = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(W + j));
        const __m256i wpv = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(Wp + j));
        for (int k = 0; k < 8; ++k) {
            do_round<FF, GG>(j + k, extract_epi32(wv, k), extract_epi32(wpv, k),
                             A, B, C, D, E, F, G, H);
        }
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

void compress_avx2(std::uint32_t state[8], const std::uint8_t block[64])
{
    std::uint32_t W[68];
    std::uint32_t Wp[64];
    expand_avx2(block, W, Wp);
    rounds_hybrid(state, W, Wp);
}

bool cpu_has_avx2() noexcept
{
#if defined(__AVX2__)
    #if defined(_MSC_VER)
    int info[4];
    __cpuidex(info, 7, 0);
    return (info[1] & (1 << 5)) != 0;
    #else
    unsigned int eax, ebx, ecx, edx;
    if (__get_cpuid_max(0, nullptr) < 7) {
        return false;
    }
    __cpuid_count(7, 0, eax, ebx, ecx, edx);
    return (ebx & (1 << 5)) != 0;
    #endif
#else
    return false;
#endif
}

} // namespace sm3::detail

#endif
