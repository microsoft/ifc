//
// Microsoft (R) C/C++ Optimizing Compiler Front-End
// Copyright (C) Microsoft Corp 1984-2017. All rights reserved.
//

#ifndef UTILITY_ENUM_UTIL
#define UTILITY_ENUM_UTIL

// Some utility functions to make using enumerations (and bitmask types implemented as scoped enums) easier.

#include <type_traits>
#include <concepts>
#include <bit>

namespace bits {

    // Raw underlying integer type of an enumeration.
    template<typename T>
    using raw = std::underlying_type_t<T>;

    template <typename T>
    concept Enum = std::is_enum_v<T>;

    // Return the underlying integer value representation of an enumerator, with the appropriate type.
    template<Enum T>
    __declspec(non_user_code) constexpr raw<T> rep(T t) noexcept
    {
        return raw<T>(t);
    }
    // For symmetry, define 'rep' for integral types (can be called with either enum or integral). AKA "identity".
    template<std::integral T>
    __declspec(non_user_code) constexpr T rep(T t) noexcept
    {
        return t;
    }

    // The minimum number of bits necessary to represent a whole number, where zero takes 1 bit.
    template<std::unsigned_integral T>
    constexpr unsigned int length(T n) noexcept
    {
        return (n == 0) ? 1u : static_cast<unsigned int>(std::bit_width(n));
    }

    // The unit element is the first enumerator of an enum in practice, but it conceptually
    // models the identity or "neutral" element.
    template<Enum T>
    constexpr T unit = T{};

    template<Enum T>
    constexpr bool is_unit(T t)
    {
        return t == unit<T>;
    }

    namespace detail {
        struct MustBeNonZeroBit { };
        struct MustBePowerOf2 { };
        // This class validates at compile-time that an enumerator of some enumeration
        // is a bit in a bitset (i.e. it is some power of 2).
        template <Enum T>
        struct Bit
        {
            consteval Bit(T bit):
                bit{ bit }
            {
                if (is_unit<T>(bit))
                    throw MustBeNonZeroBit{};
                if (not power_of_2(rep(bit)))
                    throw MustBePowerOf2{};
            }

            template <std::unsigned_integral U>
            static constexpr bool power_of_2(U v)
            {
                if (v == 0)
                    return false;
                return (v & (v - 1)) == 0;
            }

            T bit;
        };
    }

    template <Enum T>
    using BitValidator = std::type_identity_t<detail::Bit<T>>;
}

template<bits::Enum T>
[[nodiscard]] constexpr T operator|(T e1, T e2) noexcept
{
    return static_cast<T>(bits::rep(e1) | bits::rep(e2));
}

template<bits::Enum T>
[[nodiscard]] constexpr T operator&(T e1, T e2) noexcept
{
    return static_cast<T>(bits::rep(e1) & bits::rep(e2));
}

template<bits::Enum T>
[[nodiscard]] constexpr T operator^(T e1, T e2) noexcept
{
    return static_cast<T>(bits::rep(e1) ^ bits::rep(e2));
}

template<bits::Enum T>
[[nodiscard]] constexpr T operator~(T e) noexcept
{
    return static_cast<T>(~bits::rep(e));
}

// If 'b' then return 'e', else return T{} (default value).
template <typename T>
auto select_if(bool b, T e)
{
    return b ? e : T{};
}

template <bits::Enum T>
[[nodiscard]] constexpr T set_bit(T v, bits::BitValidator<T> b, bool value)
{
    if (value)
        return v | b.bit;
    return v & ~b.bit;
}

template<bits::Enum T>
constexpr bool implies(T x, T y) noexcept
{
    return (x & y) == y;
}

// Doesn't logically belong here but it's good to keep overloads together.
constexpr inline bool implies(bool a, bool b) noexcept
{
    return !a || b;
}

template<bits::Enum T>
constexpr bool implies_any_of(T x, T y) noexcept
{
    return (x & y) != bits::unit<T>;
}

template <bits::Enum T>
[[nodiscard]] constexpr T extend(T x, bits::raw<T> y = 1) noexcept
{
    return T(bits::rep(x) + y);
}

template <bits::Enum T>
[[nodiscard]] constexpr T retract(T x, bits::raw<T> y = 1) noexcept
{
    return T(bits::rep(x) - y);
}

template<bits::Enum T>
[[nodiscard]] constexpr T add_flags(T x, T y) noexcept
{
    return x | y;
}

template<bits::Enum T>
constexpr T& operator|=(T& e1, T e2) noexcept
{
    return e1 = e1 | e2;
}

template<bits::Enum T>
constexpr T& operator&=(T& e1, T e2) noexcept
{
    return e1 = e1 & e2;
}

template<bits::Enum T>
constexpr T& operator^=(T& e1, T e2) noexcept
{
    return e1 = e1 ^ e2;
}

// Helper functions for wrapper enums with Yes/No enumerators used in
// place of raw bool parameters.

// Conceptually, a Yes/No answer type used in MSVC is an enumeration which
// follows the pattern that T::Yes is true and T::No is false.
template <typename T>
concept YesNoEnum = bits::Enum<T>
                    && std::same_as<bits::raw<T>, bool>
                    && bits::rep(T::Yes) == true
                    && bits::rep(T::No) == false;

template <YesNoEnum T>
__declspec(non_user_code)
constexpr bool is_yes(T e) noexcept
{
    return e == T::Yes;
}

template <YesNoEnum T>
__declspec(non_user_code)
constexpr bool is_no(T e) noexcept
{
    return e == T::No;
}

template <YesNoEnum T>
__declspec(non_user_code)
constexpr T make_yes_no(bool value) noexcept
{
    return static_cast<T>(value);
}

// Return a bitmask for bits in the range [start, end]. Useful in enumerator values for `flags' enums.
// Range for both indices is 0..wordsize-1.
template<typename T>
constexpr T bitmask(T start, T end)
{
    return ((((T(1) << end) - 1) << 1) + 1) & ~(((T(1) << start) - 1));
}


#endif // UTILITY_ENUM_UTIL
