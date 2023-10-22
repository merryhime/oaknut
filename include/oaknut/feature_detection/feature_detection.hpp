// SPDX-FileCopyrightText: Copyright (c) 2023 merryhime <https://mary.rs>
// SPDX-License-Identifier: MIT

#pragma once

#if defined(__APPLE__)
#    define OAKNUT_CPU_FEATURE_DETECTION 1
#    include "oaknut/feature_detection/feature_detection_apple.hpp"
#elif defined(__linux__)
#    define OAKNUT_CPU_FEATURE_DETECTION 1
#    include "oaknut/feature_detection/feature_detection_linux.hpp"
#elif defined(__FreeBSD__)
#    define OAKNUT_CPU_FEATURE_DETECTION 1
#    include "oaknut/feature_detection/feature_detection_freebsd.hpp"
#elif defined(_WIN32)
#    define OAKNUT_CPU_FEATURE_DETECTION 1
#    include "oaknut/feature_detection/feature_detection_w32.hpp"
#else
#    define OAKNUT_CPU_FEATURE_DETECTION 0
#    warning "Unsupported operating system for CPU feature detection"
#    include "oaknut/feature_detection/feature_detection_generic.hpp"
#endif
