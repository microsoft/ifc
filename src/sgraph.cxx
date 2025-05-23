// Copyright Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <iterator>
#include <map>
#include <string_view>
#include <algorithm>

#include <ifc/abstract-sgraph.hxx>

namespace ifc {
    namespace {
        template<typename T>
        struct SortNameMapEntry {
            constexpr SortNameMapEntry(T t, const char* n) : name{n}, sort{t} {}
            const char* name;
            T sort;
        };

        // This predicate holds whenever its argument (a table) is a retractible space/object
        // when looking at its keys.
        // That means its entries' keys form a linear progression where each key is
        // equal to that entry's position in the table, after appropriate isomorphic conversion.
        // When the predicate holds at compile time, that implies runtime look up by key
        // can be implemented by a simple constant-time indexing instead of an elaborate search.
        template<typename T, auto N>
        constexpr bool retractible_by_key(const SortNameMapEntry<T> (&table)[N])
        {
            if constexpr (N != count<T>)
                return false;
            for (std::underlying_type_t<T> i = 0; i < count<T>; ++i)
            {
                if (i != ifc::to_underlying(table[i].sort))
                    return false;
            }
            return true;
        }

        constexpr SortNameMapEntry<DeclSort> declsort_table[] = {
            {DeclSort::VendorExtension, "decl.vendor-extension"},
            {DeclSort::Enumerator, "decl.enumerator"},
            {DeclSort::Variable, "decl.variable"},
            {DeclSort::Parameter, "decl.parameter"},
            {DeclSort::Field, "decl.field"},
            {DeclSort::Bitfield, "decl.bitfield"},
            {DeclSort::Scope, "decl.scope"},
            {DeclSort::Enumeration, "decl.enum"},
            {DeclSort::Alias, "decl.alias"},
            {DeclSort::Temploid, "decl.temploid"},
            {DeclSort::Template, "decl.template"},
            {DeclSort::PartialSpecialization, "decl.partial-specialization"},
            {DeclSort::Specialization, "decl.specialization"},
            {DeclSort::DefaultArgument, "decl.default-arg"},
            {DeclSort::Concept, "decl.concept"},
            {DeclSort::Function, "decl.function"},
            {DeclSort::Method, "decl.method"},
            {DeclSort::Constructor, "decl.constructor"},
            {DeclSort::InheritedConstructor, "decl.inherited-constructor"},
            {DeclSort::Destructor, "decl.destructor"},
            {DeclSort::Reference, "decl.reference"},
            {DeclSort::Using, "decl.using-declaration"},
            {DeclSort::UnusedSort0, "decl.unused0"},
            {DeclSort::Friend, "decl.friend"},
            {DeclSort::Expansion, "decl.expansion"},
            {DeclSort::DeductionGuide, "decl.deduction-guide"},
            {DeclSort::Barren, "decl.barren"},
            {DeclSort::Tuple, "decl.tuple"},
            {DeclSort::SyntaxTree, "decl.syntax-tree"},
            {DeclSort::Intrinsic, "decl.intrinsic"},
            {DeclSort::Property, "decl.property"},
            {DeclSort::OutputSegment, "decl.segment"},
        };

        static_assert(retractible_by_key(declsort_table));

        constexpr SortNameMapEntry<TypeSort> typesort_table[] = {
            {TypeSort::VendorExtension, "type.vendor-extension"},
            {TypeSort::Fundamental, "type.fundamental"},
            {TypeSort::Designated, "type.designated"},
            {TypeSort::Tor, "type.tor"},
            {TypeSort::Syntactic, "type.syntactic"},
            {TypeSort::Expansion, "type.expansion"},
            {TypeSort::Pointer, "type.pointer"},
            {TypeSort::PointerToMember, "type.pointer-to-member"},
            {TypeSort::LvalueReference, "type.lvalue-reference"},
            {TypeSort::RvalueReference, "type.rvalue-reference"},
            {TypeSort::Function, "type.function"},
            {TypeSort::Method, "type.nonstatic-member-function"},
            {TypeSort::Array, "type.array"},
            {TypeSort::Typename, "type.typename"},
            {TypeSort::Qualified, "type.qualified"},
            {TypeSort::Base, "type.base"},
            {TypeSort::Decltype, "type.decltype"},
            {TypeSort::Placeholder, "type.placeholder"},
            {TypeSort::Tuple, "type.tuple"},
            {TypeSort::Forall, "type.forall"},
            {TypeSort::Unaligned, "type.unaligned"},
            {TypeSort::SyntaxTree, "type.syntax-tree"},
        };

        static_assert(retractible_by_key(typesort_table));

