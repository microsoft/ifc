// Copyright Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "common.hxx"

namespace ifc::util {
    // Try to produce a short string for an type, if possible.

    namespace {
        struct TypeTranslator {
            Loader& ctx;

            std::string operator()(const symbolic::FundamentalType& type)
            {
                return to_string(type);
            }

            std::string operator()(const symbolic::DesignatedType& as_type)
            {
                std::string result("decl-type(");
                result.append(ctx.ref(as_type.decl));
                result.push_back(')');
                return result;
            }

            std::string operator()(const symbolic::SyntacticType& type)
            {
                std::string result("syntactic-type(");
                result.append(ctx.ref(type.expr));
                result.push_back(')');
                return result;
            }

            std::string operator()(const symbolic::ExpansionType& type)
            {
                return ctx.ref(type.operand) + to_string(type.mode);
            }

            std::string operator()(const symbolic::PointerType& ptr)
            {
                return ctx.ref(ptr.operand) + "*";
            }

            std::string operator()(const symbolic::PointerToMemberType& ptr)
            {
                return ctx.ref(ptr.type) + " " + ctx.ref(ptr.scope) + "::*";
            }

            std::string operator()(const symbolic::LvalueReferenceType& type)
            {
                return ctx.ref(type.operand) + "&";
            }
            std::string operator()(const symbolic::RvalueReferenceType& type)
            {
                return ctx.ref(type.operand) + "&&";
            }
            std::string operator()(const symbolic::FunctionType& type)
            {
                // int(int,int)
                std::string result(ctx.ref(type.target));
                result.push_back('(');
                result.append(ctx.ref(type.source));
                result.push_back(')');
                // TODO: util::append_misc(result, type.eh_spec, type.convention, type.traits);
                return result;
            }

            std::string operator()(const symbolic::MethodType& type)
            {
                // int(MyClass: int,int)
                std::string result(ctx.ref(type.target));
                result.push_back('(');
                result.append(ctx.ref(type.class_type));
                result.append(": ");
                result.append(ctx.ref(type.source));
                result.push_back(')');
                // TODO: util::append_misc(result, type.eh_spec, type.convention, type.traits);
                return result;
            }

            std::string operator()(const symbolic::ArrayType& arr)
            {
                return ctx.ref(arr.element) + '[' + ctx.ref(arr.bound) + ']';
            }
            std::string operator()(const symbolic::TypenameType& unresolved_type)
            {
                return "typename " + ctx.ref(unresolved_type.path);
            }
            std::string operator()(const symbolic::QualifiedType& qual)
            {
                return ctx.ref(qual.unqualified_type) + " " + to_string(qual.qualifiers);
            }

            std::string operator()(const symbolic::BaseType& base)
            {
                std::string result = ctx.ref(base.type);
                if (base.traits == symbolic::BaseClassTraits::Shared)
                    result = "virtual " + result;
                if (base.traits == symbolic::BaseClassTraits::Expanded)
                    result = result + "...";
                return to_string(base.access) + " " + result;
            }

            std::string operator()(const symbolic::DecltypeType& val)
            {
                return "decltype(" + ctx.ref(val.expression) + ")";
            }

            std::string operator()(const symbolic::PlaceholderType& type)
            {
                std::string result = "type-placeholder(" + to_string(type.basis);
                if (not null(type.elaboration))
                    result += " " + ctx.ref(type.elaboration);
                if (not null(type.constraint))
                    result.append(" " + ctx.ref(type.constraint));
                result += ')';
                return result;
            }

            std::string operator()(const symbolic::TupleType& tuple)
            {
                std::string result;
                for (auto& index : ctx.reader.sequence(tuple))
                {
                    if (not result.empty())
                        result.push_back(',');
                    result.append(ctx.ref(index));
                }
                return result;
            }

            std::string operator()(const symbolic::ForallType&)
            {
                return "";
            }

            std::string operator()(const symbolic::UnalignedType& type)
            {
                return "__unaligned " + ctx.ref(type.operand);
            }

