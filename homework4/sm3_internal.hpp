#pragma once

#include <array>
#include <cstdint>
#include <functional>

namespace sm3::detail {

constexpr std::uint32_t rotl32(std::uint32_t x, int n) noexcept
{
    return (x << n) | (x >> (32 - n));
}

constexpr std::uint32_t p0(std::uint32_t x) noexcept
{
    return x ^ rotl32(x, 9) ^ rotl32(x, 17);
}

constexpr std::uint32_t p1(std::uint32_t x) noexcept
{
    return x ^ rotl32(x, 15) ^ rotl32(x, 23);
}

constexpr std::uint32_t ff0(std::uint32_t x, std::uint32_t y, std::uint32_t z) noexcept
{
    return x ^ y ^ z;
}

constexpr std::uint32_t ff1(std::uint32_t x, std::uint32_t y, std::uint32_t z) noexcept
{
    return (x & y) | (x & z) | (y & z);
}

constexpr std::uint32_t gg0(std::uint32_t x, std::uint32_t y, std::uint32_t z) noexcept
{
    return x ^ y ^ z;
}

constexpr std::uint32_t gg1(std::uint32_t x, std::uint32_t y, std::uint32_t z) noexcept
{
    return (x & y) | (~x & z);
}

inline constexpr std::array<std::uint32_t, 8> kIv = {
    0x7380166fu, 0x4914b2b9u, 0x172442d7u, 0xda8a0600u,
    0xa96f30bcu, 0x163138aau, 0x0e38dee4u, 0xbfd1b4b0u
};

inline constexpr std::array<std::uint32_t, 64> kTj = {
    0x79cc4519u, 0x79cc4519u, 0x79cc4519u, 0x79cc4519u,
    0x79cc4519u, 0x79cc4519u, 0x79cc4519u, 0x79cc4519u,
    0x79cc4519u, 0x79cc4519u, 0x79cc4519u, 0x79cc4519u,
    0x79cc4519u, 0x79cc4519u, 0x79cc4519u, 0x79cc4519u,
    0x7a879d8au, 0x7a879d8au, 0x7a879d8au, 0x7a879d8au,
    0x7a879d8au, 0x7a879d8au, 0x7a879d8au, 0x7a879d8au,
    0x7a879d8au, 0x7a879d8au, 0x7a879d8au, 0x7a879d8au,
    0x7a879d8au, 0x7a879d8au, 0x7a879d8au, 0x7a879d8au,
    0x7a879d8au, 0x7a879d8au, 0x7a879d8au, 0x7a879d8au,
    0x7a879d8au, 0x7a879d8au, 0x7a879d8au, 0x7a879d8au,
    0x7a879d8au, 0x7a879d8au, 0x7a879d8au, 0x7a879d8au,
    0x7a879d8au, 0x7a879d8au, 0x7a879d8au, 0x7a879d8au,
    0x7a879d8au, 0x7a879d8au, 0x7a879d8au, 0x7a879d8au,
    0x7a879d8au, 0x7a879d8au, 0x7a879d8au, 0x7a879d8au,
    0x7a879d8au, 0x7a879d8au, 0x7a879d8au, 0x7a879d8au,
    0x7a879d8au, 0x7a879d8au, 0x7a879d8au, 0x7a879d8au
};

constexpr std::uint32_t load_be32(const std::uint8_t *p) noexcept
{
    return (static_cast<std::uint32_t>(p[0]) << 24) |
           (static_cast<std::uint32_t>(p[1]) << 16) |
           (static_cast<std::uint32_t>(p[2]) << 8)  |
           static_cast<std::uint32_t>(p[3]);
}

constexpr void store_be32(std::uint8_t *p, std::uint32_t v) noexcept
{
    p[0] = static_cast<std::uint8_t>(v >> 24);
    p[1] = static_cast<std::uint8_t>(v >> 16);
    p[2] = static_cast<std::uint8_t>(v >> 8);
    p[3] = static_cast<std::uint8_t>(v);
}

using CompressFn = void (*)(std::uint32_t state[8], const std::uint8_t block[64]);

void compress_scalar(std::uint32_t state[8], const std::uint8_t block[64]);

#if defined(__aarch64__) || defined(_M_ARM64)
void compress_arm64(std::uint32_t state[8], const std::uint8_t block[64]);
#endif

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
void compress_avx2(std::uint32_t state[8], const std::uint8_t block[64]);
void compress_avx512(std::uint32_t state[8], const std::uint8_t block[64]);
[[nodiscard]] bool cpu_has_avx2() noexcept;
[[nodiscard]] bool cpu_has_avx512() noexcept;
#endif

CompressFn &active_compress() noexcept;
Impl &active_impl() noexcept;

} // namespace sm3::detail
