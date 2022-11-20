// SPDX-FileCopyrightText: Copyright (c) 2022 merryhime <https://mary.rs>
// SPDX-License-Identifier: MIT

#include <cstdint>
#include <cstdio>
#include <limits>

#include <catch2/catch_test_macros.hpp>

#include "oaknut/code_block.hpp"
#include "oaknut/oaknut.hpp"
#include "rand_int.hpp"

using namespace oaknut;
using namespace oaknut::util;

TEST_CASE("Basic Test")
{
    CodeBlock mem{4096};
    CodeGenerator code{mem.ptr()};

    mem.unprotect();

    code.MOV(W0, 42);
    code.RET();

    mem.protect();
    mem.invalidate_all();

    int result = ((int (*)())mem.ptr())();
    REQUIRE(result == 42);
}

TEST_CASE("Fibonacci")
{
    CodeBlock mem{4096};
    CodeGenerator code{mem.ptr()};

    mem.unprotect();

    auto fib = code.ptr<int (*)(int)>();
    Label start, end, zero, recurse;

    code.l(start);
    code.STP(X29, X30, SP, PRE_INDEXED, -32);
    code.STP(X20, X19, SP, 16);
    code.MOV(X29, SP);
    code.MOV(W19, W0);
    code.SUBS(W0, W0, 1);
    code.B(LT, zero);
    code.B(NE, recurse);
    code.MOV(W0, 1);
    code.B(end);

    code.l(zero);
    code.MOV(W0, WZR);
    code.B(end);

    code.l(recurse);
    code.BL(start);
    code.MOV(W20, W0);
    code.SUB(W0, W19, 2);
    code.BL(start);
    code.ADD(W0, W0, W20);

    code.l(end);
    code.LDP(X20, X19, SP, 16);
    code.LDP(X29, X30, SP, POST_INDEXED, 32);
    code.RET();

    mem.protect();
    mem.invalidate_all();

    REQUIRE(fib(0) == 0);
    REQUIRE(fib(1) == 1);
    REQUIRE(fib(5) == 5);
    REQUIRE(fib(9) == 34);
}

TEST_CASE("Immediate generation (32-bit)")
{
    CodeBlock mem{4096};

    for (int i = 0; i < 0x100000; i++) {
        const std::uint32_t value = RandInt<std::uint32_t>(0, 0xffffffff);

        CodeGenerator code{mem.ptr()};

        auto f = code.ptr<std::uint64_t (*)()>();
        mem.unprotect();
        code.MOV(W0, value);
        code.RET();
        mem.protect();
        mem.invalidate_all();

        REQUIRE(f() == value);
    }
}

TEST_CASE("Immediate generation (64-bit)")
{
    CodeBlock mem{4096};

    for (int i = 0; i < 0x100000; i++) {
        const std::uint64_t value = RandInt<std::uint64_t>(0, 0xffffffff'ffffffff);

        CodeGenerator code{mem.ptr()};

        auto f = code.ptr<std::uint64_t (*)()>();
        mem.unprotect();
        code.MOV(X0, value);
        code.RET();
        mem.protect();
        mem.invalidate_all();

        REQUIRE(f() == value);
    }
}

TEST_CASE("ADR")
{
    CodeBlock mem{4096};

    for (std::int64_t i = -1048576; i < 1048576; i++) {
        const std::intptr_t value = reinterpret_cast<std::intptr_t>(mem.ptr()) + i;

        CodeGenerator code{mem.ptr()};

        auto f = code.ptr<std::intptr_t (*)()>();
        mem.unprotect();
        code.ADR(X0, reinterpret_cast<void*>(value));
        code.RET();
        mem.protect();
        mem.invalidate_all();

        INFO(i);
        REQUIRE(f() == value);
    }
}

TEST_CASE("ADRP")
{
    CodeBlock mem{4096};

    for (int i = 0; i < 0x200000; i++) {
        const std::int64_t diff = RandInt<std::int64_t>(-4294967296, 4294967295);
        const std::intptr_t value = reinterpret_cast<std::intptr_t>(mem.ptr()) + diff;
        const std::uint64_t expect = static_cast<std::uint64_t>(value) & ~static_cast<std::uint64_t>(0xfff);

        CodeGenerator code{mem.ptr()};

        auto f = code.ptr<std::uint64_t (*)()>();
        mem.unprotect();
        code.ADRP(X0, reinterpret_cast<void*>(value));
        code.RET();
        mem.protect();
        mem.invalidate_all();

        INFO(i);
        REQUIRE(f() == expect);
    }
}

TEST_CASE("ADRL")
{
    CodeBlock mem{4096};

    for (int i = 0; i < 0x200000; i++) {
        const std::int64_t diff = RandInt<std::int64_t>(-4294967296, 4294967295);
        const std::intptr_t value = reinterpret_cast<std::intptr_t>(mem.ptr()) + diff;

        CodeGenerator code{mem.ptr()};

        auto f = code.ptr<std::uint64_t (*)()>();
        mem.unprotect();
        code.ADRL(X0, reinterpret_cast<void*>(value));
        code.RET();
        mem.protect();
        mem.invalidate_all();

        INFO(i);
        REQUIRE(f() == static_cast<std::uint64_t>(value));
    }
}

TEST_CASE("MOVP2R")
{
    CodeBlock mem{4096};

    for (int i = 0; i < 0x200'0000; i++) 
    {
        const std::int64_t diff = RandInt<std::int64_t>(std::numeric_limits<std::int64_t>::min(), 
                                                        std::numeric_limits<std::int64_t>::max());
        const std::intptr_t value = reinterpret_cast<std::intptr_t>(mem.ptr()) + diff;

        CodeGenerator code{mem.ptr()};

        auto f = code.ptr<std::uint64_t (*)()>();
        mem.unprotect();
        code.MOVP2R(X0, reinterpret_cast<void*>(value));
        code.RET();
        mem.protect();
        mem.invalidate_all();

        REQUIRE(f() == static_cast<std::uint64_t>(value));
    }
}
