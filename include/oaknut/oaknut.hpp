// SPDX-FileCopyrightText: 2022 merryhime
// SPDX-License-Identifier: MIT

#include <bit>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "oaknut/impl/enum.hpp"
#include "oaknut/impl/imm.hpp"
#include "oaknut/impl/multi_typed_name.hpp"
#include "oaknut/impl/offset.hpp"
#include "oaknut/impl/reg.hpp"
#include "oaknut/impl/string_literal.hpp"

namespace oaknut {

namespace detail {

template<StringLiteral bs, StringLiteral barg>
constexpr std::uint32_t get_bits()
{
    std::uint32_t result = 0;
    for (std::size_t i = 0; i < 32; i++) {
        for (std::size_t a = 0; a < barg.strlen; a++) {
            if (bs.value[i] == barg.value[a]) {
                result |= 1 << (31 - i);
            }
        }
    }
    return result;
}

constexpr std::uint32_t pdep(std::uint32_t val, std::uint32_t mask)
{
    std::uint32_t res = 0;
    for (std::uint32_t bb = 1; mask; bb += bb) {
        if (val & bb)
            res |= mask & -mask;
        mask &= mask - 1;
    }
    return res;
}

}  // namespace detail

class CodeGenerator {
public:
    explicit CodeGenerator(std::uint32_t* ptr)
        : m_ptr(ptr)
    {}

    template<typename T>
    T ptr()
    {
        static_assert(std::is_pointer_v<T>);
        return reinterpret_cast<T>(m_ptr);
    }

    void set_ptr(std::uint32_t* ptr)
    {
        m_ptr = ptr;
    }

#include "oaknut/impl/arm64_mnemonics.inc"

private:
    template<StringLiteral bs, StringLiteral... bargs, typename... Ts>
    void emit(Ts... args)
    {
        std::uint32_t encoding = detail::get_bits<bs, "1">();
        encoding |= (0 | ... | encode<detail::get_bits<bs, bargs>()>(std::forward<Ts>(args)));

        *m_ptr = encoding;
        m_ptr++;
    }

#define OAKNUT_STD_ENCODE(TYPE, ACCESS, SIZE)                           \
    template<std::uint32_t splat>                                       \
    std::uint32_t encode(TYPE v)                                        \
    {                                                                   \
        static_assert(std::popcount(splat) == SIZE);                    \
        return detail::pdep(static_cast<std::uint32_t>(ACCESS), splat); \
    }

    OAKNUT_STD_ENCODE(RReg, v.index() & 31, 5)
    OAKNUT_STD_ENCODE(XReg, v.index() & 31, 5)
    OAKNUT_STD_ENCODE(WReg, v.index() & 31, 5)
    OAKNUT_STD_ENCODE(XRegSp, v.index() & 31, 5)
    OAKNUT_STD_ENCODE(WRegWsp, v.index() & 31, 5)

    OAKNUT_STD_ENCODE(AddSubImm, v.m_encoded, 13)
    OAKNUT_STD_ENCODE(BitImm32, v.m_encoded, 12)
    OAKNUT_STD_ENCODE(BitImm64, v.m_encoded, 13)
    OAKNUT_STD_ENCODE(LslShift<32>, v.m_amount, 12)
    OAKNUT_STD_ENCODE(LslShift<64>, v.m_amount, 12)

    OAKNUT_STD_ENCODE(Cond, v, 4)
    OAKNUT_STD_ENCODE(AddSubExt, v, 3)
    OAKNUT_STD_ENCODE(IndexExt, v, 3)
    OAKNUT_STD_ENCODE(AddSubShift, v, 2)
    OAKNUT_STD_ENCODE(LogShift, v, 2)
    OAKNUT_STD_ENCODE(PstateField, v, 6)
    OAKNUT_STD_ENCODE(SystemReg, v, 15)
    OAKNUT_STD_ENCODE(AtOp, v, 7)
    OAKNUT_STD_ENCODE(BarrierOp, v, 4)
    OAKNUT_STD_ENCODE(DcOp, v, 10)
    OAKNUT_STD_ENCODE(IcOp, v, 10)
    OAKNUT_STD_ENCODE(PrfOp, v, 5)
    OAKNUT_STD_ENCODE(TlbiOp, v, 10)

    template<std::uint32_t splat>
    std::uint32_t encode(MovImm16 v)
    {
        static_assert(std::popcount(splat) == 17 || std::popcount(splat) == 18);
        constexpr std::uint32_t mask = (1 << std::popcount(splat)) - 1;
        if ((v.m_encoded & mask) != v.m_encoded)
            throw "invalid MovImm16";
        return detail::pdep(v.m_encoded, splat);
    }

    template<std::uint32_t splat, std::size_t imm_size>
    std::uint32_t encode(Imm<imm_size> v)
    {
        static_assert(std::popcount(splat) >= imm_size);
        return detail::pdep(v.value(), splat);
    }

    template<std::uint32_t splat, int A, int B>
    std::uint32_t encode(ImmChoice<A, B> v)
    {
        static_assert(std::popcount(splat) == 1);
        return detail::pdep(v.m_encoded, splat);
    }

    template<std::uint32_t splat, std::size_t size, std::size_t align>
    std::uint32_t encode(AddrOffset<size, align> v)
    {
        static_assert(std::popcount(splat) == size - align);
        return detail::pdep(v.m_encoded, splat);
    }

    template<std::uint32_t splat, std::size_t size>
    std::uint32_t encode(PageOffset<size> v)
    {
        throw "to be implemented";
    }

