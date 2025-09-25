// Copyright Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef IFC_TOOLING_INCLUDED
#define IFC_TOOLING_INCLUDED

#if defined(IFC_BUILD_USING_STD_MODULE)
    import std;
#else
    #include <span>
    #include <string_view>
    #include <string>
    #include <filesystem>
    #include <vector>
#endif

#include <gsl/span>

namespace ifc {
    // Filesystem component.  Forward to standard facilities.
    namespace fs = std::filesystem;
}

// Basic datatypes for tooling extension.
namespace ifc::tool {
    // -- String type preferred by the host OS to specify pathnames.
    using SystemPath = std::filesystem::path::string_type;

    // -- Native pathname character type.
    using NativeChar = SystemPath::value_type;

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

    // -- Type for the error code values used by the host OS.
#ifdef _WIN32
    using ErrorCode = unsigned long;    // DWORD
#else
    using ErrorCode = int;
#endif

    // -- Exception type used to signal inability of the host OS to access a file.
    struct AccessError {
        SystemPath path;
        ErrorCode error_code;
    };

    // -- Exception type used to signal the file designated by th `path` is not a regular file.
    struct RegularFileError {
        SystemPath path;
    };

    // -- Exception type used to signal inability of the host OS to memory-map a file.
    struct FileMappingError {
        SystemPath path;
        ErrorCode error_code;
    };

    // -- Input file mapped to memory as sequence of raw bytes.
    struct InputFile {
        using View = gsl::span<const std::byte>;

        explicit InputFile(const SystemPath&);
        InputFile(InputFile&&) noexcept;
        ~InputFile();
        View contents() const noexcept { return view; }
    private:
        View view;
    };

}

#endif
