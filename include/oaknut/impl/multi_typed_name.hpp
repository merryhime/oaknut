// SPDX-FileCopyrightText: 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

namespace oaknut {

template<auto... Vs>
struct MultiTypedName;

template<>
struct MultiTypedName<> {};

template<auto V, auto... Vs>
struct MultiTypedName<V, Vs...> : public MultiTypedName<Vs...> {
    constexpr operator decltype(V)() const { return V; }
};

}  // namespace oaknut
