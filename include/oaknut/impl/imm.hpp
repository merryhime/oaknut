// SPDX-FileCopyrightText: 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

#include <bit>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace oaknut {

template<std::size_t bit_size_>
struct Imm {
public:
    static_assert(bit_size_ != 0 && bit_size_ <= 32, "Invalid bit_size");
    static constexpr std::size_t bit_size = bit_size_;
    static constexpr std::uint32_t mask = (1 << bit_size) - 1;

    constexpr /* implicit */ Imm(std::uint32_t value_)
        : m_value(value_)
    {
        if (!is_valid(value_))
            throw "outsized Imm value";
    }

    constexpr auto operator<=>(const Imm& other) const { return m_value <=> other.m_value; }
    constexpr auto operator<=>(std::uint32_t other) const { return operator<=>(Imm{other}); }

    constexpr std::uint32_t value() const { return m_value; }

    static bool is_valid(std::uint32_t value_)
    {
        return ((value_ & mask) == value_);
    }

private:
    template<typename Policy>
    friend class BasicCodeGenerator;
    std::uint32_t m_value;
};

enum class AddSubImmShift {
    SHL_0,
    SHL_12,
};

struct AddSubImm {
public:
    constexpr AddSubImm(std::uint32_t value_, AddSubImmShift shift_)
        : m_encoded(value_ | ((shift_ == AddSubImmShift::SHL_12) ? 1 << 12 : 0))
    {
        if ((value_ & 0xFFF) != value_)
            throw "invalid AddSubImm";
    }

    constexpr /* implicit */ AddSubImm(std::uint64_t value_)
    {
        if ((value_ & 0xFFF) == value_) {
            m_encoded = value_;
        } else if ((value_ & 0xFFF000) == value_) {
            m_encoded = (value_ >> 12) | (1 << 12);
        } else {
            throw "invalid AddSubImm";
        }
    }

    static constexpr bool is_valid(std::uint64_t value_)
    {
        return ((value_ & 0xFFF) == value_) || ((value_ & 0xFFF000) == value_);
    }

private:
    template<typename Policy>
    friend class BasicCodeGenerator;
    std::uint32_t m_encoded;
};

enum class MovImm16Shift {
    SHL_0,
    SHL_16,
    SHL_32,
    SHL_48,
};

struct MovImm16 {
public:
    MovImm16(std::uint16_t value_, MovImm16Shift shift_)
        : m_encoded(static_cast<std::uint32_t>(value_) | (static_cast<std::uint32_t>(shift_) << 16))
    {}

    constexpr /* implict */ MovImm16(std::uint64_t value_)
    {
        std::uint32_t shift = 0;
        while (value_ != 0) {
            const std::uint32_t lsw = static_cast<std::uint16_t>(value_ & 0xFFFF);
            if (value_ == lsw) {
                m_encoded = lsw | (shift << 16);
                return;
            } else if (lsw != 0) {
                throw "invalid MovImm16";
            }
            value_ >>= 16;
            shift++;
        }
    }

    static constexpr bool is_valid(std::uint64_t value_)
    {
        return ((value_ & 0xFFFF) == value_) || ((value_ & 0xFFFF0000) == value_) || ((value_ & 0xFFFF00000000) == value_) || ((value_ & 0xFFFF000000000000) == value_);
    }

private:
    template<typename Policy>
    friend class BasicCodeGenerator;
    std::uint32_t m_encoded;
};

namespace detail {

constexpr std::uint64_t mask_from_esize(std::size_t esize)
{
    return (~std::uint64_t{0}) >> (64 - esize);
}

constexpr std::uint64_t inverse_mask_from_trailing_ones(std::uint64_t value)
{
    return ~value | (value + 1);
}

constexpr std::uint64_t is_contiguous_mask_from_lsb(std::uint64_t value)
{
    return value && ((value + 1) & value) == 0;
}

constexpr std::optional<std::uint32_t> encode_bit_imm(std::uint64_t value)
{
    if (value == 0 || (~value) == 0)
        return std::nullopt;

    const std::size_t esize = [value] {
        for (std::size_t esize = 64; esize > 1; esize /= 2) {
            if (value != std::rotr(value, esize / 2)) {
                return esize;
            }
        }
        return std::size_t{0};
    }();

    if (esize == 0)
        return std::nullopt;

    const std::size_t rotation = std::countr_zero(value & inverse_mask_from_trailing_ones(value));

    const std::uint64_t emask = mask_from_esize(esize);
    const std::uint64_t rot_element = std::rotr(value, rotation) & emask;

    if (!is_contiguous_mask_from_lsb(rot_element))
        return std::nullopt;

    const std::uint32_t S = ((-esize) << 1) | (std::popcount(rot_element) - 1);
    const std::uint32_t R = (esize - rotation) & (esize - 1);
    const std::uint32_t N = (~S >> 6) & 1;

    return static_cast<std::uint32_t>((S & 0b111111) | (R << 6) | (N << 12));
}

constexpr std::optional<std::uint32_t> encode_bit_imm(std::uint32_t value)
{
    const std::uint64_t value_u64 = (static_cast<std::uint64_t>(value) << 32) | static_cast<std::uint64_t>(value);
    const auto result = encode_bit_imm(value_u64);
    if (result && (*result & 0b0'111111'111111) != *result)
        return std::nullopt;
    return result;
}

}  // namespace detail

struct BitImm32 {
public:
    constexpr BitImm32(Imm<6> imms, Imm<6> immr)
        : m_encoded((imms.value() << 6) | immr.value())
    {}

    constexpr /* implicit */ BitImm32(std::uint32_t value)
    {
        const auto encoded = detail::encode_bit_imm(value);
        if (!encoded || (*encoded & 0x1000) != 0)
            throw "invalid BitImm32";
        m_encoded = *encoded;
    }

private:
    template<typename Policy>
    friend class BasicCodeGenerator;
    std::uint32_t m_encoded;
};

struct BitImm64 {
public:
    constexpr BitImm64(bool N, Imm<6> imms, Imm<6> immr)
        : m_encoded((N ? 1 << 12 : 0) | (imms.value() << 6) | immr.value())
    {}

    constexpr /* implicit */ BitImm64(std::uint64_t value)
    {
        const auto encoded = detail::encode_bit_imm(value);
        if (!encoded)
            throw "invalid BitImm64";
        m_encoded = *encoded;
    }

private:
    template<typename Policy>
    friend class BasicCodeGenerator;
    std::uint32_t m_encoded;
};

template<int A, int B>
struct ImmChoice {
    constexpr /* implicit */ ImmChoice(int value)
    {
        if (value == A) {
            m_encoded = 0;
        } else if (value == B) {
            m_encoded = 1;
        } else {
            throw "invalid ImmChoice";
        }
    }

private:
    template<typename Policy>
    friend class BasicCodeGenerator;
    std::uint32_t m_encoded;
};

template<std::size_t max_value>
struct LslShift {
    constexpr /* implicit */ LslShift(std::size_t amount)
        : m_encoded((((-amount) & (max_value - 1)) << 6) | (max_value - amount - 1))
    {
        if (amount >= max_value)
            throw "LslShift out of range";
    }

private:
    template<typename Policy>
    friend class BasicCodeGenerator;
    std::uint32_t m_encoded;
};

}  // namespace oaknut
