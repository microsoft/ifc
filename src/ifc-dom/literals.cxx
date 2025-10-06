// Copyright Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "common.hxx"
#include "ifc/util.hxx"

namespace ifc::util {
    std::string to_string(Loader& ctx, LitIndex index)
    {
        switch (index.sort())
        {
        case LiteralSort::Immediate:
            return std::to_string(ifc::to_underlying(index.index()));
        case LiteralSort::Integer:
            return std ::to_string(ctx.reader.get<std::int64_t>(index));
        case LiteralSort::FloatingPoint:
            return std ::to_string(ctx.reader.get<double>(index));
        default:
            return "unknown-literal-sort-" + std::to_string(ifc::to_underlying(index.sort()));
        }
    }

    std::string to_string(Loader& ctx, StringIndex index)
    {
        auto& str          = ctx.reader.get(index);
        std::string suffix = index_like::null(str.suffix) ? "" : ctx.reader.get(str.suffix);
        std::string prefix;
        switch (index.sort())
        {
        case StringSort::UTF8:
            prefix = "u8";
            [[fallthrough]];
        case StringSort::Ordinary:
            return prefix + "\"" + std::string(ctx.reader.get(str.start), ifc::to_underlying(str.size)) + "\""
                   + suffix;
        case StringSort::UTF16:
            prefix = "u16";
            break;
        case StringSort::UTF32:
            prefix = "u32";
            break;
        case StringSort::Wide:
            prefix = "L";
            break;
        default:
            return "unknown-string-sort-" + std::to_string(ifc::to_underlying(index.sort()));
        }
        return prefix + "\"<TODO: convert to utf8>\"" + suffix;
    }
} // namespace ifc::util
