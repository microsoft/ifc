#include "common.hxx"
#include "ifc/util.hxx"

namespace Module::util
{
    namespace
    {
        // we add to_string property for those items that can have be represented as a "short" by value string.
        struct Expr_loader : detail::Loader_visitor_base
        {
            void operator()(const Symbolic::EmptyExpression& expr)
            {
            }

            void operator()(const Symbolic::LiteralExpr& expr)
            {
                auto str = to_string(ctx, expr.value);
                node.props.emplace("value", str);
            }

            void operator()(const Symbolic::LambdaExpression& expr)
            {
                add_child(expr.introducer);
                add_child(expr.template_parameters);
                add_child(expr.declarator);
                add_child(expr.requires_clause);
                add_child(expr.body);
            }

            void operator()(const Symbolic::TypeExpression& expr)
            {
                node.props.emplace("denotation", ctx.ref(expr.denotation));
            }

            void operator()(const Symbolic::NamedDeclExpression& expr)
            {
                auto str = ctx.ref(expr.decl);
                node.props.emplace("ref", str);
            }

            void operator()(const Symbolic::UnresolvedIdExpression& expr)
            {
                add_child(expr.name);
            }

            void operator()(const Symbolic::TemplateIdExpression& expr)
            {
                add_child(expr.primary_template);
                add_child(expr.arguments);
            }

            void operator()(const Symbolic::UnqualifiedIdExpr& expr)
            {
                add_child(expr.name);
                add_child(expr.symbol);
            }

            void operator()(const Symbolic::SimpleIdentifier& expr)
            {
                node.props["name"] = to_string(ctx, expr.name);
            }

            void operator()(const Symbolic::Pointer& expr)
            {
                // nothing here. This is a '*' part of ::* path expression (qualified name).
            }

            void operator()(const Symbolic::QualifiedNameExpr& expr)
            {
                add_child(expr.elements);
            }

            void operator()(const Symbolic::PathExpression& expr)
            {
                add_child(expr.scope);
                add_child(expr.member);
            }

            void operator()(const Symbolic::ReadExpression& expr)
            {
                node.props.emplace("kind", util::to_string(expr.kind));
                add_child(expr.child);
            }

            void operator()(const Symbolic::MonadicTree& expr)
            {
                node.props.emplace("assort", to_string(expr.assort));
                add_child(expr.arg[0]);
            }

            void operator()(const Symbolic::DyadicTree& expr)
            {
                node.props.emplace("assort", to_string(expr.assort));
                add_child(expr.arg[0]);
                add_child(expr.arg[1]);
            }

            void operator()(const Symbolic::TriadicTree& expr)
            {
                node.props.emplace("assort", to_string(expr.assort));
                add_child(expr.arg[0]);
                add_child(expr.arg[1]);
                add_child(expr.arg[1]);
            }

            void operator()(const Symbolic::StringExpression& expr)
            {
                node.props.emplace("value", to_string(ctx, expr.string));
            }

            void operator()(const Symbolic::TemporaryExpression& expr)
            {
                node.props.emplace("index", std::to_string(expr.index));
            }

            void operator()(const Symbolic::CallExpression& expr)
            {
                add_child(expr.function);
                add_child(expr.arguments);
            }

            void operator()(const Symbolic::MemberInitializer& expr)
            {
                if (not null(expr.member))
                    node.props.emplace("member", ctx.ref(expr.member));
                if (not null(expr.base))
                    node.props.emplace("base", ctx.ref(expr.base));
                add_child(expr.expression);
            }

            void operator()(const Symbolic::MemberAccess& expr)
            {
                node.props.emplace("name", ctx.reader.get(expr.name));
                node.props.emplace("parent", ctx.ref(expr.parent));
                add_child(expr.offset);
            }

            void operator()(const Symbolic::InheritancePath& expr)
            {
                add_child(expr.path);
            }

