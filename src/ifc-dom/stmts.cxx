#include "common.hxx"
#include "ifc/util.hxx"

namespace Module::util
{
    namespace
    {
        struct Stmt_loader : detail::Loader_visitor_base
        {
            void operator()(const Symbolic::ReturnStatement& ret)
            {
                node.props.emplace("function_type", ctx.ref(ret.function_type));
                node.props.emplace("expression_type", ctx.ref(ret.type));
                add_child_if_not_null(ret.expr);
            }

            void operator()(const Symbolic::TupleStatement& stmt_tuple)
            {
                for (auto item: ctx.reader.sequence(stmt_tuple))
                    add_child(item);
            }

            void operator()(const Symbolic::BlockStatement& stmt_block)
            {
                node.props.emplace("locus", to_string(stmt_block.locus));
                for (auto item : ctx.reader.sequence(stmt_block))
                    add_child(item);
            }

            void operator()(const Symbolic::DeclStatement& decl_stmt)
            {
                add_child(decl_stmt.decl);
            }

            void operator()(const Symbolic::ExpressionStatement& expr_stmt)
            {
                add_child(expr_stmt.expr);
            }

            void operator()(const Symbolic::IfStatement& stmt)
            {
                add_child_if_not_null(stmt.init);
                add_child(stmt.condition);
                add_child(stmt.consequence);
                add_child(stmt.alternative);
            }

            void operator()(const Symbolic::WhileStatement& while_stmt)
            {
                add_child(while_stmt.condition);
                add_child(while_stmt.body);
            }

            void operator()(const Symbolic::DoWhileStatement& do_stmt)
            {
                node.props.emplace("locus", to_string(do_stmt.locus));
                add_child(do_stmt.body);
                add_child(do_stmt.condition);
            }

            void operator()(const Symbolic::ForStatement& for_stmt)
            {
                add_child(for_stmt.init);
                add_child(for_stmt.condition);
                add_child(for_stmt.increment);
                add_child(for_stmt.body);
            }

            void operator()(const Symbolic::SwitchStatement& switch_stmt)
            {
                add_child_if_not_null(switch_stmt.init);
                add_child(switch_stmt.control);
                add_child(switch_stmt.body);
            }

            void operator()(const Symbolic::LabeledStatement& labeled)
            {
                add_child(labeled.label);
                add_child(labeled.statement);
            }

            void operator()(const Symbolic::GotoStatement& goto_stmt)
            {
                add_child(goto_stmt.target);
            }

            // nothing todo for these
            // clang-format off
            void operator()(const Symbolic::BreakStatement&) {}
            void operator()(const Symbolic::ContinueStatement&){}
            // clang-format on

            template <typename T>
            void operator()(const T&)
            {
                // a new statement was added?
                node.id = "!!!!! TODO: " + node.id;
            }
        };
    }  // namespace [anon]

    void load(Loader& ctx, Node& node, StmtIndex index)
    {
        if (null(index))
        {
            node.id = "no-stmt";
            return;
        }

        node.id = sort_name(index.sort());
        ctx.reader.visit(index, Stmt_loader{ctx, node});
    }

} // namespace Module::util
