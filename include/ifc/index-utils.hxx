// Copyright Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef IFC_INDEX_UTILS_INCLUDED
#define IFC_INDEX_UTILS_INCLUDED

#if defined(IFC_BUILD_USING_STD_MODULE)
    import std;
#else
    #include <cstdint>
    #include <limits>
    #include <type_traits>
#endif

#include <ifc/assertions.hxx>
#include <ifc/underlying.hxx>

// Provide support infrastructure for common operations for index-like types.
// These are types that have the same representational characteristics as
// the 32-bit unsigned integer type std::uint32_t.
namespace index_like {
    // Useful for generating sentinel values for enumerations.  Typically placeholder for null values.
    template<ifc::Enum E>
    inline constexpr auto sentinel_for = std::numeric_limits<ifc::raw<E>>::max();

    // Check whether the type parameter is std::uint32_t.
    // In fact, it would be acceptable if values of that type
    // promotes to an integer type with rank less than std::uint32_t.  This concept
    // is useful mainly to guard against the anarchic implicit conversions
    // between builtin types.  It is a best-effort attempt to prevent value truncation.
    template<typename S>
    concept Uint32 = std::same_as<S, std::uint32_t>;

    template<typename T>
    concept U32Enum = ifc::Enum<T> and Uint32<ifc::raw<T>>;

    template<U32Enum T>
    inline constexpr std::uint32_t wilderness = sentinel_for<T>;

    // Generic representational index value type.
    // Every other type is representationally isomorphic to this (or std::uint32_t).
    enum class Index : std::uint32_t {};

    // Advance an index by a give amount.
    template<Uint32 T>
    constexpr Index operator+(Index x, T n)
    {
        return Index{ifc::to_underlying(x) + n};
    }

    // -- An index-like type is either a unisorted algebra, typically implemented as
    // -- an enumeration isomorphic to Index, or a multi-sorted algebra, typically
    // -- implemented as an abstract reference class over a given set of sorts.

    // This predicate holds for well-behaved enums that are index-like types.
    template<typename T>
    concept Unisorted = std::is_enum_v<T> and std::same_as<std::underlying_type_t<T>, std::underlying_type_t<Index>>;

    // This predicate holds for any multi-sorted abstract reference type.
    // clang-format off
    template<typename T>
    concept MultiSorted = sizeof(T) == sizeof(Index) and requires(T t) {
        typename T::SortType;
        {t.sort()} -> std::same_as<typename T::SortType>;
        {t.index()} -> std::same_as<Index>;
    };
    // clang-format on

    // This predicates holds if the type parameter is an index-like algebra.
    // Note: well, this is a crude and simple sanity check.  The full accurate
    //       check is left as an exercise to the reader.
    template<typename T>
    concept Algebra = Unisorted<T> or MultiSorted<T>;

    template<Algebra T>
    inline bool null(T t)
    {
        return t == T{};
    }

    // Use the appropriate specialization of this class template as a base class when providing the
    // fiber implementation for a given sort value.  See also Fiber concept.
    template<auto>
    struct SortTag {};

    // Return the sort value associated with fiber.
    // This is an internal/helper function. Don't use directly.  Use algebra_sort instead.
    // Note: Although this declaration has the flavour of a variable template, it offers key properties
    //       different from what a variable template provides:
    //           (a) template argument deduction relying on derived->base conversion, so that
    //                existential quantification can be expressed simply.  E.g. Fiber concept below.
    //           (b) if a data type fails to derive from SortTag<s>, overload resolution fails,
    //               leading to gracious concept-based overloading in the right context.
    template<auto s>
    constexpr auto sort_value(SortTag<s>)
    {
        return s;
    }

    // A fiber is a data type that, for a given sort value, provides a symbolic representation.
    // Note: it should be noted that the sort value is not explicitly provided.
    //       It is computed/inferred from the data type itself.
    //       In mathematical terms: T is a fiber iff exists s . T derives from SortTag<s>
    template<typename T>
    concept Fiber = std::derived_from<T, SortTag<sort_value(T{})>>;

    // This predicate holds for a given data type that is a fiber at a specified sort value.
    template<typename T, auto s>
    concept FiberAt = std::derived_from<T, SortTag<s>>;

    // When a data type is certified as a bona fide fiber, return the sort value derived from that certification
    template<Fiber T>
    inline constexpr auto algebra_sort = sort_value(T{});