        constexpr SortNameMapEntry<StmtSort> stmtsort_table[] = {
            {StmtSort::VendorExtension, "stmt.vendor-extension"},
            {StmtSort::Try, "stmt.try"},
            {StmtSort::If, "stmt.if"},
            {StmtSort::For, "stmt.for"},
            {StmtSort::Labeled, "stmt.labeled"},
            {StmtSort::While, "stmt.while"},
            {StmtSort::Block, "stmt.block"},
            {StmtSort::Break, "stmt.break"},
            {StmtSort::Switch, "stmt.switch"},
            {StmtSort::DoWhile, "stmt.do-while"},
            {StmtSort::Goto, "stmt.goto"},
            {StmtSort::Continue, "stmt.continue"},
            {StmtSort::Expression, "stmt.expression"},
            {StmtSort::Return, "stmt.return"},
            {StmtSort::Decl, "stmt.decl"},
            {StmtSort::Expansion, "stmt.expansion"},
            {StmtSort::SyntaxTree, "stmt.syntax-tree"},
            {StmtSort::Handler, "stmt.handler"},
            {StmtSort::Tuple, "stmt.tuple"},
            {StmtSort::Dir, "stmt.dir"},
        };

        static_assert(retractible_by_key(stmtsort_table));

        constexpr SortNameMapEntry<ExprSort> exprsort_table[] = {
            {ExprSort::VendorExtension, "expr.vendor-extension"},
            {ExprSort::Empty, "expr.empty"},
            {ExprSort::Literal, "expr.literal"},
            {ExprSort::Lambda, "expr.lambda"},
            {ExprSort::Type, "expr.type"},
            {ExprSort::NamedDecl, "expr.decl"},
            {ExprSort::UnresolvedId, "expr.unresolved"},
            {ExprSort::TemplateId, "expr.template-id"},
            {ExprSort::UnqualifiedId, "expr.unqualified-id"},
            {ExprSort::SimpleIdentifier, "expr.simple-identifier"},
            {ExprSort::Pointer, "expr.pointer"},
            {ExprSort::QualifiedName, "expr.qualified-name"},
            {ExprSort::Path, "expr.path"},
            {ExprSort::Read, "expr.read"},
            {ExprSort::Monad, "expr.monad"},
            {ExprSort::Dyad, "expr.dyad"},
            {ExprSort::Triad, "expr.triad"},
            {ExprSort::String, "expr.strings"},
            {ExprSort::Temporary, "expr.temporary"},
            {ExprSort::Call, "expr.call"},
            {ExprSort::MemberInitializer, "expr.member-initializer"},
            {ExprSort::MemberAccess, "expr.member-access"},
            {ExprSort::InheritancePath, "expr.inheritance-path"},
            {ExprSort::InitializerList, "expr.initializer-list"},
            {ExprSort::Cast, "expr.cast"},
            {ExprSort::Condition, "expr.condition"},
            {ExprSort::ExpressionList, "expr.expression-list"},
            {ExprSort::SizeofType, "expr.sizeof-type"},
            {ExprSort::Alignof, "expr.alignof"},
            {ExprSort::Label, "expr.label"},
            {ExprSort::UnusedSort0, "expr.unused0"},
            {ExprSort::Typeid, "expr.typeid"},
            {ExprSort::DestructorCall, "expr.destructor-call"},
            {ExprSort::SyntaxTree, "expr.syntax-tree"},
            {ExprSort::FunctionString, "expr.function-string"},
            {ExprSort::CompoundString, "expr.compound-string"},
            {ExprSort::StringSequence, "expr.string-sequence"},
            {ExprSort::Initializer, "expr.initializer"},
            {ExprSort::Requires, "expr.requires"},
            {ExprSort::UnaryFold, "expr.unary-fold"},
            {ExprSort::BinaryFold, "expr.binary-fold"},
            {ExprSort::HierarchyConversion, "expr.hierarchy-conversion"},
            {ExprSort::ProductTypeValue, "expr.product-type-value"},
            {ExprSort::SumTypeValue, "expr.sum-type-value"},
            {ExprSort::UnusedSort1, "expr.unused1"},
            {ExprSort::ArrayValue, "expr.array-value"},
            {ExprSort::DynamicDispatch, "expr.dynamic-dispatch"},
            {ExprSort::VirtualFunctionConversion, "expr.virtual-function-conversion"},
            {ExprSort::Placeholder, "expr.placeholder"},
            {ExprSort::Expansion, "expr.expansion"},
            {ExprSort::Generic, "expr.generic"},
            {ExprSort::Tuple, "expr.tuple"},
            {ExprSort::Nullptr, "expr.nullptr"},
            {ExprSort::This, "expr.this"},
            {ExprSort::TemplateReference, "expr.template-reference"},
            {ExprSort::Statement, "expr.stmt"},
            {ExprSort::TypeTraitIntrinsic, "expr.type-trait"},
            {ExprSort::DesignatedInitializer, "expr.designated-init"},
            {ExprSort::PackedTemplateArguments, "expr.packed-template-arguments"},
            {ExprSort::Tokens, "expr.tokens"},
            {ExprSort::AssignInitializer, "expr.assign-initializer"},
        };

