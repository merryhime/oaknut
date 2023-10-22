// SPDX-FileCopyrightText: Copyright (c) 2023 merryhime <https://mary.rs>
// SPDX-License-Identifier: MIT

#pragma once

#include "oaknut/feature_detection/cpu_feature.hpp"

namespace oaknut {

inline CpuFeatures detect_features()
{
    return CpuFeatures{CpuFeature::FP, CpuFeature::ASIMD};
}

}  // namespace oaknut
