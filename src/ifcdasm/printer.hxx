#ifndef IFC_TOOLS_PRINTER_HXX
#define IFC_TOOLS_PRINTER_HXX

#include "ifc/dom/node.hxx"
#include <ostream>

namespace Module::util
{
    enum class Print_options : int8_t
    {
        None = 0,
        Use_color       = 1 << 0,
        Top_level_index = 1 << 1,
    };
    void print(const Node& gs, std::ostream& os, Print_options options = Print_options::None);
}

#endif // IFC_TOOLS_PRINTER_HXX