        static_assert(retractible_by_key(exprsort_table));

        constexpr SortNameMapEntry<ChartSort> chartsort_table[] = {
            {ChartSort::None, "chart.none"},
            {ChartSort::Unilevel, "chart.unilevel"},
            {ChartSort::Multilevel, "chart.multilevel"},
        };

        static_assert(retractible_by_key(chartsort_table));

        constexpr SortNameMapEntry<NameSort> namesort_table[] = {
            {NameSort::Identifier, "name.identifier"},  {NameSort::Operator, "name.operator"},
            {NameSort::Conversion, "name.conversion"},  {NameSort::Literal, "name.literal"},
            {NameSort::Template, "name.template"},      {NameSort::Specialization, "name.specialization"},
            {NameSort::SourceFile, "name.source-file"}, {NameSort::Guide, "name.guide"},
        };

        static_assert(retractible_by_key(namesort_table));

        constexpr SortNameMapEntry<SyntaxSort> syntax_sort_table[] = {
            {SyntaxSort::VendorExtension, "syntax.vendor-extension"},
            {SyntaxSort::SimpleTypeSpecifier, "syntax.simple-type-specifier"},
            {SyntaxSort::DecltypeSpecifier, "syntax.decltype-specifier"},
            {SyntaxSort::PlaceholderTypeSpecifier, "syntax.placeholder-type-specifier"},
            {SyntaxSort::TypeSpecifierSeq, "syntax.type-specifier-seq"},
            {SyntaxSort::DeclSpecifierSeq, "syntax.decl-specifier-seq"},
            {SyntaxSort::VirtualSpecifierSeq, "syntax.virtual-specifier-seq"},
            {SyntaxSort::NoexceptSpecification, "syntax.noexcept-specification"},
            {SyntaxSort::ExplicitSpecifier, "syntax.explicit-specifier"},
            {SyntaxSort::EnumSpecifier, "syntax.enum-specifier"},
            {SyntaxSort::EnumeratorDefinition, "syntax.enumerator-definition"},
            {SyntaxSort::ClassSpecifier, "syntax.class-specifier"},
            {SyntaxSort::MemberSpecification, "syntax.member-specification"},
            {SyntaxSort::MemberDeclaration, "syntax.member-declaration"},
            {SyntaxSort::MemberDeclarator, "syntax.member-declarator"},
            {SyntaxSort::AccessSpecifier, "syntax.access-specifier"},
            {SyntaxSort::BaseSpecifierList, "syntax.base-specifier-list"},
            {SyntaxSort::BaseSpecifier, "syntax.base-specifier"},
            {SyntaxSort::TypeId, "syntax.type-id"},
            {SyntaxSort::TrailingReturnType, "syntax.trailing-return-type"},
            {SyntaxSort::Declarator, "syntax.declarator"},
            {SyntaxSort::PointerDeclarator, "syntax.pointer-declarator"},
            {SyntaxSort::ArrayDeclarator, "syntax.array-declarator"},
            {SyntaxSort::FunctionDeclarator, "syntax.function-declarator"},
            {SyntaxSort::ArrayOrFunctionDeclarator, "syntax.array-or-function-declarator"},
            {SyntaxSort::ParameterDeclarator, "syntax.parameter-declarator"},
            {SyntaxSort::InitDeclarator, "syntax.init-declarator"},
            {SyntaxSort::NewDeclarator, "syntax.new-declarator"},
            {SyntaxSort::SimpleDeclaration, "syntax.simple-declaration"},
            {SyntaxSort::ExceptionDeclaration, "syntax.exception-declaration"},
            {SyntaxSort::ConditionDeclaration, "syntax.condition-declaration"},
            {SyntaxSort::StaticAssertDeclaration, "syntax.static_assert-declaration"},
            {SyntaxSort::AliasDeclaration, "syntax.alias-declaration"},
            {SyntaxSort::ConceptDefinition, "syntax.concept-definition"},
            {SyntaxSort::CompoundStatement, "syntax.compound-statement"},
            {SyntaxSort::ReturnStatement, "syntax.return-statement"},
            {SyntaxSort::IfStatement, "syntax.if-statement"},
            {SyntaxSort::WhileStatement, "syntax.while-statement"},
            {SyntaxSort::DoWhileStatement, "syntax.do-while-statement"},
            {SyntaxSort::ForStatement, "syntax.for-statement"},
            {SyntaxSort::InitStatement, "syntax.init-statement"},
            {SyntaxSort::RangeBasedForStatement, "syntax.range-based-for-statement"},
            {SyntaxSort::ForRangeDeclaration, "syntax.for-range-declaration"},
            {SyntaxSort::LabeledStatement, "syntax.labeled-statement"},
            {SyntaxSort::BreakStatement, "syntax.break-statement"},
            {SyntaxSort::ContinueStatement, "syntax.continue-statement"},
            {SyntaxSort::SwitchStatement, "syntax.switch-statement"},
            {SyntaxSort::GotoStatement, "syntax.goto-statement"},
            {SyntaxSort::DeclarationStatement, "syntax.declaration-statement"},
            {SyntaxSort::ExpressionStatement, "syntax.expression-statement"},
            {SyntaxSort::TryBlock, "syntax.try-block"},
            {SyntaxSort::Handler, "syntax.handler"},
            {SyntaxSort::HandlerSeq, "syntax.handler-seq"},
            {SyntaxSort::FunctionTryBlock, "syntax.function-try-block"},
            {SyntaxSort::TypeIdListElement, "syntax.type-list-element"},
            {SyntaxSort::DynamicExceptionSpec, "syntax.dynamic-exception-specification"},
            {SyntaxSort::StatementSeq, "syntax.statement-seq"},
            {SyntaxSort::FunctionBody, "syntax.function-body"},
            {SyntaxSort::Expression, "syntax.expression"},
            {SyntaxSort::FunctionDefinition, "syntax.function-definition"},
            {SyntaxSort::MemberFunctionDeclaration, "syntax.member-function-declaration"},
            {SyntaxSort::TemplateDeclaration, "syntax.template-declaration"},
            {SyntaxSort::RequiresClause, "syntax.requires-clause"},
            {SyntaxSort::SimpleRequirement, "syntax.simple-requirement"},
            {SyntaxSort::TypeRequirement, "syntax.type-requirement"},
            {SyntaxSort::CompoundRequirement, "syntax.compound-requirement"},
            {SyntaxSort::NestedRequirement, "syntax.nested-requirement"},
            {SyntaxSort::RequirementBody, "syntax.requirement-body"},
            {SyntaxSort::TypeTemplateParameter, "syntax.type-template-parameter"},
            {SyntaxSort::TemplateTemplateParameter, "syntax.template-template-parameter"},
            {SyntaxSort::TypeTemplateArgument, "syntax.type-template-argument"},
            {SyntaxSort::NonTypeTemplateArgument, "syntax.non-type-template-argument"},
            {SyntaxSort::TemplateParameterList, "syntax.template-parameter-list"},
            {SyntaxSort::TemplateArgumentList, "syntax.template-argument-list"},
            {SyntaxSort::TemplateId, "syntax.template-id"},
            {SyntaxSort::MemInitializer, "syntax.mem-initializer"},
            {SyntaxSort::CtorInitializer, "syntax.ctor-initializer"},
            {SyntaxSort::LambdaIntroducer, "syntax.lambda-introducer"},
            {SyntaxSort::LambdaDeclarator, "syntax.lambda-declarator"},
            {SyntaxSort::CaptureDefault, "syntax.capture-default"},
            {SyntaxSort::SimpleCapture, "syntax.simple-capture"},
            {SyntaxSort::InitCapture, "syntax.init-capture"},
            {SyntaxSort::ThisCapture, "syntax.this-capture"},
            {SyntaxSort::AttributedStatement, "syntax.attributed-statement"},
            {SyntaxSort::AttributedDeclaration, "syntax.attributed-declaration"},
            {SyntaxSort::AttributeSpecifierSeq, "syntax.attribute-specifier-seq"},
            {SyntaxSort::AttributeSpecifier, "syntax.attribute-specifier"},
            {SyntaxSort::AttributeUsingPrefix, "syntax.attribute-using-prefix"},
            {SyntaxSort::Attribute, "syntax.attribute"},
            {SyntaxSort::AttributeArgumentClause, "syntax.attribute-argument-clause"},
            {SyntaxSort::Alignas, "syntax.alignas"},
            {SyntaxSort::UsingDeclaration, "syntax.using-declaration"},
            {SyntaxSort::UsingDeclarator, "syntax.using-declarator"},
            {SyntaxSort::UsingDirective, "syntax.using-directive"},
            {SyntaxSort::ArrayIndex, "syntax.array-index"},
            {SyntaxSort::SEHTry, "syntax.structured-exception-try"},
            {SyntaxSort::SEHExcept, "syntax.structured-exception-except"},
            {SyntaxSort::SEHFinally, "syntax.structured-exception-finally"},
            {SyntaxSort::SEHLeave, "syntax.structured-exception-leave"},
            {SyntaxSort::TypeTraitIntrinsic, "syntax.type-trait-intrinsic"},
            {SyntaxSort::Tuple, "syntax.tuple"},
            {SyntaxSort::AsmStatement, "syntax.asm-statement"},
            {SyntaxSort::NamespaceAliasDefinition, "syntax.namespace-alias-definition"},
            {SyntaxSort::Super, "syntax.super"},
            {SyntaxSort::UnaryFoldExpression, "syntax.unary-fold-expression"},
            {SyntaxSort::BinaryFoldExpression, "syntax.binary-fold-expression"},
            {SyntaxSort::EmptyStatement, "syntax.empty-statement"},
            {SyntaxSort::StructuredBindingDeclaration, "syntax.structured-binding-declaration"},
            {SyntaxSort::StructuredBindingIdentifier, "syntax.structured-binding-identifier"},
            {SyntaxSort::UsingEnumDeclaration, "syntax.using-enum-declaration"},
            {SyntaxSort::IfConsteval, "syntax.if-consteval"},
        };

