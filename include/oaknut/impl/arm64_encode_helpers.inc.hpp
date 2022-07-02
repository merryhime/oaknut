// SPDX-FileCopyrightText: 2022 merryhime
// SPDX-License-Identifier: MIT

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
