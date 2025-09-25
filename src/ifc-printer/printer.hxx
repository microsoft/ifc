// Copyright Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef IFC_TOOLS_PRINTER_HXX
#define IFC_TOOLS_PRINTER_HXX

#include "ifc/dom/node.hxx"

#if defined(IFC_BUILD_USING_STD_MODULE)
    import std;
#else
    #include <ostream>
#endif

namespace ifc::util {
    enum class PrintOptions : int8_t {
        None            = 0,
        Use_color       = 1 << 0,
        Top_level_index = 1 << 1,
    };
    [[nodiscard]] constexpr PrintOptions operator|(PrintOptions e1, PrintOptions e2) noexcept
    {
        return static_cast<PrintOptions>(ifc::to_underlying(e1) | ifc::to_underlying(e2));
    }
    constexpr PrintOptions& operator|=(PrintOptions& e1, PrintOptions e2) noexcept
    {
        return e1 = e1 | e2;
    }

    void print(const Node& gs, std::ostream& os, PrintOptions options = PrintOptions::None);
} // namespace ifc::util

#endif // IFC_TOOLS_PRINTER_HXX