        static_assert(retractible_by_key(syntax_sort_table));

        constexpr SortNameMapEntry<MacroSort> macro_sort_table[] = {
            {MacroSort::ObjectLike, "macro.object-like"},
            {MacroSort::FunctionLike, "macro.function-like"},
        };

        static_assert(retractible_by_key(macro_sort_table));

        constexpr SortNameMapEntry<HeapSort> heapsort_table[] = {
            {HeapSort::Decl, "heap.decl"},   {HeapSort::Type, "heap.type"},  {HeapSort::Stmt, "heap.stmt"},
            {HeapSort::Expr, "heap.expr"},   {HeapSort::Syntax, "heap.syn"}, {HeapSort::Word, "heap.word"},
            {HeapSort::Chart, "heap.chart"}, {HeapSort::Spec, "heap.spec"},  {HeapSort::Form, "heap.pp"},
            {HeapSort::Attr, "heap.attr"},   {HeapSort::Dir, "heap.dir"},    {HeapSort::Vendor, "heap.vendor"},
        };

        static_assert(retractible_by_key(heapsort_table));

        constexpr SortNameMapEntry<FormSort> formsort_table[] = {
            {FormSort::Identifier, "pp.ident"},    {FormSort::Number, "pp.num"},
            {FormSort::Character, "pp.char"},      {FormSort::String, "pp.string"},
            {FormSort::Operator, "pp.op"},         {FormSort::Keyword, "pp.key"},
            {FormSort::Whitespace, "pp.space"},    {FormSort::Parameter, "pp.param"},
            {FormSort::Stringize, "pp.to-string"}, {FormSort::Catenate, "pp.catenate"},
            {FormSort::Pragma, "pp.pragma"},       {FormSort::Header, "pp.header"},
            {FormSort::Parenthesized, "pp.paren"}, {FormSort::Tuple, "pp.tuple"},
            {FormSort::Junk, "pp.junk"},
        };

