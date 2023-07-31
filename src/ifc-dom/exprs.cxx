#include "common.hxx"
#include "ifc/util.hxx"

namespace ifc::util {
    namespace {
        // we add to_string property for those items that can have be represented as a "short" by value string.
        struct ExprLoader : detail::LoaderVisitorBase {
            void operator()(const symbolic::EmptyExpr& expr) {}

            void operator()(const symbolic::LiteralExpr& expr)
            {
                auto str = to_string(ctx, expr.value);
                node.props.emplace("value", str);
            }

            void operator()(const symbolic::LambdaExpr& expr)
            {
                add_child(expr.introducer);
                add_child(expr.template_parameters);
                add_child(expr.declarator);
                add_child(expr.requires_clause);
                add_child(expr.body);
            }

            void operator()(const symbolic::TypeExpr& expr)
            {
                node.props.emplace("denotation", ctx.ref(expr.denotation));
            }

            void operator()(const symbolic::NamedDeclExpr& expr)
            {
                auto str = ctx.ref(expr.decl);
                node.props.emplace("ref", str);
            }

            void operator()(const symbolic::UnresolvedIdExpr& expr)
            {
                add_child(expr.name);
            }

            void operator()(const symbolic::TemplateIdExpr& expr)
            {
                add_child(expr.primary_template);
                add_child(expr.arguments);
            }

            void operator()(const symbolic::UnqualifiedIdExpr& expr)
            {
                add_child(expr.name);
                add_child(expr.symbol);
            }

            void operator()(const symbolic::SimpleIdentifierExpr& expr)
            {
                node.props["name"] = to_string(ctx, expr.name);
            }

            void operator()(const symbolic::PointerExpr& expr)
            {
                // nothing here. This is a '*' part of ::* path expression (qualified name).
            }

            void operator()(const symbolic::QualifiedNameExpr& expr)
            {
                add_child(expr.elements);
            }

            void operator()(const symbolic::PathExpr& expr)
            {
                add_child(expr.scope);
                add_child(expr.member);
            }

            void operator()(const symbolic::ReadExpr& expr)
            {
                node.props.emplace("kind", util::to_string(expr.kind));
                add_child(expr.child);
            }

            void operator()(const symbolic::MonadicExpr& expr)
            {
                node.props.emplace("assort", to_string(expr.assort));
                add_child(expr.arg[0]);
            }

            void operator()(const symbolic::DyadicExpr& expr)
            {
                node.props.emplace("assort", to_string(expr.assort));
                add_child(expr.arg[0]);
                add_child(expr.arg[1]);
            }

            void operator()(const symbolic::TriadicExpr& expr)
            {
                node.props.emplace("assort", to_string(expr.assort));
                add_child(expr.arg[0]);
                add_child(expr.arg[1]);
                add_child(expr.arg[1]);
            }

            void operator()(const symbolic::StringExpr& expr)
            {
                node.props.emplace("value", to_string(ctx, expr.string));
            }

            void operator()(const symbolic::TemporaryExpr& expr)
            {
                node.props.emplace("index", std::to_string(expr.index));
            }

            void operator()(const symbolic::CallExpr& expr)
            {
                add_child(expr.function);
                add_child(expr.arguments);
            }

            void operator()(const symbolic::MemberInitializerExpr& expr)
            {
                if (not null(expr.member))
                    node.props.emplace("member", ctx.ref(expr.member));
                if (not null(expr.base))
                    node.props.emplace("base", ctx.ref(expr.base));
                add_child(expr.expression);
            }

            void operator()(const symbolic::MemberAccessExpr& expr)
            {
                node.props.emplace("name", ctx.reader.get(expr.name));
                node.props.emplace("parent", ctx.ref(expr.parent));
                add_child(expr.offset);
            }

            void operator()(const symbolic::InheritancePathExpr& expr)
            {
                add_child(expr.path);
            }

            void operator()(const symbolic::InitializerListExpr& expr)
            {
                add_child(expr.elements);
            }

            void operator()(const symbolic::CastExpr& expr)
            {
                node.props.emplace("assort", to_string(expr.assort));
                node.props.emplace("target", ctx.ref(expr.target));
                add_child(expr.source);
            }

