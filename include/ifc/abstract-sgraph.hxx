//
// Microsoft (R) C/C++ Optimizing Compiler Front-End
// Copyright (C) Microsoft. All rights reserved.
//

#ifndef IFC_ABSTRACT_SGRAPH
#define IFC_ABSTRACT_SGRAPH

#include <concepts>
#include <string_view>

#include "ifc/index-utils.hxx"
#include "ifc/file.hxx"
#include "ifc/source-word.hxx"
#include "ifc/operators.hxx"
#include "ifc/pp-forms.hxx"

// This file contains a minimal set of data structures supporting the Abstract Semantics Graph of entities
// involved in the processing (import, export) of a module interface unit.  An Abstract Semantics Graph
// is the result of performing semantics analysis (e.g. type-checking) on a collection of Abstract Syntax
// Trees, usually coming from a translation unit.  It is a graph because, well, C++'s semantics has recursion
// almost everywhere at its meta description level, especially in declarations.
// The intent is that these data structures can be shared with external module-support tooling.  Consequently,
// it must be independent of c1xx internal data structures, while faithful to the core C++ semantics.

namespace Module {
    using index_like::Index;
    using ColumnNumber = msvc::ColumnNumber;
    using LineNumber = msvc::LineNumber;

    // For every sort-like enumeration, return the maximum value
    // as indicated by its Count enumerator.  This short-hand provides
    // a simpler notation, especially when the enumeration is scoped.
    template<typename S>
    constexpr auto count = bits::rep(S::Count);

    // Source line info index into the global line table.
    enum class LineIndex : uint32_t { };

    // Index into a word stream.
    enum class WordIndex : uint32_t { };

    // Index into the token stream table.
    enum class SentenceIndex : uint32_t { };

    // Index into the specialization form (template-id?) table.
    enum class SpecFormIndex : uint32_t { };

    // The variety of string literals
    enum class StringSort : uint8_t {
        Ordinary,               // Ordinary string literal -- no prefix.
        UTF8,                   // UTF-8 narrow string literal -- only u8 prefix.
        UTF16,                  // A char16_t string literal -- only u prefix.
        UTF32,                  // A char32_t string literal -- only U prefix.
        Wide,                   // A wide string literal -- only L prefix.
        Count,
    };

    struct StringIndex : index_like::Over<StringSort> {
        using Over<StringSort>::Over;
    };

    // The sort of module ownership or entities.
    enum class Ownership : uint8_t {
        Unknown,    // No particular information is known.
        Imported,   // The reference is to an imported module.
        Exported,   // The reference is to an exported module.
        Merged,     // The reference is to a module that will be merged into a larger entity.
    };

    // Set of declarative properties reachable from any program points
    // outside from a module boundaries.
    enum class ReachableProperties : uint8_t {
        Nothing             = 0,        // nothing beyond name, type, scope.
        Initializer         = 1 << 0,   // IPR-initializer exported.
        DefaultArguments    = 1 << 1,   // function or template default arguments exported
        Attributes          = 1 << 2,   // standard attributes exported.
        All                 = Initializer | DefaultArguments | Attributes // Everything.
    };

    // Standard C++ access specifiers.
    enum class Access : uint8_t {
        None,               // No access specified.
        Private,            // "private"
        Protected,          // "protected"
        Public,             // "public"
        Count
    };

    // Common declaration specifiers.  By default, we assume the language linkage to C++.
    enum class BasicSpecifiers : uint8_t {
        Cxx                     = 0,            // C++ language linkage
        C                       = 1 << 0,       // C language linkage
        Internal                = 1 << 1,       // Exported entities should have external linkage, not sure we need this.
        Vague                   = 1 << 2,       // Vague linkage, e.g. COMDAT, still external
        External                = 1 << 3,       // External linkage.
        Deprecated              = 1 << 4,       // [[deprecated("foo")]]
        InitializedInClass      = 1 << 5,       // Is this entity defined in a class or does it have an in-class initializer?
        NonExported             = 1 << 6,       // This is an entity that was not explicitly exported from the module
        IsMemberOfGlobalModule  = 1 << 7        // This entity is a member of the global module
    };

    // The various calling conventions currently supported by VC.
    enum class CallingConvention : uint8_t {
        Cdecl,              // "__cdecl"
        Fast,               // "__fastcall"
        Std,                // "__stdcall"
        This,               // "__thiscall"
        Clr,                // "__clrcall"
        Vector,             // "__vectorcall"
        Eabi,               // "__eabi"
        Count               // Just for counting.
    };

    enum class FunctionTypeTraits : uint8_t {
        None        = 0x00,     // Just a regular function parameter type list
        Const       = 0x01,     // 'void (int) const'
        Volatile    = 0x02,     // 'void (int) volatile'
        Lvalue      = 0x04,     // 'void (int) &'
        Rvalue      = 0x08,     // 'void (int) &&'
    };

    enum class ExceptionSpecification : uint8_t {
        None,               // None specified, not the same as no-exception
        NonNoexcept,        // "noexcept(false)" specification
        Noexcept,           // "noexcept" or "noexcept(true)" specification.
        Conditional,        // "noexcept(unevaluated-expression)" specification.
        Empty,              // "throw()"-style specification
        ExplicitList,       // Explicitly list a sequence of types.
        Count
    };

    enum class NoexceptSort : uint8_t {
        None,                   // No specifier
        False,                  // "noexcept(false)" specifier.
        True,                   // "noexcept(true)" specifier.
        Expression,             // "noexcept(expression)" specifier
        InferredSpecialMember,  // an inferred noexcept specifier for a special member function that is dependent on
                                // whether associated functions the member will invoke from bases and members are noexcept
        Unenforced,             // noexcept for the purposes of the type system, but not enforced by termination at runtime
        // V2 TODO: Add ImplicitFalse and ImplicitTrue.
        Count
    };

    enum class ScopeTraits : uint8_t {
        None                = 0,
        Unnamed             = 1 << 0,       // unnamed namespace, or unnamed class types.
        Inline              = 1 << 1,       // inline namespace.
        InitializerExported = 1 << 2,       // This object has its initializer exported
        ClosureType         = 1 << 3,       // lambda-like scope
        Final               = 1 << 4,       // a derived class marked as 'final'
        Vendor              = 1 << 7,       // The scope has extended vendor specific traits
    };

    // Variable and object trait specifiers.
    enum class ObjectTraits : uint8_t {
        None                = 0,
        Constexpr           = 1 << 0,       // Constexpr object.
        Mutable             = 1 << 1,       // Mutable object.
        ThreadLocal         = 1 << 2,       // thread_local storage: AKA '__declspec(thread)'
        Inline              = 1 << 3,       // An 'inline' object (this is distinct from '__declspec(selectany)')
        InitializerExported = 1 << 4,       // This object has its initializer exported
        NoUniqueAddress     = 1 << 5,       // The '[[no_unique_address]]' attribute was applied to this data member
        Vendor              = 1 << 7,       // The object has extended vendor specific traits
    };

    enum class PackSize : uint16_t { };

    enum class FunctionTraits : uint16_t
    {
        None            = 0,
        Inline          = 1 << 0,           // inline function
        Constexpr       = 1 << 1,           // constexpr function
        Explicit        = 1 << 2,           // For conversion functions.
        Virtual         = 1 << 3,           // virtual function
        NoReturn        = 1 << 4,           // The 'noreturn' attribute
        PureVirtual     = 1 << 5,           // A pure virtual function ('= 0')
        HiddenFriend    = 1 << 6,           // A hidden friend function
        Defaulted       = 1 << 7,           // A '= default' function
        Deleted         = 1 << 8,           // A '= delete' function
        Constrained     = 1 << 9,           // For functions which have constraint expressions.
        Immediate       = 1 << 10,          // Immediate function
        Final           = 1 << 11,          // A function marked as 'final'
        Override        = 1 << 12,          // A function marked as 'override'
        Vendor          = 1 << 15,          // The function has extended vendor specific traits
    };
    
    enum class GuideTraits : uint8_t {
        None = 0,                           // nothing
        Explicit = 1 << 0,                  // the deduction guide is declared 'explicit'.
    };
    
    enum class VendorTraits : uint32_t
    {
        None                    = 0,
        ForceInline             = 1 << 0,           // __forceinline function
        Naked                   = 1 << 1,           // __declspec(naked)
        NoAlias                 = 1 << 2,           // __declspec(noalias)
        NoInline                = 1 << 3,           // __declspec(noinline)
        Restrict                = 1 << 4,           // __declspec(restrict)
        SafeBuffers             = 1 << 5,           // __declspec(safebuffers)
        DllExport               = 1 << 6,           // __declspec(dllexport)
        DllImport               = 1 << 7,           // __declspec(dllimport)
        CodeSegment             = 1 << 8,           // __declspec(code_seg("segment"))
        NoVtable                = 1 << 9,           // __declspec(novtable) for a class type.
        IntrinsicType           = 1 << 10,          // __declspec(intrin_type)
        EmptyBases              = 1 << 11,          // __declspec(empty_bases)
        Process                 = 1 << 12,          // __declspec(process)
        Allocate                = 1 << 13,          // __declspec(allocate("segment"))
        SelectAny               = 1 << 14,          // __declspec(selectany)
        Comdat                  = 1 << 15,
        Uuid                    = 1 << 16,          // __declspec(uuid(....))
        NoCtorDisplacement      = 1 << 17,          // #pragma vtordisp(0)
        DefaultCtorDisplacement = 1 << 18,          // #pragma vtordisp(1)
        FullCtorDisplacement    = 1 << 19,          // #pragma vtordisp(2)
        NoSanitizeAddress       = 1 << 20,          // __declspec(no_sanitize_address)
        NoUniqueAddress         = 1 << 21,          // '[[msvc::no_unique_address]]'
        NoInitAll               = 1 << 22,          // __declspec(no_init_all)
        DynamicInitialization   = 1 << 23,          // Indicates that this entity is used for implementing aspects of dynamic initialization.
        LexicalScopeIndex       = 1 << 24,          // Indicates this entity has a local lexical scope index associated with it.
        HasAssociatedIpr        = 1u << 31,         // does this template have associated IPR (transitory trait)
                                                    // 31 was picked so that it could be removed easily later and not
                                                    // affect any important traits that are not transitory
    };

    enum class SuppressedWarningSequenceIndex : uint32_t { };
    enum class SuppressedWarning : uint16_t { };

    // Attributes of segments.
    enum class SegmentTraits : uint32_t { };

    enum class SegmentType : uint8_t { };

    // Account for all kinds of valid C++ names.
    enum class NameSort : uint8_t {
        Identifier,             // Normal alphabetic identifiers.
        Operator,               // Operator names.
        Conversion,             // Conversion function name.
        Literal,                // Literal operator name.
        Template,               // A nested-name assumed to designate a template.
        Specialization,         // A template-id name.
        SourceFile,             // A source file name
        Guide,                  // A deduction guide name
        Count
    };

    struct NameIndex : index_like::Over<NameSort> {
        using Over<NameSort>::Over;
    };

    // Sort of template parameter set (also called chart.)
    enum class ChartSort : uint8_t {
        None,                   // No template parameters; e.g. explicit specialization.
        Unilevel,               // Unidimensional set of template parameters; e.g. templates that are not members of templates.
        Multilevel,             // Multidimensional set of template parameters; e.g. member templates of templates.
        Count
    };

    struct ChartIndex : index_like::Over<ChartSort> {
        using Over<ChartSort>::Over;
    };

    // Sorts of declarations that can be exported, or part of exportable entities.
    enum class DeclSort : uint8_t {
        VendorExtension,        // A vendor-specific extension.
        Enumerator,             // An enumerator declaration.
        Variable,               // A variable declaration; a static-data member is also considered a variable.
        Parameter,              // A parameter declaration - for a function or a template
        Field,                  // A non-static data member declaration.
        Bitfield,               // A bit-field declaration.
        Scope,                  // A type declaration.
        Enumeration,            // An enumeration declaration.
        Alias,                  // An alias declaration for a (general) type.
        Temploid,               // A member of a parameterized scope -- does not have template parameters of its own.
        Template,               // A template declaration: class, function, constructor, type alias, variable.
        PartialSpecialization,  // A partial specialization of a template (class-type or function).
        Specialization,         // A specialization of some template decl.
        UnusedSort0,            // Empty slot.
        Concept,                // A concept
        Function,               // A function declaration; both free-standing and static member functions.
        Method,                 // A non-static member function declaration.
        Constructor,            // A constructor declaration.
        InheritedConstructor,   // A constructor inherited from a base class.
        Destructor,             // A destructor declaration.
        Reference,              // A reference to a declaration from a given module.
        UsingDeclaration,       // A using declaration
        UsingDirective,         // A using-directive
        Friend,                 // A friend declaration
        Expansion,              // A pack-expansion of a declaration
        DeductionGuide,         // C(T) -> C<U>
        Barren,                 // Declaration introducing no names, e.g. static_assert, asm-declaration, empty-declaration
        Tuple,                  // A sequence of entities
        SyntaxTree,             // A syntax tree for an unelaborated declaration.
        Intrinsic,              // An intrinsic function declaration.
        Property,               // VC's extension of property declaration.
        OutputSegment,          // Code segment. These are 'declared' via pragmas.
        Count,
    };

    struct DeclIndex : index_like::Over<DeclSort> {
        using Over<DeclSort>::Over;
    };

    // Sorts types of exported entities.
    enum class TypeSort : uint8_t {
        VendorExtension,        // Vendor-specific type constructor extensions.
        Fundamental,            // Fundamental type, in standard C++ sense
        Designated,             // A type designated by a declared name.  Really a proxy type designator.
        Tor,                    // Type of a constructor or a destructor.
        Syntactic,              // Source-level expression designating unelaborated type.
        Expansion,              // A pack expansion of a type.
        Pointer,                // Pointer type, whether the pointee is data type or function type
        PointerToMember,        // Type of a non-static data member that includes the enclosing class type.  Useful for pointer-to-data-member.
        LvalueReference,        // Classic reference type, e.g. A&
        RvalueReference,        // Rvalue reference type, e.g. A&&
        Function,               // Function type, excluding non-static member function type
        Method,                 // Non-static member function type.
        Array,                  // Builtin array type
        Typename,               // An unresolved qualified type expression
        Qualified,              // Qualified types.
        Base,                   // Use of a type as a base-type in a class inheritance
        Decltype,               // A 'decltype' type.
        Placeholder,            // A placeholder type - e.g. uses of 'auto' in function signatures.
        Tuple,                  // A sequence of types.  Generalized type for uniform description.
        Forall,                 // Template type.  E.g. type of `template<typename T> constexpr T zro = T{};`
        Unaligned,              // A type with __unaligned.  This is a curiosity with no clearly defined semantics.
        SyntaxTree,             // A syntax tree for an unelaborated type.
        Count                   // not a type; only for checking purposes
    };

    // Abstract pointer to a type data structure.
    // Types are stored in (homogeneous) tables per sort (as indicated by TypeSort).
    // Only one instance (per structural components) of a given type sort is stored,
    // and referred to by its index number.  It is a design goal to keep type indices
    // (abstract pointers) representable by 32-bit integer quantities.
    struct TypeIndex : index_like::Over<TypeSort> {
        using Over<TypeSort>::Over;
    };