            void operator()(const Symbolic::InitializerList& expr)
            {
                add_child(expr.elements);
            }

            void operator()(const Symbolic::CastExpr& expr)
            {
                node.props.emplace("assort", to_string(expr.assort));
                node.props.emplace("target", ctx.ref(expr.target));
                add_child(expr.source);
            }

            void operator()(const Symbolic::ConditionExpr& expr)
            {
                add_child(expr.expression);
            }

            void operator()(const Symbolic::ExpressionList& expr)
            {
                node.props.emplace("delimiter", to_string(expr.delimiter));
                add_child(expr.expressions);
            }

            void operator()(const Symbolic::SizeofTypeId& expr)
            {
                auto str = ctx.ref(expr.operand);
                node.props.emplace("type-id", str);
            }

            void operator()(const Symbolic::Alignof& expr)
            {
                auto str = ctx.ref(expr.type_id);
                node.props.emplace("type-id", str);
            }

            void operator()(const Symbolic::TypeidExpression& expr)
            {
                node.props.emplace("type-id", ctx.ref(expr.operand));
            }

            void operator()(const Symbolic::DestructorCall& expr)
            {
                node.props.emplace("kind", to_string(expr.kind));
                add_child_if_not_null(expr.decltype_specifier);
                add_child_if_not_null(expr.name);
            }

            void operator()(const Symbolic::SyntaxTreeExpr& expr)
            {
                add_child(expr.syntax);
            }

            void operator()(const Symbolic::FunctionStringExpression& expr)
            {
                node.props.emplace("macro", ctx.reader.get(expr.macro));
            }

            void operator()(const Symbolic::CompoundStringExpression& expr)
            {
                node.props.emplace("prefix", ctx.reader.get(expr.prefix));
                add_child(expr.string);
            }

            void operator()(const Symbolic::StringSequenceExpression& expr)
            {
                add_child(expr.strings);
            }

            void operator()(const Symbolic::Initializer& expr)
            {
                node.props.emplace("kind", to_string(expr.kind));
                add_child(expr.initializer);
            }

            void operator()(const Symbolic::RequiresExpression& expr)
            {
                add_child(expr.parameters);
                add_child(expr.body);
            }

            void operator()(const Symbolic::UnaryFoldExpression& expr)
            {
                node.props.emplace("assort", to_string(expr.op));
                if (expr.assoc != Symbolic::Associativity::Unspecified)
                    node.props.emplace("associativity", to_string(expr.assoc));
                add_child(expr.expr);
            }

            void operator()(const Symbolic::BinaryFoldExpression& expr)
            {
                node.props.emplace("assort", to_string(expr.op));
                if (expr.assoc != Symbolic::Associativity::Unspecified)
                    node.props.emplace("associativity", to_string(expr.assoc));
                add_child(expr.left);
                add_child(expr.right);
            }

            void operator()(const Symbolic::HierarchyConversionExpression& expr)
            {
                node.props.emplace("assort", to_string(expr.assort));
                node.props.emplace("target", ctx.ref(expr.target));
                add_child(expr.source);
                add_child(expr.inheritance_path);
                add_child(expr.override_inheritance_path);
            }

            void operator()(const Symbolic::ProductTypeValue& expr)
            {
                node.props.emplace("ref", ctx.ref(expr.structure));
                add_child_if_not_null(expr.members);
                add_child_if_not_null(expr.base_class_values);
            }

            void operator()(const Symbolic::SumTypeValue& expr)
            {
                node.props.emplace("ref", ctx.ref(expr.variant));
                node.props.emplace("index", std::to_string((int)expr.active_member));
                add_child(expr.value.value); // reach inside. no need to have it doubly nested
            }

            void operator()(const Symbolic::SubobjectValue& expr)
            {
                add_child(expr.value);
            }

            void operator()(const Symbolic::ArrayValue& expr)
            {
                node.props.emplace("element_type", ctx.ref(expr.element_type));
                add_child(expr.elements);
            }

