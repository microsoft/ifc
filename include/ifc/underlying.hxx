// Copyright Microsoft.

#ifndef IFC_UNDERLYING_H
#define IFC_UNDERLYING_H

#include <utility>
#include <bit>

namespace ifc {
    // Consider moving to std::to_underlying (C++23) at some point.
    template <class Enum>
    constexpr std::underlying_type_t<Enum> to_underlying(Enum e) noexcept
    {
        return static_cast<std::underlying_type_t<Enum>>(e);
    }

    // The minimum number of bits necessary to represent a whole number, where zero takes 1 bit.
    template <std::unsigned_integral T>
    constexpr unsigned int bit_length(T n) noexcept
    {
        return (n == 0) ? 1u : static_cast<unsigned int>(std::bit_width(n));
    }

    template <typename T>
    constexpr bool implies(T x, T y) noexcept
    {
        return (to_underlying(x) & to_underlying(y)) == to_underlying(y);
    }
} // namespace ifc
#endif // IFC_UNDERLYING_H