    // The set of exported syntactic elements
    enum class SyntaxSort : uint8_t {
        VendorExtension,            // Vendor-specific extension for syntax. What fresh hell is this?
        SimpleTypeSpecifier,        // A simple type-specifier (i.e. no declarator)
        DecltypeSpecifier,          // A decltype-specifier - 'decltype(expression)'
        PlaceholderTypeSpecifier,   // A placeholder-type-specifier
        TypeSpecifierSeq,           // A type-specifier-seq - part of a type-id
        DeclSpecifierSeq,           // A decl-specifier-seq - part of a declarator
        VirtualSpecifierSeq,        // A virtual-specifier-seq (includes pure-specifier)
        NoexceptSpecification,      // A noexcept-specification
        ExplicitSpecifier,          // An explicit-specifier
        EnumSpecifier,              // An enum-specifier
        EnumeratorDefinition,       // An enumerator-definition
        ClassSpecifier,             // A class-specifier
        MemberSpecification,        // A member-specification
        MemberDeclaration,          // A member-declaration
        MemberDeclarator,           // A member-declarator
        AccessSpecifier,            // An access-specifier
        BaseSpecifierList,          // A base-specifier-list
        BaseSpecifier,              // A base-specifier
        TypeId,                     // A complete type used as part of an expression (can contain an abstract declarator)
        TrailingReturnType,         // a trailing return type: '-> T'
        Declarator,                 // A declarator: i.e. something that has not (yet) been resolved down to a symbol
        PointerDeclarator,          // A sub-declarator for a pointer: '*D'
        ArrayDeclarator,            // A sub-declarator for an array: 'D[e]'
        FunctionDeclarator,         // A sub-declarator for a function: 'D(T1, T2, T3) <stuff>'
        ArrayOrFunctionDeclarator,  // Either an array or a function sub-declarator
        ParameterDeclarator,        // A function parameter declaration
        InitDeclarator,             // A declaration with an initializer
        NewDeclarator,              // A new declarator (used in new expressions)
        SimpleDeclaration,          // A simple-declaration
        ExceptionDeclaration,       // An exception-declaration
        ConditionDeclaration,       // A declaration within if or switch statement: if (decl; cond)
        StaticAssertDeclaration,    // A static_assert-declaration
        AliasDeclaration,           // An alias-declaration
        ConceptDefinition,          // A concept-definition
        CompoundStatement,          // A compound statement
        ReturnStatement,            // A return statement
        IfStatement,                // An if statement
        WhileStatement,             // A while statement
        DoWhileStatement,           // A do-while statement
        ForStatement,               // A for statement
        InitStatement,              // An init-statement
        RangeBasedForStatement,     // A range-based for statement
        ForRangeDeclaration,        // A for-range-declaration
        LabeledStatement,           // A labeled statement
        BreakStatement,             // A break statement
        ContinueStatement,          // A continue statement
        SwitchStatement,            // A switch statement
        GotoStatement,              // A goto statement
        DeclarationStatement,       // A declaration statement
        ExpressionStatement,        // An expression statement
        TryBlock,                   // A try block
        Handler,                    // A catch handler
        HandlerSeq,                 // A sequence of catch handlers
        FunctionTryBlock,           // A function try block
        TypeIdListElement,          // a type-id-list element
        DynamicExceptionSpec,       // A dynamic exception specification
        StatementSeq,               // A sequence of statements
        FunctionBody,               // The body of a function
        Expression,                 // A wrapper around an ExprSort node
        FunctionDefinition,         // A function-definition
        MemberFunctionDeclaration,  // A member function declaration
        TemplateDeclaration,        // A template head definition
        RequiresClause,             // A requires clause
        SimpleRequirement,          // A simple requirement
        TypeRequirement,            // A type requirement
        CompoundRequirement,        // A compound requirement
        NestedRequirement,          // A nested requirement
        RequirementBody,            // A requirement body
        TypeTemplateParameter,      // A type template-parameter
        TemplateTemplateParameter,  // A template template-parameter
        TypeTemplateArgument,       // A type template-argument
        NonTypeTemplateArgument,    // A non-type template-argument
        TemplateParameterList,      // A template parameter list
        TemplateArgumentList,       // A template argument list
        TemplateId,                 // A template-id
        MemInitializer,             // A mem-initializer
        CtorInitializer,            // A ctor-initializer
        LambdaIntroducer,           // A lambda-introducer
        LambdaDeclarator,           // A lambda-declarator
        CaptureDefault,             // A capture-default
        SimpleCapture,              // A simple-capture
        InitCapture,                // An init-capture
        ThisCapture,                // A this-capture
        AttributedStatement,        // An attributed statement
        AttributedDeclaration,      // An attributed declaration
        AttributeSpecifierSeq,      // An attribute-specifier-seq
        AttributeSpecifier,         // An attribute-specifier
        AttributeUsingPrefix,       // An attribute-using-prefix
        Attribute,                  // An attribute
        AttributeArgumentClause,    // An attribute-argument-clause
        Alignas,                    // An alignas( expression )
        UsingDeclaration,           // A using-declaration
        UsingDeclarator,            // A using-declarator
        UsingDirective,             // A using-directive
        ArrayIndex,                 // An array index
        SEHTry,                     // An SEH try-block
        SEHExcept,                  // An SEH except-block
        SEHFinally,                 // An SEH finally-block
        SEHLeave,                   // An SEH leave
        TypeTraitIntrinsic,         // A type trait intrinsic
        Tuple,                      // A sequence of zero or more syntactic elements
        AsmStatement,               // An __asm statement,
        NamespaceAliasDefinition,   // A namespace-alias-definition
        Super,                      // The '__super' keyword in a qualified id
        UnaryFoldExpression,        // A unary fold expression
        BinaryFoldExpression,       // A binary fold expression
        EmptyStatement,             // An empty statement: ';'
        StructuredBindingDeclaration,   // A structured binding
        StructuredBindingIdentifier,    // A structured binding identifier
        UsingEnumDeclaration,       // A using-enum-declaration
        Count,
    };

    // An abstract pointer for a syntactic element
    struct SyntaxIndex : index_like::Over<SyntaxSort> {
        using Over<SyntaxSort>::Over;
    };

    // The various kinds of parameters.
    enum class ParameterSort : uint8_t {
        Object,                     // Function parameter
        Type,                       // Type template parameter
        NonType,                    // Non-type template parameter
        Template,                   // Template template parameter
        Count
    };

    // The various sort of literal constants.
    enum class LiteralSort : uint8_t {
        Immediate,                  // Immediate integer constants, directly representable by an index value.
        Integer,                    // Unsigned 64-bit integer constant that are not immediates.
        FloatingPoint,              // Floating point constant.
        Count
    };

    struct LitIndex : index_like::Over<LiteralSort> {
        using Over<LiteralSort>::Over;
    };

    // Various kinds of statements (incomplete)
    enum class StmtSort : uint8_t {
        VendorExtension,            // Vendor-specific extensions
        Try,                        // A try-block containing a list of handlers
        If,                         // If statements
        For,                        // C-style for loop, ranged-based for
        Labeled,                    // A labeled-statement. (was Case in IFC < 0.42)
        While,                      // A while loop, not do-while
        Block,                      // A {}-delimited sequence of statements
        Break,                      // Break statements (as in a loop or switch)
        Switch,                     // Switch statement
        DoWhile,                    // Do-while loops
        Goto,                       // A goto statement. (was Default in IFC < 0.42)
        Continue,                   // Continue statement (as in a loop)
        Expression,                 // A naked expression terminated with a semicolon
        Return,                     // A return statement
        Decl,                       // A declaration
        Expansion,                  // A pack-expansion of a statement
        SyntaxTree,                 // A syntax tree for an unelaborated statement.
        Handler,                    // An exception-handler statement (catch-clause).
        Tuple,                      // A general tuple of statements.
        Count
    };

    // Abstract typed pointer to statement data structures
    struct StmtIndex : index_like::Over<StmtSort> {
        using Over<StmtSort>::Over;
    };

    // Expression trees.
    enum class ExprSort : uint8_t {
        VendorExtension,            // Vendor-specific extension for expressions.
        Empty,                      // An empty expression.
        Literal,                    // Literal constants.
        Lambda,                     // Lambda expression
        Type,                       // A type expression.  Useful in template argument contexts, and other more general context.
        NamedDecl,                  // Use of a name designating a declaration.
        UnresolvedId,               // An unresolved or dependent name as expression.
        TemplateId,                 // A template-id expression.
        UnqualifiedId,              // An unqualified-id + some other stuff like 'template' and/or 'typename'
        SimpleIdentifier,           // Just an identifier: nothing else
        Pointer,                    // A '*' when it appears as part of a qualified-name
        QualifiedName,              // A raw qualified-name: i.e. one that has not been fully resolved
        Path,                       // A path expression, e.g. a fully bound qualified-id
        Read,                       // Lvalue-to-rvalue conversions.
        Monad,                      // 1-part expression.
        Dyad,                       // 2-part expressions.
        Triad,                      // 3-part expressions.
        String,                     // A string literal
        Temporary,                  // A temporary expression (i.e. 'L_ALLOTEMP')
        Call,                       // An invocation of some form of function object
        MemberInitializer,          // A base or member initializer
        MemberAccess,               // The member being accessed + offset
        InheritancePath,            // The representation of the 'path' to a base-class member
        InitializerList,            // An initializer-list
        Cast,                       // A cast: either old-style '(T)e'; new-style 'static_cast<T>(e)'; or functional 'T(e)'
        Condition,                  // A statement condition: 'if (e)' or 'if (T v = e)'
        ExpressionList,             // Either '(e1, e2, e3)' or '{ e1, e2, e2 }'
        SizeofType,                 // 'sizeof(type-id)'
        Alignof,                    // 'alignof(type-id)'
        Label,                      // A symbolic label resulting from the expression in a labeled statement.
        UnusedSort0,                // Empty slot.
        Typeid,                     // A 'typeid' expression
        DestructorCall,             // A destructor call: i.e. the right sub-expression of 'expr->~T()'
        SyntaxTree,                 // A syntax tree for an unelaborated expression.
        FunctionString,             // A string literal expanded from a built-in macro like __FUNCTION__.
        CompoundString,             // A string literal of the form __LPREFIX(string-literal).
        StringSequence,             // A sequence of string literals.
        Initializer,                // An initializer for a symbol
        Requires,                   // A requires expression
        UnaryFoldExpression,        // A unary fold expression of the form (pack @ ...)
        BinaryFoldExpression,       // A binary fold expression of the form (expr @ ... @ pack)
        HierarchyConversion,        // A class hierarchy conversion. Distinct from ExprSort::Cast, which represents
                                    // a syntactic expression of intent, whereas this represents the semantics of a
                                    // (possibly implicit) conversion
        ProductTypeValue,           // The representation of an object value expression tree
                                    // (really an associative map of declarations to values)
        SumTypeValue,               // The representation of a union object value expression tree.
                                    // Unions have a single active SubobjectValue
        SubobjectValue,             // A key-value pair under a ProductTypeValue
        ArrayValue,                 // A constant-initialized array value object
        DynamicDispatch,            // A dynamic dispatch expression, i.e. call to virtual function
        VirtualFunctionConversion,  // A conversion of a virtual function reference to its underlying target: C::f // 'f' is a virtual function.
        Placeholder,                // A placeholder expression.
        Expansion,                  // A pack-expansion expression
        Generic,                    // A generic-selection -- C11 extension.
        Tuple,                      // General tuple expressions.
        Nullptr,                    // 'nullptr'
        This,                       // 'this'
        TemplateReference,          // A reference to a member of a template
        UnusedSort1,                // Empty slot.
        TypeTraitIntrinsic,         // A use of a type trait intrinsic
        DesignatedInitializer,      // A single designated initializer clause: '.a = e'
        PackedTemplateArguments,    // The template argument set for a template parameter pack
        Tokens,                     // A token stream (i.e. a complex default template argument)
        AssignInitializer,          // '= e'.  FIXME:  This is NOT an expression.
        Count                       // Total count.
    };

    // Abstract typed pointer to expression data structures
    struct ExprIndex : index_like::Over<ExprSort> {
        using Over<ExprSort>::Over;
    };

    enum class InheritanceSort : uint8_t {
        None,
        Single,
        Multiple,
        Count
    };

    // Type qualifiers.
    // Note: VC's __unaligned qualifier is made a type constructor of its own.
    enum class Qualifier : uint8_t {
        None        = 0,                    // No qualifier
        Const       = 1 << 0,               // "const" qualifier
        Volatile    = 1 << 1,               // "volatile" qualifier
        Restrict    = 1 << 2,               // "restrict" qualifier, C extension
    };

    enum class WordCategory : uint16_t { };

    enum class MacroSort : uint8_t {
        ObjectLike,                         // #define NAME <replacement-text>
        FunctionLike,                       // #define F(A, B) <replacement-text>
        Count
    };

    // Abstract typed pointer to expression data structures
    struct MacroIndex : index_like::Over<MacroSort> {
        using Over<MacroSort>::Over;
    };

    enum class PragmaSort : uint8_t {
        VendorExtension,
        Count
    };

    struct PragmaIndex : index_like::Over<PragmaSort> {
        using Over<PragmaSort>::Over;
    };

    enum class AttrSort : uint8_t {
        Nothing,    // no attribute - [[ ]]
        Basic,      // any token    - [[ foo ]]
        Scoped,     // scoped attribute - [[foo :: bar]]
        Labeled,    // (key, value) pair, not required by C++ syntax - [[key : value]]
        Called,     // function call syntax - [[opt(1)]]
        Expanded,   // an attribute expansion [[fun(arg)...]]
        Factored,   // attribute with scope factored out - [[using msvc: opt(2), dbg(1)]]
        Elaborated, // an expression inside an attribute, not required by C++ - [[requires: x > 0 and x < 10 ]]
        Tuple,      // comma-separated sequence of attributes
        Count
    };

    struct AttrIndex : index_like::Over<AttrSort> {
        using Over<AttrSort>::Over;
    };

    // Symbolic name of the various heaps stored in an IFC.
    enum class HeapSort : uint8_t {
        Decl,                       // DeclIndex heap
        Type,                       // TypeIndex heap
        Stmt,                       // StmtIndex heap
        Expr,                       // ExprIndex heap
        Syntax,                     // SyntaxIndex heap
        Word,                       // WordIndex heap
        Chart,                      // ChartIndex heap
        Spec,                       // specialization form heap
        Form,                       // FormIndex heap
        Attr,                       // AttrIndex heap
        Count
    };

    // Typed projection of a homogeneous sequence.
    template <typename, auto... s> requires (sizeof...(s) < 2)
    struct Sequence {
        Index start = { };              // Offset of the first member of this sequence
        Cardinality cardinality = { };  // Number of elements in this sequence.
    };

    // All entities are externally represented in a symbolic form.
    // The data structures for symbolic representation are defined in this namespace.
    namespace Symbolic {
        struct Declaration {
            DeclIndex index;
        };

        // A tag embedding an abstract reference sort value in the static type.  We are doing this only because of
        // a weakness in the implementation language (C++).  What we really want is to define a function
        // from types to values without going through the usual cumbersome explicit specialization dance.
        template<auto s>
        struct Tag : index_like::SortTag<s> {
            static constexpr auto algebra_sort = s;
        };

        struct ConversionFunctionId : Tag<NameSort::Conversion> {
            TypeIndex target { };                       // The type to convert to
            TextOffset name { };                        // The index for the name (if we have one)
        };

        struct OperatorFunctionId : Tag<NameSort::Operator> {
            TextOffset name { };                        // The index for the name
            Operator symbol { };                        // The symbolic operator
            // Note: 16 bits hole left.
        };

        struct LiteralOperatorId : Tag<NameSort::Literal> {
            TextOffset name_index { };           // Note: ideally should be literal operator suffix, as plain identifier.
        };

        struct TemplateName : Tag<NameSort::Template> {
            TemplateName() : name{ } { }
            TemplateName(NameIndex x) : name{ x } { }
            NameIndex name;             // template name; can't be itself another TemplateName.
        };

        struct TemplateId : Tag<NameSort::Specialization> {
            TemplateId()
                : primary_template{ }, arguments{ } { }
            TemplateId(NameIndex primary_template, ExprIndex arguments)
                : primary_template{ primary_template }, arguments{ arguments } { }
            NameIndex primary_template;    // The primary template name.  Note: NameIndex is over-generic here.  E.g. it can't be a decltype, nor can it be another template-id.
            ExprIndex arguments;
        };

        struct SourceFileName : Tag<NameSort::SourceFile> {
            SourceFileName()
                : name{ }, include_guard{ } { }
            SourceFileName(TextOffset name, TextOffset include_guard)
                : name{ name }, include_guard{ include_guard } { }
            TextOffset name;
            TextOffset include_guard;
        };

        struct GuideName : Tag<NameSort::Guide> {
            DeclIndex primary_template { };
        };

        // A referenced to a module.
        struct ModuleReference {
            TextOffset owner;       // The owner of this reference type.  If the owner is null the owner is the global module.
            TextOffset partition;   // Optional partition where this reference belongs.  If owner is null, this is the path to a header unit.
        };

        // Source location
        struct SourceLocation {
            LineIndex line = { };
            ColumnNumber column = { };
        };

        // Conceptually, this is a token; but we have too many token type and it would be confusing to call this Token.
        struct Word {
            SourceLocation locus = { };
            union {
                TextOffset text = { };      // Text associated with this token
                ExprIndex expr;             // Literal constant expression / binding associated with this token
                TypeIndex type;             // Literal resolved type associated with this token
                Index state;                // State of preprocessor to push to
                StringIndex string;         // String literal
            };
            union {
                WordCategory category = { };                // FIXME: Horrible workaround deficiencies.  Remove.
                Source::Directive src_directive;
                Source::Punctuator src_punctuator;
                Source::Literal src_literal;
                Source::Operator src_operator;
                Source::Keyword src_keyword;
                Source::Identifier src_identifier;
            };
            WordSort algebra_sort = { };
        };

        static_assert(sizeof(Word) == 4 * sizeof(Index));

        // Description of token sequences.
        struct WordSequence {
            Index start = { };
            Cardinality cardinality = { };
            SourceLocation locus = { };
        };

        struct NoexceptSpecification {
            SentenceIndex words = { };      // Tokens of the operand to noexcept, if a non-trivial noexcept specification.
            NoexceptSort sort = { };

            // Implementation details
            bool operator==(const NoexceptSpecification&) const = default;
        };

        struct UnaryTypeCtorInstance {
            TypeIndex operand;
        };

        // The set of fundamental type basis.
        enum class TypeBasis : uint8_t {
            Void,                       // "void"
            Bool,                       // "bool"
            Char,                       // "char"
            Wchar_t,                    // "wchar_t"
            Int,                        // "int"
            Float,                      // "float"
            Double,                     // "double"
            Nullptr,                    // "decltype(nullptr)"
            Ellipsis,                   // "..."        -- generalized type
            SegmentType,                // "segment"
            Class,                      // "class"
            Struct,                     // "struct"
            Union,                      // "union"
            Enum,                       // "enum"
            Typename,                   // "typename"
            Namespace,                  // "namespace"
            Interface,                  // "__interface"
            Function,                   // concept of function type.
            Empty,                      // an empty pack expansion
            VariableTemplate,           // a variable template
            Concept,                    // a concept
            Auto,                       // "auto"
            DecltypeAuto,               // "decltype(auto)"
            Overload,                   // fundamental type for an overload set
            Count                       // cardinality of fundamental type basis.
        };

        enum class TypePrecision : uint8_t {
            Default,                    // Default bit width, whatever that is.
            Short,                      // The short version.
            Long,                       // The long version.
            Bit8,                       // The  8-bit version.
            Bit16,                      // The 16-bit version.
            Bit32,                      // The 32-bit version.
            Bit64,                      // The 64-bit (long long) version.
            Bit128,                     // The 128-bit version.
            Count
        };

        // Signed-ness of a fundamental type.
        enum class TypeSign : uint8_t {
            Plain,                      // No sign specified, default to standard interpretation.
            Signed,                     // Specified sign, or implied
            Unsigned,                   // Specified sign.
            Count
        };