            void operator()(const symbolic::ConditionExpr& expr)
            {
                add_child(expr.expression);
            }

            void operator()(const symbolic::ExpressionListExpr& expr)
            {
                node.props.emplace("delimiter", to_string(expr.delimiter));
                add_child(expr.expressions);
            }

            void operator()(const symbolic::SizeofTypeExpr& expr)
            {
                auto str = ctx.ref(expr.operand);
                node.props.emplace("type-id", str);
            }

            void operator()(const symbolic::AlignofExpr& expr)
            {
                auto str = ctx.ref(expr.type_id);
                node.props.emplace("type-id", str);
            }

            void operator()(const symbolic::TypeidExpr& expr)
            {
                node.props.emplace("type-id", ctx.ref(expr.operand));
            }

            void operator()(const symbolic::DestructorCallExpr& expr)
            {
                node.props.emplace("kind", to_string(expr.kind));
                add_child_if_not_null(expr.decltype_specifier);
                add_child_if_not_null(expr.name);
            }

            void operator()(const symbolic::SyntaxTreeExpr& expr)
            {
                add_child(expr.syntax);
            }

            void operator()(const symbolic::FunctionStringExpr& expr)
            {
                node.props.emplace("macro", ctx.reader.get(expr.macro));
            }

            void operator()(const symbolic::CompoundStringExpr& expr)
            {
                node.props.emplace("prefix", ctx.reader.get(expr.prefix));
                add_child(expr.string);
            }

            void operator()(const symbolic::StringSequenceExpr& expr)
            {
                add_child(expr.strings);
            }

            void operator()(const symbolic::InitializerExpr& expr)
            {
                node.props.emplace("kind", to_string(expr.kind));
                add_child(expr.initializer);
            }

            void operator()(const symbolic::RequiresExpr& expr)
            {
                add_child(expr.parameters);
                add_child(expr.body);
            }

            void operator()(const symbolic::UnaryFoldExpr& expr)
            {
                node.props.emplace("assort", to_string(expr.op));
                if (expr.assoc != symbolic::Associativity::Unspecified)
                    node.props.emplace("associativity", to_string(expr.assoc));
                add_child(expr.expr);
            }

            void operator()(const symbolic::BinaryFoldExpr& expr)
            {
                node.props.emplace("assort", to_string(expr.op));
                if (expr.assoc != symbolic::Associativity::Unspecified)
                    node.props.emplace("associativity", to_string(expr.assoc));
                add_child(expr.left);
                add_child(expr.right);
            }

            void operator()(const symbolic::HierarchyConversionExpr& expr)
            {
                node.props.emplace("assort", to_string(expr.assort));
                node.props.emplace("target", ctx.ref(expr.target));
                add_child(expr.source);
                add_child(expr.inheritance_path);
                add_child(expr.override_inheritance_path);
            }

            void operator()(const symbolic::ProductTypeValueExpr& expr)
            {
                node.props.emplace("ref", ctx.ref(expr.structure));
                add_child_if_not_null(expr.members);
                add_child_if_not_null(expr.base_class_values);
            }

            void operator()(const symbolic::SumTypeValueExpr& expr)
            {
                node.props.emplace("ref", ctx.ref(expr.variant));
                node.props.emplace("index", std::to_string((int)expr.active_member));
                add_child(expr.value.value); // reach inside. no need to have it doubly nested
            }

            void operator()(const symbolic::SubobjectValueExpr& expr)
            {
                add_child(expr.value);
            }

            void operator()(const symbolic::ArrayValueExpr& expr)
            {
                node.props.emplace("element_type", ctx.ref(expr.element_type));
                add_child(expr.elements);
            }

            void operator()(const symbolic::DynamicDispatchExpr& expr)
            {
                add_child(expr.postfix_expr);
            }

            void operator()(const symbolic::VirtualFunctionConversionExpr& expr)
            {
                node.props.emplace("ref", ctx.ref(expr.function));
            }

            void operator()(const symbolic::PlaceholderExpr& expr) {}

            void operator()(const symbolic::ExpansionExpr& expr)
            {
                add_child(expr.operand);
            }

