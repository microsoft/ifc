// Copyright Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef IFC_TOOLING_INCLUDED
#define IFC_TOOLING_INCLUDED

#include <string_view>
#include <string>
#include <filesystem>
#include <vector>

namespace ifc {
    // Filesystem component.  Forward to standard facilities.
    namespace fs = std::filesystem;
}

// Basic datatypes for tooling extension.
namespace ifc::tool {
    // -- Native pathname character type.
    using NativeChar = fs::path::value_type;

    // -- A read-only view type over native strings.
    using StringView = std::basic_string_view<NativeChar>;

    // -- A container type for native string.
    using String = std::basic_string<NativeChar>;

    // -- Type of an IFC subcommand name
    using Name = StringView;

    // -- A container type for the arguments to a subcommand.
    using Arguments = std::vector<StringView>;

    // -- Base class for an ifc subcommand extension.
    struct Extension {
        virtual Name name() const = 0;
        virtual int run_with(const Arguments&) const = 0;
    };
}

#endif