        static_assert(retractible_by_key(formsort_table));

        constexpr SortNameMapEntry<TraitSort> traitsort_table[] = {
            {TraitSort::MappingExpr, "trait.mapping-expr"}, {TraitSort::AliasTemplate, "trait.alias-template"},
            {TraitSort::Friends, "trait.friend"},           {TraitSort::Specializations, "trait.specialization"},
            {TraitSort::Requires, "trait.requires"},        {TraitSort::Attributes, "trait.attribute"},
            {TraitSort::Deprecated, "trait.deprecated"},    {TraitSort::DeductionGuides, "trait.deduction-guides"},
        };

        static_assert(retractible_by_key(traitsort_table));

        constexpr SortNameMapEntry<MsvcTraitSort> msvc_traitsort_table[] = {
            {MsvcTraitSort::Uuid, ".msvc.trait.uuid"},
            {MsvcTraitSort::Segment, ".msvc.trait.code-segment"},
            {MsvcTraitSort::SpecializationEncoding, ".msvc.trait.specialization-encodings"},
            {MsvcTraitSort::SalAnnotation, ".msvc.trait.code-analysis.sal"},
            {MsvcTraitSort::FunctionParameters, ".msvc.trait.named-function-parameters"},
            {MsvcTraitSort::InitializerLocus, ".msvc.trait.entity-initializer-locus"},
            {MsvcTraitSort::TemplateTemplateParameters, ".msvc.trait.template-template-parameter-classes"},
            {MsvcTraitSort::CodegenExpression, ".msvc.trait.codegen-expression-trees"},
            {MsvcTraitSort::Vendor, ".msvc.trait.vendor-traits"},
            {MsvcTraitSort::DeclAttributes, ".msvc.trait.decl-attrs"}, // FIXME: This should go away once we have a bit
                                                                       // in the graph for C++ attributes.
            {MsvcTraitSort::StmtAttributes, ".msvc.trait.stmt-attrs"}, // FIXME: this should go away once we have a bit
                                                                       // in the graph for C++ attributes.
            {MsvcTraitSort::CodegenMappingExpr, ".msvc.trait.codegen-mapping-expr"},
            {MsvcTraitSort::DynamicInitVariable, ".msvc.trait.dynamic-init-variable"},
            {MsvcTraitSort::CodegenLabelProperties, ".msvc.trait.codegen-label-properties"},
            {MsvcTraitSort::CodegenSwitchType, ".msvc.trait.codegen-switch-type"},
            {MsvcTraitSort::CodegenDoWhileStmt, ".msvc.trait.codegen-dowhile-stmt"},
            {MsvcTraitSort::LexicalScopeIndex, ".msvc.trait.lexical-scope-index"},
            {MsvcTraitSort::FileBoundary, ".msvc.trait.file-boundary"},
            {MsvcTraitSort::HeaderUnitSourceFile, ".msvc.trait.header-unit-source-file"},
            {MsvcTraitSort::FileHash, ".msvc.trait.file-hash"},
            {MsvcTraitSort::DebugRecord, ".msvc.trait.debug-record"},
        };