            std::string operator()(const symbolic::SyntaxTreeType& syntax)
            {
                return "syntax-tree(" + ctx.ref(syntax.syntax) + ")";
            }

            std::string operator()(const symbolic::TorType& type)
            {
                // #TOR(int, float)
                std::string result("#TOR(");
                result.append(ctx.ref(type.source));
                result.push_back(')');
                // TODO: util::append_misc(result, type.eh_spec, type.convention);
                return result;
            }
        };

    } // namespace

    std::string get_string_if_possible(Loader& ctx, TypeIndex index)
    {
        if (not null(index))
            return ctx.reader.visit(index, TypeTranslator{ctx});
        return "no-type";
    }

    // Load types as full blown nodes.

    namespace {
        struct TypeLoader : detail::LoaderVisitorBase {
            void operator()(const symbolic::FundamentalType& type)
            {
                node.props.emplace("type", to_string(type));
            }

            void operator()(const symbolic::DesignatedType& as_type)
            {
                node.props.emplace("ref", ctx.ref(as_type.decl));
            }

            void operator()(const symbolic::SyntacticType& type)
            {
                add_child(type.expr);
            }

            void operator()(const symbolic::ExpansionType& type)
            {
                node.props.emplace("mode", to_string(type.mode));
                add_child(type.operand);
            }

            void operator()(const symbolic::PointerType& ptr)
            {
                add_child(ptr.operand);
            }

            void operator()(const symbolic::PointerToMemberType& ptr)
            {
                add_child(ptr.type);
                add_child(ptr.scope);
            }

            void operator()(const symbolic::LvalueReferenceType& type)
            {
                add_child(type.operand);
            }

            void operator()(const symbolic::RvalueReferenceType& type)
            {
                add_child(type.operand);
            }

            void operator()(const symbolic::FunctionType& type)
            {
                add_child(type.target);
                add_child_if_not_null(type.source);
            }

            void operator()(const symbolic::MethodType& type)
            {
                add_child(type.class_type);
                add_child(type.target);
                add_child_if_not_null(type.source);
            }

            void operator()(const symbolic::ArrayType& arr)
            {
                add_child(arr.element);
                add_child(arr.bound);
            }

            void operator()(const symbolic::TypenameType& unresolved_type)
            {
                add_child(unresolved_type.path);
            }

            void operator()(const symbolic::QualifiedType& qual)
            {
                node.props.emplace("qualifiers", to_string(qual.qualifiers));
                add_child(qual.unqualified_type);
            }

            void operator()(const symbolic::BaseType& base)
            {
                node.props.emplace("access", to_string(base.access));
                node.props.emplace("traits", to_string(base.traits));
                add_child(base.type);
            }

            void operator()(const symbolic::DecltypeType& val)
            {
                add_child(val.expression);
            }

            void operator()(const symbolic::PlaceholderType& type)
            {
                add_prop("basis", to_string(type.basis));
                add_child_if_not_null("elaboration", type.elaboration);
                add_child_if_not_null("constraint", type.elaboration);
            }

            void operator()(const symbolic::TupleType& tuple)
            {
                for (auto& index : ctx.reader.sequence(tuple))
                    add_child(index);
            }

            void operator()(const symbolic::ForallType& type)
            {
                add_child(type.chart);
                add_child(type.subject);
            }

            void operator()(const symbolic::UnalignedType& type)
            {
                add_child(type.operand);
            }

            void operator()(const symbolic::SyntaxTreeType& type)
            {
                add_child(type.syntax);
            }

            void operator()(const symbolic::TorType& type)
            {
                add_child(type.source);
                // TODO: add props convention, eh_spec
            }
        };

    } // namespace

    void load(Loader& ctx, Node& node, TypeIndex index)
    {
        IFCASSERT(not null(index));
        node.id = sort_name(index.sort());
        ctx.reader.visit(index, TypeLoader{ctx, node});
    }

} // namespace ifc::util
