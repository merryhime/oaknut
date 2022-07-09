// SPDX-FileCopyrightText: Copyright (c) 2022 merryhime <https://mary.rs>
// SPDX-License-Identifier: MIT

#include <cstdint>
#include <cstdio>

#include <catch2/catch_test_macros.hpp>

#include "oaknut/code_block.hpp"
#include "oaknut/oaknut.hpp"

TEST_CASE("Basic Test")
{
    using namespace oaknut;
    using namespace oaknut::util;

    CodeBlock mem{4096};
    CodeGenerator code{mem.ptr()};

    mem.unprotect();

    code.MOVZ(W0, 42);
    code.RET(X30);

    mem.protect();
    mem.invalidate_all();

    int result = ((int (*)())mem.ptr())();
    REQUIRE(result == 42);
}