        static_assert(retractible_by_key(msvc_traitsort_table));

        using ToCMemberOffset = PartitionSummaryData TableOfContents::*;

        constexpr SortNameMapEntry<ToCMemberOffset> toc_members_table[] = {
            {&TableOfContents::command_line, "command_line"},
            {&TableOfContents::exported_modules, "module.exported"},
            {&TableOfContents::imported_modules, "module.imported"},
            {&TableOfContents::u64s, "const.i64"},
            {&TableOfContents::fps, "const.f64"},
            {&TableOfContents::string_literals, "const.str"},
            {&TableOfContents::states, "pragma.state"},
            {&TableOfContents::lines, "src.line"},
            {&TableOfContents::words, "src.word"},
            {&TableOfContents::sentences, "src.sentence"},
            {&TableOfContents::scopes, "scope.desc"},
            {&TableOfContents::entities, "scope.member"},
            {&TableOfContents::spec_forms, "form.spec"},
            //{ &TableOfContents::names, "name."  },
            //{ &TableOfContents::decls, "decl."  },
            //{ &TableOfContents::types, "type."  },
            //{ &TableOfContents::stmts, "stmt."  },
            //{ &TableOfContents::exprs, "expr."  },
            //{ &TableOfContents::elements, "syntax."  },
            {&TableOfContents::charts, chartsort_table[ifc::to_underlying(ChartSort::Unilevel)].name},
            {&TableOfContents::multi_charts, chartsort_table[ifc::to_underlying(ChartSort::Multilevel)].name},
            // { &TableOfContents::heaps, "heap." },
            //{ &TableOfContents::requires_traits, "trait.requires" },
            //{ &TableOfContents::deprecated_traits, "trait.deprecated" },
            //{ &TableOfContents::specialization_traits, "trait.specialization" },
            // { &TableOfContents::friend_traits, "trait.friend" },
            // { &TableOfContents::constexpr_function_traits,"trait.mapping-expr" },
            // { &TableOfContents::alias_template_traits, "trait.alias-template" },
            //{ &TableOfContents::attributes, "trait.attribute" },
            //{ &TableOfContents::segments, ".msvc.trait.code-segment" },
            //{ &TableOfContents::vendor_traits, ".msvc.trait.vendor-traits" },
            // { &TableOfContents::uuids, ".msvc.trait.uuid" },
            {&TableOfContents::pragma_warnings, ".msvc.trait.pragma-warnings"},
            //{ &TableOfContents::specialization_encodings, ".msvc.trait.specialization-encodings" },
            //{ &TableOfContents::named_function_parameters, ".msvc.trait.named-function-parameters" },
            //{ &TableOfContents::template_template_parameter_hacks, ".msvc.trait.template-template-parameter-classes"
            //}, { &TableOfContents::codegen_expression_trees, ".msvc.trait.codegen-expression-trees" }, {
            //&TableOfContents::entity_initializer_locus, ".msvc.trait.entity-initializer-locus" },
            // { &TableOfContents::macros, "macro." },
            //{ &TableOfContents::sal_annotations, ".msvc.trait.code-analysis.sal"},
            {&TableOfContents::implementation_pragmas, ".msvc.trait.impl-pragmas"},
            {&TableOfContents::debug_records, ".msvc.trait.debug-records"},
            {&TableOfContents::gmf_specializations, ".msvc.trait.gmf-specializations"},
        };

        constexpr SortNameMapEntry<PragmaSort> pragma_sort_table[] = {
            {PragmaSort::VendorExtension, "pragma-directive.vendor-extension"},
            {PragmaSort::Expr, "pragma-directive.expr"},
        };

        static_assert(retractible_by_key(pragma_sort_table));

        constexpr SortNameMapEntry<AttrSort> attrsort_table[] = {
            {AttrSort::Nothing, "attr.nothing"},   {AttrSort::Basic, "attr.basic"},
            {AttrSort::Scoped, "attr.scoped"},     {AttrSort::Labeled, "attr.labeled"},
            {AttrSort::Called, "attr.called"},     {AttrSort::Expanded, "attr.expanded"},
            {AttrSort::Factored, "attr.factored"}, {AttrSort::Elaborated, "attr.elaborated"},
            {AttrSort::Tuple, "attr.tuple"},
        };

        static_assert(retractible_by_key(attrsort_table));

