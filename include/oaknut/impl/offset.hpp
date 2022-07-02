// SPDX-FileCopyrightText: 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <cstdint>

namespace oaknut {

namespace detail {

constexpr std::uint64_t inverse_mask_from_size(std::size_t size)
{
    return (~std::uint64_t{0}) << size;
}

constexpr std::uint64_t mask_from_size(std::size_t size)
{
    return (~std::uint64_t{0}) >> (64 - size);
}

template<std::size_t bit_count>
constexpr std::uint64_t sign_extend(std::uint64_t value)
{
    static_assert(bit_count != 0, "cannot sign-extend zero-sized value");
    constexpr size_t shift_amount = 64 - bit_count;
    return static_cast<std::uint64_t>(static_cast<std::int64_t>(value << shift_amount) >> shift_amount);
}

}  // namespace detail

template<std::size_t bitsize, std::size_t alignment>
struct AddrOffset {
    AddrOffset(std::ptrdiff_t diff)
    {
        const std::uint64_t diff_u64 = static_cast<std::uint64_t>(diff);
        if (detail::sign_extend<bitsize>(diff_u64) != diff_u64)
            throw "out of range";
        if (diff_u64 != (diff_u64 & detail::inverse_mask_from_size(alignment)))
            throw "misalignment";

        m_encoded = static_cast<std::uint32_t>((diff_u64 & detail::mask_from_size(bitsize)) >> alignment);
    }

private:
    friend class CodeGenerator;
    std::uint32_t m_encoded;
};

template<std::size_t bitsize>
struct PageOffset {
    PageOffset(void* ptr)
        : m_ptr(ptr)
    {}

private:
    friend class CodeGenerator;
    void* m_ptr;
};

template<std::size_t bitsize, std::size_t alignment>
struct SOffset {
    SOffset(std::int64_t offset)
    {
        const std::uint64_t diff_u64 = static_cast<std::uint64_t>(offset);
        if (detail::sign_extend<bitsize>(diff_u64) != diff_u64)
            throw "out of range";
        if (diff_u64 != (diff_u64 & detail::inverse_mask_from_size(alignment)))
            throw "misalignment";

        m_encoded = static_cast<std::uint32_t>((diff_u64 & detail::mask_from_size(bitsize)) >> alignment);
    }

private:
    friend class CodeGenerator;
    std::uint32_t m_encoded;
};

template<std::size_t bitsize, std::size_t alignment>
struct POffset {
    POffset(std::int64_t offset)
    {
        const std::uint64_t diff_u64 = static_cast<std::uint64_t>(offset);
        if (diff_u64 > detail::mask_from_size(bitsize))
            throw "out of range";
        if (diff_u64 != (diff_u64 & detail::inverse_mask_from_size(alignment)))
            throw "misalignment";

        m_encoded = static_cast<std::uint32_t>((diff_u64 & detail::mask_from_size(bitsize)) >> alignment);
    }

private:
    friend class CodeGenerator;
    std::uint32_t m_encoded;
};

}  // namespace oaknut