        // Fundamental types: standard fundamental types, and extended.
        // They are represented externally as possibly signed variation of a core basis
        // of builtin types, with versions expressing "bitness".
        struct FundamentalType : Tag<TypeSort::Fundamental> {
            FundamentalType() : basis{ }, precision{ }, sign{ } { }
            FundamentalType(TypeBasis b, TypePrecision p, TypeSign s) : basis{ b }, precision{ p }, sign{ s } { }
            TypeBasis basis;
            TypePrecision precision;
            TypeSign sign;
            uint8_t unused { };
        };

        // Template parameter pack and template-id expansion mode.
        enum class ExpansionMode : uint8_t {
            Full,
            Partial
        };

        // Designation of a type by a declared name.
        struct DesignatedType : Tag<TypeSort::Designated> {
            DesignatedType() : decl{ } { }
            DesignatedType(DeclIndex idx) : decl{ idx } { }
            DeclIndex decl;         // A declaration for this type.
        };

        struct TorType : Tag<TypeSort::Tor> {
            TypeIndex source;                   // Parameter type sequence.
            NoexceptSpecification eh_spec;      // Noexcept specification.
            CallingConvention convention;       // Calling convention.
        };

        struct SyntacticType : Tag<TypeSort::Syntactic> {
            SyntacticType() : expr{ } { }
            SyntacticType(ExprIndex x) : expr{ x } { }
            ExprIndex expr;
        };

        // Type-id expansion involving a template parameter pack.
        struct ExpansionType : Tag<TypeSort::Expansion> {
            ExpansionType() = default;
            ExpansionType(TypeIndex t, ExpansionMode m) : operand{ t }, mode{ m } { }
            TypeIndex operand = { };
            ExpansionMode mode = { };
        };

        // Pointer type.
        struct PointerType : Tag<TypeSort::Pointer>, UnaryTypeCtorInstance {
            PointerType() : UnaryTypeCtorInstance{ } { }
            PointerType(TypeIndex t) : UnaryTypeCtorInstance{ t } { }

            TypeIndex pointee() const
            {
                return operand;
            }
        };

        // Lvalue reference type.
        struct LvalueReferenceType : Tag<TypeSort::LvalueReference>, UnaryTypeCtorInstance {
            LvalueReferenceType() : UnaryTypeCtorInstance{ } { }
            LvalueReferenceType(TypeIndex t) : UnaryTypeCtorInstance{ t } { }

            TypeIndex referee() const
            {
                return operand;
            }
        };

        // Rvalue reference type.
        struct RvalueReferenceType : Tag<TypeSort::RvalueReference>, UnaryTypeCtorInstance {
            RvalueReferenceType() : UnaryTypeCtorInstance{ } { }
            RvalueReferenceType(TypeIndex t) : UnaryTypeCtorInstance{ t } { }

            TypeIndex referee() const
            {
                return operand;
            }
        };

        // Unaligned type -- an MS VC curiosity.
        struct UnalignedType : Tag<TypeSort::Unaligned>, UnaryTypeCtorInstance {
            UnalignedType() : UnaryTypeCtorInstance{ } { }
            UnalignedType(TypeIndex t) : UnaryTypeCtorInstance{ t } { }

            TypeIndex type() const
            {
                return operand;
            }
        };

        struct DecltypeType : Tag<TypeSort::Decltype> {
            SyntaxIndex expression { };
        };

        struct PlaceholderType : Tag<TypeSort::Placeholder> {
            ExprIndex constraint;               // The predicate associated with this type placeholder.  Null means no constraint.
            TypeBasis basis;                    // auto/decltype(auto)
            TypeIndex elaboration;              // The type this placeholder was deduced to.
        };

        // A pointer to non-static member type.
        struct PointerToMemberType : Tag<TypeSort::PointerToMember> {
            PointerToMemberType() = default;
            PointerToMemberType(TypeIndex s, TypeIndex t) : scope{ s }, type{ t } { }
            TypeIndex scope = { };              // The enclosing class
            TypeIndex type = { };               // Type of the pointed-to member.
        };

        // A sequence of zero of more types.  A generalized type, providing simpler accounts of
        // disparate type notions in C++.
        struct TupleType : Tag<TypeSort::Tuple>, Sequence<TypeIndex, HeapSort::Type> {
            TupleType() = default;
            TupleType(Index f, Cardinality n) : Sequence{ f, n } { }
        };

        // A universally quantified type.  This is a generalized type, useful for describing the types of templates.
        // E.g.
        //    template<typename T> constexpr T* nil = nullptr;
        // has type `forall(T: typename). T*`.
        // Similarly, the type alias
        //    template<typename T> using ptr = T*;
        // has type `forall(T: typename). typename`.
        // Furthermore, the function template
        //    template<typename T> T abs(T);
        // has type `forall(T: typename). T (T)`.
        // Note: in the current design, constraints on a template declaration are properties of the
        //       template, not part of its type.  The language facility that allows "overloading on concepts"
        //       is not modelled as overload resolution (because it is not).  Rather it is predicate
        //       refinement/subsumption, and as such does not belong (int the current design) in the type.
        struct ForallType : Tag<TypeSort::Forall> {
            ForallType(ChartIndex c, TypeIndex t) : chart{ c }, subject{ t } { }
            ChartIndex chart;                   // The set of parameters of this parametric type
            TypeIndex subject;                  // The type being parameterized
        };

        // Ordinary function types
        struct FunctionType : Tag<TypeSort::Function> {
            TypeIndex target;                   // result type
            TypeIndex source;                   // Description of parameter types.
            NoexceptSpecification eh_spec;      // Noexcept specification.
            CallingConvention convention;       // Calling convention.
            FunctionTypeTraits traits;          // Various (rare) function-type traits
        };

        // Non-static member function types.
        struct MethodType : Tag<TypeSort::Method> {
            TypeIndex target;                   // result type
            TypeIndex source;                   // Description of parameter types.
            TypeIndex class_type;               // The enclosing class associated with this non-static member function type
            NoexceptSpecification eh_spec;      // Noexcept specification.
            CallingConvention convention;       // Calling convention.
            FunctionTypeTraits traits;          // Various (rare) function-type traits
        };

        // Builtin array types.
        struct ArrayType : Tag<TypeSort::Array> {
            ArrayType() = default;
            ArrayType(TypeIndex t, ExprIndex n) : element{ t }, bound{ n } { }
            TypeIndex element = { };            // The array element type.
            ExprIndex bound = { };              // The number of element in this array.
        };

        // Qualified types:
        struct QualifiedType : Tag<TypeSort::Qualified> {
            QualifiedType() = default;
            QualifiedType(TypeIndex t, Qualifier q) : unqualified_type{ t }, qualifiers{ q } { }
            TypeIndex unqualified_type = { };
            Qualifier qualifiers = { };
        };

        // Unresolved type names.
        struct TypenameType : Tag<TypeSort::Typename> {
            TypenameType() : path{ } { }
            TypenameType(ExprIndex x) : path{ x } { }
            ExprIndex path;
        };

        enum class BaseClassTraits : uint8_t {
            None        = 0x00,         // Nothing
            Shared      = 0x01,         // Base class inherited virtually
            Expanded    = 0x02,     // Base class pack expanded
        };

        // Base-type in a class inheritance.
        struct BaseType : Tag<TypeSort::Base> {
            TypeIndex type;                     // The actual base type, without specifiers
            Access access;                      // Access specifier.
            BaseClassTraits traits;             // Additional base class semantics.
        };

        // Type-id in parse tree form
        struct SyntaxTreeType : Tag<TypeSort::SyntaxTree> {
            SyntaxIndex syntax { };
        };

        // The types that provide a syntactic representation of a program
        namespace Syntax
        {
            // DecltypeSpecifer is 'decltype(expression)'. DecltypeAutoSpecifier is 'decltype(auto)'. See below.
            struct DecltypeSpecifier : Tag<SyntaxSort::DecltypeSpecifier> {
                ExprIndex expression { };                // The expression (for 'auto', see DecltypeAutoSpecifier)
                SourceLocation decltype_keyword { };     // The source location of the 'decltype'
                SourceLocation left_paren { };           // The source location of the '('
                SourceLocation right_paren { };          // The source location of the ')'
            };

            struct PlaceholderTypeSpecifier : Tag<SyntaxSort::PlaceholderTypeSpecifier> {
                Symbolic::PlaceholderType type { };      // The placeholder type of the placeholder-type-specifier
                SourceLocation keyword { };              // The source location of the 'auto'/'decltype(auto)'
                SourceLocation locus { };                // The source location of the placeholder-type-specifier.
                                                            // This can be different from keyword_ if type-constraint
                                                            // is present
            };

            struct SimpleTypeSpecifier : Tag<SyntaxSort::SimpleTypeSpecifier> {
                TypeIndex type;            // The type (if we know it)
                ExprIndex expr;            // The expression (if we don't)
                SourceLocation locus;      // The source location
            };

            struct TypeSpecifierSeq : Tag<SyntaxSort::TypeSpecifierSeq> {
                SyntaxIndex type_script { };
                TypeIndex type { };
                SourceLocation locus { };
                Qualifier qualifiers { }; // cv-qualifiers associated with this type specifier
                bool is_unhashed { };
            };

            struct Keyword {
                enum class Kind : uint8_t {
                    None, Class, Struct, Union, Public,
                    Protected, Private, Default, Delete,
                    Mutable, Constexpr, Consteval, Typename,
                    Constinit,
                };
                SourceLocation locus { };            // The source location of the access-specifier token
                Kind kind = Kind::None;                // What kind of keyword do we have?
            };

            enum class StorageClass : uint32_t {
                None            = 0,
                Auto            = 1 <<  0,      // 'auto' (use as a storage-class requires '/Zc:auto-')
                Constexpr       = 1 <<  1,      // 'constexpr'
                Explicit        = 1 <<  2,      // 'explicit'
                Extern          = 1 <<  3,      // 'extern'
                Force_inline    = 1 <<  4,      // '__forceinline'
                Friend          = 1 <<  5,      // 'friend'
                Inline          = 1 <<  6,      // 'inline'
                Mutable         = 1 <<  7,      // 'mutable'
                New_slot        = 1 <<  8,      // 'new' (used in C++/CLI)
                Register        = 1 <<  9,      // 'register' (removed in C++17)
                Static          = 1 << 10,      // 'static'
                Thread_local    = 1 << 11,      // 'thread_local'
                Tile_static     = 1 << 12,      // 'tile_static' (C++AMP)
                Typedef         = 1 << 13,      // 'typedef'
                Virtual         = 1 << 14,      // 'virtual'
                Consteval       = 1 << 15,      // 'consteval' (C++20)
                Constinit       = 1 << 16,      // 'constinit' (C++20)
            };

            struct DeclSpecifierSeq : Tag<SyntaxSort::DeclSpecifierSeq> {
                TypeIndex type { };                  // The type (if it is a basic-type)
                SyntaxIndex type_script { };         // The type-name (if it is something other than a basic-type)
                SourceLocation locus { };            // The source location of this decl-specifier-seq
                StorageClass storage_class { };      // The storage-class (if any)
                SentenceIndex declspec { };          // Stream of __declspec tokens
                SyntaxIndex explicit_specifier { };  // The explicit-specifier (if any)
                Qualifier qualifiers { };            // Any cv-qualifiers etc.
            };

            struct EnumSpecifier : Tag<SyntaxSort::EnumSpecifier> {
                ExprIndex name { };                  // The name of the enumeration
                Keyword class_or_struct { };         // The 'class' or 'struct' keyword (if present)
                SyntaxIndex enumerators { };         // The set of enumerators
                SyntaxIndex enum_base { };           // The underlying type (if present)
                SourceLocation locus { };            // The source location of the enum-specifier
                SourceLocation colon { };            // The source location of the ':' (if present)
                SourceLocation left_brace { };       // The source location of the '{' (if present)
                SourceLocation right_brace { };      // The source location of the '}' (if present)
            };

            struct EnumeratorDefinition : Tag<SyntaxSort::EnumeratorDefinition> {
                TextOffset name { };                 // The name of the enumerator
                ExprIndex expression { };            // The initializer (if any)
                SourceLocation locus { };            // The source location of the enumerator
                SourceLocation assign { };           // The source location of the '=' (if present)
                SourceLocation comma { };            // The source location of the ',' (if present)
            };

            struct ClassSpecifier : Tag<SyntaxSort::ClassSpecifier> {
                ExprIndex name { };                  // The name of this class
                Keyword class_key { };               // The class-key: 'class', 'struct' or 'union'
                SyntaxIndex base_classes { };        // Any base-classes
                SyntaxIndex members { };             // Any members
                SourceLocation left_brace { };       // The source location of the '{' (if present)
                SourceLocation right_brace { };      // The source location of the '}' (if present)
            };

            struct BaseSpecifierList : Tag<SyntaxSort::BaseSpecifierList> {
                SyntaxIndex base_specifiers { };     // The set of base-classes
                SourceLocation colon { };            // The source location of the ':'
            };

            struct BaseSpecifier : Tag<SyntaxSort::BaseSpecifier> {
                ExprIndex name { };                  // The name of the base-class
                Keyword access_keyword { };          // The access-specifier (if present)
                SourceLocation virtual_keyword { };    // The source location of the 'virtual' keyword (if present)
                SourceLocation locus { };            // The source location of the base-specifier
                SourceLocation ellipsis { };         // The source location of the '...' (if present)
                SourceLocation comma { };            // The source location of the ',' (if present)
            };

            struct MemberSpecification : Tag<SyntaxSort::MemberSpecification> {
                SyntaxIndex declarations { };        // The set of member declarations
            };

            struct AccessSpecifier : Tag<SyntaxSort::AccessSpecifier> {
                Keyword keyword { };                 // The access-specifier keyword
                SourceLocation colon { };            // The source location of the ':'
            };

            struct MemberDeclaration : Tag<SyntaxSort::MemberDeclaration> {
                SyntaxIndex decl_specifier_seq { };      // The decl-specifier-seq
                SyntaxIndex declarators { };             // The the list of declarators
                SourceLocation semi_colon { };           // The source location of the ';'
            };

            struct MemberDeclarator : Tag<SyntaxSort::MemberDeclarator> {
                SyntaxIndex declarator { };          // The declarator
                SyntaxIndex requires_clause {};      // The requires clause (if present)
                ExprIndex expression { };            // The bitfield size (if present)
                ExprIndex initializer { };           // The initializer (if present)
                SourceLocation locus { };            // The source location of this member-declarator
                SourceLocation colon { };            // The source location of the ':' (if present)
                SourceLocation comma { };            // The source location of the ',' (if present)
            };

            struct TypeId : Tag<SyntaxSort::TypeId> {
                SyntaxIndex type { };            // The type
                SyntaxIndex declarator { };      // The declarator (if present)
                SourceLocation locus { };        // The source location of the type-id
            };

            struct TrailingReturnType : Tag<SyntaxSort::TrailingReturnType> {
                SyntaxIndex type { };            // The return type
                SourceLocation locus { };        // The source location of the '->'
            };

            struct PointerDeclarator : Tag<SyntaxSort::PointerDeclarator> {
                enum class Kind : uint8_t {
                    None,
                    Pointer,            // T*
                    LvalueReference,    // T&
                    RvalueReference,    // T&&
                    PointerToMember,    // T X::*
                };
                ExprIndex owner { };                 // The owning class if this is a pointer-to-member
                SyntaxIndex child { };               // Our child (if we have multiple indirections)
                SourceLocation locus { };            // The source location
                Kind kind { };                       // What kind of pointer do we have?
                Qualifier qualifiers { };            // any cv-qualifiers etc.
                CallingConvention convention { };    // possible calling convention with this type-specifier (only used if is_function_ is set)
                bool is_function { };                // does this declarator name a function type?
            };

            struct ArrayDeclarator : Tag<SyntaxSort::ArrayDeclarator> {
                ExprIndex bounds { };                // The bounds expression (if any)
                SourceLocation left_bracket { };     // The source location of the '['
                SourceLocation right_bracket { };    // The source location of the ']'
            };

            struct FunctionDeclarator : Tag<SyntaxSort::FunctionDeclarator> {
                SyntaxIndex parameters { };                  // The function parameters
                SyntaxIndex exception_specification { };     // The exception-specification (if present)
                SourceLocation left_paren { };               // The source location of the '('
                SourceLocation right_paren { };              // The source location of the ')'
                SourceLocation ellipsis { };                 // The source location of the '...' (if present)
                SourceLocation ref_qualifier { };            // The source location of the '&' or '&&' (if present)
                FunctionTypeTraits traits { };               // Any cv-qualifiers etc. associated with the function
            };

            struct ArrayOrFunctionDeclarator : Tag<SyntaxSort::ArrayOrFunctionDeclarator> {
                SyntaxIndex declarator { };              // This declarator
                SyntaxIndex next { };                    // The next declarator (if we have multiple indirections)
            };

            struct ParameterDeclarator : Tag<SyntaxSort::ParameterDeclarator> {
                SyntaxIndex decl_specifier_seq { };      // The decl-specifier_seq for this parameter
                SyntaxIndex declarator { };              // The declarator for this parameter
                ExprIndex default_argument { };          // Any default argument
                SourceLocation locus { };                // The source location of this parameter
                ParameterSort sort { };
            };

            struct VirtualSpecifierSeq : Tag<SyntaxSort::VirtualSpecifierSeq> {
                SourceLocation locus { };                // The source location (should probably be deduced)
                SourceLocation final_keyword { };        // The source location of the 'final' keyword (if present)
                SourceLocation override_keyword { };     // The source location of the 'override' keyword (if present)
                bool is_pure { };                        // Is this a pure virtual function?
            };

            struct NoexceptSpecification : Tag<SyntaxSort::NoexceptSpecification> {
                ExprIndex expression { };                // The constant-expression (if present)
                SourceLocation locus { };                // The source location
                SourceLocation left_paren { };           // The source location of the '(' (if present)
                SourceLocation right_paren { };          // The source location of the ')' (if present)
            };