        constexpr SortNameMapEntry<DirSort> dirsort_table[] = {
            {DirSort::VendorExtension, "dir.vendor-extension"},
            {DirSort::Empty, "dir.empty"},
            {DirSort::Attribute, "dir.attribute"},
            {DirSort::Pragma, "dir.pragma"},
            {DirSort::Using, "dir.using"},
            {DirSort::DeclUse, "dir.decl-use"},
            {DirSort::Expr, "dir.expr"},
            {DirSort::StructuredBinding, "dir.struct-binding"},
            {DirSort::SpecifiersSpread, "dir.specifiers-spread"},
            {DirSort::Stmt, "dir.stmt"},
            {DirSort::Unused1, "dir.unused1"},
            {DirSort::Unused2, "dir.unused2"},
            {DirSort::Unused3, "dir.unused3"},
            {DirSort::Unused4, "dir.unused4"},
            {DirSort::Unused5, "dir.unused5"},
            {DirSort::Unused6, "dir.unused6"},
            {DirSort::Unused7, "dir.unused7"},
            {DirSort::Unused8, "dir.unused8"},
            {DirSort::Unused9, "dir.unused9"},
            {DirSort::Unused10, "dir.unused10"},
            {DirSort::Unused11, "dir.unused11"},
            {DirSort::Unused12, "dir.unused12"},
            {DirSort::Unused13, "dir.unused13"},
            {DirSort::Unused14, "dir.unused14"},
            {DirSort::Unused15, "dir.unused15"},
            {DirSort::Unused16, "dir.unused16"},
            {DirSort::Unused17, "dir.unused17"},
            {DirSort::Unused18, "dir.unused18"},
            {DirSort::Unused19, "dir.unused19"},
            {DirSort::Unused20, "dir.unused20"},
            {DirSort::Unused21, "dir.unused21"},
            {DirSort::Tuple, "dir.tuple"},
        };

        static_assert(retractible_by_key(dirsort_table));

        constexpr SortNameMapEntry<VendorSort> vendor_sort_table[] = {
            {VendorSort::SEHTry, "vendor.seh-try"},
            {VendorSort::SEHFinally, "vendor.seh-finally"},
            {VendorSort::SEHExcept, "vendor.seh-except"},
            {VendorSort::SEHLeave, "vendor.seh-leave"},
        };

        static_assert(retractible_by_key(vendor_sort_table));

        // This is a sorted array by the 'name' field in 'SortNameMapEntry'
        template<typename S, auto N>
        class FieldOffsetTable {
        public:
            struct Entry {
                std::string_view name;
                S sort{};
            };

            explicit constexpr FieldOffsetTable(const SortNameMapEntry<S> (&seq)[N]) noexcept
            {
                for (std::size_t i = 0; i != N; ++i)
                {
                    sorted_data[i].name = seq[i].name;
                    sorted_data[i].sort = seq[i].sort;
                }

                std::sort(begin(), end(),
                          [](const auto& entry1, const auto& entry2) { return entry1.name < entry2.name; });
            }

            constexpr auto begin() noexcept
            {
                return sorted_data.begin();
            }
            constexpr auto begin() const noexcept
            {
                return sorted_data.begin();
            }

            constexpr auto end() noexcept
            {
                return sorted_data.end();
            }
            constexpr auto end() const noexcept
            {
                return sorted_data.end();
            }

            auto find(std::string_view name) const noexcept
            {
                auto iter = std::lower_bound(
                    begin(), end(), name, [](const auto& entry, std::string_view name_) { return entry.name < name_; });
                if ((iter != end()) and (iter->name == name))
                {
                    return iter;
                }

                return end();
            }

        private:
            std::array<Entry, N> sorted_data;
        };

        template<typename S, auto N>
        S offset(const FieldOffsetTable<S, N>& table, std::string_view name)
        {
            auto p = table.find(name);
            if (p == table.end())
                throw InvalidPartitionName{name};
            return p->sort;
        }

        template<typename T, auto N>
        consteval FieldOffsetTable<T, N> order_by_name(const SortNameMapEntry<T> (&seq)[N])
        {
            FieldOffsetTable<T, N> map{seq};
            return map;
        }

        template<auto& table, auto partitions>
        PartitionSummaryData& entry_by_name(TableOfContents& toc, std::string_view name)
        {
            static constinit auto field_map = order_by_name(table);
            return (toc.*partitions)[ifc::to_underlying(offset(field_map, name))];
        }

        PartitionSummaryData& name_summary_by_partition_name(TableOfContents& toc, std::string_view name)
        {
            static constinit auto field_map = order_by_name(namesort_table);
            auto sort                       = offset(field_map, name);
            IFCASSERT(sort != NameSort::Identifier);        // No name.identifier partition is ever emitted.
            return toc.names[ifc::to_underlying(sort) - 1]; // Off by 1, to reflect TableOfContents::names
        }

        bool has_prefix(std::string_view lhs, const char* rhs)
        {
            std::string_view prefix = rhs;
            if (lhs.length() < prefix.length())
                return false;
            return strncmp(lhs.data(), prefix.data(), prefix.length()) == 0;
        }

