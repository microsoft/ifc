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

    // Minimum and current version components.
    inline constexpr Version MajorVersion { 0 }; 
    inline constexpr Version MinimumMinorVersion { 43 };
    inline constexpr Version CurrentMinorVersion { 44 };

    // Minimum supported file format version
    inline constexpr FormatVersion MinimumFormatVersion{MajorVersion, MinimumMinorVersion};

    // The current version of file format emitted by the toolset
    inline constexpr FormatVersion CurrentFormatVersion{MajorVersion, CurrentMinorVersion};
} // namespace ifc

#endif // IFC_VERSION_INCLUDED
