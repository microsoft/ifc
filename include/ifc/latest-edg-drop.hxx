// Microsoft (R) C/C++ Optimizing Compiler Front-End
// Copyright (C) Microsoft. All rights reserved.

#ifndef IFC_LATEST_EDG_DROP
#define IFC_LATEST_EDG_DROP

#include "ifc/abstract-sgraph.hxx"

// Structures no longer supported, but emitted in the latest drop to EDG are defined
// here for transitional purposes.  They are ephemeral and should be removed as soon
// as we get a turnaround from EDG.  The existence of these structures is a technical
// debt that needs to be paid down at the earliest convenience, with high priority.
// They need to be eradicated.  The ideal semantic content of this file is 'nothing'.
namespace Module::LatestEDGDrop {
    // IFC Version < 0.42
    enum class OldStmtSort : uint8_t {
        VendorExtension,            // Vendor-specific extensions
        Empty,                      // An empty statement (;)
        If,                         // If statements
        For,                        // C-style for loop, ranged-based for
        Case,                       // Case statements (as in switch statements)
        While,                      // A while loop, not do-while
        Block,                      // A {}-delimited sequence of statements
        Break,                      // Break statements (as in a loop or switch)
        Switch,                     // Switch statement
        DoWhile,                    // Do-while loops
        Default,                    // Default statement (as in a switch)
        Continue,                   // Continue statement (as in a loop)
        Expression,                 // A naked expression terminated with a semicolon
        Return,                     // A return statement
        Decl,                       // A declaration
        Expansion,                  // A pack-expansion of a statement
        SyntaxTree,                 // A syntax tree for an unelaborated statement.
        Count
    };
    // In order for this to work we need to ensure that this new statement sort has the same size as the old.  This way we do not need to
    // adjust the index structure itself.
    static_assert(index_like::tag_precision<OldStmtSort> == index_like::tag_precision<StmtSort>);
    static_assert(index_like::index_precision<OldStmtSort> == index_like::index_precision<StmtSort>);

    // A sequence of zero or more statements. Used to represent blocks/compound statements.
    struct TupleStatement : Symbolic::Tag<StmtSort::Block>, Sequence<StmtIndex, HeapSort::Stmt> {
        TupleStatement() = default;
        TupleStatement(Index f, Cardinality n) : Sequence{ f, n } { }
    };

    struct CaseStatement : Symbolic::Tag<StmtSort::Labeled> {
        CaseStatement() = default;
        CaseStatement(ExprIndex expr, const Symbolic::SourceLocation& locus) :
            expr{ expr }, locus{ locus } {}
        ExprIndex expr { };
        Symbolic::SourceLocation locus { };
    };

    struct DefaultStatement : Symbolic::Tag<StmtSort::Goto> {
        DefaultStatement() = default;
        DefaultStatement(const Symbolic::SourceLocation& locus) :
            locus{ locus } {}
        Symbolic::SourceLocation locus { };
    };

    struct ExpressionStatement : Symbolic::Tag<StmtSort::Expression> {
        ExpressionStatement() = default;
        ExpressionStatement(ExprIndex expr, const Symbolic::SourceLocation& locus) :
            expr{ expr }, locus{ locus } {}
        ExprIndex expr { };
        Symbolic::SourceLocation locus { };

        Symbolic::ExpressionStatement mimic() const
        {
            return { { .locus = locus }, expr };
        }
    };

    struct IfStatement : Symbolic::Tag<StmtSort::If> {
        IfStatement() = default;
        IfStatement(StmtIndex init, StmtIndex condition, StmtIndex then, StmtIndex el, const Symbolic::SourceLocation& locus) :
            init{init}, condition{condition}, then_stmt{then}, else_stmt{el}, locus{locus} {}
        StmtIndex init { };
        StmtIndex condition { };
        StmtIndex then_stmt { };
        StmtIndex else_stmt { };
        Symbolic::SourceLocation locus { };

        Symbolic::IfStatement mimic() const
        {
            return { { .locus = locus }, init, condition, then_stmt, else_stmt };
        }
    };

