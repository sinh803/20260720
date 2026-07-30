#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace sm3 {

inline constexpr std::size_t digest_size = 32;
inline constexpr std::size_t block_size  = 64;

enum class Impl {
    Scalar  = 0,
    Arm64   = 1,
    Avx2    = 2,
    Avx512  = 3,
    Auto    = -1
};

class Hasher {
public:
    Hasher();

    void reset();
    void update(std::span<const std::uint8_t> data);
    void update(const void *data, std::size_t len);
    std::array<std::uint8_t, digest_size> final();

    static std::array<std::uint8_t, digest_size> hash(std::span<const std::uint8_t> data);
    static std::array<std::uint8_t, digest_size> hash(const void *data, std::size_t len);

private:
    void process_block(const std::uint8_t block[block_size]);

    std::array<std::uint32_t, 8>          state_{};
    std::uint64_t                           total_bytes_ = 0;
    std::array<std::uint8_t, block_size>    buffer_{};
    std::size_t                             buffer_len_ = 0;
};

[[nodiscard]] Impl        detect_best() noexcept;
[[nodiscard]] Impl        get_impl() noexcept;
[[nodiscard]] std::string_view impl_name(Impl impl) noexcept;

void set_impl(Impl impl);

} // namespace sm3