            struct ExplicitSpecifier : Tag<SyntaxSort::ExplicitSpecifier> {
                ExprIndex expression { };                // The constant-expression. This node is used only for conditional explicit where the expression exists.
                SourceLocation locus { };                // The source location of the 'explicit' keyword
                SourceLocation left_paren { };           // The source location of the '('
                SourceLocation right_paren { };          // The source location of the ')'
            };

            struct Declarator : Tag<SyntaxSort::Declarator> {
                SyntaxIndex pointer { };                         // A pointer declarator: '*D'
                SyntaxIndex parenthesized_declarator { };        // A parenthesized declarator: '(D)'
                SyntaxIndex array_or_function_declarator { };    // An array or function declarator: 'D[...]' or 'D(...)'
                SyntaxIndex trailing_return_type { };            // Any trailing return type
                SyntaxIndex virtual_specifiers { };              // Any virtual or pure specifiers: 'override', 'final', '= 0'
                ExprIndex name { };                              // The name of the declarator
                SourceLocation ellipsis { };                     // The location of '...' (if present)
                SourceLocation locus { };                        // The source location of this declarator
                Qualifier qualifiers { };                        // Any cv-qualifiers etc.
                CallingConvention convention { };                // Possible calling convention with this type-specifier (only used if is_function_ is set)
                bool is_function { };                            // Is this declarator name a function type?
            };

            struct InitDeclarator : Tag<SyntaxSort::InitDeclarator> {
                SyntaxIndex declarator { };      // The declarator
                SyntaxIndex requires_clause {};  // The requires clause (if present)
                ExprIndex initializer { };       // The associated initializer
                SourceLocation comma { };        // The source location of the ',' (if present)
            };

            struct NewDeclarator : Tag<SyntaxSort::NewDeclarator> {
                SyntaxIndex declarator { };      // The declarator
            };

            struct SimpleDeclaration : Tag<SyntaxSort::SimpleDeclaration> {
                SyntaxIndex decl_specifier_seq { };      // The decl-specifier-seq
                SyntaxIndex declarators { };             // The declarator(s)
                SourceLocation locus { };                // The source location of this declaration
                SourceLocation semi_colon { };           // The source location of the ';'
            };

            struct ExceptionDeclaration : Tag<SyntaxSort::ExceptionDeclaration> {
                SyntaxIndex type_specifier_seq { };       // The type-specifier-seq
                SyntaxIndex declarator { };              // The declarator
                SourceLocation locus { };                // The source location of this declaration
                SourceLocation ellipsis { };             // The source location of the '...' (if present)
            };

            struct ConditionDeclaration : Tag<SyntaxSort::ConditionDeclaration> {
                SyntaxIndex decl_specifier { };          // The decl-specifier
                SyntaxIndex init_statement { };          // The initialization expression
                SourceLocation locus { };                // The source location of this declaration
            };

            struct StaticAssertDeclaration : Tag<SyntaxSort::StaticAssertDeclaration> {
                ExprIndex expression { };                // The constant expression
                ExprIndex message { };                   // The string-literal (if present)
                SourceLocation locus { };                // The source location of this declaration
                SourceLocation left_paren { };           // The source location of the '('
                SourceLocation right_paren { };          // The source location of the ')'
                SourceLocation semi_colon { };           // The source location of the ';'
                SourceLocation comma { };                // The source location of the ',' (if present)
            };

            struct AliasDeclaration : Tag<SyntaxSort::AliasDeclaration> {
                TextOffset identifier { };           // The identifier
                SyntaxIndex defining_type_id { };    // The defining-type-id
                SourceLocation locus { };            // The source location of this declaration
                SourceLocation assign { };           // The source location of the '='
                SourceLocation semi_colon { };       // The source location of the ';'
            };

            struct ConceptDefinition : Tag<SyntaxSort::ConceptDefinition> {
                SyntaxIndex parameters { };          // Any template parameters
                SourceLocation locus { };            // The source location of the 'template' keyword
                TextOffset identifier { };           // The identifier
                ExprIndex expression { };            // The constraint-expression
                SourceLocation concept_keyword { };          // The source location of the 'concept' keyword
                SourceLocation assign { };           // The source location of the '='
                SourceLocation semi_colon { };       // The source location of the ';'
            };

            struct StructuredBindingDeclaration : Tag<SyntaxSort::StructuredBindingDeclaration> {
                enum class RefQualifierKind : uint8_t {
                    None,
                    Rvalue,
                    Lvalue,
                };
                SourceLocation locus { };                    // The location of this declaration
                SourceLocation ref_qualifier { };            // The source location of the ref-qualifier (if any)
                SyntaxIndex decl_specifier_seq { };          // The decl-specifier-seq
                SyntaxIndex identifier_list { };             // The list of identifiers: [ a, b, c ]
                ExprIndex initializer { };                   // The expression initializer
                RefQualifierKind ref_qualifier_kind { };     // The ref-qualifier on the type (if any)
            };

            struct StructuredBindingIdentifier : Tag<SyntaxSort::StructuredBindingIdentifier> {
                ExprIndex identifier { };            // Identifier expression
                SourceLocation comma { };            // The source location of the ',' (if present)
            };

            struct AsmStatement : Tag<SyntaxSort::AsmStatement> {
                AsmStatement() : tokens{}, locus{} { }
                AsmStatement(SentenceIndex t, const SourceLocation& l) : tokens{ t }, locus{ l } { }
                SentenceIndex tokens;
                SourceLocation locus;
            };

            struct ReturnStatement : Tag<SyntaxSort::ReturnStatement> {
                enum class ReturnKind : uint8_t {
                    Return,
                    Co_return,
                };

                SentenceIndex pragma_tokens {};      // The index for any preceding #pragma tokens.
                ExprIndex expr { };                  // The (optional) expression
                ReturnKind return_kind { };          // 'return' or 'co_return'
                SourceLocation return_locus { };     // The source location of the 'return' keyword
                SourceLocation semi_colon { };       // The source location of the ';'
            };

            struct CompoundStatement : Tag<SyntaxSort::CompoundStatement> {
                SentenceIndex pragma_tokens {};      // The index for any preceding #pragma tokens.
                SyntaxIndex statements { };          // The sub-statement(s)
                SourceLocation left_curly { };       // The source location of the '{'
                SourceLocation right_curly { };      // The source location of the '}'
            };

            struct IfStatement : Tag<SyntaxSort::IfStatement> {
                SentenceIndex pragma_tokens {};      // The index for any preceding #pragma tokens.
                SyntaxIndex init_statement { };      // The init-statement (opt)
                ExprIndex condition { };             // The condition
                SyntaxIndex if_true { };               // The sub-statement(s) to be executed if the condition is true
                SyntaxIndex if_false { };            // The sub-statement(s) to be executed if the condition is false
                SourceLocation if_keyword { };       // The source location of the 'if' keyword
                SourceLocation constexpr_locus { };  // The source location of the 'constexpr' keyword (if provided)
                SourceLocation else_keyword { };     // THe source location of the 'else' keyword
            };

            struct WhileStatement : Tag<SyntaxSort::WhileStatement> {
                SentenceIndex pragma_tokens {};      // The index for any preceding #pragma tokens.
                ExprIndex condition { };             // The condition
                SyntaxIndex statement { };           // The sub-statement(s)
                SourceLocation while_keyword { };    // The source location of the 'while' keyword
            };

            struct DoWhileStatement : Tag<SyntaxSort::DoWhileStatement> {
                SentenceIndex pragma_tokens {};      // The index for any preceding #pragma tokens.
                ExprIndex condition { };             // The condition
                SyntaxIndex statement { };           // The sub-statement(s)
                SourceLocation do_keyword { };       // The source location of the 'do' keyword
                SourceLocation while_keyword { };    // The source location of the 'while' keyword
                SourceLocation semi_colon { };       // The source location of the ';'
            };

            struct ForStatement : Tag<SyntaxSort::ForStatement> {
                SentenceIndex pragma_tokens {};      // The index for any preceding #pragma tokens.
                SyntaxIndex init_statement { };      // The init-statement
                ExprIndex condition { };             // The condition
                ExprIndex expression { };            // The expression
                SyntaxIndex statement { };           // The sub-statement(s)
                SourceLocation for_keyword { };      // The source location of the 'for' keyword
                SourceLocation left_paren { };       // The source location of the '('
                SourceLocation right_paren { };      // The source location of the ')'
                SourceLocation semi_colon { };       // The source location of the 2nd ';'
            };

            struct InitStatement : Tag<SyntaxSort::InitStatement> {
                SentenceIndex pragma_tokens {};      // The index for any preceding #pragma tokens.
                SyntaxIndex expression_or_declaration { };   // Either an expression or a simple declaration
            };

            struct RangeBasedForStatement : Tag<SyntaxSort::RangeBasedForStatement> {
                SentenceIndex pragma_tokens {};      // The index for any preceding #pragma tokens.
                SyntaxIndex init_statement { };      // Ths init-statement (if any)
                SyntaxIndex declaration { };         // The declaration
                ExprIndex initializer { };           // The initializer
                SyntaxIndex statement { };           // The sub-statement(s)
                SourceLocation for_keyword { };      // The source location of the 'for' keyword
                SourceLocation left_paren { };       // The source location of the '('
                SourceLocation right_paren { };      // The source location of the ')'
                SourceLocation colon { };            // The source location of the ':'
            };

            struct ForRangeDeclaration : Tag<SyntaxSort::ForRangeDeclaration> {
                SyntaxIndex decl_specifier_seq { };      // The decl-specifier_seq
                SyntaxIndex declarator { };              // The declarator
            };

            struct LabeledStatement : Tag<SyntaxSort::LabeledStatement> {
                enum class Kind : uint8_t { None, Case, Default, Label };
                SentenceIndex pragma_tokens {};      // The index for any preceding #pragma tokens.
                ExprIndex expression { };            // The expression for a case statement or the name for a label
                SyntaxIndex statement { };           // The sub-statement
                SourceLocation locus { };            // The source location of this label
                SourceLocation colon { };            // The source location of the ':'
                Kind kind { };                       // What kind of label do we have?
            };

            struct BreakStatement : Tag<SyntaxSort::BreakStatement> {
                SourceLocation break_keyword { };    // The source location of the 'break' keyword
                SourceLocation semi_colon { };       // The source location of the ';'
            };

            struct ContinueStatement : Tag<SyntaxSort::ContinueStatement> {
                SourceLocation continue_keyword { };     // The source location of the 'continue' keyword
                SourceLocation semi_colon { };           // The source location of the ';'
            };

            struct SwitchStatement : Tag<SyntaxSort::SwitchStatement> {
                SentenceIndex pragma_tokens {};      // The index for any preceding #pragma tokens.
                SyntaxIndex init_statement { };      // The init-statement (opt)
                ExprIndex condition { };             // The condition
                SyntaxIndex statement { };           // The sub-statement(s)
                SourceLocation switch_keyword { };   // The source location of the 'switch' keyword
            };

            struct GotoStatement : Tag<SyntaxSort::GotoStatement> {
                SentenceIndex pragma_tokens {};      // The index for any preceding #pragma tokens.
                TextOffset name { };                  // The associated label
                SourceLocation locus { };            // The source location of the 'goto' keyword
                SourceLocation label { };            // The source location of the label;
                SourceLocation semi_colon { };       // The source location of the ';'
            };

            struct DeclarationStatement : Tag<SyntaxSort::DeclarationStatement> {
                SentenceIndex pragma_tokens {};      // The index for any preceding #pragma tokens.
                SyntaxIndex declaration { };         // The syntactic representation of the declaration
            };

            struct ExpressionStatement : Tag<SyntaxSort::ExpressionStatement> {
                SentenceIndex pragma_tokens {};      // The index for any preceding #pragma tokens.
                ExprIndex expression { };            // The expression
                SourceLocation semi_colon { };       // The source location of the ';'
            };

            struct TryBlock : Tag<SyntaxSort::TryBlock> {
                SentenceIndex pragma_tokens {};      // The index for any preceding #pragma tokens.
                SyntaxIndex statement { };           // The compound statement
                SyntaxIndex handler_seq { };         // The handlers
                SourceLocation try_keyword { };      // The source location of the 'try' keyword
            };

            struct Handler : Tag<SyntaxSort::Handler> {
                SentenceIndex pragma_tokens {};      // The index for any preceding #pragma tokens.
                SyntaxIndex exception_declaration { };   // The exception-declaration
                SyntaxIndex statement { };               // The compound statement
                SourceLocation catch_keyword { };        // The source location of the 'catch' keyword
                SourceLocation left_paren { };           // The source location of the '('
                SourceLocation right_paren { };          // The source location of the ')'
            };

            struct HandlerSeq : Tag<SyntaxSort::HandlerSeq> {
                SyntaxIndex handlers { };            // One of more catch handlers
            };

            struct FunctionTryBlock : Tag<SyntaxSort::FunctionTryBlock> {
                SyntaxIndex statement { };           // The compound statement)
                SyntaxIndex handler_seq { };         // The catch handlers
                SyntaxIndex initializers { };        // Any base-or-member initializers (if this is for a constructor)
            };

            struct TypeIdListElement : Tag<SyntaxSort::TypeIdListElement> {
                SyntaxIndex type_id { };             // The type-id
                SourceLocation ellipsis { };         // The source location of the '...' (if present)
            };

            struct DynamicExceptionSpec : Tag<SyntaxSort::DynamicExceptionSpec> {
                SyntaxIndex type_list { };               // The list of associated types
                SourceLocation throw_keyword { };        // The source location of the 'throw' keyword
                SourceLocation left_paren { };           // The source location of the '('
                SourceLocation ellipsis { };             // The source location of the '...' (MS-only extension) 'type_list_' will be empty
                SourceLocation right_paren { };          // The source location of the ')'
            };

            struct StatementSeq : Tag<SyntaxSort::StatementSeq> {
                SyntaxIndex statements { };          // A sequence of statements
            };

            struct MemberFunctionDeclaration : Tag<SyntaxSort::MemberFunctionDeclaration> {
                SyntaxIndex definition { };              // The definition of this member function
            };

            struct FunctionDefinition : Tag<SyntaxSort::FunctionDefinition> {
                SyntaxIndex decl_specifier_seq { };      // The decl-specifier-seq for this function
                SyntaxIndex declarator { };              // The declarator for this function
                SyntaxIndex requires_clause { };         // The requires clause (if present)
                SyntaxIndex body { };                    // The body of this function
            };

            struct FunctionBody : Tag<SyntaxSort::FunctionBody> {
                SyntaxIndex statements { };              // The statement(s)
                SyntaxIndex function_try_block { };      // The function-try-block (if any)
                SyntaxIndex initializers { };            // Any base-or-member initializers (if this is for a constructor)
                Keyword default_or_delete { };           // Is the function 'default'ed' or 'delete'ed'
                SourceLocation assign { };               // The source location of the '=' (if any)
                SourceLocation semi_colon { };           // THe source location of the ';' (if any)
            };

            struct Expression : Tag<SyntaxSort::Expression> {
                ExprIndex expression { };
            };

            struct TemplateParameterList : Tag<SyntaxSort::TemplateParameterList> {
                SyntaxIndex parameters { };          // Any template parameters
                SyntaxIndex requires_clause {};      // The requires clause (if present)
                SourceLocation left_angle { };       // The source location of the '<'
                SourceLocation right_angle { };      // The source location of the '>'
            };

            struct TemplateDeclaration : Tag<SyntaxSort::TemplateDeclaration> {
                SyntaxIndex parameters { };          // Any template parameters
                SyntaxIndex declaration { };          // The declaration under the template
                SourceLocation locus { };            // The source location of the 'template' keyword
            };

            struct RequiresClause : Tag<SyntaxSort::RequiresClause> {
                ExprIndex expression { };            // The expression for this requires clause
                SourceLocation locus { };            // The source location
            };

            struct SimpleRequirement : Tag<SyntaxSort::SimpleRequirement> {
                ExprIndex expression { };            // The expression for this requirement
                SourceLocation locus { };            // The source location
            };

            struct TypeRequirement : Tag<SyntaxSort::TypeRequirement> {
                ExprIndex type { };                  // The type for this requirement
                SourceLocation locus { };            // The source location
            };

            struct CompoundRequirement : Tag<SyntaxSort::CompoundRequirement> {
                ExprIndex expression { };               // The expression for this requirement
                ExprIndex type_constraint { };          // The optional type constraint
                SourceLocation locus { };            // The source location
                SourceLocation right_curly { };      // The source location of the '}'
                SourceLocation noexcept_keyword { };            // The source location of the 'noexcept' keyword
            };

            struct NestedRequirement : Tag<SyntaxSort::NestedRequirement> {
                ExprIndex expression { };            // The expression for this requirement
                SourceLocation locus { };            // The source location
            };

            struct RequirementBody : Tag<SyntaxSort::RequirementBody> {
                SyntaxIndex requirements {};         // The requirements
                SourceLocation locus { };            // The source location
                SourceLocation right_curly { };      // The source location of the '}'
            };

            struct TypeTemplateParameter : Tag<SyntaxSort::TypeTemplateParameter> {
                TextOffset name { };                 // The identifier for this template parameter
                ExprIndex type_constraint {};        // The optional type constraint
                SyntaxIndex default_argument { };    // The optional default argument
                SourceLocation locus { };            // The source location of the parameter
                SourceLocation ellipsis { };         // The source location of the '...' (if present)
            };

            struct TemplateTemplateParameter : Tag<SyntaxSort::TemplateTemplateParameter> {
                TextOffset name { };                 // The identifier for this template template-parameter (if present)
                SyntaxIndex default_argument { };    // The optional default argument
                SyntaxIndex parameters { };          // The template parameters
                SourceLocation locus { };            // The source location of the 'template' keyword
                SourceLocation ellipsis { };         // The source location of the '...' (if present)
                SourceLocation comma { };            // The source location of the ',' (if present)
                Keyword type_parameter_key { };      // The type parameter key (typename|class)
            };