            void operator()(const symbolic::TupleExpr& expr)
            {
                for (auto item : ctx.reader.sequence(expr))
                    add_child(item);
            }

            void operator()(const symbolic::NullptrExpr& expr) {}

            void operator()(const symbolic::ThisExpr& expr) {}

            void operator()(const symbolic::TemplateReferenceExpr& expr)
            {
                node.props.emplace("name", ctx.ref(expr.member));
                node.props.emplace("base", ctx.ref(expr.parent));
                add_child(expr.template_arguments);
            }

            void operator()(const symbolic::TypeTraitIntrinsicExpr& expr)
            {
                node.props.emplace("assort", to_string(expr.intrinsic));
                node.props.emplace("arguments", ctx.ref(expr.arguments));
            }

            void operator()(const symbolic::DesignatedInitializerExpr& expr)
            {
                node.props.emplace("name", ctx.reader.get(expr.member));
                add_child(expr.initializer);
            }

            void operator()(const symbolic::PackedTemplateArgumentsExpr& expr)
            {
                add_child(expr.arguments);
            }

            void operator()(const symbolic::TokenExpr& expr)
            {
                add_child(expr.tokens);
            }

            void operator()(const symbolic::AssignInitializerExpr& expr)
            {
                add_child(expr.initializer);
            }

            void operator()(const symbolic::LabelExpr& expr)
            {
                add_child(expr.designator);
            }

            template<typename T>
            void operator()(const T&)
            {
                // a new expression was added?
                node.id = "!!!!! TODO: " + node.id;
            }
        };
    } // namespace

    void load(Loader& ctx, Node& node, ExprIndex expr)
    {
        if (null(expr))
        {
            node.id = "no-expr";
            return;
        }
        if (expr.sort() == ExprSort::VendorExtension)
        {
            node.id = "expr-vendor-" + std::to_string((int)expr.index());
            return;
        }
        node.id = sort_name(expr.sort());
        ctx.reader.visit(expr, ExprLoader{ctx, node});
    }

    void load(Loader& ctx, Node& node, symbolic::DefaultIndex index)
    {
        load(ctx, node, std::bit_cast<ExprIndex>(index));
    }

    std::string expr_list(Loader& ctx, ExprIndex index, std::string delimiters)
    {
        std::string result;
        if (index.sort() == ExprSort::Tuple)
        {
            auto& tuple = ctx.reader.get<symbolic::TupleExpr>(index);
            bool first  = true;
            for (auto item : ctx.reader.sequence(tuple))
            {
                if (not result.empty())
                    result.push_back(',');
                result.append(ctx.ref(item));
            }
        }
        else if (not null(index))
        {
            result.append(ctx.ref(index));
        }
        if (delimiters.size() > 0)
            result.insert(0, 1, delimiters[0]);
        if (delimiters.size() > 1)
            result.push_back(delimiters[1]);
        return result;
    }

    // Try to produce a short string for an expression, if possible.

    namespace {
        struct ExpxTranslator {
            Loader& ctx;

            std::string operator()(const symbolic::EmptyExpr& expr)
            {
                return "empty-expr";
            }

            std::string operator()(const symbolic::NullptrExpr& expr)
            {
                return "nullptr";
            }

            std::string operator()(const symbolic::ThisExpr& expr)
            {
                return "this";
            }

            std::string operator()(const symbolic::LiteralExpr& expr)
            {
                return to_string(ctx, expr.value);
            }

            std::string operator()(const symbolic::TypeExpr& expr)
            {
                return ctx.ref(expr.denotation);
            }

            std::string operator()(const symbolic::NamedDeclExpr& expr)
            {
                return std::string("decl-ref(") + ctx.ref(expr.decl) + ")";
            }

            template<typename T>
            std::string operator()(const T&)
            {
                return "";
            }
        };

    } // namespace

    std::string get_string_if_possible(Loader& ctx, ExprIndex expr)
    {
        if (null(expr))
            return "no-expr";
        if (expr.sort() == ExprSort::VendorExtension)
            return "expr-vendor-" + std::to_string((int)expr.index());
        return ctx.reader.visit(expr, ExpxTranslator{ctx});
    }

} // namespace ifc::util
