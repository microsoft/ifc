#include "common.hxx"

namespace ifc::util
{
    void load(Loader&, Node& node, SyntaxIndex index)
    {
        // very lonely at the moment. Add syntax handling if/when desired.
        node.id = to_string(index);
    }

}  // namespace ifc::util
