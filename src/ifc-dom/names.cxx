#include "common.hxx"

namespace Module::util
{
    struct Name_visitor
    {
        Loader& ctx;

        std::string operator()(const Symbolic::OperatorFunctionId& val)
        {
            return std::string("operator") + ctx.reader.get(val.name);
        }
        std::string operator()(const Symbolic::ConversionFunctionId& val)
        {
            return std::string("operator ") + ctx.ref(val.target);
        }
        std::string operator()(const Symbolic::LiteralOperatorId& val)
        {
            return std::string("operator ") + ctx.reader.get(val.name_index);
        }
        std::string operator()(const Symbolic::TemplateName& val)
        {
            return std::string("TODO: ") + sort_name(val.algebra_sort);
        }
        std::string operator()(const Symbolic::TemplateId& val)
        {
            return std::string("TODO: ") + sort_name(val.algebra_sort);
        }
        std::string operator()(const Symbolic::SourceFileName& val)
        {
            return std::string("src:") + ctx.reader.get(val.name) + ":" + ctx.reader.get(val.include_guard);
        }
        std::string operator()(const Symbolic::GuideName& val)
        {
            return "guide(" + ctx.ref(val.primary_template) + ")";
        }

        std::string operator()(TextOffset offset)
        {
            if (index_like::null(offset))
                return "";
            return ctx.reader.get(offset);
        }
    };

    std::string to_string(Loader& ctx, NameIndex index)
    {
        return ctx.reader.visit(index, Name_visitor{ctx});
    }

    std::string Loader::ref(const Symbolic::Identity<NameIndex>& id)
    {
        return to_string(*this, id.name);
    }

    std::string Loader::ref(const Symbolic::Identity<TextOffset>& id)
    {
        if (index_like::null(id.name))
            return "";
        return reader.get(id.name);
    }

    void load(Loader&, Node& node, NameIndex index)
    {
        node.id = to_string(index);
    }

}