    struct WhileStatement : Symbolic::Tag<StmtSort::While> {
        WhileStatement() = default;
        WhileStatement(StmtIndex condition, StmtIndex body, const Symbolic::SourceLocation& locus) :
            condition{ condition }, body{ body }, locus{ locus } {}
        StmtIndex condition { };
        StmtIndex body { };
        Symbolic::SourceLocation locus { };

        Symbolic::WhileStatement mimic() const
        {
            return { { .locus = locus }, condition, body };
        }
    };

    struct DoWhileStatement : Symbolic::Tag<StmtSort::DoWhile> {
        DoWhileStatement() = default;
        DoWhileStatement(StmtIndex condition, StmtIndex body, const Symbolic::SourceLocation& locus) :
            condition{ condition }, body{ body }, locus{ locus } {}
        StmtIndex condition { };
        StmtIndex body { };
        Symbolic::SourceLocation locus { };

        Symbolic::DoWhileStatement mimic() const
        {
            return { { .locus = locus }, std::bit_cast<ExprIndex>(condition), body };
        }
    };

    struct ForStatement : Symbolic::Tag<StmtSort::For> {
        ForStatement() = default;
        ForStatement(StmtIndex init, StmtIndex condition, StmtIndex expression, StmtIndex body, const Symbolic::SourceLocation& locus) :
            init{ init }, condition{ condition }, expression{ expression }, body{ body }, locus{ locus } {}
        StmtIndex init { };
        StmtIndex condition { };
        StmtIndex expression { };
        StmtIndex body { };
        Symbolic::SourceLocation locus { };

        Symbolic::ForStatement mimic() const
        {
            return { { .locus = locus }, init, condition, expression, body };
        }
    };

    struct BreakStatement : Symbolic::Tag<StmtSort::Break> {
        BreakStatement() = default;
        BreakStatement(const Symbolic::SourceLocation& locus) :
            locus{ locus } {}
        Symbolic::SourceLocation locus { };

        Symbolic::BreakStatement mimic() const
        {
            return { { .locus = locus } };
        }
    };

    struct ContinueStatement : Symbolic::Tag<StmtSort::Continue> {
        ContinueStatement() = default;
        ContinueStatement(const Symbolic::SourceLocation& locus) :
            locus{ locus } {}
        Symbolic::SourceLocation locus {};

        Symbolic::ContinueStatement mimic() const
        {
            return { { .locus = locus } };
        }
    };

    struct SwitchStatement : Symbolic::Tag<StmtSort::Switch> {
        SwitchStatement() = default;
        SwitchStatement(StmtIndex init, ExprIndex control, StmtIndex body, const Symbolic::SourceLocation& locus) :
            init{init}, control{control}, body{body}, locus{locus} {}
        StmtIndex init { };
        ExprIndex control { };
        StmtIndex body { };
        Symbolic::SourceLocation locus { };

        Symbolic::SwitchStatement mimic() const
        {
            return { { .locus = locus }, init, control, body };
        }
    };

    struct DeclStatement : Symbolic::Tag<StmtSort::Decl> {
        DeclStatement() = default;
        DeclStatement(DeclIndex decl, const Symbolic::SourceLocation& locus):
            decl{ decl }, locus{ locus } { }
        DeclIndex decl { };
        Symbolic::SourceLocation locus { };

        Symbolic::DeclStatement mimic() const
        {
            return { { .locus = locus }, decl };
        }
    };

    struct ReturnStatement : Symbolic::Tag<StmtSort::Return> {
        ReturnStatement() = default;
        ReturnStatement(ExprIndex expr, TypeIndex function_type, TypeIndex expression_type, const Symbolic::SourceLocation& locus)
            : expr{ expr }, function_type{ function_type }, expression_type{ expression_type }, locus{ locus } { }
        ExprIndex expr { };
        TypeIndex function_type { };
        TypeIndex expression_type { };
        Symbolic::SourceLocation locus { };

        Symbolic::ReturnStatement mimic() const
        {
            return { { .locus = locus, .type = expression_type }, expr, function_type };
        }
    };
} // namespace Module::LatestEDGDrop

#endif  // IFC_LATEST_EDG_DROP
