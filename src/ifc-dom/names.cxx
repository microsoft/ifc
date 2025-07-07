// Copyright Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "common.hxx"

namespace ifc::util {
    struct NameVisitor {
        Loader& ctx;

        std::string operator()(const symbolic::OperatorFunctionId& val)
        {
            return std::string("operator") + ctx.reader_get(val.name);
        }
        std::string operator()(const symbolic::ConversionFunctionId& val)
        {
            return std::string("operator ") + ctx.ref(val.target);
        }
        std::string operator()(const symbolic::LiteralOperatorId& val)
        {
            return std::string("operator ") + ctx.reader_get(val.name_index);
        }
        std::string operator()(const symbolic::TemplateName& val)
        {
            return std::string("TODO: ") + sort_name(val.algebra_sort);
        }
        std::string operator()(const symbolic::SpecializationName& val)
        {
            return std::string("TODO: ") + sort_name(val.algebra_sort);
        }
        std::string operator()(const symbolic::SourceFileName& val)
        {
            return std::string("src:") + ctx.reader_get(val.name) + ":" + ctx.reader_get(val.include_guard);
        }
        std::string operator()(const symbolic::GuideName& val)
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
        return ctx.reader.visit(index, NameVisitor{ctx});
    }

    std::string Loader::ref(const symbolic::Identity<NameIndex>& id)
    {
        return to_string(*this, id.name);
    }

    std::string Loader::ref(const symbolic::Identity<TextOffset>& id)
    {
        if (index_like::null(id.name))
            return "";
        return reader.get(id.name);
    }

    void load(Loader&, Node& node, NameIndex index)
    {
        node.id = to_string(index);
    }

} // namespace ifc::util
