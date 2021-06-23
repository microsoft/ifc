// Copyright Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef IFC_UTIL_DOM_COMMON_H
#define IFC_UTIL_DOM_COMMON_H

#include "ifc/dom/node.hxx"
#include "ifc/reader.hxx"

namespace ifc::util {
    std::string to_string(Loader&, NameIndex);
    std::string to_string(Loader&, LitIndex);
    std::string to_string(Loader&, StringIndex);
    std::string to_string(Loader&, symbolic::NoexceptSpecification);
    std::string expr_list(Loader&, ExprIndex, std::string delimiters = "");

    namespace detail {
        struct LoaderVisitorBase {
            Loader& ctx;
            Node& node;

            template<index_like::Algebra Index>
            void add_child(Index abstract_index)
            {
                node.children.push_back(&ctx.get(abstract_index));
            }

            template<index_like::Algebra Index>
            void add_child(char const*, Index abstract_index)
            {
                // ignore name for now
                node.children.push_back(&ctx.get(abstract_index));
            }

            void add_type(char const* name, TypeIndex type)
            {
                add_prop(name, ctx.ref(type));
            }

            void add_prop(char const* name, std::string&& str)
            {
                if (not str.empty())
                    node.props.emplace(name, str);
            }

            template<index_like::Algebra Index>
            void add_child_if_not_null(Index abstract_index)
            {
                if (not null(abstract_index))
                    node.children.push_back(&ctx.get(abstract_index));
            }

            template<index_like::Algebra Index>
            void add_child_if_not_null(char const*, Index abstract_index)
            {
                if (not null(abstract_index))
                    node.children.push_back(&ctx.get(abstract_index));
            }
        };
    } // namespace detail
} // namespace ifc::util

#endif // IFC_UTIL_DOM_COMMON_H