            struct TypeTemplateArgument : Tag<SyntaxSort::TypeTemplateArgument> {
                SyntaxIndex argument { };        // The template-argument
                SourceLocation ellipsis { };     // The source location of the '...' (if present)
                SourceLocation comma { };        // The source location of the ',' (if present)
            };

            struct NonTypeTemplateArgument : Tag<SyntaxSort::NonTypeTemplateArgument> {
                ExprIndex argument { };          // The template-argument
                SourceLocation ellipsis { };     // The source location of the '...' (if present)
                SourceLocation comma { };        // The source location of the ',' (if present)
            };

            struct TemplateArgumentList : Tag<SyntaxSort::TemplateArgumentList> {
                SyntaxIndex arguments { };           // Any template arguments
                SourceLocation left_angle { };       // The source location of the '<'
                SourceLocation right_angle { };      // The source location of the '>'
            };

            struct TemplateId : Tag<SyntaxSort::TemplateId> {
                SyntaxIndex name { };                // The name of the primary template (if it is not bound)
                ExprIndex symbol { };                // The symbol for the primary template (if it is bound)
                SyntaxIndex arguments { };           // The template argument list
                SourceLocation locus { };            // The source location
                SourceLocation template_keyword { }; // The source location of the optional 'template' keyword
            };

            struct MemInitializer : Tag<SyntaxSort::MemInitializer> {
                ExprIndex name { };                  // The member or base to initialize
                ExprIndex initializer { };           // The initializer
                SourceLocation ellipsis { };         // The source location of the '...' (if present)
                SourceLocation comma { };            // The source location of the ',' (if present)
            };

            struct CtorInitializer : Tag<SyntaxSort::CtorInitializer> {
                SyntaxIndex initializers { };        // The set of mem-initializers
                SourceLocation colon { };            // The source location of the ':'
            };

            struct CaptureDefault : Tag<SyntaxSort::CaptureDefault> {
                SourceLocation locus { };                // The source location of this capture
                SourceLocation comma { };                // The source location of the ',' (if present)
                bool default_is_by_reference;              // Is the capture-default '&' or '='?
            };

            struct SimpleCapture : Tag<SyntaxSort::SimpleCapture> {
                ExprIndex name { };                  // The name of this simple-capture
                SourceLocation ampersand { };        // The source location of the '&' (if present)
                SourceLocation ellipsis { };         // The source location of the '...' (if present)
                SourceLocation comma { };            // The source location of the ',' (if present)
            };

            struct InitCapture : Tag<SyntaxSort::InitCapture> {
                ExprIndex name { };                  // The name of the init-capture
                ExprIndex initializer { };           // The associated initializer
                SourceLocation ellipsis { };         // The source location of the '...' (if present)
                SourceLocation ampersand { };        // The source location of the '&' (if present)
                SourceLocation comma { };            // The source location of the ',' (if present)
            };

            struct ThisCapture : Tag<SyntaxSort::ThisCapture> {
                SourceLocation locus { };            // The source location of this capture
                SourceLocation asterisk { };         // The source location of the '*' capture (if present)
                SourceLocation comma { };            // The source location of the ',' (if present)
            };

            struct LambdaIntroducer : Tag<SyntaxSort::LambdaIntroducer> {
                SyntaxIndex captures { };            // Any associated captures
                SourceLocation left_bracket { };     // The source location of the '['
                SourceLocation right_bracket { };    // The source location of the ']'
            };

            struct LambdaDeclarator : Tag<SyntaxSort::LambdaDeclarator> {
                SyntaxIndex parameters { };                  // The parameters
                SyntaxIndex exception_specification { };     // The exception-specification (if present)
                SyntaxIndex trailing_return_type { };        // The trailing return type (if present)
                Keyword keyword { };                         // 'mutable' or 'constexpr' (if present)
                SourceLocation left_paren { };               // The source location of the '('
                SourceLocation right_paren { };              // The source location of the ')'
                SourceLocation ellipsis { };                 // The source location of the '...' (if present)
            };

            struct UsingDeclaration : Tag<SyntaxSort::UsingDeclaration> {
                SyntaxIndex declarators { };             // The associated using-declarators
                SourceLocation using_keyword { };        // The source location of the 'using' keyword
                SourceLocation semi_colon { };           // The source location of the ';'
            };

            struct UsingEnumDeclaration : Tag<SyntaxSort::UsingEnumDeclaration> {
                ExprIndex name { };                      // The specified enumeration
                SourceLocation using_keyword { };        // The source location of the 'using' keyword
                SourceLocation enum_keyword { };         // The source location of the 'enum' keyword
                SourceLocation semi_colon { };           // The source location of the ';'
            };

            struct UsingDeclarator : Tag<SyntaxSort::UsingDeclarator> {
                ExprIndex qualified_name { };            // The qualified-name
                SourceLocation typename_keyword { };     // The source location of the 'typename' (if present)
                SourceLocation ellipsis { };             // The source location of the '...' (if present)
                SourceLocation comma { };                // The source location of the ',' (if present)
            };

            struct UsingDirective : Tag<SyntaxSort::UsingDirective> {
                ExprIndex qualified_name {};             // The qualified-name being used
                SourceLocation using_keyword {};         // The source location of the 'using' keyword
                SourceLocation namespace_keyword {};     // The source location of the 'namespace' keyword
                SourceLocation semi_colon {};            // The source location of the ';'
            };

            struct NamespaceAliasDefinition : Tag<SyntaxSort::NamespaceAliasDefinition> {
                ExprIndex identifier {};             // The identifier
                ExprIndex namespace_name {};         // The name of the namespace
                SourceLocation namespace_keyword {}; // The source location of the 'namespace' keyword
                SourceLocation assign {};            // The source location of the '='
                SourceLocation semi_colon {};        // The source location of the ';'
            };

            struct ArrayIndex : Tag<SyntaxSort::ArrayIndex> {
                ExprIndex array { };                 // The array expression
                ExprIndex index { };                 // The index expression
                SourceLocation left_bracket { };     // The source location of the '['
                SourceLocation right_bracket { };    // The source location of the ']'
            };

            struct TypeTraitIntrinsic : Tag<SyntaxSort::TypeTraitIntrinsic> {
                SyntaxIndex arguments { };              // Any arguments;
                SourceLocation locus { };               // The source location of the intrinsic
                Operator intrinsic { };                 // The index of the intrinsic
            };

            struct SEHTry : Tag<SyntaxSort::SEHTry> {
                SyntaxIndex statement { };           // The associated compound statement
                SyntaxIndex handler { };             // The associated handler
                SourceLocation try_keyword { };      // The source location of the '__try' keyword
            };

            struct SEHExcept : Tag<SyntaxSort::SEHExcept> {
                ExprIndex expression { };                // The associated expression
                SyntaxIndex statement { };               // The associated compound statement
                SourceLocation except_keyword { };       // The source location of the '__except' keyword
                SourceLocation left_paren { };           // The source location of the '('
                SourceLocation right_paren { };          // The source location of the ')'
            };

            struct SEHFinally : Tag<SyntaxSort::SEHFinally> {
                SyntaxIndex statement { };               // The associated compound statement
                SourceLocation finally_keyword { };      // The source location of the '__finally' keyword
            };

            struct SEHLeave : Tag<SyntaxSort::SEHLeave> {
                SourceLocation leave_keyword { };        // The source location of the '__leave' keyword
                SourceLocation semi_colon { };           // The source location of the ';'
            };

            struct Super : Tag<SyntaxSort::Super> {
                SourceLocation locus {};         // The source location of the '__super' keyword
            };

            enum class FoldKind : uint32_t {        // FIXME: this should be uint8_t
                Unknown,
                LeftFold,       // Some form of left fold expression
                RightFold,      // Some form of righr fold expression
            };

            struct UnaryFoldExpression : Tag<SyntaxSort::UnaryFoldExpression> {
                FoldKind kind = FoldKind::Unknown;       // FIXME: Move last
                ExprIndex expression { };                // The associated expression
                DyadicOperator op { };             // The operator kind
                SourceLocation locus { };                // The source location of the unary fold expression
                SourceLocation ellipsis { };             // The source location of the '...'
                SourceLocation operator_locus { };       // The source location of the operator
                SourceLocation right_paren { };          // The source location of the ')'
            };

            struct BinaryFoldExpression : Tag<SyntaxSort::BinaryFoldExpression> {
                FoldKind kind = FoldKind::Unknown;       // FIXME: Move last
                ExprIndex left_expression { };           // The left expression
                ExprIndex right_expression { };          // The right expression (if present)
                DyadicOperator op { };             // The operator kind
                SourceLocation locus { };                // The source location of the fold expression
                SourceLocation ellipsis { };             // The source location of the '...'
                SourceLocation left_operator_locus { };  // The source location of the left operator
                SourceLocation right_operator_locus { }; // The source location of the right operator
                SourceLocation right_paren { };          // The source location of the ')'
            };

            struct EmptyStatement : Tag<SyntaxSort::EmptyStatement> {
                SourceLocation locus { };                // The source location of the ';'
            };

            struct AttributedStatement : Tag<SyntaxSort::AttributedStatement> {
                SentenceIndex pragma_tokens {};  // The index for any preceding #pragma tokens.
                SyntaxIndex statement { };       // The statement
                SyntaxIndex attributes { };      // The associated attributes
            };

            struct AttributedDeclaration : Tag<SyntaxSort::AttributedDeclaration> {
                SourceLocation locus { };            // The source location of the attributes
                SyntaxIndex declaration { };         // The declaration
                SyntaxIndex attributes { };          // The associated attributes
            };

            struct AttributeSpecifierSeq : Tag<SyntaxSort::AttributeSpecifierSeq> {
                SyntaxIndex attributes { };          // The sequence of attributes
            };

            struct AttributeSpecifier : Tag<SyntaxSort::AttributeSpecifier> {
                SyntaxIndex using_prefix { };            // The attribute using prefix (if any)
                SyntaxIndex attributes { };              // The attributes (if any)
                SourceLocation left_bracket_1 { };       // The source location of the first '['
                SourceLocation left_bracket_2 { };       // The source location of the second '['
                SourceLocation right_bracket_1 { };      // The source location of the first ']'
                SourceLocation right_bracket_2 { };      // The source location of the second ']'
            };

            struct AttributeUsingPrefix : Tag<SyntaxSort::AttributeUsingPrefix> {
                ExprIndex attribute_namespace { };       // The attribute namespace
                SourceLocation using_locus { };                // The source location of the 'using'
                SourceLocation colon { };                // The source location of the  ':'
            };

            struct Attribute : Tag<SyntaxSort::Attribute> {
                ExprIndex identifier { };                // The name of the attribute
                ExprIndex attribute_namespace { };       // The attribute namespace (if present)
                SyntaxIndex argument_clause { };         // The attribute argument clause (if present)
                SourceLocation double_colon { };         // The source location of the '::' (if present)
                SourceLocation ellipsis { };             // The source location of the '...' (if present)
                SourceLocation comma { };                // The source location of the ',' (if present)
            };

            struct AttributeArgumentClause : Tag<SyntaxSort::AttributeArgumentClause> {
                SentenceIndex tokens { };            // The argument tokens (if present)
                SourceLocation left_paren { };       // The source location of the '('
                SourceLocation right_paren { };      // The source location of the ')'
            };

            struct AlignasSpecifier : Tag<SyntaxSort::Alignas> {
                SyntaxIndex expression { };          // The expression of this alignas
                SourceLocation locus { };            // The source location of the alignas specifier
                SourceLocation left_paren { };       // The source location of the '('
                SourceLocation right_paren { };      // The source location of the ')'
            };

            // A sequence of zero or syntactic elements
            struct Tuple : Tag<SyntaxSort::Tuple>, Sequence<SyntaxIndex, HeapSort::Syntax> {
                Tuple() = default;
                Tuple(Index f, Cardinality n) : Sequence{ f, n } { }
            };

            namespace Microsoft
            {
                // There are a few operators that accept either a type or an expression -- or in some cases template name.
                // Usually such an operator will establish a default kind (choose either type or expression) and have
                // an indirection to encode the other kind, instead of maintaining an ambiguity to be resolved by a side
                // bit stored elsewhere.  The current encoding of MSVC's __uuidof extension maintains such an ambiguity.
                struct TypeOrExprOperand {
                    union {
                        TypeIndex as_type;
                        ExprIndex as_expr;
                    };

                    void operator=(TypeIndex idx) { as_type = idx; }
                    void operator=(ExprIndex idx) { as_expr = idx; }
                };
                static_assert(sizeof(TypeOrExprOperand) == sizeof(Index));

                // the types of msvc-specific C++ syntax
                enum class Kind : uint8_t
                {
                    Unknown,
                    Declspec,               // __declspec( expression )
                    BuiltinAddressOf,       // __builtin_addressof( expression )
                    UUIDOfTypeID,           // __uuidof( typeid )
                    UUIDOfExpr,             // __uuidof( expression )
                    BuiltinHugeValue,       // __builtin_huge_val()
                    BuiltinHugeValuef,      // __builtin_huge_valf()
                    BuiltinNan,             // __builtin_nan
                    BuiltinNanf,            // __builtin_nanf
                    BuiltinNans,            // __builtin_nans
                    BuiltinNansf,           // __builtin_nansf
                    IfExists,               // __if_exists ( id-expression ) brace-enclosed-region
                    Count
                };
                struct Declspec {                   // __declspec( expression )
                    SourceLocation left_paren;     // location of '('
                    SourceLocation right_paren;    // location of ')'
                    SentenceIndex tokens;          // token expression
                };
                struct BuiltinAddressOf {           // __builtin_addressof( expression )
                    ExprIndex expr;                // inner expression
                };
                struct UUIDOf {                     // __uuidof( type-id ) | __uuidof( expression )
                    SourceLocation left_paren;     // location of '('
                    SourceLocation right_paren;    // location of ')'
                    TypeOrExprOperand operand;      // kind: UUIDOfTypeId => TypeIndex; UUIDIfExpr => ExprIndex
                };
                struct Intrinsic {
                    SourceLocation left_paren;     // location of '('
                    SourceLocation right_paren;    // location of ')'
                    ExprIndex argument;            // expression
                };
                struct IfExists {
                    enum class Kind : uint8_t { Statement, Initializer, MemberDeclaration };
                    enum class Keyword : uint8_t { IfExists, IfNotExists };
                    Kind kind;                     // the declaration context of __if_exists
                    Keyword keyword;               // distinction between __if_exists and __if_not_exists
                    ExprIndex subject;             // the id-expression to be tested
                    SourceLocation left_paren;     // location of '('
                    SourceLocation right_paren;    // location of ')'
                    SentenceIndex tokens;          // brace-enclosed region
                };

                struct VendorSyntax : Tag<SyntaxSort::VendorExtension> {
                    VendorSyntax() : ms_declspec{ } { }
                    Kind kind = Kind::Unknown;
                    SourceLocation locus { };        // The root source location of the syntax
                    union
                    {
                        Declspec ms_declspec;
                        BuiltinAddressOf ms_builtin_addressof;
                        UUIDOf ms_uuidof;
                        Intrinsic ms_intrinsic;
                        IfExists ms_if_exists;
                    };
                };
            }
        }

        // Location description of a source file + line pair
        struct FileAndLine : std::pair<NameIndex, LineNumber> {
            FileAndLine() : std::pair<NameIndex, LineNumber>() { }
            FileAndLine(NameIndex index, LineNumber line) : std::pair<NameIndex, LineNumber>(index, line) { }

            NameIndex file() const
            {
                return first;
            }

            LineNumber line() const
            {
                return second;
            }
        };

        // Almost any C++ declaration can be parameterized, either directly (as a template),
        // or indirectly (when declared at the lexical scope of a template) without being template.
        struct ParameterizedEntity {
            DeclIndex decl { };             // The entity being parameterized.
            SentenceIndex head { };         // The sequence of words making up the declarative part of current instantiation.
            SentenceIndex body { };         // The sequence of words making up the body of this template
            SentenceIndex attributes { };   // Attributes of this template.
        };

        // Symbolic representation of a specialization request, whether implicit or explicit.
        struct SpecializationForm {
            DeclIndex template_decl;
            ExprIndex arguments;
        };

        // This structure is used to represent a body of a constexpr function.
        // In the future, it will be also used to represent the bodies of inline functions and function templates.
        // It is stored in the traits partition indexed by the DeclIndex of its FunctionDecl, NonStaticMemberFunctionDecl or ConstructorDecl.
        // TODO: initializer is a bit out of place here for a mapping. Fix it or learn to live with it.
        struct MappingDefinition {
            ChartIndex parameters { };     // Function parameter list (if any) in the defining declaration
            ExprIndex initializers { };    // Any base-or-member initializers (if this is for a constructor)
            StmtIndex body { };            // The body of the function
        };

        template<typename T>
        struct Identity {
            T name;                 // The name of the entity (either 'NameIndex' or 'TextOffset')
            SourceLocation locus;   // Source location of this entity
        };

        // Symbolic representation of a function declaration.
        struct FunctionDecl : Tag<DeclSort::Function> {
            Identity<NameIndex> identity;    // The name and location of this function
            TypeIndex type;                  // Sort and index of this decl's type.  Null means no type.
            DeclIndex home_scope;            // Enclosing scope of this declaration.
            ChartIndex chart;                // Function parameter list.
            FunctionTraits traits;           // Function traits
            BasicSpecifiers basic_spec;      // Basic declaration specifiers.
            Access access;                   // Access right to this function.
            ReachableProperties properties;  // Set of reachable properties of this declaration
        };

        static_assert(offsetof(FunctionDecl, identity) == 0, "The name population code expects 'identity' to be at offset zero");

