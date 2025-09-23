// Copyright Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef IFC_BASIC_TYPES
#define IFC_BASIC_TYPES

#include <ifc/underlying.hxx>

namespace ifc {
    enum class ColumnNumber : std::int32_t {
        Invalid = -1,
    };
    enum LineNumber : std::int32_t {
        Max = 0x00ffffff,
    };

    // Return the next line number.
    constexpr LineNumber next(LineNumber n)
    {
        return LineNumber{ifc::to_underlying(n) + 1};
    }

    // Return the previous line number.
    constexpr LineNumber previous(LineNumber n)
    {
        return LineNumber{ifc::to_underlying(n) - 1};
    }

    // Return the next column number.
    constexpr ColumnNumber next(ColumnNumber n)
    {
        return ColumnNumber{ifc::to_underlying(n) + 1};
    }
} // namespace ifc

#endif // IFC_BASIC_TYPES
