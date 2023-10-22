// SPDX-FileCopyrightText: Copyright (c) 2022 merryhime <https://mary.rs>
// SPDX-License-Identifier: MIT

#include <cstdio>

#include <catch2/catch_test_macros.hpp>

#include "oaknut/feature_detection/feature_detection.hpp"

using namespace oaknut;

TEST_CASE("Print CPU features")
{
    CpuFeatures features = detect_features();

    std::fputs("CPU Features: ", stdout);

#define OAKNUT_CPU_FEATURE(name)        \
    if (features.has(CpuFeature::name)) \
        std::fputs(#name " ", stdout);
#include "oaknut/impl/cpu_feature.inc.hpp"
#undef OAKNUT_CPU_FEATURE

    std::fputs("\n", stdout);
}
