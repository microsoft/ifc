//
// Microsoft (R) C/C++ Optimizing Compiler Front-End
// Copyright (C) Microsoft. All rights reserved.
//

#ifndef IFC_PATHNAME_INCLUDED
#define IFC_PATHNAME_INCLUDED

#include <cstring>

#include <compare>
#include <string>
#include <string_view>
#include <vector>

namespace ifc {

    // Evaluate to true if the type parameter is a character type.
    template<typename T>
    concept CharType = std::is_same_v<T, char> or
        std::is_same_v<T, unsigned char> or
        std::is_same_v<T, signed char> or
        std::is_same_v<T, char8_t>;

    // Return the number of characters in a C-style NUL-terminated byte string.
    template<CharType T>
    inline std::size_t ntbs_length(const T* s)
    {
        using S = const char*;
        return std::strlen(S(s));
    }

    // Data structure for manipulating path names.  Existing code happily mixes
    // C-style 'const char*' and 'const unsigned char*' at various places for path names.
    // So this type can be 'std::string' without merry type punning all over the place.
    // It can't be current filesystem path either.  It is somewhere a minimal cross.
    // Note: The private inheritance is used as implementation detail to avoid gratuitous
    //       boilerplate, and it is perfectly OK.
    // Note: This data structure uses the standard allocator, for simple reasons:
    //          (1) Most pathnames do not grow once fully constructed/minted.
    //          (2) They have either indefinite lifetime (because they get added to global search paths
    //              of file reference lists), or they are short-lived (because they are used locally
    //              to build paths to query the filesystem.)
    //              But they don't display any elaborated lifetime structure.
    //          (3) The standard allocators are always ready when the global objects are initialized.
    struct Pathname : private std::vector<char8_t> {
        using Base = std::vector<char8_t>;
        using Base::value_type;
        using Base::iterator;
        using Base::const_iterator;

        Pathname()
        {
            Base::push_back({});
        }

        template<CharType T>
        Pathname(const T* s, std::size_t len) : Base{ reinterpret_cast<const value_type*>(s),
                                                      reinterpret_cast<const value_type*>(s) + len }
        {
            Base::push_back({});
        }

        template<CharType T>
        Pathname(const T* s) : Pathname{ s, ntbs_length(s) } { }

        Pathname(std::u8string_view utf8):
            Base{ utf8.begin(),
                  utf8.end() }
        {
            Base::push_back({});
        }

        std::size_t length() const
        {
            return Base::size() - 1;
        }

        bool empty() const
        {
            return length() == 0;
        }

        explicit operator bool() const
        {
            return not empty();
        }

        using Base::begin;

        iterator end()
        {
            return Base::end() - 1;
        }

        const_iterator end() const
        {
            return Base::end() - 1;
        }

        const value_type* c_str() const
        {
            return Base::data();
        }

        value_type back() const
        {
            IFCASSERT(length() > 0);
            return begin()[length() - 1];
        }

        void push_back(value_type c)
        {
            Base::insert(end(), c);
        }

        void append(const value_type* s)
        {
            Base::insert(end(), s, s + ntbs_length(s));
        }

        void append(const Pathname& x)
        {
            Base::insert(end(), x.begin(), x.end());
        }

        const Base& impl() const
        {
            return *this;
        }

        Pathname& minted()
        {
            Base::shrink_to_fit();
            return *this;
        }

        Pathname& extend_with_type(const value_type* ext)
        {
            push_back(u8'.');
            append(ext);
            return minted();
        }

        std::strong_ordering operator<=>(const Pathname&) const = default;
        bool operator==(const Pathname&) const = default;

        bool operator==(std::u8string_view sv) const
        {
            return std::equal(begin(), end(), std::begin(sv), std::end(sv));
        }
    };

} // namespace ifc

#endif // IFC_PATHNAME_INCLUDED