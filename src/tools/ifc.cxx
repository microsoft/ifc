// Copyright Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <stdlib.h>
#include <vector>
#include <span>
#include <algorithm>
#include <iostream>

#ifdef WIN32
#   include <windows.h>
#endif

#include "ifc/tooling.hxx"

#ifdef WIN32
#   define STR(S) L ## S
#   define IFC_MAIN wmain
#   define IFC_ERR std::wcerr
#else 
#   define STR(S) S
#   define IFC_MAIN main
#   define IFC_ERR std::cerr
#endif

namespace {
    using ProgramName = ifc::tool::StringView;

    void print_usage(const ProgramName& prog)
    {
        IFC_ERR << prog << STR(" usage:\n\t")
            << prog << STR(" <cmd> [options] <ifc-files>\n");
    }

    // -- Subcommand printing the Spec version from an IFC file.
    struct VersionCommand : ifc::tool::Extension {
        ifc::tool::Name name() const final { return STR("version"); }
        int run_with(const ifc::tool::Arguments&) const final
        {
            return 0;
        }
    };

    constexpr VersionCommand version_cmd { };

    // -- List of all builtin subcommands, sorted by their name.
    constexpr const ifc::tool::Extension* builtin_extensions[] {
        &version_cmd,
    };
    static_assert(std::ranges::is_sorted(builtin_extensions, { }, &ifc::tool::Extension::name));

    const ifc::tool::Extension* builtin_operation(const ifc::tool::Name& cmd)
    {
        auto ext = std::ranges::lower_bound(builtin_extensions, cmd, { }, &ifc::tool::Extension::name);
        if (ext < std::end(builtin_extensions) and (*ext)->name() == cmd)
            return *ext;
        return nullptr;
    }

    // Enclose the argument in double quotes.
    ifc::tool::String quote(const ifc::tool::StringView& s)
    {
        static constexpr auto kwote_str = STR("\"");
        ifc::tool::String r = kwote_str;
        r += s;
        r += kwote_str;
        return r;
    }
}


int IFC_MAIN(int argc, ifc::tool::NativeChar* argv[])
{
    // The `ifc` tool itself does not accept any option.
    int idx = 1;
    while (idx < argc) {
        ifc::tool::StringView s { argv[idx] };
        if (not s.starts_with(STR("-")))
            break;
        ++idx;
    }

    if (argc < 2 or idx > 1 or idx >= argc) {
        print_usage(argv[0]);
        return 1;
    }

    // The subcommand name is next, after possibly invalid options.
    // It shall not contain any path separator character.
    ifc::tool::Name cmd { argv[idx] };
    if (cmd.contains(STR("/")[0]) or cmd.contains(STR("\\")[0])) {
        IFC_ERR << STR("ifc subcommand cannot contain pathname separator") << std::endl;
        return 1;
    }

    ifc::tool::Arguments args { argv + idx + 1, argv + argc };

    if (auto op = builtin_operation(cmd))
        return op->run_with(args);

    ifc::tool::String tool = STR("ifc-");
    tool += cmd;
    ifc::tool::String command = quote(tool);
    for (auto& arg : args) {
        command += STR(" ");
        command += quote(arg);
    }

#ifdef WIN32
    auto status = _wsystem(command.c_str());
#else
    auto status = system(command.c_str());
#endif
    if (status != 0)
        IFC_ERR << STR("ifc: no subcommand named '") << cmd << STR("'") << std::endl;
    return status;
}

