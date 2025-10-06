// Copyright Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef IFC_UNDERLYING_H
#define IFC_UNDERLYING_H

#if defined(IFC_BUILD_USING_STD_MODULE)
    import std;
#else
    #include <bit>
    #include <type_traits>
    #include <utility>
#endif

namespace ifc {
    template <typename T>
    concept Enum = std::is_enum_v<T>;

    // Raw underlying integer type of an enumeration.
    template<Enum T>
    using raw = std::underlying_type_t<T>;

    // Consider moving to std::to_underlying (C++23) at some point.
    template<Enum T>
    constexpr raw<T> to_underlying(T e) noexcept
    {
        return static_cast<raw<T>>(e);
    }

    // The minimum number of bits necessary to represent a whole number, where zero takes 1 bit.
    template<std::unsigned_integral T>
    constexpr unsigned int bit_length(T n) noexcept
    {
        return (n == 0) ? 1u : static_cast<unsigned int>(std::bit_width(n));
    }

    // FIXME: This should be constrained with 'Enum', but due to an overlapping definition inside the msvc codebase, this needs to remain unconstrained.
    // so it does not compete with the copy in the msvc codebase which is constrained with 'Enum' during overload resolution.
    template<typename T>
    constexpr bool implies(T x, T y) noexcept
    {
        return (to_underlying(x) & to_underlying(y)) == to_underlying(y);
    }
} // namespace ifc
#endif // IFC_UNDERLYING_H
