#include "common.hxx"

namespace Module::util
{
    // Try to produce a short string for an type, if possible.

    namespace
    {
        struct Type_translator
        {
            Loader& ctx;

            std::string operator()(const Symbolic::FundamentalType& type)
            {
                return to_string(type);
            }

            std::string operator()(const Symbolic::DesignatedType& as_type)
            {
                std::string result("decl-type(");
                result.append(ctx.ref(as_type.decl));
                result.push_back(')');
                return result;
            }

            std::string operator()(const Symbolic::SyntacticType& type)
            {
                std::string result("syntactic-type(");
                result.append(ctx.ref(type.expr));
                result.push_back(')');
                return result;
            }

            std::string operator()(const Symbolic::ExpansionType& type)
            {
                return ctx.ref(type.operand) + to_string(type.mode);
            }

            std::string operator()(const Symbolic::PointerType& ptr)
            {
                return ctx.ref(ptr.operand) + "*";
            }

            std::string operator()(const Symbolic::PointerToMemberType& ptr)
            {
                return ctx.ref(ptr.type) + " " + ctx.ref(ptr.scope) + "::*";
            }

            std::string operator()(const Symbolic::LvalueReferenceType& type)
            {
                return ctx.ref(type.operand) + "&";
            }
            std::string operator()(const Symbolic::RvalueReferenceType& type)
            {
                return ctx.ref(type.operand) + "&&";
            }
            std::string operator()(const Symbolic::FunctionType& type)
            {
                // int(int,int)
                std::string result(ctx.ref(type.target));
                result.push_back('(');
                result.append(ctx.ref(type.source));
                result.push_back(')');
                // TODO: util::append_misc(result, type.eh_spec, type.convention, type.traits);
                return result;
            }

            std::string operator()(const Symbolic::MethodType& type)
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

            std::string operator()(const Symbolic::ArrayType& arr)
            {
                return ctx.ref(arr.element) + '[' + ctx.ref(arr.bound) + ']';
            }
            std::string operator()(const Symbolic::TypenameType& unresolved_type)
            {
                return "typename " + ctx.ref(unresolved_type.path);
            }
            std::string operator()(const Symbolic::QualifiedType& qual)
            {
                return ctx.ref(qual.unqualified_type) + " " + to_string(qual.qualifiers);
            }

            std::string operator()(const Symbolic::BaseType& base)
            {
                std::string result = ctx.ref(base.type);
                if (base.traits == Symbolic::BaseClassTraits::Shared)
                    result = "virtual " + result;
                if (base.traits == Symbolic::BaseClassTraits::Expanded)
                    result = result + "...";
                return to_string(base.access) + " " + result;
            }

            std::string operator()(const Symbolic::DecltypeType& val)
            {
                return "decltype(" + ctx.ref(val.expression) + ")";
            }

            std::string operator()(const Symbolic::PlaceholderType& type)
            {
                std::string result = "type-placeholder(" + to_string(type.basis);
                if (not null(type.elaboration))
                    result += " " + ctx.ref(type.elaboration);
                if (not null(type.constraint))
                    result.append(" " + ctx.ref(type.constraint));
                result += ')';
                return result;
            }

            std::string operator()(const Symbolic::TupleType& tuple)
            {
                std::string result;
                for (auto& index : ctx.reader.sequence(tuple))
                {
                    if (!result.empty())
                        result.push_back(',');
                    result.append(ctx.ref(index));
                }
                return result;
            }

            std::string operator()(const Symbolic::ForallType&)
            {
                return "";
            }

            std::string operator()(const Symbolic::UnalignedType& type)
            {
                return "__unaligned " + ctx.ref(type.operand);
            }

            std::string operator()(const Symbolic::SyntaxTreeType& syntax)
            {

                return "syntax-tree(" + ctx.ref(syntax.syntax) + ")";
            }

            std::string operator()(const Symbolic::TorType& type)
            {
                // #TOR(int, float)
                std::string result("#TOR(");
                result.append(ctx.ref(type.source));
                result.push_back(')');
                // TODO: util::append_misc(result, type.eh_spec, type.convention);
                return result;
            }
        };

    }  // namespace [anon]

    std::string get_string_if_possible(Loader& ctx, TypeIndex index)
    {
        if (!null(index))
            return ctx.reader.visit(index, Type_translator{ctx});
        return "no-type";
    }

    // Load types as full blown nodes.

    namespace
    {
        struct Type_loader : detail::Loader_visitor_base
        {
            void operator()(const Symbolic::FundamentalType& type)
            {
                node.props.emplace("type", to_string(type));
            }

            void operator()(const Symbolic::DesignatedType& as_type)
            {
                node.props.emplace("ref", ctx.ref(as_type.decl));
            }

            void operator()(const Symbolic::SyntacticType& type)
            {
                add_child(type.expr);
            }

            void operator()(const Symbolic::ExpansionType& type)
            {
                node.props.emplace("mode", to_string(type.mode));
                add_child(type.operand);
            }

            void operator()(const Symbolic::PointerType& ptr)
            {
                add_child(ptr.operand);
            }

            void operator()(const Symbolic::PointerToMemberType& ptr)
            {
                add_child(ptr.type);
                add_child(ptr.scope);
            }

            void operator()(const Symbolic::LvalueReferenceType& type)
            {
                add_child(type.operand);
            }

            void operator()(const Symbolic::RvalueReferenceType& type)
            {
                add_child(type.operand);
            }

            void operator()(const Symbolic::FunctionType& type)
            {
                add_child(type.target);
                add_child_if_not_null(type.source);
            }

            void operator()(const Symbolic::MethodType& type)
            {
                add_child(type.class_type);
                add_child(type.target);
                add_child_if_not_null(type.source);
            }

            void operator()(const Symbolic::ArrayType& arr)
            {
                add_child(arr.element);
                add_child(arr.bound);
            }

            void operator()(const Symbolic::TypenameType& unresolved_type)
            {
                add_child(unresolved_type.path);
            }

            void operator()(const Symbolic::QualifiedType& qual)
            {
                node.props.emplace("qualifiers", to_string(qual.qualifiers));
                add_child(qual.unqualified_type);
            }

            void operator()(const Symbolic::BaseType& base)
            {
                node.props.emplace("access", to_string(base.access));
                node.props.emplace("traits", to_string(base.traits));
                add_child(base.type);
            }

            void operator()(const Symbolic::DecltypeType& val)
            {
                add_child(val.expression);
            }

            void operator()(const Symbolic::PlaceholderType& type)
            {
                add_prop("basis", to_string(type.basis));
                add_child_if_not_null("elaboration", type.elaboration);
                add_child_if_not_null("constraint", type.elaboration);
            }

            void operator()(const Symbolic::TupleType& tuple)
            {
                for (auto& index : ctx.reader.sequence(tuple))
                    add_child(index);
            }

            void operator()(const Symbolic::ForallType& type)
            {
                add_child(type.chart);
                add_child(type.subject);
            }

            void operator()(const Symbolic::UnalignedType& type)
            {
                add_child(type.operand);
            }

            void operator()(const Symbolic::SyntaxTreeType& type)
            {
                add_child(type.syntax);
            }

            void operator()(const Symbolic::TorType& type)
            {
                add_child(type.source);
                // TODO: add props convention, eh_spec
            }
        };

    }  // namespace

    void load(Loader& ctx, Node& node, TypeIndex index)
    {
        DASSERT(!null(index));
        node.id = sort_name(index.sort());
        ctx.reader.visit(index, Type_loader{ctx, node});
    }

}  // namespace Module::util