        PartitionSummaryData& uncategorized_partition_lookup(TableOfContents& toc, std::string_view name)
        {
            // FIXME, this can't be marked as constinit due to issues in constexpr evaluation.
            static const auto field_map = order_by_name(toc_members_table);
            return toc.*(offset(field_map, name));
        }

        PartitionSummaryData& msvc_trait_lookup(TableOfContents& toc, std::string_view name)
        {
            // A couple of msvc traits are not AssociatedTraits and require special handling
            if (has_prefix(name, ".msvc.trait.impl-pragmas")
                or has_prefix(name, ".msvc.trait.suppressed-warnings")
                or has_prefix(name, ".msvc.trait.debug-records")
                or has_prefix(name, ".msvc.trait.pragma-warnings")
                or has_prefix(name, ".msvc.trait.gmf-specializations"))
                return uncategorized_partition_lookup(toc, name);

            return entry_by_name<msvc_traitsort_table, &TableOfContents::msvc_traits>(toc, name);
        }

        struct PartitionDispatch {
            const char* category;
            PartitionSummaryData& (*dispatcher)(TableOfContents&, std::string_view);
        };

        constexpr PartitionDispatch partition_dispatch_table[] = {
            {"decl.", entry_by_name<declsort_table, &TableOfContents::decls>},
            {"type.", entry_by_name<typesort_table, &TableOfContents::types>},
            {"name.", name_summary_by_partition_name},
            {"expr.", entry_by_name<exprsort_table, &TableOfContents::exprs>},
            {"stmt.", entry_by_name<stmtsort_table, &TableOfContents::stmts>},
            {"syntax.", entry_by_name<syntax_sort_table, &TableOfContents::elements>},
            {"macro.", entry_by_name<macro_sort_table, &TableOfContents::macros>},
            {"heap.", entry_by_name<heapsort_table, &TableOfContents::heaps>},
            {"pragma-directive.", entry_by_name<pragma_sort_table, &TableOfContents::pragma_directives>},
            {"pp.", entry_by_name<formsort_table, &TableOfContents::forms>},
            {"trait.", entry_by_name<traitsort_table, &TableOfContents::traits>},
            {"attr.", entry_by_name<attrsort_table, &TableOfContents::attrs>},
            {"dir.", entry_by_name<dirsort_table, &TableOfContents::dirs>},
            {".msvc.", &msvc_trait_lookup},
            {"vendor.", &entry_by_name<vendor_sort_table, &TableOfContents::vendor>},
        };

        // Helper function: map a given sort to its external name.
        template<auto& table, typename T>
        auto retrieve_name(T s)
        {
            IFCASSERT(ifc::to_underlying(s) < std::size(table));
            return table[ifc::to_underlying(s)].name;
        }
    } // namespace

    const char* sort_name(DeclSort s)
    {
        return retrieve_name<declsort_table>(s);
    }

    const char* sort_name(TypeSort s)
    {
        return retrieve_name<typesort_table>(s);
    }

    const char* sort_name(StmtSort s)
    {
        return retrieve_name<stmtsort_table>(s);
    }

    const char* sort_name(ExprSort s)
    {
        return retrieve_name<exprsort_table>(s);
    }

    const char* sort_name(ChartSort s)
    {
        return retrieve_name<chartsort_table>(s);
    }

    const char* sort_name(NameSort s)
    {
        return retrieve_name<namesort_table>(s);
    }

    const char* sort_name(SyntaxSort s)
    {
        return retrieve_name<syntax_sort_table>(s);
    }

    const char* sort_name(MacroSort s)
    {
        return retrieve_name<macro_sort_table>(s);
    }

    const char* sort_name(PragmaSort s)
    {
        return retrieve_name<pragma_sort_table>(s);
    }

    const char* sort_name(AttrSort s)
    {
        return retrieve_name<attrsort_table>(s);
    }

    const char* sort_name(DirSort s)
    {
        return retrieve_name<dirsort_table>(s);
    }

    const char* sort_name(HeapSort s)
    {
        return retrieve_name<heapsort_table>(s);
    }

    const char* sort_name(FormSort s)
    {
        return retrieve_name<formsort_table>(s);
    }

    const char* sort_name(TraitSort s)
    {
        return retrieve_name<traitsort_table>(s);
    }

    const char* sort_name(MsvcTraitSort s)
    {
        return retrieve_name<msvc_traitsort_table>(s);
    }

    const char* sort_name(VendorSort s)
    {
        return retrieve_name<vendor_sort_table>(s);
    }

    PartitionSummaryData& summary_by_partition_name(TableOfContents& toc, std::string_view name)
    {
        for (auto& entry : partition_dispatch_table)
        {
            if (has_prefix(name, entry.category))
                return entry.dispatcher(toc, name);
        }
        return uncategorized_partition_lookup(toc, name);
    }
} // namespace ifc