    template<std::uint32_t splat, std::size_t size, std::size_t align>
    std::uint32_t encode(SOffset<size, align> v)
    {
        static_assert(std::popcount(splat) == size - align);
        return detail::pdep(v.m_encoded, splat);
    }

    template<std::uint32_t splat, std::size_t size, std::size_t align>
    std::uint32_t encode(POffset<size, align> v)
    {
        static_assert(std::popcount(splat) == size - align);
        return detail::pdep(v.m_encoded, splat);
    }

    template<std::uint32_t splat>
    std::uint32_t encode(std::uint32_t v)
    {
        return detail::pdep(v, splat);
    }

#undef OAKNUT_STD_ENCODE

    void addsubext_lsl_correction(AddSubExt& ext, XRegSp)
    {
        if (ext == AddSubExt::LSL)
            ext = AddSubExt::UXTX;
    }
    void addsubext_lsl_correction(AddSubExt& ext, WRegWsp)
    {
        if (ext == AddSubExt::LSL)
            ext = AddSubExt::UXTW;
    }
    void addsubext_lsl_correction(AddSubExt& ext, XReg)
    {
        if (ext == AddSubExt::LSL)
            ext = AddSubExt::UXTX;
    }
    void addsubext_lsl_correction(AddSubExt& ext, WReg)
    {
        if (ext == AddSubExt::LSL)
            ext = AddSubExt::UXTW;
    }

    void addsubext_verify_reg_size(AddSubExt ext, RReg rm)
    {
        if (rm.bitsize() == 32 && (static_cast<int>(ext) & 0b011) != 0b011)
            return;
        if (rm.bitsize() == 64 && (static_cast<int>(ext) & 0b011) == 0b011)
            return;
        throw "invalid AddSubExt choice for rm size";
    }

    void indexext_verify_reg_size(IndexExt ext, RReg rm)
    {
        if (rm.bitsize() == 32 && (static_cast<int>(ext) & 1) == 0)
            return;
        if (rm.bitsize() == 64 && (static_cast<int>(ext) & 1) == 1)
            return;
        throw "invalid IndexExt choice for rm size";
    }

    void tbz_verify_reg_size(RReg rt, Imm<6> imm)
    {
        if (rt.bitsize() == 32 && imm.value() >= 32)
            throw "invalid imm choice for rt size";
    }

    std::uint32_t* m_ptr;
};

namespace util {

inline constexpr WReg W0{0}, W1{1}, W2{2}, W3{3}, W4{4}, W5{5}, W6{6}, W7{7}, W8{8}, W9{9}, W10{10}, W11{11}, W12{12}, W13{13}, W14{14}, W15{15}, W16{16}, W17{17}, W18{18}, W19{19}, W20{20}, W21{21}, W22{22}, W23{23}, W24{24}, W25{25}, W26{26}, W27{27}, W28{28}, W29{29}, W30{30};
inline constexpr XReg X0{0}, X1{1}, X2{2}, X3{3}, X4{4}, X5{5}, X6{6}, X7{7}, X8{8}, X9{9}, X10{10}, X11{11}, X12{12}, X13{13}, X14{14}, X15{15}, X16{16}, X17{17}, X18{18}, X19{19}, X20{20}, X21{21}, X22{22}, X23{23}, X24{24}, X25{25}, X26{26}, X27{27}, X28{28}, X29{29}, X30{30};
inline constexpr ZrReg ZR{};
inline constexpr WzrReg WZR{};
inline constexpr SpReg SP{};
inline constexpr WspReg WSP{};

inline constexpr Cond EQ{Cond::EQ}, NE{Cond::NE}, CS{Cond::CS}, CC{Cond::CC}, MI{Cond::MI}, PL{Cond::PL}, VS{Cond::VS}, VC{Cond::VC}, HI{Cond::HI}, LS{Cond::LS}, GE{Cond::GE}, LT{Cond::LT}, GT{Cond::GT}, LE{Cond::LE}, AL{Cond::AL}, NV{Cond::NV}, HS{Cond::HS}, LO{Cond::LO};

inline constexpr auto UXTB{MultiTypedName<AddSubExt::UXTB>{}};
inline constexpr auto UXTH{MultiTypedName<AddSubExt::UXTH>{}};
inline constexpr auto UXTW{MultiTypedName<AddSubExt::UXTW, IndexExt::UXTW>{}};
inline constexpr auto UXTX{MultiTypedName<AddSubExt::UXTX>{}};
inline constexpr auto SXTB{MultiTypedName<AddSubExt::SXTB>{}};
inline constexpr auto SXTH{MultiTypedName<AddSubExt::SXTH>{}};
inline constexpr auto SXTW{MultiTypedName<AddSubExt::SXTW, IndexExt::SXTW>{}};
inline constexpr auto SXTX{MultiTypedName<AddSubExt::SXTX, IndexExt::SXTX>{}};
inline constexpr auto LSL{MultiTypedName<AddSubExt::LSL, IndexExt::LSL, AddSubShift::LSL, LogShift::LSL>{}};
inline constexpr auto LSR{MultiTypedName<AddSubShift::LSR, LogShift::LSR>{}};
inline constexpr auto ASR{MultiTypedName<AddSubShift::ASR, LogShift::ASR>{}};
inline constexpr auto ROR{MultiTypedName<LogShift::ROR>{}};

inline constexpr PostIndexed POST_INDEXED{};
inline constexpr PreIndexed PRE_INDEXED{};

}  // namespace util

}  // namespace oaknut