        struct IntrinsicDecl : Tag<DeclSort::Intrinsic> {
            Identity<TextOffset> identity;  // // The name and location of this intrinsic
            TypeIndex type;                 // Sort and index of this decl's type.  Null means no type.
            DeclIndex home_scope;           // Enclosing scope of this declaration.
            BasicSpecifiers basic_spec;     // Basic declaration specifiers.
            Access access;                  // Access right to this function.
        };

        static_assert(offsetof(IntrinsicDecl, identity) == 0, "The name population code expects 'identity' to be at offset zero");

        // An enumerator declaration.
        struct EnumeratorDecl : Tag<DeclSort::Enumerator> {
            Identity<TextOffset> identity;  // // The name and location of this enumerator
            TypeIndex type;                 // Sort and index of this decl's type.  Null means no type.
            ExprIndex initializer;          // Sort and index of this declaration's initializer, if any.  Null means absent.
            BasicSpecifiers basic_spec;     // Basic declaration specifiers.
            Access access;                  // Access right to this enumerator.
        };

        struct ParameterDecl : Tag<DeclSort::Parameter> {
            Identity<TextOffset> identity;  // The name and location of this function parameter
            TypeIndex type;                 // Sort and index of this decl's type.  Null means no type.
            ExprIndex type_constraint;      // Optional type-constraint on the parameter type.
            ExprIndex initializer;          // Default argument. Null means none was provided.
            uint32_t level;                 // The nesting depth of this parameter (template or function).
            uint32_t position;              // The 1-based position of this parameter.
            ParameterSort sort;             // The kind of parameter.
            ReachableProperties properties; // The set of semantic properties reaching to outside importers.
        };

        // A variable declaration, including static data members.
        struct VariableDecl : Tag<DeclSort::Variable> {
            Identity<NameIndex> identity;   // The name and location of this variable
            TypeIndex type;                 // Sort and index of this decl's type.  Null means no type.
            DeclIndex home_scope;           // Enclosing scope of this declaration.
            ExprIndex initializer;          // Sort and index of this declaration's initializer, if any.  Null means absent.
            ExprIndex alignment;            // If non-zero, explicit alignment specification, e.g. alignas(N).
            ObjectTraits obj_spec;          // Object traits associated with this variable.
            BasicSpecifiers basic_spec;     // Basic declaration specifiers.
            Access access;                  // Access right to this variable.
            ReachableProperties properties; // The set of semantic properties reaching to outside importers.
        };

        static_assert(offsetof(VariableDecl, identity) == 0, "The name population code expects 'identity' to be at offset zero");

        // A non-static data member declaration that is not a bitfield.
        struct FieldDecl : Tag<DeclSort::Field> {
            Identity<TextOffset> identity;  // The name and location of this non-static data member
            TypeIndex type;                 // Sort and index of this decl's type.  Null means no type.
            DeclIndex home_scope;           // Enclosing scope of this declaration.
            ExprIndex initializer;          // The field initializer (if any)
            ExprIndex alignment;            // If non-zero, explicit alignment specification, e.g. alignas(N).
            ObjectTraits obj_spec;          // Object traits associated with this field.
            BasicSpecifiers basic_spec;     // Basic declaration specifiers.
            Access access;                  // Access right to this data member.
            ReachableProperties properties; // Set of reachable semantic properties of this declaration
        };

        // A bitfield declaration.
        struct BitfieldDecl : Tag<DeclSort::Bitfield> {
            Identity<TextOffset> identity;  // The name and location of this bitfield
            TypeIndex type;                 // Sort and index of this decl's type.  Null means no type.
            DeclIndex home_scope;           // Enclosing scope of this declaration.
            ExprIndex width;                // Number of bits requested for this bitfield.
            ExprIndex initializer;          // Sort and index of this declaration's initializer, if any.  Null means absent.
            ObjectTraits obj_spec;          // Object traits associated with this field.
            BasicSpecifiers basic_spec;     // Basic declaration specifiers.
            Access access;                  // Access right to this bitfield.
            ReachableProperties properties; // Set of reachable semantic properties of this declaration
        };

        // A declaration for a named scope, e.g. class, namespaces.
        struct ScopeDecl : Tag<DeclSort::Scope> {
            Identity<NameIndex> identity;   // The name and location of this scope (class or namespace)
            TypeIndex type;                 // Sort index of this decl's type.
            TypeIndex base;                 // Base type(s) in class inheritance.
            ScopeIndex initializer;         // Type definition (i.e. initializer) of this type declaration.
            DeclIndex home_scope;           // Enclosing scope of this declaration.
            ExprIndex alignment;            // If non-zero, explicit alignment specification, e.g. alignas(N).
            PackSize pack_size;             // The pack size value applied to all members of this class (#pragma pack(n))
            BasicSpecifiers basic_spec;     // Basic declaration specifiers.
            ScopeTraits scope_spec;         // Property of this scope type declaration.
            Access access;                  // Access right to the name of this user-defined type.
            ReachableProperties properties; // The set of semantic properties reaching to outside importers.
        };

        struct EnumerationDecl : Tag<DeclSort::Enumeration> {
            Identity<TextOffset> identity;  // The name and location of this enumeration
            TypeIndex type;                 // Sort index of this decl's type.
            TypeIndex base;                 // Enumeration base type.
            Sequence<EnumeratorDecl> initializer; // Type definition (i.e. initializer) of this type declaration.
            DeclIndex home_scope;           // Enclosing scope of this declaration.
            ExprIndex alignment;            // If non-zero, explicit alignment specification, e.g. alignas(N).
            BasicSpecifiers basic_spec;     // Basic declaration specifiers.
            Access access;                  // Access right to this enumeration.
            ReachableProperties properties; // The set of semantic properties reaching to outside importers.
        };

        static_assert(offsetof(EnumerationDecl, identity) == 0, "The name population code expects 'identity' to be at offset zero");

        // A general alias declaration for a type.
        // E.g.
        //     using T = int;
        //     namespace N = VeryLongCompanyName::NiftyDivision::AwesomeDepartment;
        // are all (type) alias declarations in the IFC.
        struct AliasDecl : Tag<DeclSort::Alias> {
            Identity<TextOffset> identity;  // The name and location of this alias
            TypeIndex type;                 // The type of this alias (TypeBasis::Typename for conventional C++ type, TypeBasis::Namespace for a namespace, etc.)
            DeclIndex home_scope;           // Enclosing scope of this declaration.
            TypeIndex aliasee;              // The type this declaration is introducing a name for.
            BasicSpecifiers basic_spec;     // Basic declaration specifiers.
            Access access;                  // Access right to the name of this type alias.
        };

        static_assert(offsetof(AliasDecl, identity) == 0, "The name population code expects 'identity' to be at offset zero");

        // A temploid declaration
        struct TemploidDecl : Tag<DeclSort::Temploid>, ParameterizedEntity {
            ChartIndex chart;                   // The enclosing set template parameters of this entity.
            ReachableProperties properties;    // The set of semantic properties reaching to outside importers.
        };

        // The core of a template.
        struct Template {
            Identity<NameIndex> identity;   // What identifies this template
            DeclIndex home_scope;           // Enclosing scope of this declaration.
            ChartIndex chart;               // Template parameter list.
            ParameterizedEntity entity;     // The core parameterized entity.
        };

        static_assert(offsetof(Template, identity) == 0, "The name population code expects 'identity' to be at offset zero");

        // A template declaration.
        struct TemplateDecl : Tag<DeclSort::Template>, Template {
            TypeIndex type;                                     // The type of the parameterized entity.  FIXME: In some sense this is redundant with `entity' but VC's internal representation is currently too irregular.
            BasicSpecifiers basic_spec;                         // Basic declaration specifiers.
            Access access;                                      // Access right to the name of this template declaration.
            ReachableProperties properties;                     // The set of semantic properties reaching to outside importers.
        };

        // A specialization of a template.
        struct PartialSpecializationDecl : Tag<DeclSort::PartialSpecialization>, Template {
            SpecFormIndex specialization_form;                  // The specialization pattern: primary template + argument list.
            BasicSpecifiers basic_spec;                         // Basic declaration specifiers.
            Access access;                                      // Access right to the name of this template specialization.
            ReachableProperties properties;                     // The set of semantic properties reaching to outside importers.
        };

        enum class SpecializationSort : uint8_t {
            Implicit,       // An implicit specialization
            Explicit,       // An explicit specialization (user provided)
            Instantiation,  // An explicit instantiation (user provided)
        };

        struct SpecializationDecl : Tag<DeclSort::Specialization> {
            SpecFormIndex specialization_form; // The specialization pattern: primary template + argument list.
            DeclIndex decl;                    // The entity declared by this specialization.
            SpecializationSort sort;           // The specialization category.
            BasicSpecifiers basic_spec;
            Access access;
            ReachableProperties properties;
        };

        // A concept.
        struct Concept : Tag<DeclSort::Concept> {
            Identity<TextOffset> identity;                      // What identifies this concept.
            DeclIndex home_scope;                               // Enclosing scope of this declaration.
            TypeIndex type;                                     // The type of the parameterized entity.
            ChartIndex chart;                                   // Template parameter list.
            ExprIndex constraint;                               // The associated constraint-expression.
            BasicSpecifiers basic_spec;                         // Basic declaration specifiers.
            Access access;                                      // Access right to the name of this template declaration.
            SentenceIndex head;                                 // The sequence of words making up the declarative part of current instantiation.
            SentenceIndex body;                                 // The sequence of words making up the body of this concept.
        };

        static_assert(offsetof(Template, identity) == 0, "The name population code expects 'identity' to be at offset zero");

        // A non-static member function declaration.
        struct NonStaticMemberFunctionDecl : Tag<DeclSort::Method> {
            Identity<NameIndex> identity;   // What identifies this non-static member function
            TypeIndex type;                 // Sort and index of this decl's type.  Null means no type.
            DeclIndex home_scope;           // Enclosing scope of this declaration.
            ChartIndex chart;               // Function parameter list.
            FunctionTraits traits;          // Function traits ('const' etc.)
            BasicSpecifiers basic_spec;     // Basic declaration specifiers.
            Access access;                  // Access right to this member.
            ReachableProperties properties; // Set of reachable semantic properties of this declaration
        };

        // A constructor declaration.
        struct ConstructorDecl : Tag<DeclSort::Constructor> {
            Identity<TextOffset> identity;      // What identifies this constructor
            TypeIndex type;                     // Type of this constructor declaration.
            DeclIndex home_scope;               // Enclosing scope of this declaration.
            ChartIndex chart;                   // Function parameter list.
            FunctionTraits traits;              // Function traits
            BasicSpecifiers basic_spec;         // Basic declaration specifiers.
            Access access;                      // Access right to this constructor.
            ReachableProperties properties;     // Set of reachable semantic properties of this declaration
        };

        // A constructor declaration.
        struct InheritedConstructorDecl : Tag<DeclSort::InheritedConstructor> {
            Identity<TextOffset> identity;      // What identifies this constructor
            TypeIndex type;                     // Type of this contructor declaration.
            DeclIndex home_scope;               // Enclosing scope of this declaration.
            ChartIndex chart;                   // Function parameter list.
            FunctionTraits traits;              // Function traits
            BasicSpecifiers basic_spec;         // Basic declaration specifiers.
            Access access;                      // Access right to this constructor.
            DeclIndex base_ctor;                // The base class ctor this ctor references.
        };

        // A destructor declaration.
        struct DestructorDecl : Tag<DeclSort::Destructor> {
            Identity<TextOffset> identity;      // What identifies this destructor
            DeclIndex home_scope;               // Enclosing scope of this declaration.
            NoexceptSpecification eh_spec;      // Exception specification.
            FunctionTraits traits;              // Function traits.
            BasicSpecifiers basic_spec;         // Basic declaration specifiers.
            Access access;                      // Access right to this destructor.
            CallingConvention convention;       // Calling convention.
            ReachableProperties properties;     // Set of reachable semantic properties of this declaration
        };

        // A deduction guide for class template.
        // Note: If the deduction guide was paramterized by a template, then this would be
        // the corresponding parameterized decl.  The template declaration itself has no name.
        struct DeductionGuideDecl : Tag<DeclSort::DeductionGuide> {
            Identity<NameIndex> identity;       // associated primary template and location of declaration
            DeclIndex home_scope;               // Enclosing scope of this declaration
            ChartIndex source;                  // Function parameters mentioned in the deduction guide declaration
            ExprIndex target;                   // Pattern for the template-argument list to deduce
            GuideTraits traits;                 // Deduction guide traits.
            BasicSpecifiers basic_spec;         // Basic declaration specifiers.
        };

        struct ReferenceDecl : Tag<DeclSort::Reference> {
            ModuleReference translation_unit;   // The owning TU of this decl.
            DeclIndex local_index;              // The core declaration.
        };

        // A property declaration -- VC extensions.
        struct PropertyDeclaration : Tag<DeclSort::Property> {
            DeclIndex data_member;          // The pseudo data member that represents this property
            TextOffset get_method_name;     // The name of the 'get' method
            TextOffset set_method_name;     // The name of the 'set' method
        };

        struct SegmentDecl : Tag<DeclSort::OutputSegment> {
            TextOffset name;            // offset of name's text in the string table.
            TextOffset class_id;        // Class ID of the segment.
            SegmentTraits seg_spec;     // Attributes collected from #pragmas, for linker use.
            SegmentType type;           // The type of segment.
        };

        struct UsingDeclaration : Tag<DeclSort::UsingDeclaration> {
            Identity<TextOffset> identity;  // What identifies this using declaration
            DeclIndex home_scope;           // Enclosing scope of this declaration.
            DeclIndex resolution;           // Designates the used set of declarations.
            ExprIndex parent;               // If this is a member using declaration then this is the introducing base-class
            TextOffset name;                // If this is a member using declaration then this is the name of member
            BasicSpecifiers basic_spec;     // Basic declaration specifiers.
            Access access;                  // If this is a member using declaration then this is its access
            bool is_hidden;                 // Is this using-declaration hidden?
        };

        struct FriendDeclaration : Tag<DeclSort::Friend> {
            ExprIndex index { }; // The expression representing a reference to the declaration.
                                 // Note: most of the time this is a NamedDeclExpression but it
                                 // can also be a TemplateIdExpression in the case of the friend
                                 // decl being referenced is some specialization of a template.
        };

        struct ExpansionDecl : Tag<DeclSort::Expansion> {
            SourceLocation locus;           // Location of the expansion
            DeclIndex operand;              // The declaration to expand
        };

        struct SyntacticDecl : Tag<DeclSort::SyntaxTree> {
            SyntaxIndex index { };
        };

        // A sequence of zero or more entities
        struct TupleDecl : Tag<DeclSort::Tuple>, Sequence<DeclIndex, HeapSort::Decl> {
            TupleDecl() = default;
            TupleDecl(Index f, Cardinality n) : Sequence{ f, n } { }
        };

        static_assert(offsetof(UsingDeclaration, identity) == 0, "The name population code expects 'identity' to be at offset zero");

        // A sequence of declaration indices.
        struct Scope : Sequence<Declaration> { };

        struct UnilevelChart : Tag<ChartSort::Unilevel>, Sequence<ParameterDecl> {
            ExprIndex requires_clause = { };
        };

        struct MultiChart : Tag<ChartSort::Multilevel>, Sequence<ChartIndex, HeapSort::Chart> {
        };

        // Note: This class captures that commonality. It is parameterized by any sort,
        //       but that parameterization is more for convenience than fundamental.
        //       If the many isomorphic specializations are demonstrated to induce undue
        //       compile-time memory pressure, then this parameterization can be refactored
        //       into the parameterized empty class Tag + the non-parameterized class.
        template<auto s>
        struct LocationAndType : Tag<s> {
            SourceLocation locus { };       // The source location (span) of this node.
            TypeIndex type { };             // The (possibly generalized) type of this node.
        };

        // Like the structure above, but drops the 'type' field in favor of the node deriving the
        // type from its children.  This can be useful in compressing the on-disk representation
        // due to collapsing redundant semantic information.
        template <auto s>
        struct Location : Tag<s> {
            SourceLocation locus { };     // The source location of this node.
        };

        // Note: In IPR, every statement inherits from Expr which also provides a type and location
        // for the deriving Stmt nodes.

        // A sequence of zero or more statements. Used to represent blocks/compound statements.
        struct BlockStatement : Location<StmtSort::Block>, Sequence<StmtIndex, HeapSort::Stmt> { };

        struct TryStatement : Location<StmtSort::Try>, Sequence<StmtIndex, HeapSort::Stmt> {
            StmtIndex handlers{ }; // The handler statement or tuple of handler statements.
        };

        struct ExpressionStatement : Location<StmtSort::Expression> {
            ExprIndex expr { };
        };

        struct IfStatement : Location<StmtSort::If> {
            StmtIndex init { };
            StmtIndex condition { };
            StmtIndex consequence { };
            StmtIndex alternative { };
        };

        struct WhileStatement : Location<StmtSort::While> {
            StmtIndex condition { };
            StmtIndex body { };
        };

        struct DoWhileStatement : Location<StmtSort::DoWhile> {
            ExprIndex condition { };  // Grammatically, this can only ever be an expression.
            StmtIndex body { };
        };

        struct ForStatement : Location<StmtSort::For> {
            StmtIndex init { };
            StmtIndex condition { };
            StmtIndex increment { };
            StmtIndex body { };
        };

        struct BreakStatement : Location<StmtSort::Break> { };

        struct ContinueStatement : Location<StmtSort::Continue> { };

        struct GotoStatement : Location<StmtSort::Goto> {
            ExprIndex target { };
        };

        struct SwitchStatement : Location<StmtSort::Switch> {
            StmtIndex init { };
            ExprIndex control { };
            StmtIndex body { };
        };

        struct LabeledStatement : LocationAndType<StmtSort::Labeled> {
            ExprIndex label { };    // The label is an expression to offer more flexibility in the types of labels allowed.
                                    // Note: a null index here indicates that this is the default case label in a switch
                                    // statement.
            StmtIndex statement{ }; // The statement associated with this label.
        };

