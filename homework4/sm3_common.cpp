#include "sm3.hpp"
#include "sm3_internal.hpp"

#include <algorithm>
#include <cstring>

namespace sm3::detail {

CompressFn &active_compress() noexcept
{
    static CompressFn fn = compress_scalar;
    return fn;
}

Impl &active_impl() noexcept
{
    static Impl impl = Impl::Scalar;
    return impl;
}

} // namespace sm3::detail

namespace sm3 {

Hasher::Hasher()
{
    reset();
}

void Hasher::reset()
{
    std::copy(detail::kIv.begin(), detail::kIv.end(), state_.begin());
    total_bytes_ = 0;
    buffer_len_  = 0;
}

void Hasher::process_block(const std::uint8_t block[block_size])
{
    detail::active_compress()(state_.data(), block);
}

void Hasher::update(std::span<const std::uint8_t> data)
{
    if (data.empty()) {
        return;
    }

    total_bytes_ += data.size();
    const std::uint8_t *ptr = data.data();
    std::size_t len = data.size();

    if (buffer_len_ > 0) {
        const std::size_t need = block_size - buffer_len_;
        if (len < need) {
            std::memcpy(buffer_.data() + buffer_len_, ptr, len);
            buffer_len_ += len;
            return;
        }
        std::memcpy(buffer_.data() + buffer_len_, ptr, need);
        process_block(buffer_.data());
        ptr += need;
        len -= need;
        buffer_len_ = 0;
    }

    while (len >= block_size) {
        process_block(ptr);
        ptr += block_size;
        len -= block_size;
    }

    if (len > 0) {
        std::memcpy(buffer_.data(), ptr, len);
        buffer_len_ = len;
    }
}

void Hasher::update(const void *data, std::size_t len)
{
    update(std::span{static_cast<const std::uint8_t *>(data), len});
}

std::array<std::uint8_t, digest_size> Hasher::final()
{
    const std::uint64_t bit_len = total_bytes_ * 8;
    const std::size_t pad_len = (buffer_len_ < 56)
        ? (56 - buffer_len_)
        : (120 - buffer_len_);

    const std::array<std::uint8_t, 128> pad = [] {
        std::array<std::uint8_t, 128> p{};
        p[0] = 0x80;
        return p;
    }();
    update(std::span{pad.data(), pad_len});

    std::array<std::uint8_t, 8> len_be{};
    for (int i = 0; i < 8; ++i) {
        len_be[i] = static_cast<std::uint8_t>(bit_len >> (56 - i * 8));
    }
    update(len_be);

    std::array<std::uint8_t, digest_size> digest{};
    for (int i = 0; i < 8; ++i) {
        detail::store_be32(digest.data() + i * 4, state_[i]);
    }

    buffer_len_ = 0;
    return digest;
}

std::array<std::uint8_t, digest_size> Hasher::hash(std::span<const std::uint8_t> data)
{
    Hasher h;
    h.update(data);
    return h.final();
}

std::array<std::uint8_t, digest_size> Hasher::hash(const void *data, std::size_t len)
{
    return hash(std::span{static_cast<const std::uint8_t *>(data), len});
}

std::string_view impl_name(Impl impl) noexcept
{
    switch (impl) {
    case Impl::Scalar:  return "scalar";
    case Impl::Arm64:   return "arm64-neon";
    case Impl::Avx2:    return "x86-avx2";
    case Impl::Avx512:  return "x86-avx512";
    default:            return "unknown";
    }
}

void set_impl(Impl impl)
{
    if (impl == Impl::Auto) {
        impl = detect_best();
    }

    switch (impl) {
#if defined(__aarch64__) || defined(_M_ARM64)
    case Impl::Arm64:
        detail::active_compress() = detail::compress_arm64;
        detail::active_impl() = Impl::Arm64;
        return;
#endif
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    case Impl::Avx512:
        if (detail::cpu_has_avx512()) {
            detail::active_compress() = detail::compress_avx512;
            detail::active_impl() = Impl::Avx512;
            return;
        }
        [[fallthrough]];
    case Impl::Avx2:
        if (detail::cpu_has_avx2()) {
            detail::active_compress() = detail::compress_avx2;
            detail::active_impl() = Impl::Avx2;
            return;
        }
        [[fallthrough]];
#endif
    case Impl::Scalar:
    default:
        detail::active_compress() = detail::compress_scalar;
        detail::active_impl() = Impl::Scalar;
        return;
    }
}

Impl get_impl() noexcept
{
    return detail::active_impl();
}

} // namespace sm3