            void operator()(const Symbolic::DynamicDispatch& expr)
            {
                add_child(expr.postfix_expr);
            }

            void operator()(const Symbolic::VirtualFunctionConversion& expr)
            {
                node.props.emplace("ref", ctx.ref(expr.function));
            }

            void operator()(const Symbolic::PlaceholderExpr& expr)
            {
            }

            void operator()(const Symbolic::ExpansionExpr& expr)
            {
                add_child(expr.operand);
            }

            void operator()(const Symbolic::TupleExpression& expr)
            {
                for (auto item : ctx.reader.sequence(expr))
                    add_child(item);
            }

            void operator()(const Symbolic::Nullptr& expr)
            {
            }

            void operator()(const Symbolic::This& expr)
            {
            }

            void operator()(const Symbolic::TemplateReference& expr)
            {
                node.props.emplace("name", ctx.ref(expr.member));
                node.props.emplace("base", ctx.ref(expr.parent));
                add_child(expr.template_arguments);
            }

            void operator()(const Symbolic::TypeTraitIntrinsic& expr)
            {
                node.props.emplace("assort", to_string(expr.intrinsic));
                node.props.emplace("arguments", ctx.ref(expr.arguments));
            }

            void operator()(const Symbolic::DesignatedInitializer& expr)
            {
                node.props.emplace("name", ctx.reader.get(expr.member));
                add_child(expr.initializer);
            }

            void operator()(const Symbolic::PackedTemplateArguments& expr)
            {
                add_child(expr.arguments);
            }

            void operator()(const Symbolic::TokenExpression& expr)
            {
                add_child(expr.tokens);
            }

            void operator()(const Symbolic::AssignInitializer& expr)
            {
                add_child(expr.initializer);
            }

            void operator()(const Symbolic::Label& expr)
            {
                add_child(expr.designator);
            }

            template <typename T>
            void operator()(const T&)
            {
                // a new expression was added?
                node.id = "!!!!! TODO: " + node.id;
            }

        };
    }  // namespace [anon]

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
        ctx.reader.visit(expr, Expr_loader{ctx, node});
    }

    void load(Loader& ctx, Node& node, Symbolic::DefaultIndex index)
    {
        load(ctx, node, std::bit_cast<ExprIndex>(index));
    }

    std::string expr_list(Loader& ctx, ExprIndex index, std::string delimiters)
    {
        std::string result;
        if (index.sort() == ExprSort::Tuple)
        {
            auto& tuple = ctx.reader.get<Symbolic::TupleExpression>(index);
            bool first = true;
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

    namespace
    {
        struct Expr_translator
        {
            Loader& ctx;

            std::string operator()(const Symbolic::EmptyExpression& expr)
            {
                return "empty-expr";
            }

            std::string operator()(const Symbolic::Nullptr& expr)
            {
                return "nullptr";
            }

            std::string operator()(const Symbolic::This& expr)
            {
                return "this";
            }

            std::string operator()(const Symbolic::LiteralExpr& expr)
            {
                return to_string(ctx, expr.value);
            }

            std::string operator()(const Symbolic::TypeExpression& expr)
            {
                return ctx.ref(expr.denotation);
            }

            std::string operator()(const Symbolic::NamedDeclExpression& expr)
            {
                return std::string("decl-ref(") + ctx.ref(expr.decl) + ")";
            }

            template <typename T>
            std::string operator()(const T&)
            {
                return "";
            }
        };

    }  // namespace [anon]

    std::string get_string_if_possible(Loader& ctx, ExprIndex expr)
    {
        if (null(expr))
            return "no-expr";
        if (expr.sort() == ExprSort::VendorExtension)
            return "expr-vendor-" + std::to_string((int)expr.index());
        return ctx.reader.visit(expr, Expr_translator{ctx});
    }

} // namespace Module::util
