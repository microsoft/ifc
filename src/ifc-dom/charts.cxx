#include "common.hxx"

namespace Module::util
{
    Node* Loader::try_get(ChartIndex index)
    {
        if (index.sort() == ChartSort::None)
            return nullptr;

        NodeKey key(index);
        auto [it, inserted] = all_nodes.emplace(key, key);
        if (inserted)
            Module::util::load(*this, it->second, index);
        return &it->second;
    }

    void load(Loader& ctx, Node& node, ChartIndex index)
    {
        node.id = sort_name(index.sort());
        switch (index.sort())
        {
        case ChartSort::Unilevel:
        {
            auto& params = ctx.reader.get<Symbolic::UnilevelChart>(index);
            for (auto& item : ctx.reader.sequence(params))
                node.children.push_back(&ctx.get(item));
            break;
        }
        case ChartSort::Multilevel:
        {
            auto& params = ctx.reader.get<Symbolic::MultiChart>(index);
            for (auto& item : ctx.reader.sequence(params))
                node.children.push_back(&ctx.get(item));
            break;
        }
        default:
            break;
        }
    }


}  // namespace Module::util
