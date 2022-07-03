// SPDX-FileCopyrightText: 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>

namespace oaknut {

struct Reg;
struct RReg;
struct ZrReg;
struct WzrReg;
struct XReg;
struct WReg;
struct SpReg;
struct WspReg;
struct XRegSp;
struct XRegWsp;

struct Reg {
    constexpr explicit Reg(bool is_vector_, unsigned bitsize_, int index_)
        : m_index(index_)
        , m_bitsize(bitsize_)
        , m_is_vector(is_vector_)
    {
        assert(index_ >= -1 && index_ <= 31);
        assert(bitsize_ != 0 && (bitsize_ & (bitsize_ - 1)) == 0 && "Bitsize must be a power of two");
    }

    constexpr int index() const { return m_index; }
    constexpr unsigned bitsize() const { return m_bitsize; }
    constexpr bool is_vector() const { return m_is_vector; }

private:
    int m_index : 8;
    unsigned m_bitsize : 8;
    bool m_is_vector;
};

struct RReg : public Reg {
    constexpr explicit RReg(unsigned bitsize_, int index_)
        : Reg(false, bitsize_, index_)
    {
        assert(bitsize_ == 32 || bitsize_ == 64);
    }

    XReg toX() const;
    WReg toW() const;

    template<typename Policy>
    friend class BasicCodeGenerator;
};

struct ZrReg : public RReg {
    constexpr explicit ZrReg()
        : RReg(64, 31) {}
};

struct WzrReg : public RReg {
    constexpr explicit WzrReg()
        : RReg(32, 31) {}
};

struct XReg : public RReg {
    constexpr explicit XReg(int index_)
        : RReg(64, index_) {}

    constexpr /* implicit */ XReg(ZrReg)
        : RReg(64, 31) {}

    template<typename Policy>
    friend class BasicCodeGenerator;
};

struct WReg : public RReg {
    constexpr explicit WReg(int index_)
        : RReg(32, index_) {}

    constexpr /* implicit */ WReg(WzrReg)
        : RReg(32, 31) {}

    template<typename Policy>
    friend class BasicCodeGenerator;
};

inline XReg RReg::toX() const
{
    if (index() == -1)
        throw "cannot convert SP/WSP to XReg";
    return XReg{index()};
}

inline WReg RReg::toW() const
{
    if (index() == -1)
        throw "cannot convert SP/WSP to WReg";
    return WReg{index()};
}

struct SpReg : public RReg {
    constexpr explicit SpReg()
        : RReg(64, -1) {}
};

struct WspReg : public RReg {
    constexpr explicit WspReg()
        : RReg(64, -1) {}
};

struct XRegSp : public RReg {
    constexpr /* implict */ XRegSp(SpReg)
        : RReg(64, -1) {}

    constexpr /* implict */ XRegSp(XReg xr)
        : RReg(64, xr.index())
    {
        if (xr.index() == 31)
            throw "unexpected ZR passed into an XRegSp";
    }

    template<typename Policy>
    friend class BasicCodeGenerator;
};

struct WRegWsp : public RReg {
    constexpr /* implict */ WRegWsp(WspReg)
        : RReg(32, -1) {}

    constexpr /* implict */ WRegWsp(WReg wr)
        : RReg(32, wr.index())
    {
        if (wr.index() == 31)
            throw "unexpected WZR passed into an WRegWsp";
    }

    template<typename Policy>
    friend class BasicCodeGenerator;
};

}  // namespace oaknut
