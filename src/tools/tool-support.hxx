// Copyright Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef IFC_TOOL_SUPPORT_INCLUDED
#define IFC_TOOL_SUPPORT_INCLUDED

#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <iostream>

#include <gsl/span>

#include "ifc/file.hxx"
#include "ifc/tooling.hxx"

// -- Convenience macros bridging the wide (Windows) and narrow (POSIX) native-string worlds
// -- shared by the ifc driver and its subcommands.  STR() adorns a literal with the native
// -- character prefix; IFC_OUT/IFC_ERR select the native-width standard streams.
#ifdef _WIN32
#   define STR(S) L ## S
#   define IFC_OUT std::wcout
#   define IFC_ERR std::wcerr
#else
#   define STR(S) S
#   define IFC_OUT std::cout
#   define IFC_ERR std::cerr
#endif

// Shared support routines for building ifc subcommands: option parsing, IFC header/string-table
// reading, file-region copying, and extraction-path hardening.
namespace ifc::tool {
    // -- Return true if `s` looks like an option (it starts with a dash).
    bool resemble_option(const ifc::tool::StringView& s);

    // -- Locate the NUL-terminated UTF-8 string at `offset` within the string table `table`,
    // -- returning an empty view if the offset is out of range.
    std::u8string_view string_at(std::u8string_view table, std::uint32_t offset);

    // -- Outcome of reading an IFC file signature and fixed header.
    enum class HeaderRead {
        Ok,
        NotIfc,    // Missing or wrong signature.
        Truncated, // Signature present but the header is incomplete.
    };

    // -- Read the 4-byte signature and fixed header from `in`, positioned at the start.
    HeaderRead read_ifc_header(std::ifstream& in, ifc::Header& header);

    // -- Result of matching a value-taking command-line option.
    enum class OptionMatch {
        No,      // `args[i]` is not this option.
        Value,   // Matched; `value` was set.
        Missing, // Matched the spelling, but no value followed.
    };

    // -- Match `args[i]` as a value-taking option with spellings `short_name` (may be empty) or
    // -- `long_name`, in the `spelling value` form (consuming the next argument by advancing `i`)
    // -- or the `long_name=value` form.  Set `value` on a match.
    OptionMatch take_value(const ifc::tool::Arguments& args, std::size_t& i, ifc::tool::StringView short_name,
                           ifc::tool::StringView long_name, ifc::tool::StringView& value);

    // -- Convert a native command-line string to UTF-8 without interpreting it as a pathname.
    std::u8string to_utf8(ifc::tool::StringView s);

    // -- Return true if `bytes` begins with a valid IFC file header (signature plus room for
    // -- the fixed header).
    bool valid_ifc_image(ifc::tool::InputFile::View bytes);

    // -- Gives validated mapped-image users one definition of the fixed header location.
    // -- `bytes` must satisfy valid_ifc_image().
    const ifc::Header& ifc_header(ifc::tool::InputFile::View bytes);

    // -- Reduce a stored member filepath to a safe relative path under an extraction directory:
    // -- drop any root, skip '.', and reject '..' (and, on Windows, reserved device names and
    // -- alternate-data-stream ':' components) so a crafted archive cannot escape.  Return an
    // -- empty path if the filepath is unusable.
    ifc::fs::path safe_relative(std::u8string_view stored);

    // -- Exception thrown when a string table cannot be represented within the 32-bit IFC limits:
    // -- its bytes would overflow a TextOffset (raised by build()), or it holds more distinct
    // -- strings than a 32-bit handle can address (raised by intern()).
    struct StringTableOverflow {
        std::uint64_t size; // The offending size: table bytes, or the count of interned strings.
    };

    // -- Accumulates strings and emits a single string table with exact-duplicate dedup and
    // -- maximal suffix sharing (a string that is a suffix of another is overlaid on it).  Offsets
    // -- are assigned only at build(), so one table can back several callers -- the archive writer
    // -- now, and a future `link` that fuses every member's string table into one -- with references
    // -- resolved after.
    struct StringTableBuilder {
        // -- Opaque reference to an interned string, resolved to a TextOffset by build().
        enum class Handle : std::uint32_t {};

        StringTableBuilder() = default;
        // -- The dedup map keys are views into `strings`, so a copy would alias another builder's
        // -- storage; the builder is therefore non-copyable (nothing needs to copy it).
        StringTableBuilder(const StringTableBuilder&) = delete;
        StringTableBuilder& operator=(const StringTableBuilder&) = delete;

        // -- Register `s` and return a handle for it; equal bytes yield the same handle.
        Handle intern(std::u8string_view s);

        // -- Build the merged table, assigning every interned string a final offset.
        void build();

        // -- Offset of the string denoted by `h`, valid only after build().  Throws
        // -- UnresolvedStringHandle if `h` has no assigned offset (queried before build(), or
        // -- interned since the last build()).
        ifc::TextOffset offset(Handle h) const;

        // -- The built table; its first byte is the NUL at offset 0.  Valid after build().
        gsl::span<const std::byte> bytes() const;

    private:
        // `strings` owns each unique interned string exactly once and is indexed by handle; the
        // keys of `index_map` are views into those elements (std::deque keeps element addresses
        // stable across growth), so a lookup needs no allocation and the bytes are never stored
        // twice.  Because the map aliases `strings`, the builder is non-copyable.
        std::deque<std::u8string> strings;
        std::unordered_map<std::u8string_view, Handle> index_map;
        std::vector<ifc::TextOffset> offsets; // Each string's final offset, assigned by build().
        std::vector<std::byte> table; // The built table bytes; valid after build().
    };

    // -- Exception thrown by StringTableBuilder::offset() when the handle has no resolved offset --
    // -- queried before build(), or interned since the last build().  Enforced in every build mode.
    struct UnresolvedStringHandle {
        StringTableBuilder::Handle handle; // The handle that build() has not resolved to an offset.
    };
}

#endif
