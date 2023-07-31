#include "common.hxx"
#include "ifc/util.hxx"

namespace ifc::util {
    namespace {
        struct StmtLoader : detail::LoaderVisitorBase {
            void operator()(const symbolic::ReturnStmt& ret)
            {
                node.props.emplace("function_type", ctx.ref(ret.function_type));
                node.props.emplace("expression_type", ctx.ref(ret.type));
                add_child_if_not_null(ret.expr);
            }

            void operator()(const symbolic::TupleStmt& stmt_tuple)
            {
                for (auto item : ctx.reader.sequence(stmt_tuple))
                    add_child(item);
            }

            void operator()(const symbolic::BlockStmt& stmt_block)
            {
                node.props.emplace("locus", to_string(stmt_block.locus));
                for (auto item : ctx.reader.sequence(stmt_block))
                    add_child(item);
            }

            void operator()(const symbolic::DeclStmt& decl_stmt)
            {
                add_child(decl_stmt.decl);
            }

            void operator()(const symbolic::ExpressionStmt& expr_stmt)
            {
                add_child(expr_stmt.expr);
            }

            void operator()(const symbolic::IfStmt& stmt)
            {
                add_child_if_not_null(stmt.init);
                add_child(stmt.condition);
                add_child(stmt.consequence);
                add_child(stmt.alternative);
            }

            void operator()(const symbolic::WhileStmt& while_stmt)
            {
                add_child(while_stmt.condition);
                add_child(while_stmt.body);
            }

            void operator()(const symbolic::DoWhileStmt& do_stmt)
            {
                node.props.emplace("locus", to_string(do_stmt.locus));
                add_child(do_stmt.body);
                add_child(do_stmt.condition);
            }

            void operator()(const symbolic::ForStmt& for_stmt)
            {
                add_child(for_stmt.init);
                add_child(for_stmt.condition);
                add_child(for_stmt.increment);
                add_child(for_stmt.body);
            }

            void operator()(const symbolic::SwitchStmt& switch_stmt)
            {
                add_child_if_not_null(switch_stmt.init);
                add_child(switch_stmt.control);
                add_child(switch_stmt.body);
            }

            void operator()(const symbolic::LabeledStmt& labeled)
            {
                add_child(labeled.label);
                add_child(labeled.statement);
            }

            void operator()(const symbolic::GotoStmt& goto_stmt)
            {
                add_child(goto_stmt.target);
            }

            // nothing todo for these
            // clang-format off
            void operator()(const symbolic::BreakStmt&) {}
            void operator()(const symbolic::ContinueStmt&){}
            // clang-format on

            template<typename T>
            void operator()(const T&)
            {
                // a new statement was added?
                node.id = "!!!!! TODO: " + node.id;
            }
        };
    } // namespace

    void load(Loader& ctx, Node& node, StmtIndex index)
    {
        if (null(index))
        {
            node.id = "no-stmt";
            return;
        }

        node.id = sort_name(index.sort());
        ctx.reader.visit(index, StmtLoader{ctx, node});
    }

} // namespace ifc::util
