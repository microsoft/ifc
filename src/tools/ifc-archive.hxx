// Copyright Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef IFC_ARCHIVE_INCLUDED
#define IFC_ARCHIVE_INCLUDED

#include "ifc/tooling.hxx"
#include "tool-support.hxx" // for STR in the constexpr subcommand names

namespace ifc::tool {
    // -- Subcommand creating an archive IFC that packages a set of non-archive IFC files,
    // -- keyed by each member's UnitSort and canonical name.
    struct ArchiveCommand final : Extension {
        // -- A constexpr registry key lets the builtin table prove its ordering at compile time.
        constexpr Name name() const final { return STR("archive"); }
        // -- Keeps archive option validation and diagnostics behind the Extension boundary.
        int run_with(const Arguments& args) const final;
    };

    // -- Subcommand extracting named modules and explicitly delimited header units from an archive
    // -- IFC, writing each selected member back to its recorded filepath.
    struct ExtractCommand final : Extension {
        // -- A constexpr registry key lets the builtin table prove its ordering at compile time.
        constexpr Name name() const final { return STR("extract"); }
        // -- Keeps overwrite authority and extraction policy behind the Extension boundary.
        int run_with(const Arguments& args) const final;
    };
}

#endif
