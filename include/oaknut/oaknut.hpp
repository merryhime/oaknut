// SPDX-FileCopyrightText: 2022 merryhime
// SPDX-License-Identifier: MIT

#include <bit>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <tuple>
#include <type_traits>
#include <variant>
#include <vector>

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

template<class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

}  // namespace detail

struct Label {
public:
    Label() = default;

private:
    template<typename Policy>
    friend class BasicCodeGenerator;

    explicit Label(std::uintptr_t addr)
        : m_addr(addr)
    {}

    using EmitFunctionType = std::uint32_t (*)(std::uintptr_t wb_addr, std::uintptr_t resolved_addr);

    struct Writeback {
        std::uintptr_t m_wb_addr;
        std::uint32_t m_mask;
        EmitFunctionType m_fn;
    };

    std::optional<std::uintptr_t> m_addr;
    std::vector<Writeback> m_wbs;
};

template<typename Policy>
class BasicCodeGenerator : public Policy {
public:
    BasicCodeGenerator(typename Policy::constructor_argument_type arg)
        : Policy(arg)
    {}

    Label l()
    {
        return Label{Policy::current_address()};
    }

    void l(Label& label)
    {
        if (label.m_addr)
            throw "label already resolved";

        const auto target_addr = Policy::current_address();
        label.m_addr = target_addr;
        for (auto& wb : label.m_wbs) {
            const std::uint32_t value = wb.m_fn(wb.m_wb_addr, target_addr);
            Policy::set_at_address(wb.m_wb_addr, value, wb.m_mask);
        }
        label.m_wbs.clear();
    }

#include "oaknut/impl/arm64_mnemonics.inc.hpp"

private:
#include "oaknut/impl/arm64_encode_helpers.inc.hpp"

    template<StringLiteral bs, StringLiteral... bargs, typename... Ts>
    void emit(Ts... args)
    {
        std::uint32_t encoding = detail::get_bits<bs, "1">();
        encoding |= (0 | ... | encode<detail::get_bits<bs, bargs>()>(std::forward<Ts>(args)));
        Policy::append(encoding);
    }

    template<std::uint32_t splat, std::size_t size, std::size_t align>
    std::uint32_t encode(AddrOffset<size, align> v)
    {
        static_assert(std::popcount(splat) == size - align);

        const auto encode_fn = [](std::uintptr_t current_addr, std::uintptr_t target) {
            const std::ptrdiff_t diff = target - current_addr;
            return pdep<splat>(AddrOffset<size, align>::encode(diff));
        };

        return std::visit(detail::overloaded{
                              [&](std::uint32_t encoding) {
                                  return pdep<splat>(encoding);
                              },
                              [&](Label* label) {
                                  if (label->m_addr) {
                                      return encode_fn(Policy::current_address(), *label->m_addr);
                                  }

                                  label->m_wbs.emplace_back(Label::Writeback{Policy::current_address(), ~splat, static_cast<Label::EmitFunctionType>(encode_fn)});
                                  return 0u;
                              },
                              [&](void* p) {
                                  return encode_fn(Policy::current_address(), reinterpret_cast<std::uintptr_t>(p));
                              },
                          },
                          v.m_payload);
    }

    template<std::uint32_t splat, std::size_t size>
    std::uint32_t encode(PageOffset<size> v)
    {
        static_assert(std::popcount(splat) == size);

        const auto encode_fn = [](std::uintptr_t current_addr, std::uintptr_t target) {
            return pdep<splat>(PageOffset<size>::encode(current_addr, target));
        };

        return std::visit(detail::overloaded{
                              [&](Label* label) {
                                  if (label->m_addr) {
                                      return encode_fn(Policy::current_address(), *label->m_addr);
                                  }

                                  label->m_wbs.emplace_back(Label::Writeback{Policy::current_address(), ~splat, static_cast<Label::EmitFunctionType>(encode_fn)});
                                  return 0u;
                              },
                              [&](void* p) {
                                  return encode_fn(Policy::current_address(), reinterpret_cast<std::uintptr_t>(p));
                              },
                          },
                          v.m_payload);
    }
};

struct PointerCodeGeneratorPolicy {
public:
    template<typename T>
    T ptr()
    {
        static_assert(std::is_pointer_v<T>);
        return reinterpret_cast<T>(m_ptr);
    }

    void set_ptr(std::uint32_t* ptr_)
    {
        m_ptr = ptr_;
    }

protected:
    using constructor_argument_type = std::uint32_t*;

    PointerCodeGeneratorPolicy(std::uint32_t* ptr_)
        : m_ptr(ptr_)
    {}

    void append(std::uint32_t instruction)
    {
        *m_ptr++ = instruction;
    }

    std::uintptr_t current_address()
    {
        return reinterpret_cast<std::uintptr_t>(m_ptr);
    }

    void set_at_address(std::uintptr_t addr, std::uint32_t value, std::uint32_t mask)
    {
        std::uint32_t* p = reinterpret_cast<std::uint32_t*>(addr);
        *p = (*p & mask) | value;
    }

private:
    std::uint32_t* m_ptr;
};

using CodeGenerator = BasicCodeGenerator<PointerCodeGeneratorPolicy>;

namespace util {

inline constexpr WReg W0{0}, W1{1}, W2{2}, W3{3}, W4{4}, W5{5}, W6{6}, W7{7}, W8{8}, W9{9}, W10{10}, W11{11}, W12{12}, W13{13}, W14{14}, W15{15}, W16{16}, W17{17}, W18{18}, W19{19}, W20{20}, W21{21}, W22{22}, W23{23}, W24{24}, W25{25}, W26{26}, W27{27}, W28{28}, W29{29}, W30{30};
inline constexpr XReg X0{0}, X1{1}, X2{2}, X3{3}, X4{4}, X5{5}, X6{6}, X7{7}, X8{8}, X9{9}, X10{10}, X11{11}, X12{12}, X13{13}, X14{14}, X15{15}, X16{16}, X17{17}, X18{18}, X19{19}, X20{20}, X21{21}, X22{22}, X23{23}, X24{24}, X25{25}, X26{26}, X27{27}, X28{28}, X29{29}, X30{30};
inline constexpr ZrReg ZR{}, XZR{};
inline constexpr WzrReg WZR{};
inline constexpr SpReg SP{}, XSP{};
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
inline constexpr auto LSL{MultiTypedName<AddSubExt::LSL, IndexExt::LSL, AddSubShift::LSL, LogShift::LSL, LslSymbol::LSL>{}};
inline constexpr auto LSR{MultiTypedName<AddSubShift::LSR, LogShift::LSR>{}};
inline constexpr auto ASR{MultiTypedName<AddSubShift::ASR, LogShift::ASR>{}};
inline constexpr auto ROR{MultiTypedName<LogShift::ROR>{}};

inline constexpr PostIndexed POST_INDEXED{};
inline constexpr PreIndexed PRE_INDEXED{};

}  // namespace util

}  // namespace oaknut
