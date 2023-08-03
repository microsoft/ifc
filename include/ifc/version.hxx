// Copyright Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef IFC_VERSION_INCLUDED
#define IFC_VERSION_INCLUDED

#include <cstdint>

#include <compare>

namespace ifc {
    // File format versioning description
    enum class Version : std::uint8_t {};
    struct FormatVersion {
        Version major;
        Version minor;

        auto operator<=>(const FormatVersion&) const = default;
    };

    // Minimum supported file format version
    inline constexpr FormatVersion MinimumFormatVersion{Version{0}, Version{43}};

    // The current version of file format emitted by the toolset
    inline constexpr FormatVersion CurrentFormatVersion = MinimumFormatVersion;

    // The version of file format understood by the latest EDG drop
    inline constexpr FormatVersion EDGFormatVersion{Version{0}, Version{42}};
} // namespace ifc

#endif // IFC_VERSION_INCLUDED
