// SPDX-FileCopyrightText: Copyright (c) 2023 merryhime <https://mary.rs>
// SPDX-License-Identifier: MIT

#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#include <processthreadsapi.h>

#include "oaknut/feature_detection/cpu_feature.hpp"

namespace oaknut {

// Ref: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-isprocessorfeaturepresent

inline CpuFeatures detect_features_via_IsProcessorFeaturePresent()
{
    CpuFeatures result;

    if (::IsProcessorFeaturePresent(30))  // PF_ARM_V8_CRYPTO_INSTRUCTIONS_AVAILABLE
        result |= CpuFeatures{CpuFeature::AES, CpuFeature::PMULL, CpuFeature::SHA1, CpuFeature::SHA256};
    if (::IsProcessorFeaturePresent(31))  // PF_ARM_V8_CRC32_INSTRUCTIONS_AVAILABLE
        result |= CpuFeatures{CpuFeature::CRC32};
    if (::IsProcessorFeaturePresent(34))  // PF_ARM_V81_ATOMIC_INSTRUCTIONS_AVAILABLE
        result |= CpuFeatures{CpuFeature::LSE};
    if (::IsProcessorFeaturePresent(43))  // PF_ARM_V82_DP_INSTRUCTIONS_AVAILABLE
        result |= CpuFeatures{CpuFeature::DotProd};
    if (::IsProcessorFeaturePresent(44))  // PF_ARM_V83_JSCVT_INSTRUCTIONS_AVAILABLE
        result |= CpuFeatures{CpuFeature::JSCVT};
    if (::IsProcessorFeaturePresent(45))  // PF_ARM_V83_LRCPC_INSTRUCTIONS_AVAILABLE
        result |= CpuFeatures{CpuFeature::LRCPC};

    return result;
}

inline CpuFeatures detect_features()
{
    CpuFeatures result{CpuFeature::FP, CpuFeature::ASIMD};
    result |= detect_features_via_IsProcessorFeaturePresent();
    return result;
}

}  // namespace oaknut