        struct DeclStatement : Location<StmtSort::Decl> {
            DeclIndex decl { };
        };

        struct ReturnStatement : LocationAndType<StmtSort::Return> {
            ExprIndex expr { };
            TypeIndex function_type { };
        };

        struct HandlerStatement : Location<StmtSort::Handler> {
            DeclIndex exception { };
            StmtIndex body { };
        };

        struct ExpansionStmt : Location<StmtSort::Expansion> {
            StmtIndex operand;              // The statement to expand
        };

        struct TupleStatement : LocationAndType<StmtSort::Tuple>, Sequence<StmtIndex, HeapSort::Stmt> { };

        // Note: Every expression has a source location span, and a type.

        // Expression trees for integer constants below this threshold are directly represented by their indices.
        inline constexpr auto immediate_upper_bound = uint64_t(1) << index_like::index_precision<ExprSort>;

        struct StringLiteral {
            TextOffset start;               // beginning of the first byte in this string literal
            Cardinality size;               // The number of bytes in the object representation, including terminator(s).
            TextOffset suffix;              // Suffix, if any.
        };

        struct TypeExpression : LocationAndType<ExprSort::Type> {
            TypeIndex denotation { };
        };

        struct StringExpression : LocationAndType<ExprSort::String> {
            StringIndex string { };             // The string literal
        };

        struct FunctionStringExpression : LocationAndType<ExprSort::FunctionString> {
            TextOffset macro { };               // The name of the macro identifier (e.g. __FUNCTION__)
        };

        struct CompoundStringExpression : LocationAndType<ExprSort::CompoundString> {
            TextOffset prefix { };              // The name of the prefix identifier (e.g. __LPREFIX)
            ExprIndex string { };               // The parenthesized string literal
        };

        struct StringSequenceExpression : LocationAndType<ExprSort::StringSequence> {
            ExprIndex strings { };              // The string literals in the sequence
        };

        struct UnresolvedIdExpression : LocationAndType<ExprSort::UnresolvedId> {
            NameIndex name { };
        };

        struct TemplateIdExpression : LocationAndType<ExprSort::TemplateId> {
            ExprIndex primary_template { };
            ExprIndex arguments { };
        };

        struct TemplateReference : LocationAndType<ExprSort::TemplateReference> {
            Identity<NameIndex> member { };     // What identifies this non-static member function
            TypeIndex parent { };               // The enclosing concrete specialization
            ExprIndex template_arguments { };   // Any associated template arguments
        };

        // Use of a declared identifier as an expression.
        struct NamedDeclExpression : LocationAndType<ExprSort::NamedDecl> {
            DeclIndex decl { };                 // Declaration referenced by the name.
        };

        struct LiteralExpr : LocationAndType<ExprSort::Literal> {
            LitIndex value { };
        };

        struct EmptyExpression : LocationAndType<ExprSort::Empty> { };

        struct PathExpression : LocationAndType<ExprSort::Path> {
            ExprIndex scope { };
            ExprIndex member { };
        };

        // Read from a symbolic address.  When the address is an id-expression, the read expression
        // symbolizes an lvalue-to-rvalue conversion.
        struct ReadExpression : LocationAndType<ExprSort::Read> {
            enum class Kind : uint8_t {
                Unknown,            // Unknown
                Indirection,        // Dereference a pointer, e.g. *p
                RemoveReference,    // Convert a reference into an lvalue
                LvalueToRvalue,     // lvalue-to-rvalue conversion
                IntegralConversion, // Integral conversion (should be removed once we purge these from the FE)
                Count
            };

            ExprIndex child { };
            Kind kind { Kind::Unknown };
        };

        struct MonadicTree : LocationAndType<ExprSort::Monad> {
            DeclIndex impl { };     // An associated set of decls with this operator.  This set
                                    // is obtained through the first phase of 2-phase name lookup.
            ExprIndex arg[1] { };
            MonadicOperator assort { };
        };

        struct DyadicTree : LocationAndType<ExprSort::Dyad> {
            DeclIndex impl { };     // An associated set of decls with this operator.  This set
                                    // is obtained through the first phase of 2-phase name lookup.
            ExprIndex arg[2] { };
            DyadicOperator assort { };
        };

        struct TriadicTree : LocationAndType<ExprSort::Triad> {
            DeclIndex impl { };     // An associated set of decls with this operator.  This set
                                    // is obtained through the first phase of 2-phase name lookup.
            ExprIndex arg[3] { };
            TriadicOperator assort { };
        };

        struct HierarchyConversionExpression : LocationAndType<ExprSort::HierarchyConversion> {
            ExprIndex source { };
            TypeIndex target { };
            ExprIndex inheritance_path { };             // Path from source class to destination class. For conversion of the 'this'
                                                        // argument to a virtual call, this is the path to the introducing base class.
            ExprIndex override_inheritance_path { };    // Path from source class to destination class. Usually the same as
                                                        // inheritance_path, but for conversion of the 'this' argument to
                                                        // a virtual call, this is the path to the base class whose member
                                                        // was selected at compile time, which may be an override of the
                                                        // introducing function.
            DyadicOperator assort { };                  // The class hierarchy navigation operation
        };

        struct DestructorCall : LocationAndType<ExprSort::DestructorCall> {
            enum class Kind : uint8_t { Unknown, Destructor, Finalizer };
            ExprIndex       name { };                   // The name of the object being destructor (if present)
            SyntaxIndex     decltype_specifier { };     // The decltype specifier (if present)
            Kind            kind = Kind::Unknown;       // Do we have a destructor or a finalizer?
        };

        // A sequence of zero of more expressions.
        struct TupleExpression : LocationAndType<ExprSort::Tuple>, Sequence<ExprIndex, HeapSort::Expr> { };

        struct PlaceholderExpr : LocationAndType<ExprSort::Placeholder> { };

        struct ExpansionExpr : LocationAndType<ExprSort::Expansion> {
            ExprIndex operand { };                      // The expression to expand
        };

        // A stream of tokens (used for 'complex' default arguments)
        struct TokenExpression : LocationAndType<ExprSort::Tokens> {
            SentenceIndex tokens { };
        };

        struct CallExpression : LocationAndType<ExprSort::Call> {
            ExprIndex function { };             // The function we are calling
            ExprIndex arguments { };            // The arguments to the function call
        };

        struct TemporaryExpression : LocationAndType<ExprSort::Temporary> {
            uint32_t index { };             // The index that uniquely identifiers this temporary object
        };

        struct DynamicDispatch : LocationAndType<ExprSort::DynamicDispatch> {
            ExprIndex postfix_expr { };     // The posfix expression on a call to a virtual function.
        };

        struct VirtualFunctionConversion : LocationAndType<ExprSort::VirtualFunctionConversion> {
            DeclIndex function { };         // The virtual function referenced.
        };

        struct RequiresExpression : LocationAndType<ExprSort::Requires> {
            SyntaxIndex parameters { };  // The (optional) parameters
            SyntaxIndex body { };        // The requirement body
        };

        enum class Associativity : uint8_t {
            Unspecified,
            Left,
            Right
        };

        struct UnaryFoldExpression : LocationAndType<ExprSort::UnaryFoldExpression> {
            ExprIndex expr { };         // The associated expression
            DyadicOperator op { };      // The operator kind
            Associativity assoc = Associativity::Unspecified;
        };

        struct BinaryFoldExpression : LocationAndType<ExprSort::BinaryFoldExpression> {
            ExprIndex left { };         // The left expression
            ExprIndex right { };        // The right expression (if present)
            DyadicOperator op { };      // The operator kind
            Associativity assoc = Associativity::Unspecified;
        };

        struct TypeTraitIntrinsic : LocationAndType<ExprSort::TypeTraitIntrinsic> {
            TypeIndex arguments { };
            Operator intrinsic { };
        };

        struct MemberInitializer : LocationAndType<ExprSort::MemberInitializer> {
            DeclIndex member { };
            TypeIndex base { };
            ExprIndex expression { };
        };

        struct MemberAccess : LocationAndType<ExprSort::MemberAccess> {
            ExprIndex offset { };       // The offset of the member
            TypeIndex parent { };       // The type of the parent
            TextOffset name { };        // The name of the non-static data member
        };

        struct InheritancePath : LocationAndType<ExprSort::InheritancePath> {
            ExprIndex path { };
        };

        struct InitializerList : LocationAndType<ExprSort::InitializerList> {
            ExprIndex elements { };
        };

        struct Initializer : LocationAndType<ExprSort::Initializer> {
            enum class Kind : uint8_t {
                Unknown,
                DirectInitialization,
                CopyInitialization,
            };
            ExprIndex initializer { };          // The initializer itself
            Kind kind = Kind::Unknown;         // What kind of initialization is this?
        };

        // Any form of cast expression
        struct CastExpr : LocationAndType<ExprSort::Cast> {
            ExprIndex source { };               // The expression being cast
            TypeIndex target { };               // Target type of this cast-expression
            DyadicOperator assort { };
        };

        // A condition: currently this just supports a simple condition 'if (e)' but we will need to enhance it to support more
        // complex conditions 'if (T v = e)'
        struct ConditionExpr : LocationAndType<ExprSort::Condition> {
            ExprIndex expression { };
        };

        struct SimpleIdentifier : LocationAndType<ExprSort::SimpleIdentifier> {
            NameIndex name { };
        };

        struct Pointer : Tag<ExprSort::Pointer> {
            SourceLocation locus { };
        };

        struct UnqualifiedIdExpr : LocationAndType<ExprSort::UnqualifiedId> {
            NameIndex name { };                         // The name of the identifier
            ExprIndex symbol { };                       // The bound symbol (if any)
            SourceLocation template_keyword { };        // The source location of the 'template' keyword (if any)
        };

        struct QualifiedNameExpr : LocationAndType<ExprSort::QualifiedName> {
            ExprIndex elements { };                     // The elements that make up this qualified-name
            SourceLocation typename_keyword { };        // The source location of the 'typename' keyword (if any)
        };

        struct DesignatedInitializer : LocationAndType<ExprSort::DesignatedInitializer> {
            TextOffset member { };                      // The identifier being designated
            ExprIndex initializer { };                  // The assign-or-braced-init-expr
        };

        // FIXME: Remove at earliest convenience.
        struct ExpressionList : Tag<ExprSort::ExpressionList> {
            enum class Delimiter : uint8_t { Unknown, Brace, Parenthesis };
            SourceLocation left_delimiter{ };          // The source location of the left delimiter
            SourceLocation right_delimiter{ };         // The source location of the right delimiter
            ExprIndex expressions { };                  // The expressions in the list
            Delimiter delimiter{ };                    // The delimiter
        };

        struct AssignInitializer : Tag<ExprSort::AssignInitializer> {
            SourceLocation assign { };                  // The source location of the '='
            ExprIndex initializer { };                  // The initializer
        };

        // The representation of 'sizeof(type-id)': 'sizeof expression' is handled as a regular unary-expression
        struct SizeofTypeId : LocationAndType<ExprSort::SizeofType> {
            TypeIndex operand { };                      // The type-id argument
        };

        // The representation of 'sizeof(type-id)': 'sizeof expression' is handled as a regular unary-expression
        struct Alignof : LocationAndType<ExprSort::Alignof> {
            TypeIndex type_id { };                      // The type-id argument
        };

        // Label-expression.  Appears in goto-statements.
        struct Label : LocationAndType<ExprSort::Label> {
            ExprIndex designator { };                   // The name of this label.  This is an expression in order
                                                        // to offer flexibility in what the 'identifier' really is (it
                                                        // might be a handle).
        };

        struct Nullptr : LocationAndType<ExprSort::Nullptr> { };

        struct This : LocationAndType<ExprSort::This> { };

        struct PackedTemplateArguments : LocationAndType<ExprSort::PackedTemplateArguments> {
            ExprIndex arguments { };
        };

        struct LambdaExpression : Tag<ExprSort::Lambda> {
            SyntaxIndex introducer { };              // The lambda introducer
            SyntaxIndex template_parameters { };     // The template parameters (if present)
            SyntaxIndex declarator { };              // The lambda declarator (if present)
            SyntaxIndex requires_clause { };         // The requires clause (if present)
            SyntaxIndex body { };                    // The body of the lambda
        };

        struct TypeidExpression : LocationAndType<ExprSort::Typeid> {
            TypeIndex operand { };                      // TypeSort::Syntax => operand is in parse tree form
        };

        struct SyntaxTreeExpr : Tag<ExprSort::SyntaxTree> {
            SyntaxIndex syntax { };                     // The representation of this expression as a parse tree
        };

        struct SubobjectValue : Tag<ExprSort::SubobjectValue> {
            ExprIndex value { };                    // The value of this member object
        };

        struct ProductTypeValue : LocationAndType<ExprSort::ProductTypeValue> {
            TypeIndex structure { };                // The class type which this value is associated
            ExprIndex members { };                  // The members (zero or more) which are initialized
            ExprIndex base_class_values { };        // The values (zero or more) of base class members
        };

        enum class ActiveMemberIndex : uint32_t { };

        struct SumTypeValue : LocationAndType<ExprSort::SumTypeValue> {
            TypeIndex variant { };                  // The union type which this value is associated
            ActiveMemberIndex active_member { };    // The active member index
            SubobjectValue value { };               // The object representing the value of the active member
        };

        struct ArrayValue : LocationAndType<ExprSort::ArrayValue> {
            ExprIndex elements { };                 // The tuple containing the element expressions
            TypeIndex element_type { };             // The type of elements within the array
            // TypeIndex array_type { };               // The type of the array object
        };

        struct ObjectLikeMacro : Tag<MacroSort::ObjectLike> {
            SourceLocation locus { };
            TextOffset name { };            // macro name
            FormIndex replacement_list { }; // The replacement list of the macro.
        };

        struct FunctionLikeMacro : Tag<MacroSort::FunctionLike> {
            SourceLocation locus { };
            TextOffset name { };            // macro name
            FormIndex parameters { };       // The parameters of the macro.
            FormIndex replacement_list { }; // The replacement list of the macro.
            uint32_t arity : 31 { };        // Arity for function-like macros.
            uint32_t variadic : 1 { };      // True if this macro is variadic.
        };

        struct PragmaWarningRegion {
            SourceLocation start_locus { };
            SourceLocation end_locus { };
            SuppressedWarning suppressed_warning { };
        };

        // Note: this class is not meant to be used to create objects -- it is just a traits class.
        template<typename T, LiteralSort s>
        struct constant_traits : Tag<s> {
            using ValueType = T;
        };

// Note: VC and the build settings do not want to see idiomatic C++ by default.
#pragma warning(push)
#pragma warning(disable: 4624)
        struct IntegerLiteral : constant_traits<uint64_t, LiteralSort::Integer> { };

#pragma pack(4)
        struct LiteralReal {
            double value;
            uint16_t size;
        };
#pragma pack()

        struct FloatingPointLiteral : constant_traits<LiteralReal, LiteralSort::FloatingPoint> { };
#pragma warning(pop)

        struct PragmaPushState {
            DeclIndex text_segment;                         // Symbol for #pragma code_seg
            DeclIndex const_segment;                        // Symbol for #pragma const_seg
            DeclIndex data_segment;                         // Symbol for #pragma data_seg
            DeclIndex bss_segment;                          // Symbol for #pragma data_seg
            uint32_t pack_size : 8;                         // value of #pragma pack
            uint32_t fp_control : 8;                        // value of #pragma float_control
            uint32_t exec_charset : 8;                      // value of #pragma execution_character_set
            uint32_t vtor_disp : 8;                         // value of #pragma vtordisp
            uint32_t std_for_scope : 1;                     // value of #pragma conform(forScope)
            uint32_t pure_cil : 1;                          // value of #pragma managed
            uint32_t strict_gs_check : 1;                   // value of #pragma strict_gs_check
        };

        struct BasicAttr : Tag<AttrSort::Basic> {
            Word word { }; // the token in [[ token ]]
        };

        struct ScopedAttr : Tag<AttrSort::Scoped> {
            Word scope { };  // e.g. 'foo' in [[ foo::attr ]]
            Word member { }; // e.g. 'attr' in [[ foo::attr ]]
        };

        struct LabeledAttr : Tag<AttrSort::Labeled> {
            Word label { };     // 'key' in [[ key : value ]]
            AttrIndex attribute { }; // 'value' in [[ key : value ]]
        };

        struct CalledAttr : Tag<AttrSort::Called> {
            AttrIndex function { };  // Function postfix 'opt' in [[ opt(args) ]]
            AttrIndex arguments { }; // Function argument expression 'args' in [[ opt(args) ]]
        };

        struct ExpandedAttr : Tag<AttrSort::Expanded> {
            AttrIndex operand { }; // Expansion expression 'fun(arg)' in [[ fun(arg) ... ]]
        };

        struct FactoredAttr : Tag<AttrSort::Factored> {
            Word factor { }; // The scope 'msvc' factored out of the attr in [[ using msvc: opt(args), dbg(1) ]]
            AttrIndex terms { };  // The attribute(s) using the factored out scope.
        };

        struct ElaboratedAttr : Tag<AttrSort::Elaborated> {
            ExprIndex expr { }; // A generalized expression in an attribute.
        };

        // Sequence of one or more attributes.
        struct TupleAttr : Tag<AttrSort::Tuple>, Sequence<AttrIndex, HeapSort::Attr> { };

        namespace Microsoft
        {
            enum class PragmaCommentSort : uint8_t {
                Unknown,
                Compiler,
                Lib,
                Exestr,
                User,
                Nolib,
                Linker,
            };

            // Models #pragma comment(category, "text")
            struct PragmaComment : Tag<PragmaSort::VendorExtension> {
                TextOffset comment_text = { };
                PragmaCommentSort sort = PragmaCommentSort::Unknown;
            };
        } // namespace Microsoft
    } // namespace Symbolic

