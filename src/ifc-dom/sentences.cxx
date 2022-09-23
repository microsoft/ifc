#include "common.hxx"

namespace Module::util
{
    void load(Loader& ctx, Node& node, SentenceIndex index)
    {
        // At some point all of the token based initializers will be going
        // away. If that does not happen. We can add token loading here.
        node.id = to_string(index);
    }
}