    // This predicate holds for any fiber U (an implementation for a given sort) with index type embedded in
    // a multi-sorted algebra V.
    // Note: The value U::algebra_sort gives the sort at the base of the fiber U, i.e. the sort for which the fiber U
    //       is an implementation.
    // clang-format off
    template<typename U, typename V>
    concept FiberEmbedding = MultiSorted<V> and requires {
        {U::algebra_sort} -> std::convertible_to<typename V::SortType>;
    };
    // clang-format on

    // -- The next two functions (rep and per) are conceptual inverse of each other.
    //    The name 'rep' is purposefully kept short (for 'representation') because it
    //    is supposed to be both lower-level (looking under the hood) and not directly
    //    used in interfaces.
    //    Finally, this operator has a well-established pedigree in algebraically-
    //    (and more generally mathematically-) oriented programming languages for
    //    exactly the same purpose.
    //
    //  Convert a value from an index-like type to a canonical abstract representation.
    // Note: This enables a word read of the argument, even if it is of class type.
    template<Algebra T>
    inline Index rep(T t)
    {
        return reinterpret_cast<Index&>(t);
    }

    // Convert a concrete index value to an abstract index-like type.
    template<Algebra T>
    inline T per(Index t)
    {
        return reinterpret_cast<T&>(t);
    }

    inline namespace operators {
        template<Algebra T>
        constexpr auto operator<=>(T x, T y)
        {
            return rep(x) <=> rep(y);
        }
        template<Algebra T>
        constexpr auto operator==(T x, T y)
        {
            return rep(x) == rep(y);
        }
    } // namespace operators

    // For an index-like type over a sort S, this constant holds the number of bits
    // necessary to represent the tags from S.
    // Note: There is no satisfactory concept expression of 'sort' at this point.
    //       Just defining that concept as a check for 'Count' enumerator is not
    //       desirable as that does not capture the notion of sort -- it would only
    //       inappropriately elevate an implementation detail/trick to specification.
    // Note: suffices to subtract 1 from 'Count' since it is 1 greater than the actual largest value.
    template<typename S>
    inline constexpr auto tag_precision =
        ifc::to_underlying(S::Count) == 0 ? 0u : ifc::bit_length(ifc::to_underlying(S::Count) - 1u);

    // For an index-like type T over a sort S, return the number of bits
    // available for representation of indicies over T.
    template<typename S>
    inline constexpr auto index_precision = 32 - tag_precision<S>;

    // Basic representation of an index-like type with specified bit precision
    // for the sort.  This structure is parameterized directly by the sort type
    // parameter, instead of the bit precision parameter, so we can distinguish
    // multiple index-like instances with same value precision.
    template<typename S>
    struct Over {
        using SortType = S;
        constexpr Over() : tag(), value() {}
        constexpr Over(S s, std::uint32_t v) : tag(ifc::to_underlying(s)), value(v) {}

        constexpr S sort() const
        {
            return S(tag);
        }

        constexpr Index index() const
        {
            return Index{value};
        }

    private:
        std::underlying_type_t<Index> tag : tag_precision<S>;
        std::underlying_type_t<Index> value : index_precision<S>;
    };

    template<MultiSorted T, Uint32 V>
    constexpr T make(typename T::SortType s, V v)
    {
        using S = typename T::SortType;
        IFCASSERT(ifc::bit_length(v) <= index_precision<S>);
        return {s, v};
    }

    // Sometimes, an index-like type conceptually represents a (nullable) pointer type.
    // Let's call it a pointed index-like type.  These types have distinguished values
    // that are indicators of absence of values.  The distinguished value does not
    // represent any material value.  Consequently, the concretized value system starts
    // at index 0 while conceptually the materialization starts at 1.
    // So we have two operations: (a) inject(), and (b) retract().
    // The inject() operation maps a concrete index value into the abstract index type space.
    // Conversely, the retract() operation pulls back a concrete index value out of an abstract index type space.
    template<typename T>
    struct pointed {
        template<Uint32 S>
        static T inject(S s)
        {
            IFCASSERT(s < std::numeric_limits<S>::max());
            return T(s + 1);
        }

        static auto retract(T t)
        {
            IFCASSERT(t > T{});
            auto n = ifc::to_underlying(t);
            return --n;
        }

        static auto index(T t)
        {
            return Index{retract(t)};
        }
    };
} // namespace index_like

#endif // IFC_INDEX_UTILS_INCLUDED