    namespace Symbolic::Preprocessing {
        struct IdentifierForm : Tag<FormSort::Identifier>
        {
            SourceLocation locus = {};
            TextOffset spelling = {};
        };

        struct NumberForm : Tag<FormSort::Number>
        {
            SourceLocation locus = {};
            TextOffset spelling = {};
        };

        struct CharacterForm : Tag<FormSort::Character>
        {
            SourceLocation locus = {};
            TextOffset spelling = {};
        };

        struct StringForm : Tag<FormSort::String>
        {
            SourceLocation locus = {};
            TextOffset spelling = {};
        };

        struct OperatorForm : Tag<FormSort::Operator>
        {
            SourceLocation locus = {};
            TextOffset spelling = {};
            PPOperator value = {};
        };

        struct KeywordForm : Tag<FormSort::Keyword>
        {
			SourceLocation locus;
			TextOffset spelling;
		};

        struct WhitespaceForm : Tag<FormSort::Whitespace>
        {
            SourceLocation locus = {};
        };

        struct ParameterForm : Tag<FormSort::Parameter>
        {
            SourceLocation locus = {};
            TextOffset spelling = {};
        };

        struct StringizeForm : Tag<FormSort::Stringize>
        {
            SourceLocation locus = {};
            FormIndex operand = {};
        };

        struct CatenateForm : Tag<FormSort::Catenate>
        {
            SourceLocation locus = {};
            FormIndex first = {};
            FormIndex second = {};
        };

        struct PragmaForm : Tag<FormSort::Pragma>
        {
            SourceLocation locus = {};
            FormIndex operand = {};
        };

        struct HeaderForm : Tag<FormSort::Header>
        {
            SourceLocation locus = {};
            TextOffset spelling = {};
        };

        struct ParenthesizedForm : Tag<FormSort::Parenthesized>
        {
            SourceLocation locus = {};
            FormIndex operand = {};
        };

        struct JunkForm : Tag<FormSort::Junk>
        {
            SourceLocation locus = {};
            TextOffset spelling = {};
        };

        struct Tuple : Tag<FormSort::Tuple>, Sequence<FormIndex, HeapSort::Form>
        {
            Tuple() = default;
            Tuple(Index f, Cardinality n) : Sequence{ f, n } { }
        };
    }

    enum class TraitSort : uint8_t {
        MappingExpr,             // function definition associated with a declaration
        AliasTemplate,           // extra information required for alias templates
        Friends,                 // sequence of friends
        Specializations,         // sequence of specializations
        Requires,                // information for requires clause
        Attributes,              // attributes associated with a declaration
        Deprecated,              // deprecation message.
        DeductionGuides,         // Declared deduction guides associated with a class
        Count,
    };

    enum class MsvcTraitSort : uint8_t {
        Uuid,                   // uuid associated with a declaration
        Segment,                // segment associated with a declaration
        SpecializationEncoding, // template specialization encoding associated with a specialization
        SalAnnotation,          // sal annotation text.
        FunctionParameters,     // a parameter lists for a function (why not encompassed by MappingExpr???)
        InitializerLocus,       // the source location of declaration initializer
        TemplateTemplateParameters, // Deprecated: to be removed in a future update.
        CodegenExpression,      // expression used for codegen
        Vendor,                 // extended vendor traits
        DeclAttributes,         // C++ attributes associated with a declaration
        StmtAttributes,         // C++ attributes associated with a statement
        BlockLocus,             // the source location of blocks
        CodegenMappingExpr,     // definitions of inline functions intended for code generation
        DynamicInitVariable,    // A mapping from a real variable to the variable created as part of a dynamic initialization implementation.
        LabelKey,               // A unique value attached to label expressions.  Used for code generation.
        CodegenSwitchType,      // A mapping between a switch statement and its 'control' expression type.
        CodegenDoWhileStmt,     // Captures extra statements associated with a do-while statement condition.
        LexicalScopeIndex,      // A decl has an association with a specific local lexical scope index which can impact name mangling.
        Count,
    };

    template <typename T>
    concept TraitIndexType = std::same_as<T, DeclIndex>
                                || std::same_as<T, SyntaxIndex>
                                || std::same_as<T, ExprIndex>
                                || std::same_as<T, StmtIndex>;

    template <typename T>
    concept AnyTraitSort = std::same_as<T, TraitSort> || std::same_as<T, MsvcTraitSort>;

    // Declarations have various extension traits, such as deprecation message, code segment allocation, etc.
    // This is the data type used as entry into the decl-trait association table.
    template<TraitIndexType T, typename U>
    struct AssociatedTrait {
        using KeyType = T;
        using ValueType = U;
        T entity;   // The index of entity that is being associated with a trait
        U trait;    // The index of the associated trait
    };

    template <AnyTraitSort auto Tag>
    struct TraitTag : index_like::SortTag<Tag> { 
        static constexpr auto partition_tag = Tag;
    };

    template <typename T>
    concept AnyTrait =
        std::derived_from<T, AssociatedTrait<typename T::KeyType, typename T::ValueType>>
           && std::derived_from<T, TraitTag<T::partition_tag>>;

    // Ordering used for traits
    struct TraitOrdering
    {
        template <AnyTrait T>
        bool operator()(T& x, T& y) const { return x.entity < y.entity; }

        template <AnyTrait T>
        bool operator()(typename T::KeyType x, T& y) const { return x < y.entity; }

        template <AnyTrait T>
        bool operator()(T& x, typename T::KeyType y) const { return x.entity < y; }
    };

    namespace Symbolic::Trait
    {
        struct MappingExpr     : AssociatedTrait<DeclIndex, MappingDefinition>,     TraitTag<TraitSort::MappingExpr> { };
        struct AliasTemplate   : AssociatedTrait<DeclIndex, SyntaxIndex>,           TraitTag<TraitSort::AliasTemplate> { };
        struct Friends         : AssociatedTrait<DeclIndex, Sequence<Declaration>>, TraitTag<TraitSort::Friends> { };
        struct Specializations : AssociatedTrait<DeclIndex, Sequence<Declaration>>, TraitTag<TraitSort::Specializations> { };
        struct Requires        : AssociatedTrait<DeclIndex, SyntaxIndex>,           TraitTag<TraitSort::Requires> { };
        struct Attributes      : AssociatedTrait<SyntaxIndex, SyntaxIndex>,         TraitTag<TraitSort::Attributes> { };
        struct Deprecated      : AssociatedTrait<DeclIndex, TextOffset>,            TraitTag<TraitSort::Deprecated> { };
        struct DeductionGuides : AssociatedTrait<DeclIndex, DeclIndex>,             TraitTag<TraitSort::DeductionGuides> { };
    }

    // Msvc specific traits. Should they be in their own namespace, like Symbolic::MsvcTrait?
    namespace Symbolic::Trait
    {
        using LocusSpan = msvc::pair<SourceLocation, SourceLocation>;
        enum class MsvcLabelKey : std::uint32_t { };
        enum class MsvcLexicalScopeIndex : std::uint32_t { };

        template <typename I>
        using AttributeAssociation = AssociatedTrait<I, AttrIndex>;

        struct MsvcUuid                   : AssociatedTrait<DeclIndex, StringIndex>,            TraitTag<MsvcTraitSort::Uuid> { };
        struct MsvcSegment                : AssociatedTrait<DeclIndex, DeclIndex>,              TraitTag<MsvcTraitSort::Segment> { };
        struct MsvcSpecializationEncoding : AssociatedTrait<DeclIndex, TextOffset>,             TraitTag<MsvcTraitSort::SpecializationEncoding> { };
        struct MsvcSalAnnotation          : AssociatedTrait<DeclIndex, TextOffset>,             TraitTag<MsvcTraitSort::SalAnnotation> { };
        struct MsvcFunctionParameters     : AssociatedTrait<DeclIndex, ChartIndex>,             TraitTag<MsvcTraitSort::FunctionParameters> { };
        struct MsvcInitializerLocus       : AssociatedTrait<DeclIndex, SourceLocation>,         TraitTag<MsvcTraitSort::InitializerLocus> { };
        struct MsvcCodegenExpression      : AssociatedTrait<ExprIndex, ExprIndex>,              TraitTag<MsvcTraitSort::CodegenExpression> { };
        struct DeclAttributes             : AttributeAssociation<DeclIndex>,                    TraitTag<MsvcTraitSort::DeclAttributes> { };
        struct StmtAttributes             : AttributeAssociation<StmtIndex>,                    TraitTag<MsvcTraitSort::StmtAttributes> { };
        struct MsvcVendor                 : AssociatedTrait<DeclIndex, VendorTraits>,           TraitTag<MsvcTraitSort::Vendor> { };
        struct MsvcBlockLocus             : AssociatedTrait<StmtIndex, LocusSpan>,              TraitTag<MsvcTraitSort::BlockLocus> { };
        struct MsvcCodegenMappingExpr     : AssociatedTrait<DeclIndex, MappingDefinition>,      TraitTag<MsvcTraitSort::CodegenMappingExpr> { };
        struct MsvcDynamicInitVariable    : AssociatedTrait<DeclIndex, DeclIndex>,              TraitTag<MsvcTraitSort::DynamicInitVariable> { };
        struct MsvcLabelKeys              : AssociatedTrait<ExprIndex, MsvcLabelKey>,           TraitTag<MsvcTraitSort::LabelKey> { };
        struct MsvcCodegenSwitchType      : AssociatedTrait<StmtIndex, TypeIndex>,              TraitTag<MsvcTraitSort::CodegenSwitchType> { };
        struct MsvcCodegenDoWhileStmt     : AssociatedTrait<StmtIndex, StmtIndex>,              TraitTag<MsvcTraitSort::CodegenDoWhileStmt> { };
        struct MsvcLexicalScopeIndices    : AssociatedTrait<DeclIndex, MsvcLexicalScopeIndex>,  TraitTag<MsvcTraitSort::LexicalScopeIndex> { };
    }

    // Partition summaries for the table of contents.
    struct TableOfContents {
        PartitionSummaryData command_line;                          // Command line information
        PartitionSummaryData exported_modules;                      // Exported modules.
        PartitionSummaryData imported_modules;                      // Set of modules explicitly imported by this interface file.
        PartitionSummaryData u64s;                                  // 64-bit unsigned integer constants.
        PartitionSummaryData fps;                                   // Floating-point constants.
        PartitionSummaryData string_literals;                       // String literals.
        PartitionSummaryData states;                                // States of #pragma push.
        PartitionSummaryData lines;                                 // Source line descriptors.
        PartitionSummaryData words;                                 // Token table.
        PartitionSummaryData sentences;                             // Token stream table.
        PartitionSummaryData scopes;                                // scope table
        PartitionSummaryData entities;                              // All the indices for the entities associated with a scope
        PartitionSummaryData spec_forms;                            // Specialization forms.
        PartitionSummaryData names[count<NameSort> - 1];            // Table of all sorts of names, except plain identifiers.
        PartitionSummaryData decls[count<DeclSort>];                // Table of declaration partitions.
        PartitionSummaryData types[count<TypeSort>];                // Table of type partitions.
        PartitionSummaryData stmts[count<StmtSort>];                // Table of statement partitions
        PartitionSummaryData exprs[count<ExprSort>];                // Table of expression partitions.
        PartitionSummaryData elements[count<SyntaxSort>];           // Table of syntax trees partitions.
        PartitionSummaryData forms[count<FormSort>];                // Table of form partitions.
        PartitionSummaryData traits[count<TraitSort>];              // Table of traits
        PartitionSummaryData msvc_traits[count<MsvcTraitSort>];     // Table of msvc specific traits
        PartitionSummaryData charts;                                // Sequence of unilevel charts.
        PartitionSummaryData multi_charts;                          // Sequence of multi-level charts.
        PartitionSummaryData heaps[count<HeapSort>];                // Set of various abstract reference sequences.
        PartitionSummaryData suppressed_warnings;                   // Association map of suppressed warnings.
        PartitionSummaryData macros[count<MacroSort>];              // Table of exported macros.
        PartitionSummaryData pragma_directives[count<PragmaSort>];  // Table of pragma directives.
        PartitionSummaryData attrs[count<AttrSort>];                // Table of all attributes.
        PartitionSummaryData implementation_pragmas;                // Sequence of PragmaIndex from the TU which can influence semantics of the current TU.

        // Facilities for iterating over the ToC as a sequence of partition summaries.
        // Note: the use of reinterpret_cast below is avoidable. One way uses
        // two static_cast; the other uses no casts but field names.  The whole point
        // of having this facility is separate field names from the abstract structure,
        // so that fields order does not matter.  The two static_casts would be
        // just being overly pedantic.  This construct is well understood and welldefined.
        // Bug: lot of comments for a one-liner.
        auto begin() { return reinterpret_cast<PartitionSummaryData*>(this); }
        auto end() { return begin() + sizeof(*this) / sizeof(PartitionSummaryData); }

        auto begin() const { return reinterpret_cast<const PartitionSummaryData*>(this); }
        auto end() const { return begin() + sizeof(*this) / sizeof(PartitionSummaryData); }

        PartitionSummaryData& operator[](StringSort s)
        {
            RASSERT(s >= StringSort::Ordinary && s < StringSort::Count);
            return string_literals;
        }

        const PartitionSummaryData& operator[](StringSort s) const
        {
            RASSERT(s >= StringSort::Ordinary && s < StringSort::Count);
            return string_literals;
        }

        PartitionSummaryData& operator[](NameSort s)
        {
            DASSERT(s > NameSort::Identifier && s < NameSort::Count);
            return names[bits::rep(s) - 1];
        }

        const PartitionSummaryData& operator[](NameSort s) const
        {
            DASSERT(s > NameSort::Identifier && s < NameSort::Count);
            return names[bits::rep(s) - 1];
        }

        PartitionSummaryData& operator[](ChartSort s)
        {
            if (s == ChartSort::Unilevel)
            {
                return charts;
            }
            else
            {
                DASSERT(s == ChartSort::Multilevel);
                return multi_charts;
            }
        }

        const PartitionSummaryData& operator[](ChartSort s) const
        {
            if (s == ChartSort::Unilevel)
            {
                return charts;
            }
            else
            {
                DASSERT(s == ChartSort::Multilevel);
                return multi_charts;
            }
        }

        template<typename S, typename T>
        static auto& partition(T& table, S s)
        {
            DASSERT(s < S::Count);
            return table[bits::rep(s)];
        }

        PartitionSummaryData& operator[](DeclSort s) { return partition(decls, s); }
        const PartitionSummaryData& operator[](DeclSort s) const { return partition(decls, s); }

        PartitionSummaryData& operator[](TypeSort s) { return partition(types, s); }
        const PartitionSummaryData& operator[](TypeSort s) const { return partition(types, s); }

        PartitionSummaryData& operator[](StmtSort s) { return partition(stmts, s); }
        const PartitionSummaryData& operator[](StmtSort s) const { return partition(stmts, s); }

        PartitionSummaryData& operator[](ExprSort s) { return partition(exprs, s); }
        const PartitionSummaryData& operator[](ExprSort s) const { return partition(exprs, s); }

        PartitionSummaryData& operator[](SyntaxSort s) { return partition(elements, s); }
        const PartitionSummaryData& operator[](SyntaxSort s) const { return partition(elements, s); }

        PartitionSummaryData& operator[](MacroSort s) { return partition(macros, s); }
        const PartitionSummaryData& operator[](MacroSort s) const { return partition(macros, s); }

        PartitionSummaryData& operator[](PragmaSort s) { return partition(pragma_directives, s); }
        const PartitionSummaryData& operator[](PragmaSort s) const { return partition(pragma_directives, s); }

        PartitionSummaryData& operator[](AttrSort s) { return partition(attrs, s); }
        const PartitionSummaryData& operator[](AttrSort s) const { return partition(attrs, s); }

        PartitionSummaryData& operator[](HeapSort s) { return partition(heaps, s); }
        const PartitionSummaryData& operator[](HeapSort s) const { return partition(heaps, s); }

        PartitionSummaryData& operator[](FormSort s) { return partition(forms, s); }
        const PartitionSummaryData& operator[](FormSort s) const { return partition(forms, s); }

        PartitionSummaryData& operator[](TraitSort s) { return partition(traits, s); }
        const PartitionSummaryData& operator[](TraitSort s) const { return partition(traits, s); }

        PartitionSummaryData& operator[](MsvcTraitSort s) { return partition(msvc_traits, s); }
        const PartitionSummaryData& operator[](MsvcTraitSort s) const { return partition(msvc_traits, s); }

        template<typename S>
        auto at(S s) const
        {
            return (*this)[s];
        }

        template<typename Index>
        ByteOffset offset(Index idx) const
        {
            return at(idx.sort()).tell(idx.index());
        }

        ByteOffset operator()(LineIndex offset) const
        {
            return lines.tell(offset);
        }
    };

    // -- exception type in case of an invalid partition name
    struct InvalidPartitionName {
        std::string_view name;
    };

    // Retrieve a partition summary based on the partition's name.
    PartitionSummaryData& summary_by_partition_name(TableOfContents&, std::string_view);

    // Retrieve the partition name for an abstract reference given by its sort.
    const char* sort_name(DeclSort);
    const char* sort_name(TypeSort);
    const char* sort_name(StmtSort);
    const char* sort_name(ExprSort);
    const char* sort_name(NameSort);
    const char* sort_name(SyntaxSort);
    const char* sort_name(ChartSort);
    const char* sort_name(MacroSort);
    const char* sort_name(PragmaSort);
    const char* sort_name(AttrSort);
    const char* sort_name(HeapSort);
    const char* sort_name(FormSort);
    const char* sort_name(TraitSort);
    const char* sort_name(MsvcTraitSort);

} // namespace Module

#endif      // IFC_ABSTRACT_SGRAPH