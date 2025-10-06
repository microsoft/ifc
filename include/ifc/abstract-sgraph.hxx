// Copyright Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// This file contains a minimal set of data structures supporting the Abstract Semantics Graph of declarations,
// expressions, and other attendant artifacts involved in the processing of a C++ translation unit (import, export)
// into an IFC file.  An Abstract Semantics Graph is the result of performing semantics analysis (e.g. type-checking)
// on a collection of Abstract Syntax Trees, usually coming from a parsing translation unit source file.
// It is a graph because, well, C++'s semantics has recursion almost everywhere at its meta description level,
// especially in declarations and types. The intent is that these data structures can be shared with external
// module-support tooling.  Consequently, they must be independent of c1xx internal data structures, while faithful
// to the core C++ semantics.  The IFC specification is available at
//          https://github.com/microsoft/ifc-spec
// Changes to this file should reflect implementations of requirements in the IFC spec, and nothing more.

#ifndef IFC_ABSTRACT_SGRAPH
#define IFC_ABSTRACT_SGRAPH

#include <concepts>
#include <string_view>
#include <type_traits>

#include <ifc/underlying.hxx>
#include <ifc/basic-types.hxx>
#include "ifc/index-utils.hxx"
#include "ifc/file.hxx"
#include "ifc/source-word.hxx"
#include "ifc/operators.hxx"
#include "ifc/pp-forms.hxx"

namespace ifc {
    using index_like::Index;

    // This describes the basic alignment that all structures in the IFC must adhere to.
    inline constexpr int partition_alignment = 4;

    // For every sort-like enumeration, return the maximum value
    // as indicated by its Count enumerator.
    template<typename S>
    constexpr auto count = std::underlying_type_t<S>(S::Count);

    // Source line info index into the global line table.
    enum class LineIndex : std::uint32_t {};

    // Index into a word stream.
    enum class WordIndex : std::uint32_t {};

    // Index into the token stream table.
    enum class SentenceIndex : std::uint32_t {};

    // Index into the specialization form table.
    enum class SpecFormIndex : std::uint32_t {};

    // The variety of string literals
    enum class StringSort : std::uint8_t {
        Ordinary, // Ordinary string literal -- no prefix.
        UTF8,     // UTF-8 narrow string literal -- only u8 prefix.
        UTF16,    // A char16_t string literal -- only u prefix.
        UTF32,    // A char32_t string literal -- only U prefix.
        Wide,     // A wide string literal -- only L prefix.
        Count,
    };
    
    // Type of A=abstract references to string literal structures.
    struct StringIndex : index_like::Over<StringSort> {
        using Over<StringSort>::Over;
    };

    // The sort of module ownership or entities.
    enum class Ownership : std::uint8_t {
        Unknown,  // No particular information is known.
        Imported, // The reference is to an imported module.
        Exported, // The reference is to an exported module.
        Merged,   // The reference is to a module that will be merged into a larger entity.
    };

    // Set of declarative properties reachable from any program points
    // outside from a module boundaries.
    enum class ReachableProperties : std::uint8_t {
        Nothing          = 0,      // nothing beyond name, type, scope.
        Initializer      = 1 << 0, // IPR-initializer exported.
        DefaultArguments = 1 << 1, // function or template default arguments exported
        Attributes       = 1 << 2, // standard attributes exported.
        All              = Initializer | DefaultArguments | Attributes // Everything.
    };

    // Standard C++ access specifiers.
    enum class Access : std::uint8_t {
        None,      // No access specified.
        Private,   // "private"
        Protected, // "protected"
        Public,    // "public"
        Count
    };

    // Common declaration specifiers.  By default, we assume the language linkage to C++.
    enum class BasicSpecifiers : std::uint8_t {
        Cxx                    = 0,      // C++ language linkage
        C                      = 1 << 0, // C language linkage
        Internal               = 1 << 1, // Exported entities should have external linkage, not sure we need this.
        Vague                  = 1 << 2, // Vague linkage, e.g. COMDAT, still external
        External               = 1 << 3, // External linkage.
        Deprecated             = 1 << 4, // [[deprecated("foo")]].  FIXME: See bug https://github.com/microsoft/ifc-spec/issues/113
        InitializedInClass     = 1 << 5, // Is this entity defined in a class or does it have an in-class initializer?
        NonExported            = 1 << 6, // This is an entity that was not explicitly exported from the module
        IsMemberOfGlobalModule = 1 << 7  // This entity is a member of the global module.
                                         // FIXME: See bug https://github.com/microsoft/ifc-spec/issues/114
    };

    // The various calling conventions currently supported by VC.
    // FIXME: See bug https://github.com/microsoft/ifc-spec/issues/115
    enum class CallingConvention : std::uint8_t {
        Cdecl,  // "__cdecl"
        Fast,   // "__fastcall"
        Std,    // "__stdcall"
        This,   // "__thiscall"
        Clr,    // "__clrcall"
        Vector, // "__vectorcall"
        Eabi,   // "__eabi"
        Count   // Just for counting.
    };

    // Modifiers of (member) function types.
    enum class FunctionTypeTraits : std::uint8_t {
        None     = 0x00, // Just a regular function parameter type list
        Const    = 0x01, // 'void (int) const'
        Volatile = 0x02, // 'void (int) volatile'
        Lvalue   = 0x04, // 'void (int) &'
        Rvalue   = 0x08, // 'void (int) &&'
    };

    // General description of exception-specification.
    // FIXME:  See https://github.com/microsoft/ifc-spec/issues/116
    enum class ExceptionSpecification : std::uint8_t {
        None,         // None specified, not the same as no-exception
        NonNoexcept,  // "noexcept(false)" specification
        Noexcept,     // "noexcept" or "noexcept(true)" specification.
        Conditional,  // "noexcept(unevaluated-expression)" specification.
        Empty,        // "throw()"-style specification
        ExplicitList, // Explicitly list a sequence of types.
        Count
    };

    // Noexcept semantics resolution.
    // FIXME: See https://github.com/microsoft/ifc-spec/issues/116
    enum class NoexceptSort : std::uint8_t {
        None,                  // No specifier
        False,                 // "noexcept(false)" specifier.
        True,                  // "noexcept(true)" specifier.
        Expression,            // "noexcept(expression)" specifier
        InferredSpecialMember, // an inferred noexcept specifier for a special member function that is dependent on
                               // whether associated functions the member will invoke from bases and members are
                               // noexcept
        Unenforced, // noexcept for the purposes of the type system, but not enforced by termination at runtime
        // V2 TODO: Add ImplicitFalse and ImplicitTrue.
        Count
    };

    // Semantics traits of scope types (classes, namespaces, etc).
    enum class ScopeTraits : std::uint8_t {
        None                = 0,
        Unnamed             = 1 << 0, // unnamed namespace, or unnamed class types.
        Inline              = 1 << 1, // inline namespace.
        InitializerExported = 1 << 2, // This object has its initializer exported.
                                      // FIXME: See bug https://github.com/microsoft/ifc-spec/issues/117
        ClosureType         = 1 << 3, // lambda-like scope
        Final               = 1 << 4, // a derived class marked as 'final'
        Vendor              = 1 << 7, // The scope has extended vendor specific traits
    };

    // Variable and object trait specifiers.
    enum class ObjectTraits : std::uint8_t {
        None                = 0,
        Constexpr           = 1 << 0, // Constexpr object.
        Mutable             = 1 << 1, // Mutable object.
        ThreadLocal         = 1 << 2, // thread_local storage.
        Inline              = 1 << 3, // An 'inline' object (this is distinct from '__declspec(selectany)')
        InitializerExported = 1 << 4, // This object has its initializer exported.
                                      // FIXME: See bug https://github.com/microsoft/ifc-spec/issues/118
        NoUniqueAddress     = 1 << 5, // The '[[no_unique_address]]' attribute was applied to this data member.
                                      // FIXME: See bug https://github.com/microsoft/ifc-spec/issues/119
        Vendor              = 1 << 7, // The object has extended vendor specific traits
    };

    // Type for #pragma pack values.
    enum class PackSize : std::uint16_t {};

    // Semantic traits of functions.
    enum class FunctionTraits : std::uint16_t {
        None                    = 0,
        Inline                  = 1 << 0,   // inline function
        Constexpr               = 1 << 1,   // constexpr function
        Explicit                = 1 << 2,   // For conversion functions.
        Virtual                 = 1 << 3,   // virtual function
        NoReturn                = 1 << 4,   // The 'noreturn' attribute.
                                            // FIXME: See bug https://github.com/microsoft/ifc-spec/issues/120
        PureVirtual             = 1 << 5,   // A pure virtual function ('= 0')
        HiddenFriend            = 1 << 6,   // A hidden friend function
        Defaulted               = 1 << 7,   // A '= default' function.
                                            // FIXME: See bug https://github.com/microsoft/ifc-spec/issues/121
        Deleted                 = 1 << 8,   // A '= delete' function.
                                            // FIXME: See bug https://github.com/microsoft/ifc-spec/issues/121
        Constrained             = 1 << 9,   // For functions which have constraint expressions.
        Immediate               = 1 << 10,  // Immediate function
        Final                   = 1 << 11,  // A function marked as 'final'
        Override                = 1 << 12,  // A function marked as 'override'
        ExplicitObjectParameter = 1 << 13,  // A function with an explicit object parameter
        Vendor                  = 1 << 15,  // The function has extended vendor specific traits
    };

    // Semantic traits of deduction guides.
    enum class GuideTraits : std::uint8_t {
        None     = 0,      // nothing
        Explicit = 1 << 0, // the deduction guide is declared 'explicit'.
    };

    // MSVC-specific declspec attributes.
    // FIXME: Sequester in an MSVC-specific file, and represent as a form of standard attributes.
    enum class VendorTraits : std::uint32_t {
        None                    = 0,
        ForceInline             = 1 << 0,  // __forceinline function
        Naked                   = 1 << 1,  // __declspec(naked)
        NoAlias                 = 1 << 2,  // __declspec(noalias)
        NoInline                = 1 << 3,  // __declspec(noinline)
        Restrict                = 1 << 4,  // __declspec(restrict)
        SafeBuffers             = 1 << 5,  // __declspec(safebuffers)
        DllExport               = 1 << 6,  // __declspec(dllexport)
        DllImport               = 1 << 7,  // __declspec(dllimport)
        CodeSegment             = 1 << 8,  // __declspec(code_seg("segment"))
        NoVtable                = 1 << 9,  // __declspec(novtable) for a class type.
        IntrinsicType           = 1 << 10, // __declspec(intrin_type)
        EmptyBases              = 1 << 11, // __declspec(empty_bases)
        Process                 = 1 << 12, // __declspec(process)
        Allocate                = 1 << 13, // __declspec(allocate("segment"))
        SelectAny               = 1 << 14, // __declspec(selectany)
        Comdat                  = 1 << 15,
        Uuid                    = 1 << 16, // __declspec(uuid(....))
        NoCtorDisplacement      = 1 << 17, // #pragma vtordisp(0)
        DefaultCtorDisplacement = 1 << 18, // #pragma vtordisp(1)
        FullCtorDisplacement    = 1 << 19, // #pragma vtordisp(2)
        NoSanitizeAddress       = 1 << 20, // __declspec(no_sanitize_address)
        NoUniqueAddress         = 1 << 21, // '[[msvc::no_unique_address]]'
        NoInitAll               = 1 << 22, // __declspec(no_init_all)
        DynamicInitialization   = 1 << 23, // Indicates that this entity is used for implementing
                                           // aspects of dynamic initialization.
        LexicalScopeIndex       = 1 << 24, // Indicates this entity has a local lexical scope index associated with it.
        ResumableFunction       = 1 << 25, // Indicates this function was a transformed coroutine function.
        PersistentTemporary     = 1 << 26, // a c1xx-ism which will create long-lived temporary symbols when expressions
                                           // need their result to live beyond the full expression, e.g. lifetime
                                           // extended temporaries.
        IneligibleForNRVO       = 1 << 27, // a c1xx-ism in which the front-end conveys to the back-end that a particular
                                           // function cannot utilize NRVO on this function.  This is important due to the
                                           // MSVC C++ calling convention which passes UDTs on the stack as a hidden parameter
                                           // to functions returning that type.
        MultiBytePTMRep         = 1 << 28, // a c1xx-ism which indicates that the pointer-to-member representation for a class
                                           // type is a generalized multi-byte representation for ABI purposes.
    };

    // FIXME: Move to an MSVC-specific file. Attributes of segments.
    enum class SegmentTraits : std::uint32_t {};

    // FIXME: Move to an MSVC-specific file.
    enum class SegmentType : std::uint8_t {};

    // Account for all kinds of valid C++ names.
    enum class NameSort : std::uint8_t {
        Identifier,     // Normal alphabetic identifiers.
        Operator,       // Operator names.
        Conversion,     // Conversion function name.
        Literal,        // Literal operator name.
        Template,       // A nested-name assumed to designate a template.
        Specialization, // A template-id name.
        SourceFile,     // A source file name
        Guide,          // A deduction guide name
        Count
    };

    // Type of abstract references to name structures.
    struct NameIndex : index_like::Over<NameSort> {
        using Over<NameSort>::Over;
    };

    // Sort of template parameter set (also called chart.)
    enum class ChartSort : std::uint8_t {
        None,       // No template parameters; e.g. explicit specialization.
        Unilevel,   // Unidimensional set of template parameters; e.g. templates that are not members of templates.
        Multilevel, // Multidimensional set of template parameters; e.g. member templates of templates.
        Count
    };

    // Type of abstract references to chart (i.e. parameter lists) structures.
    struct ChartIndex : index_like::Over<ChartSort> {
        using Over<ChartSort>::Over;
    };

    // Sorts of declarations that can be exported, or part of exportable entities.
    enum class DeclSort : std::uint8_t {
        VendorExtension,       // A vendor-specific extension.
        Enumerator,            // An enumerator declaration.
        Variable,              // A variable declaration; a static-data member is also considered a variable.
        Parameter,             // A parameter declaration - for a function or a template
        Field,                 // A non-static data member declaration.
        Bitfield,              // A bit-field declaration.
        Scope,                 // A type declaration.
        Enumeration,           // An enumeration declaration.
        Alias,                 // An alias declaration for a (general) type.
        Temploid,              // A member of a parameterized scope -- does not have template parameters of its own.
        Template,              // A template declaration: class, function, constructor, type alias, variable.
        PartialSpecialization, // A partial specialization of a template (class-type or function).
        Specialization,        // A specialization of some template decl.
        DefaultArgument,       // A default argument declaration for some parameter.
        Concept,               // A concept
        Function,              // A function declaration; both free-standing and static member functions.
        Method,                // A non-static member function declaration.
        Constructor,           // A constructor declaration.
        InheritedConstructor,  // A constructor inherited from a base class.
        Destructor,            // A destructor declaration.
        Reference,             // A reference to a declaration from a given module.
        Using,                 // A using declaration.  FIXME: See bug https://github.com/microsoft/ifc-spec/issues/122
        Prolongation,          // An out-of-home-scope definition for an entity previously declared in its home scope.
        Friend,                // A friend declaration.  FIXME: See bug https://github.com/microsoft/ifc-spec/issues/123
        Expansion,             // A pack-expansion of a declaration
        DeductionGuide,        // C(T) -> C<U>
        Barren,                // Declaration introducing no names, e.g. static_assert, asm-declaration, empty-declaration
        Tuple,                 // A sequence of declarations.
        SyntaxTree,            // A syntax tree for an unelaborated declaration.
        Intrinsic,             // An intrinsic function declaration.
        Property,              // VC's extension of property declaration.
                               // FIXME: See bug https://github.com/microsoft/ifc-spec/issues/124
        OutputSegment,         // Code segment. These are 'declared' via pragmas.
                               // FIXME: See bug https://github.com/microsoft/ifc-spec/issues/125
        Count,
    };

    // Type of abstract references to declaration structures.
    struct DeclIndex : index_like::Over<DeclSort> {
        using Over<DeclSort>::Over;
    };

    // Sorts of types.
    enum class TypeSort : std::uint8_t {
        VendorExtension, // Vendor-specific type constructor extensions.
        Fundamental,     // Fundamental type, in standard C++ sense
        Designated,      // A type designated by a declared name.  Really a proxy type designator.
        Tor,             // Type of a constructor or a destructor.
        Syntactic,       // Source-level expression designating unelaborated type.
        Expansion,       // A pack expansion of a type.
        Pointer,         // Pointer type, whether the pointee is data type or function type
        PointerToMember, // Type of a non-static data member that includes the enclosing class type.  Useful for
                         // pointer-to-data-member.
        LvalueReference, // Classic reference type, e.g. A&
        RvalueReference, // Rvalue reference type, e.g. A&&
        Function,        // Function type, excluding non-static member function type
        Method,          // Non-static member function type.
        Array,           // Builtin array type
        Typename,        // An unresolved qualified type expression
        Qualified,       // Qualified types.
        Base,            // Use of a type as a base-type in a class inheritance
        Decltype,        // A 'decltype' type.
        Placeholder,     // A placeholder type - e.g. uses of 'auto' in function signatures.
        Tuple,           // A sequence of types.  Generalized type for uniform description.
        Forall,          // Template type.  E.g. type of `template<typename T> constexpr T zro = T{};`
        Unaligned,       // A type with __unaligned.  This is a curiosity with no clearly defined semantics.
                         // FIXME: See bug https://github.com/microsoft/ifc-spec/issues/126
        SyntaxTree,      // A syntax tree for an unelaborated type.
        Count            // not a type; only for checking purposes
    };

    // Type of abstract references to type structures.
    struct TypeIndex : index_like::Over<TypeSort> {
        using Over<TypeSort>::Over;
    };

    // The set of exported syntactic elements
    // FIXME: See bug https://github.com/microsoft/ifc-spec/issues/127
    enum class SyntaxSort : std::uint8_t {
        VendorExtension,           // Vendor-specific extension for syntax. What fresh hell is this?
        SimpleTypeSpecifier,       // A simple type-specifier (i.e. no declarator)
        DecltypeSpecifier,         // A decltype-specifier - 'decltype(expression)'
        PlaceholderTypeSpecifier,  // A placeholder-type-specifier
        TypeSpecifierSeq,          // A type-specifier-seq - part of a type-id
        DeclSpecifierSeq,          // A decl-specifier-seq - part of a declarator
        VirtualSpecifierSeq,       // A virtual-specifier-seq (includes pure-specifier)
        NoexceptSpecification,     // A noexcept-specification
        ExplicitSpecifier,         // An explicit-specifier
        EnumSpecifier,             // An enum-specifier
        EnumeratorDefinition,      // An enumerator-definition
        ClassSpecifier,            // A class-specifier
        MemberSpecification,       // A member-specification
        MemberDeclaration,         // A member-declaration
        MemberDeclarator,          // A member-declarator
        AccessSpecifier,           // An access-specifier
        BaseSpecifierList,         // A base-specifier-list
        BaseSpecifier,             // A base-specifier
        TypeId,                    // A complete type used as part of an expression (can contain an abstract declarator)
        TrailingReturnType,        // a trailing return type: '-> T'
        Declarator,                // A declarator: i.e. something that has not (yet) been resolved down to a symbol
        PointerDeclarator,         // A sub-declarator for a pointer: '*D'
        ArrayDeclarator,           // A sub-declarator for an array: 'D[e]'
        FunctionDeclarator,        // A sub-declarator for a function: 'D(T1, T2, T3) <stuff>'
        ArrayOrFunctionDeclarator, // Either an array or a function sub-declarator
        ParameterDeclarator,       // A function parameter declaration
        InitDeclarator,            // A declaration with an initializer
        NewDeclarator,             // A new declarator (used in new expressions)
        SimpleDeclaration,         // A simple-declaration
        ExceptionDeclaration,      // An exception-declaration
        ConditionDeclaration,      // A declaration within if or switch statement: if (decl; cond)
        StaticAssertDeclaration,   // A static_assert-declaration
        AliasDeclaration,          // An alias-declaration
        ConceptDefinition,         // A concept-definition
        CompoundStatement,         // A compound statement
        ReturnStatement,           // A return statement
        IfStatement,               // An if statement
        WhileStatement,            // A while statement
        DoWhileStatement,          // A do-while statement
        ForStatement,              // A for statement
        InitStatement,             // An init-statement
        RangeBasedForStatement,    // A range-based for statement
        ForRangeDeclaration,       // A for-range-declaration
        LabeledStatement,          // A labeled statement
        BreakStatement,            // A break statement
        ContinueStatement,         // A continue statement
        SwitchStatement,           // A switch statement
        GotoStatement,             // A goto statement
        DeclarationStatement,      // A declaration statement
        ExpressionStatement,       // An expression statement
        TryBlock,                  // A try block
        Handler,                   // A catch handler
        HandlerSeq,                // A sequence of catch handlers
        FunctionTryBlock,          // A function try block
        TypeIdListElement,         // a type-id-list element
        DynamicExceptionSpec,      // A dynamic exception specification
        StatementSeq,              // A sequence of statements
        FunctionBody,              // The body of a function
        Expression,                // A wrapper around an ExprSort node
        FunctionDefinition,        // A function-definition
        MemberFunctionDeclaration, // A member function declaration
        TemplateDeclaration,       // A template head definition
        RequiresClause,            // A requires clause
        SimpleRequirement,         // A simple requirement
        TypeRequirement,           // A type requirement
        CompoundRequirement,       // A compound requirement
        NestedRequirement,         // A nested requirement
        RequirementBody,           // A requirement body
        TypeTemplateParameter,     // A type template-parameter
        TemplateTemplateParameter, // A template template-parameter
        TypeTemplateArgument,      // A type template-argument
        NonTypeTemplateArgument,   // A non-type template-argument
        TemplateParameterList,     // A template parameter list
        TemplateArgumentList,      // A template argument list
        TemplateId,                // A template-id
        MemInitializer,            // A mem-initializer
        CtorInitializer,           // A ctor-initializer
        LambdaIntroducer,          // A lambda-introducer
        LambdaDeclarator,          // A lambda-declarator
        CaptureDefault,            // A capture-default
        SimpleCapture,             // A simple-capture
        InitCapture,               // An init-capture
        ThisCapture,               // A this-capture
        AttributedStatement,       // An attributed statement
        AttributedDeclaration,     // An attributed declaration
        AttributeSpecifierSeq,     // An attribute-specifier-seq
        AttributeSpecifier,        // An attribute-specifier
        AttributeUsingPrefix,      // An attribute-using-prefix
        Attribute,                 // An attribute
        AttributeArgumentClause,   // An attribute-argument-clause
        Alignas,                   // An alignas( expression )
        UsingDeclaration,          // A using-declaration
        UsingDeclarator,           // A using-declarator
        UsingDirective,            // A using-directive
        ArrayIndex,                // An array index
        SEHTry,                    // An SEH try-block
        SEHExcept,                 // An SEH except-block
        SEHFinally,                // An SEH finally-block
        SEHLeave,                  // An SEH leave
        TypeTraitIntrinsic,        // A type trait intrinsic
        Tuple,                     // A sequence of zero or more syntactic elements
        AsmStatement,              // An __asm statement,
        NamespaceAliasDefinition,  // A namespace-alias-definition
        Super,                     // The '__super' keyword in a qualified id
        UnaryFoldExpression,       // A unary fold expression
        BinaryFoldExpression,      // A binary fold expression
        EmptyStatement,            // An empty statement: ';'
        StructuredBindingDeclaration, // A structured binding
        StructuredBindingIdentifier,  // A structured binding identifier
        UsingEnumDeclaration,         // A using-enum-declaration
        IfConsteval,                  // FIXME: Once we bump the major version of
                                      // the IFC fields in this should be moved to IfStatement.
        Count
    };

    // If this changes then we have a binary break.
    static_assert(index_like::tag_precision<SyntaxSort> == 7);

    // Type of abstract references to syntactic element structures.
    struct SyntaxIndex : index_like::Over<SyntaxSort> {
        using Over<SyntaxSort>::Over;
    };

    // The various kinds of parameters.
    enum class ParameterSort : std::uint8_t {
        Object,   // Function parameter
        Type,     // Type template parameter
        NonType,  // Non-type template parameter
        Template, // Template template parameter
        Count
    };

    // The various sort of literal constants.
    enum class LiteralSort : std::uint8_t {
        Immediate,     // Immediate integer constants, directly representable by an index value.
        Integer,       // Unsigned 64-bit integer constant that are not immediate values.
        FloatingPoint, // Floating point constant.
        Count
    };

    // Type of abstract references to numerical constants.
    struct LitIndex : index_like::Over<LiteralSort> {
        using Over<LiteralSort>::Over;
    };

    // Sorts of statements.
    enum class StmtSort : std::uint8_t {
        VendorExtension, // Vendor-specific extensions
        Try,             // A try-block containing a list of handlers
        If,              // If statements
        For,             // C-style for loop, ranged-based for
        Labeled,         // A labeled-statement. (was Case in IFC < 0.42)
        While,           // A while loop, not do-while
        Block,           // A {}-delimited sequence of statements
        Break,           // Break statements (as in a loop or switch)
        Switch,          // Switch statement
        DoWhile,         // Do-while loops
        Goto,            // A goto statement. (was Default in IFC < 0.42)
        Continue,        // Continue statement (as in a loop)
        Expression,      // A naked expression terminated with a semicolon
        Return,          // A return statement
        Decl,            // A declaration
        Expansion,       // A pack-expansion of a statement
        SyntaxTree,      // A syntax tree for an unelaborated statement.
        Handler,         // An exception-handler statement (catch-clause).
        Tuple,           // A general tuple of statements.
        Dir,             // A directive statement.
        Count
    };

    // If this changes then we have a binary break.
    static_assert(index_like::tag_precision<StmtSort> == 5);

    // Type of abstract references to statement structures
    struct StmtIndex : index_like::Over<StmtSort> {
        using Over<StmtSort>::Over;
    };

    // Sorts of expressions.
    enum class ExprSort : std::uint8_t {
        VendorExtension,           // Vendor-specific extension for expressions.
        Empty,                     // An empty expression.
        Literal,                   // Literal constants.
        Lambda,                    // Lambda expression
        Type,                      // A type expression.  Useful in template argument contexts, and other more general context.
        NamedDecl,                 // Use of a name to reference a declaration.
        UnresolvedId,              // An unresolved or dependent name as expression.
                                   // FIXME: See bug https://github.com/microsoft/ifc-spec/issues/128
        TemplateId,                // A template-id expression.
        UnqualifiedId,             // An unqualified-id + some other stuff like 'template' and/or 'typename'.
                                   // FIXME: See bug https://github.com/microsoft/ifc-spec/issues/128
        SimpleIdentifier,          // Just an identifier: nothing else.
                                   // FIXME: See bug https://github.com/microsoft/ifc-spec/issues/128
        Pointer,                   // A '*' when it appears as part of a qualified-name.
                                   // FIXME: See bug https://github.com/microsoft/ifc-spec/issues/129
        QualifiedName,             // A raw qualified-name: i.e. one that has not been fully resolved
        Path,                      // A path expression, e.g. a fully bound qualified-id
        Read,                      // Lvalue-to-rvalue conversions.
        Monad,                     // 1-part expression.
        Dyad,                      // 2-part expressions.
        Triad,                     // 3-part expressions.
        String,                    // A string literal
        Temporary,                 // A temporary expression.
        Call,                      // An invocation of some form of callable expression.
        MemberInitializer,         // A base or member initializer
        MemberAccess,              // The member being accessed + offset
        InheritancePath,           // The representation of the 'path' to a base-class member
        InitializerList,           // An initializer-list
        Cast,                      // A cast: either old-style '(T)e'; new-style 'static_cast<T>(e)'; or functional 'T(e)'
        Condition,                 // A statement condition: 'if (e)' or 'if (T v = e)'
        ExpressionList,            // The sequence of expressions in either '(e1, e2, e3)' or '{ e1, e2, e2 }'
        SizeofType,                // 'sizeof(type-id)'.  FIXME: See bug https://github.com/microsoft/ifc-spec/issues/130
        Alignof,                   // 'alignof(type-id)'.  FIXME: See bug https://github.com/microsoft/ifc-spec/issues/131
        Label,                     // A symbolic label resulting from the expression in a labeled statement.
        UnusedSort0,               // Empty slot.
        Typeid,                    // A 'typeid' expression
        DestructorCall,            // A destructor call: i.e. the right sub-expression of 'expr->~T()'
        SyntaxTree,                // A syntax tree for an unelaborated expression.
        FunctionString,            // A string literal expanded from a built-in macro like __FUNCTION__.
                                   // FIXME: See bug https://github.com/microsoft/ifc-spec/issues/132
        CompoundString,            // A string literal of the form __LPREFIX(string-literal).
                                   // FIXME: See bug https://github.com/microsoft/ifc-spec/issues/132
        StringSequence,            // A sequence of string literals.
                                   // FIXME: See bug https://github.com/microsoft/ifc-spec/issues/132
        Initializer,               // An initializer for an object
        Requires,                  // A requires expression
        UnaryFold,                 // A unary fold expression of the form (pack @ ...)
        BinaryFold,                // A binary fold expression of the form (expr @ ... @ pack)
        HierarchyConversion,       // A class hierarchy conversion. Distinct from ExprSort::Cast, which represents
                                   // a syntactic expression of intent, whereas this represents the semantics of a
                                   // (possibly implicit) conversion
        ProductTypeValue,          // The representation of an object value expression tree
                                   // (really an associative map of declarations to values)
        SumTypeValue,              // The representation of a union object value expression tree.
                                   // Unions have a single active SubobjectValue
        UnusedSort1,               // Empty slot.
        ArrayValue,                // A constant-initialized array value object
        DynamicDispatch,           // A dynamic dispatch expression, i.e. call to virtual function
        VirtualFunctionConversion, // A conversion of a virtual function reference to its underlying target: C::f // 'f'
                                   // is a virtual function.
        Placeholder,               // A placeholder expression.
        Expansion,                 // A pack-expansion expression
        Generic,                   // A generic-selection -- C11 extension.
        Tuple,                     // General tuple expressions.
        Nullptr,                   // 'nullptr'
        This,                      // 'this'
        TemplateReference,         // A reference to a member of a template
        Statement,                 // A statement expression.
        TypeTraitIntrinsic,        // A use of a type trait intrinsic.
                                   // FIXME: See bug https://github.com/microsoft/ifc-spec/issues/133
        DesignatedInitializer,     // A single designated initializer clause: '.a = e'
        PackedTemplateArguments,   // The template argument set for a template parameter pack
        Tokens,                    // A token stream (i.e. a complex default template argument)
        AssignInitializer,         // '= e'.  FIXME:  This is NOT an expression.
        Count                      // Total count.
    };

    // Type of abstract references to expression structures
    struct ExprIndex : index_like::Over<ExprSort> {
        using Over<ExprSort>::Over;
    };

    enum class InheritanceSort : std::uint8_t {
        None,
        Single,
        Multiple,
        Count,
    };

    // Type qualifiers.
    enum class Qualifier : std::uint8_t {
        None     = 0,      // No qualifier
        Const    = 1 << 0, // "const" qualifier
        Volatile = 1 << 1, // "volatile" qualifier
        Restrict = 1 << 2, // "restrict" qualifier, C extension
    };

    // Type of abstract references to words (generalized tokens).
    enum class WordCategory : std::uint16_t {};

    enum class MacroSort : std::uint8_t {
        ObjectLike,   // #define NAME <replacement-text>
        FunctionLike, // #define F(A, B) <replacement-text>
        Count
    };

    // Type of abstract references to expression structures
    struct MacroIndex : index_like::Over<MacroSort> {
        using Over<MacroSort>::Over;
    };

    enum class PragmaSort : std::uint8_t {
        VendorExtension,
        Expr,            // Pragma with a description and expression representing compiler information, e.g. #pragma message "literal"
        Count,
    };

    // Type of abstract references to pragma structures
    struct PragmaIndex : index_like::Over<PragmaSort> {
        using Over<PragmaSort>::Over;
    };

    static_assert(index_like::tag_precision<PragmaSort> == 1);
    static_assert(index_like::index_precision<PragmaSort> == 31);

    enum class AttrSort : std::uint8_t {
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

    // Type of abstract references to attribute structures
    struct AttrIndex : index_like::Over<AttrSort> {
        using Over<AttrSort>::Over;
    };

    enum class DirSort : std::uint8_t {
        VendorExtension,   // Vendor-specific extension for directives.
        Empty,             // An empty declaration - ;
        Attribute,         // Attribute declaration - [[nodiscard]].
        Pragma,            // Pragma directive - _Pragma
        Using,             // Using-directive declaration - using namespace N
        DeclUse,           // Using declaration - using ::X
        Expr,              // Phased-evaluation of an expression - static_assert, asm-declaration, etc.
        StructuredBinding, // Structured binding declaration - auto [a, b, c] = x
        SpecifiersSpread,  // A spread of sequence of decl-specifiers over a collection of init-declarators in a single
                           // grammatical declaration - int* p, i, **x, ...
        Stmt,              // Phased-evaluation of a statement - if, while, for, etc.
        Unused1,           // Empty slot
        Unused2,           // Empty slot
        Unused3,           // Empty slot
        Unused4,           // Empty slot
        Unused5,           // Empty slot
        Unused6,           // Empty slot
        Unused7,           // Empty slot
        Unused8,           // Empty slot
        Unused9,           // Empty slot
        Unused10,          // Empty slot
        Unused11,          // Empty slot
        Unused12,          // Empty slot
        Unused13,          // Empty slot
        Unused14,          // Empty slot
        Unused15,          // Empty slot
        Unused16,          // Empty slot
        Unused17,          // Empty slot
        Unused18,          // Empty slot
        Unused19,          // Empty slot
        Unused20,          // Empty slot
        Unused21,          // Empty slot
        Tuple,             // A collection of directives
        Count
    };

    // Type of abstract references to directive structures
    struct DirIndex : index_like::Over<DirSort> {
        using Over<DirSort>::Over;
    };

    // Symbolic name of the various heaps stored in an IFC.
    enum class HeapSort : std::uint8_t {
        Decl,   // DeclIndex heap
        Type,   // TypeIndex heap
        Stmt,   // StmtIndex heap
        Expr,   // ExprIndex heap
        Syntax, // SyntaxIndex heap
        Word,   // WordIndex heap
        Chart,  // ChartIndex heap
        Spec,   // specialization form heap
        Form,   // FormIndex heap
        Attr,   // AttrIndex heap
        Dir,    // DirIndex heap
        Vendor, // VendorIndex heap
        Count
    };

    // Vendor-specific syntax (or future additions to the IFC specification).
    enum class VendorSort : std::uint8_t {
        SEHTry,                 // A structured exception handling try block.
        SEHFinally,             // A structured exception handling finally block.
        SEHExcept,              // A structured exception handling except block.
        SEHLeave,               // A structured exception handling leave statement.
        Count
    };

    struct VendorIndex : index_like::Over<VendorSort> {
        using Over<VendorSort>::Over;
    };

    // Typed projection of a homogeneous sequence.
    template<typename, auto... s>
        requires(sizeof...(s) < 2)
    struct Sequence {
        Index start             = {}; // Offset of the first member of this sequence
        Cardinality cardinality = {}; // Number of elements in this sequence.
    };

    // All entities are externally represented in a symbolic form.
    // The data structures for symbolic representation are defined in this namespace.
    namespace symbolic {
        struct Declaration {
            DeclIndex index;
        };

        // A tag embedding an abstract reference sort value in the static type.  We are doing this only because of
        // a weakness in the implementation language (C++).  What we really want is to define a function
        // from types to values without going through the usual cumbersome explicit specialization dance.
        template<auto s>
        struct Tag : index_like::SortTag<s> {
            static constexpr auto algebra_sort = s;

            // For comparison purposes such as inserting into ordered containers, the derived
            // classes may have to explicitly default 'operator==' which requires this base class
            // to also have a defined operator==.
            constexpr bool operator==(const Tag&) const
            {
                return true;
            }
            // Similar story for ordering.
            constexpr std::strong_ordering operator<=>(const Tag&) const
            {
                return std::strong_ordering::equal;
            }
        };

        struct ConversionFunctionId : Tag<NameSort::Conversion> {
            TypeIndex target{}; // The type to convert to
            TextOffset name{};  // The index for the name (if we have one)

            bool operator==(const ConversionFunctionId&) const = default;
        };

        struct OperatorFunctionId : Tag<NameSort::Operator> {
            TextOffset name{}; // The index for the name
            Operator symbol{}; // The symbolic operator
            // Note: 16 bits hole left.

            bool operator==(const OperatorFunctionId&) const = default;
        };

        struct LiteralOperatorId : Tag<NameSort::Literal> {
            TextOffset name_index{}; // Note: ideally should be literal operator suffix, as plain identifier.

            bool operator==(const LiteralOperatorId&) const = default;
        };

        struct TemplateName : Tag<NameSort::Template> {
            NameIndex name{}; // template name; can't be itself another TemplateName.

            bool operator==(const TemplateName&) const = default;
        };

        struct SpecializationName : Tag<NameSort::Specialization> {
            NameIndex primary_template{}; // The primary template name.  Note: NameIndex is over-generic here.  E.g. it
                                          // can't be a decltype, nor can it be another template-id.
            ExprIndex arguments{};

            bool operator==(const SpecializationName&) const = default;
        };

        struct SourceFileName : Tag<NameSort::SourceFile> {
            TextOffset name{};
            TextOffset include_guard{};

            bool operator==(const SourceFileName&) const = default;
        };

        struct GuideName : Tag<NameSort::Guide> {
            DeclIndex primary_template{};

            bool operator==(const GuideName&) const = default;
        };

        // A referenced to a module.
        struct ModuleReference {
            TextOffset owner; // The owner of this reference type.  If the owner is null the owner is the global module.
            TextOffset partition; // Optional partition where this reference belongs.  If owner is null, this is the
                                  // path to a header unit.
        };

        // Source location
        struct SourceLocation {
            LineIndex line      = {};
            ColumnNumber column = {};

            bool operator==(const SourceLocation&) const = default;
        };

        // Conceptually, this is a token; but we have too many token type and it would be confusing to call this Token.
        struct Word {
            SourceLocation locus = {};
            union {
                TextOffset text = {}; // Text associated with this token
                ExprIndex expr;       // Literal constant expression / binding associated with this token
                TypeIndex type;       // Literal resolved type associated with this token
                Index state;          // State of preprocessor to push to
                StringIndex string;   // String literal
            };
            union {
                WordCategory category = {}; // FIXME: Horrible workaround deficiencies.  Remove.
                source::Directive src_directive;
                source::Punctuator src_punctuator;
                source::Literal src_literal;
                source::Operator src_operator;
                source::Keyword src_keyword;
                source::Identifier src_identifier;
            };
            WordSort algebra_sort = {};
        };

        static_assert(sizeof(Word) == 4 * sizeof(Index));

        // Description of token sequences.
        struct WordSequence {
            Index start             = {};
            Cardinality cardinality = {};
            SourceLocation locus    = {};
        };

        struct NoexceptSpecification {
            SentenceIndex words = {}; // Tokens of the operand to noexcept, if a non-trivial noexcept specification.
            NoexceptSort sort   = {};

            // Implementation details
            bool operator==(const NoexceptSpecification&) const = default;
        };

        // The set of fundamental type basis.
        enum class TypeBasis : std::uint8_t {
            Void,             // "void"
            Bool,             // "bool"
            Char,             // "char"
            Wchar_t,          // "wchar_t"
            Int,              // "int"
            Float,            // "float"
            Double,           // "double"
            Nullptr,          // "decltype(nullptr)"
            Ellipsis,         // "..."        -- generalized type
            SegmentType,      // "segment".  FIXME: See bug https://github.com/microsoft/ifc-spec/issues/134
            Class,            // "class"
            Struct,           // "struct"
            Union,            // "union"
            Enum,             // "enum"
            Typename,         // "typename"
            Namespace,        // "namespace"
            Interface,        // "__interface"
            Function,         // concept of function type.  FIXME: See bug https://github.com/microsoft/ifc-spec/issues/136
            Empty,            // an empty pack expansion
            VariableTemplate, // a variable template.  FIXME: See bug https://github.com/microsoft/ifc-spec/issues/135
            Concept,          // a concept
            Auto,             // "auto"
            DecltypeAuto,     // "decltype(auto)"
            Overload,         // fundamental type for an overload set
            Count             // cardinality of fundamental type basis.
        };

        enum class TypePrecision : std::uint8_t {
            Default, // Default bit width, whatever that is.
            Short,   // The short version.
            Long,    // The long version.
            Bit8,    // The  8-bit version.
            Bit16,   // The 16-bit version.
            Bit32,   // The 32-bit version.
            Bit64,   // The 64-bit (long long) version.
            Bit128,  // The 128-bit version.
            Count
        };

        // Signed-ness of a fundamental type.
        enum class TypeSign : std::uint8_t {
            Plain,    // No sign specified, default to standard interpretation.
            Signed,   // Specified sign, or implied
            Unsigned, // Specified sign.
            Count
        };

        // Fundamental types: standard fundamental types, and extended.
        // They are represented externally as possibly signed variation of a core basis
        // of builtin types, with versions expressing "bitness".
        struct FundamentalType : Tag<TypeSort::Fundamental> {
            TypeBasis basis{};
            TypePrecision precision{};
            TypeSign sign{};
            std::uint8_t unused{};
        };

        // Template parameter pack and template-id expansion mode.
        enum class ExpansionMode : std::uint8_t {
            Full,
            Partial,
        };

        // Designation of a type by a declared name.
        struct DesignatedType : Tag<TypeSort::Designated> {
            DeclIndex decl{}; // A declaration for the entity designated by type.
        };

        struct TorType : Tag<TypeSort::Tor> {
            TypeIndex source{};                 // Parameter type sequence.
            NoexceptSpecification eh_spec{};    // Noexcept specification.
            CallingConvention convention{};     // Calling convention.

            bool operator==(const TorType&) const = default;
        };

        struct SyntacticType : Tag<TypeSort::Syntactic> {
            ExprIndex expr{};
        };

        // Type-id expansion involving a template parameter pack.
        struct ExpansionType : Tag<TypeSort::Expansion> {
            TypeIndex pack  = {};
            ExpansionMode mode = {};
        };

        // Pointer type.
        struct PointerType : Tag<TypeSort::Pointer> {
            TypeIndex pointee{};
        };

        // Lvalue reference type.
        struct LvalueReferenceType : Tag<TypeSort::LvalueReference> {
            TypeIndex referee{};
        };

        // Rvalue reference type.
        struct RvalueReferenceType : Tag<TypeSort::RvalueReference> {
            TypeIndex referee{};
        };

        // Unaligned type -- an MS VC curiosity.
        struct UnalignedType : Tag<TypeSort::Unaligned> {
            TypeIndex operand{};
        };

        struct DecltypeType : Tag<TypeSort::Decltype> {
            SyntaxIndex expression{};
        };

        struct PlaceholderType : Tag<TypeSort::Placeholder> {
            ExprIndex constraint{};     // The predicate associated with this type placeholder.  Null means no constraint.
            TypeBasis basis{};          // auto/decltype(auto)
            TypeIndex elaboration{};    // The type this placeholder was deduced to.
        };

        // A pointer to non-static member type.
        struct PointerToMemberType : Tag<TypeSort::PointerToMember> {
            TypeIndex scope = {}; // The enclosing class
            TypeIndex type  = {}; // Type of the pointed-to member.
        };

        // A sequence of zero of more types.  A generalized type, providing simpler accounts of
        // disparate type notions in C++.
        struct TupleType : Tag<TypeSort::Tuple>, Sequence<TypeIndex, HeapSort::Type> {
            TupleType() = default;
            TupleType(Index f, Cardinality n) : Sequence{f, n} {}
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
            ChartIndex chart{};     // The set of parameters of this parametric type
            TypeIndex subject{};    // The type being parameterized
        };

        // Ordinary function types
        struct FunctionType : Tag<TypeSort::Function> {
            TypeIndex target{};                 // result type
            TypeIndex source{};                 // Description of parameter types.
            NoexceptSpecification eh_spec{};    // Noexcept specification.
            CallingConvention convention{};     // Calling convention.
            FunctionTypeTraits traits{};        // Various (rare) function-type traits
        };

        // Non-static member function types.
        struct MethodType : Tag<TypeSort::Method> {
            TypeIndex target{};                 // result type
            TypeIndex source{};                 // Description of parameter types.
            TypeIndex class_type{};             // The enclosing class associated with this non-static member function type
            NoexceptSpecification eh_spec{};    // Noexcept specification.
            CallingConvention convention{};     // Calling convention.
            FunctionTypeTraits traits{};        // Various (rare) function-type traits
        };

        // Builtin array types.
        struct ArrayType : Tag<TypeSort::Array> {
            TypeIndex element = {}; // The array element type.
            ExprIndex bound   = {}; // The number of element in this array.
        };

        // Qualified types:
        struct QualifiedType : Tag<TypeSort::Qualified> {
            TypeIndex unqualified_type = {};
            Qualifier qualifiers       = {};
        };

        // Unresolved type names.
        struct TypenameType : Tag<TypeSort::Typename> {
            ExprIndex path{};
        };

        enum class BaseClassTraits : std::uint8_t {
            None     = 0x00, // Nothing
            Shared   = 0x01, // Base class inherited virtually
            Expanded = 0x02, // Base class pack expanded
        };

        // Base-type in a class inheritance.
        struct BaseType : Tag<TypeSort::Base> {
            TypeIndex type {};         // The actual base type, without specifiers
            Access access {};          // Access specifier.
            BaseClassTraits traits;    // Additional base class semantics.
        };

        // Type-id in parse tree form
        struct SyntaxTreeType : Tag<TypeSort::SyntaxTree> {
            SyntaxIndex syntax{};
        };

        // The types that provide a syntactic representation of a program
        // Note: All structures defined in this namespaces are to be removed when `SyntaxSort` is removed.
        // See bug https://github.com/microsoft/ifc-spec/issues/127
        namespace syntax {
            // DecltypeSpecifier is 'decltype(expression)'. DecltypeAutoSpecifier is 'decltype(auto)'. See below.
            struct DecltypeSpecifier : Tag<SyntaxSort::DecltypeSpecifier> {
                ExprIndex expression{};            // The expression (for 'auto', see DecltypeAutoSpecifier)
                SourceLocation decltype_keyword{}; // The source location of the 'decltype'
                SourceLocation left_paren{};       // The source location of the '('
                SourceLocation right_paren{};      // The source location of the ')'
            };

            struct PlaceholderTypeSpecifier : Tag<SyntaxSort::PlaceholderTypeSpecifier> {
                symbolic::PlaceholderType type{}; // The placeholder type of the placeholder-type-specifier
                SourceLocation keyword{};         // The source location of the 'auto'/'decltype(auto)'
                SourceLocation locus{};           // The source location of the placeholder-type-specifier.
                                                  // This can be different from keyword_ if type-constraint
                                                  // is present
            };

            struct SimpleTypeSpecifier : Tag<SyntaxSort::SimpleTypeSpecifier> {
                TypeIndex type;       // The type (if we know it)
                ExprIndex expr;       // The expression (if we don't)
                SourceLocation locus; // The source location
            };

            struct TypeSpecifierSeq : Tag<SyntaxSort::TypeSpecifierSeq> {
                SyntaxIndex type_script{};
                TypeIndex type{};
                SourceLocation locus{};
                Qualifier qualifiers{}; // cv-qualifiers associated with this type specifier
                bool is_unhashed{};
            };

            struct Keyword {
                enum class Kind : std::uint8_t {
                    None,
                    Class,
                    Struct,
                    Union,
                    Public,
                    Protected,
                    Private,
                    Default,
                    Delete,
                    Mutable,
                    Constexpr,
                    Consteval,
                    Typename,
                    Constinit,
                };
                SourceLocation locus{}; // The source location of the access-specifier token
                Kind kind = Kind::None; // What kind of keyword do we have?
            };

            enum class StorageClass : std::uint32_t {
                None         = 0,
                Auto         = 1 << 0,  // 'auto' (use as a storage-class requires '/Zc:auto-')
                Constexpr    = 1 << 1,  // 'constexpr'
                Explicit     = 1 << 2,  // 'explicit'
                Extern       = 1 << 3,  // 'extern'
                Force_inline = 1 << 4,  // '__forceinline'
                Friend       = 1 << 5,  // 'friend'
                Inline       = 1 << 6,  // 'inline'
                Mutable      = 1 << 7,  // 'mutable'
                New_slot     = 1 << 8,  // 'new' (used in C++/CLI)
                Register     = 1 << 9,  // 'register' (removed in C++17)
                Static       = 1 << 10, // 'static'
                Thread_local = 1 << 11, // 'thread_local'
                Tile_static  = 1 << 12, // 'tile_static' (C++AMP)
                Typedef      = 1 << 13, // 'typedef'
                Virtual      = 1 << 14, // 'virtual'
                Consteval    = 1 << 15, // 'consteval' (C++20)
                Constinit    = 1 << 16, // 'constinit' (C++20)
            };

            struct DeclSpecifierSeq : Tag<SyntaxSort::DeclSpecifierSeq> {
                TypeIndex type{};                 // The type (if it is a basic-type)
                SyntaxIndex type_script{};        // The type-name (if it is something other than a basic-type)
                SourceLocation locus{};           // The source location of this decl-specifier-seq
                StorageClass storage_class{};     // The storage-class (if any)
                SentenceIndex declspec{};         // Stream of __declspec tokens
                SyntaxIndex explicit_specifier{}; // The explicit-specifier (if any)
                Qualifier qualifiers{};           // Any cv-qualifiers etc.
            };

            struct EnumSpecifier : Tag<SyntaxSort::EnumSpecifier> {
                ExprIndex name{};             // The name of the enumeration
                Keyword class_or_struct{};    // The 'class' or 'struct' keyword (if present)
                SyntaxIndex enumerators{};    // The set of enumerators
                SyntaxIndex enum_base{};      // The underlying type (if present)
                SourceLocation locus{};       // The source location of the enum-specifier
                SourceLocation colon{};       // The source location of the ':' (if present)
                SourceLocation left_brace{};  // The source location of the '{' (if present)
                SourceLocation right_brace{}; // The source location of the '}' (if present)
            };

            struct EnumeratorDefinition : Tag<SyntaxSort::EnumeratorDefinition> {
                TextOffset name{};       // The name of the enumerator
                ExprIndex expression{};  // The initializer (if any)
                SourceLocation locus{};  // The source location of the enumerator
                SourceLocation assign{}; // The source location of the '=' (if present)
                SourceLocation comma{};  // The source location of the ',' (if present)
            };

            struct ClassSpecifier : Tag<SyntaxSort::ClassSpecifier> {
                ExprIndex name{};             // The name of this class
                Keyword class_key{};          // The class-key: 'class', 'struct' or 'union'
                SyntaxIndex base_classes{};   // Any base-classes
                SyntaxIndex members{};        // Any members
                SourceLocation left_brace{};  // The source location of the '{' (if present)
                SourceLocation right_brace{}; // The source location of the '}' (if present)
            };

            struct BaseSpecifierList : Tag<SyntaxSort::BaseSpecifierList> {
                SyntaxIndex base_specifiers{}; // The set of base-classes
                SourceLocation colon{};        // The source location of the ':'
            };

            struct BaseSpecifier : Tag<SyntaxSort::BaseSpecifier> {
                ExprIndex name{};                 // The name of the base-class
                Keyword access_keyword{};         // The access-specifier (if present)
                SourceLocation virtual_keyword{}; // The source location of the 'virtual' keyword (if present)
                SourceLocation locus{};           // The source location of the base-specifier
                SourceLocation ellipsis{};        // The source location of the '...' (if present)
                SourceLocation comma{};           // The source location of the ',' (if present)
            };

            struct MemberSpecification : Tag<SyntaxSort::MemberSpecification> {
                SyntaxIndex declarations{}; // The set of member declarations
            };

            struct AccessSpecifier : Tag<SyntaxSort::AccessSpecifier> {
                Keyword keyword{};      // The access-specifier keyword
                SourceLocation colon{}; // The source location of the ':'
            };

            struct MemberDeclaration : Tag<SyntaxSort::MemberDeclaration> {
                SyntaxIndex decl_specifier_seq{}; // The decl-specifier-seq
                SyntaxIndex declarators{};        // The the list of declarators
                SourceLocation semi_colon{};      // The source location of the ';'
            };

            struct MemberDeclarator : Tag<SyntaxSort::MemberDeclarator> {
                SyntaxIndex declarator{};      // The declarator
                SyntaxIndex requires_clause{}; // The requires clause (if present)
                ExprIndex expression{};        // The bitfield size (if present)
                ExprIndex initializer{};       // The initializer (if present)
                SourceLocation locus{};        // The source location of this member-declarator
                SourceLocation colon{};        // The source location of the ':' (if present)
                SourceLocation comma{};        // The source location of the ',' (if present)
            };

            struct TypeId : Tag<SyntaxSort::TypeId> {
                SyntaxIndex type{};       // The type
                SyntaxIndex declarator{}; // The declarator (if present)
                SourceLocation locus{};   // The source location of the type-id
            };

            struct TrailingReturnType : Tag<SyntaxSort::TrailingReturnType> {
                SyntaxIndex type{};     // The return type
                SourceLocation locus{}; // The source location of the '->'
            };

            struct PointerDeclarator : Tag<SyntaxSort::PointerDeclarator> {
                enum class Kind : std::uint8_t {
                    None,
                    Pointer,         // T*
                    LvalueReference, // T&
                    RvalueReference, // T&&
                    PointerToMember, // T X::*
                };
                ExprIndex owner{};              // The owning class if this is a pointer-to-member
                SyntaxIndex child{};            // Our child (if we have multiple indirections)
                SourceLocation locus{};         // The source location
                Kind kind{};                    // What kind of pointer do we have?
                Qualifier qualifiers{};         // any cv-qualifiers etc.
                CallingConvention convention{}; // possible calling convention with this type-specifier (only used if
                                                // is_function_ is set)
                bool is_function{};             // does this declarator name a function type?
            };

            struct ArrayDeclarator : Tag<SyntaxSort::ArrayDeclarator> {
                ExprIndex bounds{};             // The bounds expression (if any)
                SourceLocation left_bracket{};  // The source location of the '['
                SourceLocation right_bracket{}; // The source location of the ']'
            };

            struct FunctionDeclarator : Tag<SyntaxSort::FunctionDeclarator> {
                SyntaxIndex parameters{};              // The function parameters
                SyntaxIndex exception_specification{}; // The exception-specification (if present)
                SourceLocation left_paren{};           // The source location of the '('
                SourceLocation right_paren{};          // The source location of the ')'
                SourceLocation ellipsis{};             // The source location of the '...' (if present)
                SourceLocation ref_qualifier{};        // The source location of the '&' or '&&' (if present)
                FunctionTypeTraits traits{};           // Any cv-qualifiers etc. associated with the function
            };

            struct ArrayOrFunctionDeclarator : Tag<SyntaxSort::ArrayOrFunctionDeclarator> {
                SyntaxIndex declarator{}; // This declarator
                SyntaxIndex next{};       // The next declarator (if we have multiple indirections)
            };

            struct ParameterDeclarator : Tag<SyntaxSort::ParameterDeclarator> {
                SyntaxIndex decl_specifier_seq{}; // The decl-specifier_seq for this parameter
                SyntaxIndex declarator{};         // The declarator for this parameter
                ExprIndex default_argument{};     // Any default argument
                SourceLocation locus{};           // The source location of this parameter
                ParameterSort sort{};
            };

            struct VirtualSpecifierSeq : Tag<SyntaxSort::VirtualSpecifierSeq> {
                SourceLocation locus{};            // The source location (should probably be deduced)
                SourceLocation final_keyword{};    // The source location of the 'final' keyword (if present)
                SourceLocation override_keyword{}; // The source location of the 'override' keyword (if present)
                bool is_pure{};                    // Is this a pure virtual function?
            };

            struct NoexceptSpecification : Tag<SyntaxSort::NoexceptSpecification> {
                ExprIndex expression{};       // The constant-expression (if present)
                SourceLocation locus{};       // The source location
                SourceLocation left_paren{};  // The source location of the '(' (if present)
                SourceLocation right_paren{}; // The source location of the ')' (if present)
            };

            struct ExplicitSpecifier : Tag<SyntaxSort::ExplicitSpecifier> {
                ExprIndex expression{};      // The constant-expression. This node is used only for conditional explicit
                                             // where the expression exists.
                SourceLocation locus{};      // The source location of the 'explicit' keyword
                SourceLocation left_paren{}; // The source location of the '('
                SourceLocation right_paren{}; // The source location of the ')'
            };

            struct Declarator : Tag<SyntaxSort::Declarator> {
                SyntaxIndex pointer{};                      // A pointer declarator: '*D'
                SyntaxIndex parenthesized_declarator{};     // A parenthesized declarator: '(D)'
                SyntaxIndex array_or_function_declarator{}; // An array or function declarator: 'D[...]' or 'D(...)'
                SyntaxIndex trailing_return_type{};         // Any trailing return type
                SyntaxIndex virtual_specifiers{}; // Any virtual or pure specifiers: 'override', 'final', '= 0'
                ExprIndex name{};                 // The name of the declarator
                SourceLocation ellipsis{};        // The location of '...' (if present)
                SourceLocation locus{};           // The source location of this declarator
                Qualifier qualifiers{};           // Any cv-qualifiers etc.
                CallingConvention convention{};   // Possible calling convention with this type-specifier (only used if
                                                  // is_function_ is set)
                bool is_function{};               // Is this declarator name a function type?
            };

            struct InitDeclarator : Tag<SyntaxSort::InitDeclarator> {
                SyntaxIndex declarator{};      // The declarator
                SyntaxIndex requires_clause{}; // The requires clause (if present)
                ExprIndex initializer{};       // The associated initializer
                SourceLocation comma{};        // The source location of the ',' (if present)
            };

            struct NewDeclarator : Tag<SyntaxSort::NewDeclarator> {
                SyntaxIndex declarator{}; // The declarator
            };

            struct SimpleDeclaration : Tag<SyntaxSort::SimpleDeclaration> {
                SyntaxIndex decl_specifier_seq{}; // The decl-specifier-seq
                SyntaxIndex declarators{};        // The declarator(s)
                SourceLocation locus{};           // The source location of this declaration
                SourceLocation semi_colon{};      // The source location of the ';'
            };

            struct ExceptionDeclaration : Tag<SyntaxSort::ExceptionDeclaration> {
                SyntaxIndex type_specifier_seq{}; // The type-specifier-seq
                SyntaxIndex declarator{};         // The declarator
                SourceLocation locus{};           // The source location of this declaration
                SourceLocation ellipsis{};        // The source location of the '...' (if present)
            };

            struct ConditionDeclaration : Tag<SyntaxSort::ConditionDeclaration> {
                SyntaxIndex decl_specifier{}; // The decl-specifier
                SyntaxIndex init_statement{}; // The initialization expression
                SourceLocation locus{};       // The source location of this declaration
            };

            struct StaticAssertDeclaration : Tag<SyntaxSort::StaticAssertDeclaration> {
                ExprIndex expression{};       // The constant expression
                ExprIndex message{};          // The string-literal (if present)
                SourceLocation locus{};       // The source location of this declaration
                SourceLocation left_paren{};  // The source location of the '('
                SourceLocation right_paren{}; // The source location of the ')'
                SourceLocation semi_colon{};  // The source location of the ';'
                SourceLocation comma{};       // The source location of the ',' (if present)
            };

            struct AliasDeclaration : Tag<SyntaxSort::AliasDeclaration> {
                TextOffset identifier{};        // The identifier
                SyntaxIndex defining_type_id{}; // The defining-type-id
                SourceLocation locus{};         // The source location of this declaration
                SourceLocation assign{};        // The source location of the '='
                SourceLocation semi_colon{};    // The source location of the ';'
            };

            struct ConceptDefinition : Tag<SyntaxSort::ConceptDefinition> {
                SyntaxIndex parameters{};         // Any template parameters
                SourceLocation locus{};           // The source location of the 'template' keyword
                TextOffset identifier{};          // The identifier
                ExprIndex expression{};           // The constraint-expression
                SourceLocation concept_keyword{}; // The source location of the 'concept' keyword
                SourceLocation assign{};          // The source location of the '='
                SourceLocation semi_colon{};      // The source location of the ';'
            };

            struct StructuredBindingDeclaration : Tag<SyntaxSort::StructuredBindingDeclaration> {
                enum class RefQualifierKind : std::uint8_t {
                    None,
                    Rvalue,
                    Lvalue,
                };
                SourceLocation locus{};                // The location of this declaration
                SourceLocation ref_qualifier{};        // The source location of the ref-qualifier (if any)
                SyntaxIndex decl_specifier_seq{};      // The decl-specifier-seq
                SyntaxIndex identifier_list{};         // The list of identifiers: [ a, b, c ]
                ExprIndex initializer{};               // The expression initializer
                RefQualifierKind ref_qualifier_kind{}; // The ref-qualifier on the type (if any)
            };

            struct StructuredBindingIdentifier : Tag<SyntaxSort::StructuredBindingIdentifier> {
                ExprIndex identifier{}; // Identifier expression
                SourceLocation comma{}; // The source location of the ',' (if present)
            };

            struct AsmStatement : Tag<SyntaxSort::AsmStatement> {
                AsmStatement() : tokens{}, locus{} {}
                AsmStatement(SentenceIndex t, const SourceLocation& l) : tokens{t}, locus{l} {}
                SentenceIndex tokens;
                SourceLocation locus;
            };

            struct ReturnStatement : Tag<SyntaxSort::ReturnStatement> {
                enum class ReturnKind : std::uint8_t {
                    Return,
                    Co_return,
                };

                SentenceIndex pragma_tokens{}; // The index for any preceding #pragma tokens.
                ExprIndex expr{};              // The (optional) expression
                ReturnKind return_kind{};      // 'return' or 'co_return'
                SourceLocation return_locus{}; // The source location of the 'return' keyword
                SourceLocation semi_colon{};   // The source location of the ';'
            };

            struct CompoundStatement : Tag<SyntaxSort::CompoundStatement> {
                SentenceIndex pragma_tokens{}; // The index for any preceding #pragma tokens.
                SyntaxIndex statements{};      // The sub-statement(s)
                SourceLocation left_curly{};   // The source location of the '{'
                SourceLocation right_curly{};  // The source location of the '}'
            };

            struct IfStatement : Tag<SyntaxSort::IfStatement> {
                SentenceIndex pragma_tokens{};    // The index for any preceding #pragma tokens.
                SyntaxIndex init_statement{};     // The init-statement (opt)
                ExprIndex condition{};            // The condition
                SyntaxIndex if_true{};            // The sub-statement(s) to be executed if the condition is true
                SyntaxIndex if_false{};           // The sub-statement(s) to be executed if the condition is false
                SourceLocation if_keyword{};      // The source location of the 'if' keyword
                SourceLocation constexpr_locus{}; // The source location of the 'constexpr' keyword (if provided)
                SourceLocation else_keyword{};    // The source location of the 'else' keyword
            };

            struct IfConsteval : Tag<SyntaxSort::IfConsteval> {
                SentenceIndex pragma_tokens{};    // The index for any preceding #pragma tokens.
                SyntaxIndex if_true{};            // The sub-statement(s) to be executed if the condition is true
                SyntaxIndex if_false{};           // The sub-statement(s) to be executed if the condition is false
                SourceLocation if_keyword{};      // The source location of the 'if' keyword
                SourceLocation consteval_locus{}; // The source location of the 'consteval' keyword (if provided)
                SourceLocation not_locus{};       // The source location of the optional '!'
                SourceLocation else_keyword{};    // The source location of the 'else' keyword
            };

            struct WhileStatement : Tag<SyntaxSort::WhileStatement> {
                SentenceIndex pragma_tokens{};  // The index for any preceding #pragma tokens.
                ExprIndex condition{};          // The condition
                SyntaxIndex statement{};        // The sub-statement(s)
                SourceLocation while_keyword{}; // The source location of the 'while' keyword
            };

            struct DoWhileStatement : Tag<SyntaxSort::DoWhileStatement> {
                SentenceIndex pragma_tokens{};  // The index for any preceding #pragma tokens.
                ExprIndex condition{};          // The condition
                SyntaxIndex statement{};        // The sub-statement(s)
                SourceLocation do_keyword{};    // The source location of the 'do' keyword
                SourceLocation while_keyword{}; // The source location of the 'while' keyword
                SourceLocation semi_colon{};    // The source location of the ';'
            };

            struct ForStatement : Tag<SyntaxSort::ForStatement> {
                SentenceIndex pragma_tokens{}; // The index for any preceding #pragma tokens.
                SyntaxIndex init_statement{};  // The init-statement
                ExprIndex condition{};         // The condition
                ExprIndex expression{};        // The expression
                SyntaxIndex statement{};       // The sub-statement(s)
                SourceLocation for_keyword{};  // The source location of the 'for' keyword
                SourceLocation left_paren{};   // The source location of the '('
                SourceLocation right_paren{};  // The source location of the ')'
                SourceLocation semi_colon{};   // The source location of the 2nd ';'
            };

            struct InitStatement : Tag<SyntaxSort::InitStatement> {
                SentenceIndex pragma_tokens{};           // The index for any preceding #pragma tokens.
                SyntaxIndex expression_or_declaration{}; // Either an expression or a simple declaration
            };

            struct RangeBasedForStatement : Tag<SyntaxSort::RangeBasedForStatement> {
                SentenceIndex pragma_tokens{}; // The index for any preceding #pragma tokens.
                SyntaxIndex init_statement{};  // Ths init-statement (if any)
                SyntaxIndex declaration{};     // The declaration
                ExprIndex initializer{};       // The initializer
                SyntaxIndex statement{};       // The sub-statement(s)
                SourceLocation for_keyword{};  // The source location of the 'for' keyword
                SourceLocation left_paren{};   // The source location of the '('
                SourceLocation right_paren{};  // The source location of the ')'
                SourceLocation colon{};        // The source location of the ':'
            };

            struct ForRangeDeclaration : Tag<SyntaxSort::ForRangeDeclaration> {
                SyntaxIndex decl_specifier_seq{}; // The decl-specifier_seq
                SyntaxIndex declarator{};         // The declarator
            };

            struct LabeledStatement : Tag<SyntaxSort::LabeledStatement> {
                enum class Kind : std::uint8_t {
                    None,
                    Case,
                    Default,
                    Label,
                };
                SentenceIndex pragma_tokens{}; // The index for any preceding #pragma tokens.
                ExprIndex expression{};        // The expression for a case statement or the name for a label
                SyntaxIndex statement{};       // The sub-statement
                SourceLocation locus{};        // The source location of this label
                SourceLocation colon{};        // The source location of the ':'
                Kind kind{};                   // What kind of label do we have?
            };

            struct BreakStatement : Tag<SyntaxSort::BreakStatement> {
                SourceLocation break_keyword{}; // The source location of the 'break' keyword
                SourceLocation semi_colon{};    // The source location of the ';'
            };

            struct ContinueStatement : Tag<SyntaxSort::ContinueStatement> {
                SourceLocation continue_keyword{}; // The source location of the 'continue' keyword
                SourceLocation semi_colon{};       // The source location of the ';'
            };

            struct SwitchStatement : Tag<SyntaxSort::SwitchStatement> {
                SentenceIndex pragma_tokens{};   // The index for any preceding #pragma tokens.
                SyntaxIndex init_statement{};    // The init-statement (opt)
                ExprIndex condition{};           // The condition
                SyntaxIndex statement{};         // The sub-statement(s)
                SourceLocation switch_keyword{}; // The source location of the 'switch' keyword
            };

            struct GotoStatement : Tag<SyntaxSort::GotoStatement> {
                SentenceIndex pragma_tokens{}; // The index for any preceding #pragma tokens.
                TextOffset name{};             // The associated label
                SourceLocation locus{};        // The source location of the 'goto' keyword
                SourceLocation label{};        // The source location of the label;
                SourceLocation semi_colon{};   // The source location of the ';'
            };

            struct DeclarationStatement : Tag<SyntaxSort::DeclarationStatement> {
                SentenceIndex pragma_tokens{}; // The index for any preceding #pragma tokens.
                SyntaxIndex declaration{};     // The syntactic representation of the declaration
            };

            struct ExpressionStatement : Tag<SyntaxSort::ExpressionStatement> {
                SentenceIndex pragma_tokens{}; // The index for any preceding #pragma tokens.
                ExprIndex expression{};        // The expression
                SourceLocation semi_colon{};   // The source location of the ';'
            };

            struct TryBlock : Tag<SyntaxSort::TryBlock> {
                SentenceIndex pragma_tokens{}; // The index for any preceding #pragma tokens.
                SyntaxIndex statement{};       // The compound statement
                SyntaxIndex handler_seq{};     // The handlers
                SourceLocation try_keyword{};  // The source location of the 'try' keyword
            };

            struct Handler : Tag<SyntaxSort::Handler> {
                SentenceIndex pragma_tokens{};       // The index for any preceding #pragma tokens.
                SyntaxIndex exception_declaration{}; // The exception-declaration
                SyntaxIndex statement{};             // The compound statement
                SourceLocation catch_keyword{};      // The source location of the 'catch' keyword
                SourceLocation left_paren{};         // The source location of the '('
                SourceLocation right_paren{};        // The source location of the ')'
            };

            struct HandlerSeq : Tag<SyntaxSort::HandlerSeq> {
                SyntaxIndex handlers{}; // One of more catch handlers
            };

            struct FunctionTryBlock : Tag<SyntaxSort::FunctionTryBlock> {
                SyntaxIndex statement{};    // The compound statement)
                SyntaxIndex handler_seq{};  // The catch handlers
                SyntaxIndex initializers{}; // Any base-or-member initializers (if this is for a constructor)
            };

            struct TypeIdListElement : Tag<SyntaxSort::TypeIdListElement> {
                SyntaxIndex type_id{};     // The type-id
                SourceLocation ellipsis{}; // The source location of the '...' (if present)
            };

            struct DynamicExceptionSpec : Tag<SyntaxSort::DynamicExceptionSpec> {
                SyntaxIndex type_list{};        // The list of associated types
                SourceLocation throw_keyword{}; // The source location of the 'throw' keyword
                SourceLocation left_paren{};    // The source location of the '('
                SourceLocation
                    ellipsis{}; // The source location of the '...' (MS-only extension) 'type_list_' will be empty
                SourceLocation right_paren{}; // The source location of the ')'
            };

            struct StatementSeq : Tag<SyntaxSort::StatementSeq> {
                SyntaxIndex statements{}; // A sequence of statements
            };

            struct MemberFunctionDeclaration : Tag<SyntaxSort::MemberFunctionDeclaration> {
                SyntaxIndex definition{}; // The definition of this member function
            };

            struct FunctionDefinition : Tag<SyntaxSort::FunctionDefinition> {
                SyntaxIndex decl_specifier_seq{}; // The decl-specifier-seq for this function
                SyntaxIndex declarator{};         // The declarator for this function
                SyntaxIndex requires_clause{};    // The requires clause (if present)
                SyntaxIndex body{};               // The body of this function
            };

            struct FunctionBody : Tag<SyntaxSort::FunctionBody> {
                SyntaxIndex statements{};         // The statement(s)
                SyntaxIndex function_try_block{}; // The function-try-block (if any)
                SyntaxIndex initializers{};       // Any base-or-member initializers (if this is for a constructor)
                Keyword default_or_delete{};      // Is the function 'default'ed' or 'delete'ed'
                SourceLocation assign{};          // The source location of the '=' (if any)
                SourceLocation semi_colon{};      // THe source location of the ';' (if any)
            };

            struct Expression : Tag<SyntaxSort::Expression> {
                ExprIndex expression{};
            };

            struct TemplateParameterList : Tag<SyntaxSort::TemplateParameterList> {
                SyntaxIndex parameters{};      // Any template parameters
                SyntaxIndex requires_clause{}; // The requires clause (if present)
                SourceLocation left_angle{};   // The source location of the '<'
                SourceLocation right_angle{};  // The source location of the '>'
            };

            struct TemplateDeclaration : Tag<SyntaxSort::TemplateDeclaration> {
                SyntaxIndex parameters{};  // Any template parameters
                SyntaxIndex declaration{}; // The declaration under the template
                SourceLocation locus{};    // The source location of the 'template' keyword
            };

            struct RequiresClause : Tag<SyntaxSort::RequiresClause> {
                ExprIndex expression{}; // The expression for this requires clause
                SourceLocation locus{}; // The source location
            };

            struct SimpleRequirement : Tag<SyntaxSort::SimpleRequirement> {
                ExprIndex expression{}; // The expression for this requirement
                SourceLocation locus{}; // The source location
            };

            struct TypeRequirement : Tag<SyntaxSort::TypeRequirement> {
                ExprIndex type{};       // The type for this requirement
                SourceLocation locus{}; // The source location
            };

            struct CompoundRequirement : Tag<SyntaxSort::CompoundRequirement> {
                ExprIndex expression{};            // The expression for this requirement
                ExprIndex type_constraint{};       // The optional type constraint
                SourceLocation locus{};            // The source location
                SourceLocation right_curly{};      // The source location of the '}'
                SourceLocation noexcept_keyword{}; // The source location of the 'noexcept' keyword
            };

            struct NestedRequirement : Tag<SyntaxSort::NestedRequirement> {
                ExprIndex expression{}; // The expression for this requirement
                SourceLocation locus{}; // The source location
            };

            struct RequirementBody : Tag<SyntaxSort::RequirementBody> {
                SyntaxIndex requirements{};   // The requirements
                SourceLocation locus{};       // The source location
                SourceLocation right_curly{}; // The source location of the '}'
            };

            struct TypeTemplateParameter : Tag<SyntaxSort::TypeTemplateParameter> {
                TextOffset name{};              // The identifier for this template parameter
                ExprIndex type_constraint{};    // The optional type constraint
                SyntaxIndex default_argument{}; // The optional default argument
                SourceLocation locus{};         // The source location of the parameter
                SourceLocation ellipsis{};      // The source location of the '...' (if present)
            };

            struct TemplateTemplateParameter : Tag<SyntaxSort::TemplateTemplateParameter> {
                TextOffset name{};              // The identifier for this template template-parameter (if present)
                SyntaxIndex default_argument{}; // The optional default argument
                SyntaxIndex parameters{};       // The template parameters
                SourceLocation locus{};         // The source location of the 'template' keyword
                SourceLocation ellipsis{};      // The source location of the '...' (if present)
                SourceLocation comma{};         // The source location of the ',' (if present)
                Keyword type_parameter_key{};   // The type parameter key (typename|class)
            };

            struct TypeTemplateArgument : Tag<SyntaxSort::TypeTemplateArgument> {
                SyntaxIndex argument{};    // The template-argument
                SourceLocation ellipsis{}; // The source location of the '...' (if present)
                SourceLocation comma{};    // The source location of the ',' (if present)
            };

            struct NonTypeTemplateArgument : Tag<SyntaxSort::NonTypeTemplateArgument> {
                ExprIndex argument{};      // The template-argument
                SourceLocation ellipsis{}; // The source location of the '...' (if present)
                SourceLocation comma{};    // The source location of the ',' (if present)
            };

            struct TemplateArgumentList : Tag<SyntaxSort::TemplateArgumentList> {
                SyntaxIndex arguments{};      // Any template arguments
                SourceLocation left_angle{};  // The source location of the '<'
                SourceLocation right_angle{}; // The source location of the '>'
            };

            struct TemplateId : Tag<SyntaxSort::TemplateId> {
                SyntaxIndex name{};                // The name of the primary template (if it is not bound)
                ExprIndex symbol{};                // The symbol for the primary template (if it is bound)
                SyntaxIndex arguments{};           // The template argument list
                SourceLocation locus{};            // The source location
                SourceLocation template_keyword{}; // The source location of the optional 'template' keyword
            };

            struct MemInitializer : Tag<SyntaxSort::MemInitializer> {
                ExprIndex name{};          // The member or base to initialize
                ExprIndex initializer{};   // The initializer
                SourceLocation ellipsis{}; // The source location of the '...' (if present)
                SourceLocation comma{};    // The source location of the ',' (if present)
            };

            struct CtorInitializer : Tag<SyntaxSort::CtorInitializer> {
                SyntaxIndex initializers{}; // The set of mem-initializers
                SourceLocation colon{};     // The source location of the ':'
            };

            struct CaptureDefault : Tag<SyntaxSort::CaptureDefault> {
                SourceLocation locus{};       // The source location of this capture
                SourceLocation comma{};       // The source location of the ',' (if present)
                bool default_is_by_reference; // Is the capture-default '&' or '='?
            };

            struct SimpleCapture : Tag<SyntaxSort::SimpleCapture> {
                ExprIndex name{};           // The name of this simple-capture
                SourceLocation ampersand{}; // The source location of the '&' (if present)
                SourceLocation ellipsis{};  // The source location of the '...' (if present)
                SourceLocation comma{};     // The source location of the ',' (if present)
            };

            struct InitCapture : Tag<SyntaxSort::InitCapture> {
                ExprIndex name{};           // The name of the init-capture
                ExprIndex initializer{};    // The associated initializer
                SourceLocation ellipsis{};  // The source location of the '...' (if present)
                SourceLocation ampersand{}; // The source location of the '&' (if present)
                SourceLocation comma{};     // The source location of the ',' (if present)
            };

            struct ThisCapture : Tag<SyntaxSort::ThisCapture> {
                SourceLocation locus{};    // The source location of this capture
                SourceLocation asterisk{}; // The source location of the '*' capture (if present)
                SourceLocation comma{};    // The source location of the ',' (if present)
            };

            struct LambdaIntroducer : Tag<SyntaxSort::LambdaIntroducer> {
                SyntaxIndex captures{};         // Any associated captures
                SourceLocation left_bracket{};  // The source location of the '['
                SourceLocation right_bracket{}; // The source location of the ']'
            };

            struct LambdaDeclaratorSpecifier {
                enum class SpecifierSort : std::uint8_t {
                    None      = 0,
                    Mutable   = 1 << 0,
                    Constexpr = 1 << 1,
                    Consteval = 1 << 2,
                    Static    = 1 << 3,
                };
                SourceLocation locus{};
                SpecifierSort spec = SpecifierSort::None;
            };

            struct LambdaDeclarator : Tag<SyntaxSort::LambdaDeclarator> {
                SyntaxIndex parameters{};              // The parameters
                SyntaxIndex exception_specification{}; // The exception-specification (if present)
                SyntaxIndex trailing_return_type{};    // The trailing return type (if present)
                LambdaDeclaratorSpecifier spec{};      // Trailing specifiers, e.g. 'constexpr', 'mutable', etc. (if present)
                SourceLocation left_paren{};           // The source location of the '('
                SourceLocation right_paren{};          // The source location of the ')'
                SourceLocation ellipsis{};             // The source location of the '...' (if present)
            };

            struct UsingDeclaration : Tag<SyntaxSort::UsingDeclaration> {
                SyntaxIndex declarators{};      // The associated using-declarators
                SourceLocation using_keyword{}; // The source location of the 'using' keyword
                SourceLocation semi_colon{};    // The source location of the ';'
            };

            struct UsingEnumDeclaration : Tag<SyntaxSort::UsingEnumDeclaration> {
                ExprIndex name{};               // The specified enumeration
                SourceLocation using_keyword{}; // The source location of the 'using' keyword
                SourceLocation enum_keyword{};  // The source location of the 'enum' keyword
                SourceLocation semi_colon{};    // The source location of the ';'
            };

            struct UsingDeclarator : Tag<SyntaxSort::UsingDeclarator> {
                ExprIndex qualified_name{};        // The qualified-name
                SourceLocation typename_keyword{}; // The source location of the 'typename' (if present)
                SourceLocation ellipsis{};         // The source location of the '...' (if present)
                SourceLocation comma{};            // The source location of the ',' (if present)
            };

            struct UsingDirective : Tag<SyntaxSort::UsingDirective> {
                ExprIndex qualified_name{};         // The qualified-name being used
                SourceLocation using_keyword{};     // The source location of the 'using' keyword
                SourceLocation namespace_keyword{}; // The source location of the 'namespace' keyword
                SourceLocation semi_colon{};        // The source location of the ';'
            };

            struct NamespaceAliasDefinition : Tag<SyntaxSort::NamespaceAliasDefinition> {
                ExprIndex identifier{};             // The identifier
                ExprIndex namespace_name{};         // The name of the namespace
                SourceLocation namespace_keyword{}; // The source location of the 'namespace' keyword
                SourceLocation assign{};            // The source location of the '='
                SourceLocation semi_colon{};        // The source location of the ';'
            };

            struct ArrayIndex : Tag<SyntaxSort::ArrayIndex> {
                ExprIndex array{};              // The array expression
                ExprIndex index{};              // The index expression
                SourceLocation left_bracket{};  // The source location of the '['
                SourceLocation right_bracket{}; // The source location of the ']'
            };

            struct TypeTraitIntrinsic : Tag<SyntaxSort::TypeTraitIntrinsic> {
                SyntaxIndex arguments{}; // Any arguments;
                SourceLocation locus{};  // The source location of the intrinsic
                Operator intrinsic{};    // The index of the intrinsic
            };

            struct SEHTry : Tag<SyntaxSort::SEHTry> {
                SyntaxIndex statement{};      // The associated compound statement
                SyntaxIndex handler{};        // The associated handler
                SourceLocation try_keyword{}; // The source location of the '__try' keyword
            };

            struct SEHExcept : Tag<SyntaxSort::SEHExcept> {
                ExprIndex expression{};          // The associated expression
                SyntaxIndex statement{};         // The associated compound statement
                SourceLocation except_keyword{}; // The source location of the '__except' keyword
                SourceLocation left_paren{};     // The source location of the '('
                SourceLocation right_paren{};    // The source location of the ')'
            };

            struct SEHFinally : Tag<SyntaxSort::SEHFinally> {
                SyntaxIndex statement{};          // The associated compound statement
                SourceLocation finally_keyword{}; // The source location of the '__finally' keyword
            };

            struct SEHLeave : Tag<SyntaxSort::SEHLeave> {
                SourceLocation leave_keyword{}; // The source location of the '__leave' keyword
                SourceLocation semi_colon{};    // The source location of the ';'
            };

            struct Super : Tag<SyntaxSort::Super> {
                SourceLocation locus{}; // The source location of the '__super' keyword
            };

            enum class FoldKind : std::uint32_t { // FIXME: this should be std::uint8_t
                Unknown,
                LeftFold,  // Some form of left fold expression
                RightFold, // Some form of right fold expression
            };

            struct UnaryFoldExpression : Tag<SyntaxSort::UnaryFoldExpression> {
                FoldKind kind = FoldKind::Unknown; // FIXME: Move last
                ExprIndex expression{};            // The associated expression
                DyadicOperator op{};               // The operator kind
                SourceLocation locus{};            // The source location of the unary fold expression
                SourceLocation ellipsis{};         // The source location of the '...'
                SourceLocation operator_locus{};   // The source location of the operator
                SourceLocation right_paren{};      // The source location of the ')'
            };

            struct BinaryFoldExpression : Tag<SyntaxSort::BinaryFoldExpression> {
                FoldKind kind = FoldKind::Unknown;     // FIXME: Move last
                ExprIndex left_expression{};           // The left expression
                ExprIndex right_expression{};          // The right expression (if present)
                DyadicOperator op{};                   // The operator kind
                SourceLocation locus{};                // The source location of the fold expression
                SourceLocation ellipsis{};             // The source location of the '...'
                SourceLocation left_operator_locus{};  // The source location of the left operator
                SourceLocation right_operator_locus{}; // The source location of the right operator
                SourceLocation right_paren{};          // The source location of the ')'
            };

            struct EmptyStatement : Tag<SyntaxSort::EmptyStatement> {
                SourceLocation locus{}; // The source location of the ';'
            };

            struct AttributedStatement : Tag<SyntaxSort::AttributedStatement> {
                SentenceIndex pragma_tokens{}; // The index for any preceding #pragma tokens.
                SyntaxIndex statement{};       // The statement
                SyntaxIndex attributes{};      // The associated attributes
            };

            struct AttributedDeclaration : Tag<SyntaxSort::AttributedDeclaration> {
                SourceLocation locus{};    // The source location of the attributes
                SyntaxIndex declaration{}; // The declaration
                SyntaxIndex attributes{};  // The associated attributes
            };

            struct AttributeSpecifierSeq : Tag<SyntaxSort::AttributeSpecifierSeq> {
                SyntaxIndex attributes{}; // The sequence of attributes
            };

            struct AttributeSpecifier : Tag<SyntaxSort::AttributeSpecifier> {
                SyntaxIndex using_prefix{};       // The attribute using prefix (if any)
                SyntaxIndex attributes{};         // The attributes (if any)
                SourceLocation left_bracket_1{};  // The source location of the first '['
                SourceLocation left_bracket_2{};  // The source location of the second '['
                SourceLocation right_bracket_1{}; // The source location of the first ']'
                SourceLocation right_bracket_2{}; // The source location of the second ']'
            };

            struct AttributeUsingPrefix : Tag<SyntaxSort::AttributeUsingPrefix> {
                ExprIndex attribute_namespace{}; // The attribute namespace
                SourceLocation using_locus{};    // The source location of the 'using'
                SourceLocation colon{};          // The source location of the  ':'
            };

            struct Attribute : Tag<SyntaxSort::Attribute> {
                ExprIndex identifier{};          // The name of the attribute
                ExprIndex attribute_namespace{}; // The attribute namespace (if present)
                SyntaxIndex argument_clause{};   // The attribute argument clause (if present)
                SourceLocation double_colon{};   // The source location of the '::' (if present)
                SourceLocation ellipsis{};       // The source location of the '...' (if present)
                SourceLocation comma{};          // The source location of the ',' (if present)
            };

            struct AttributeArgumentClause : Tag<SyntaxSort::AttributeArgumentClause> {
                SentenceIndex tokens{};       // The argument tokens (if present)
                SourceLocation left_paren{};  // The source location of the '('
                SourceLocation right_paren{}; // The source location of the ')'
            };

            struct AlignasSpecifier : Tag<SyntaxSort::Alignas> {
                SyntaxIndex expression{};     // The expression of this alignas
                SourceLocation locus{};       // The source location of the alignas specifier
                SourceLocation left_paren{};  // The source location of the '('
                SourceLocation right_paren{}; // The source location of the ')'
            };

            // A sequence of zero or syntactic elements
            struct Tuple : Tag<SyntaxSort::Tuple>, Sequence<SyntaxIndex, HeapSort::Syntax> {
                Tuple() = default;
                Tuple(Index f, Cardinality n) : Sequence{f, n} {}
            };

            namespace microsoft {
                // There are a few operators that accept either a type or an expression -- or in some cases template
                // name. Usually such an operator will establish a default kind (choose either type or expression) and
                // have an indirection to encode the other kind, instead of maintaining an ambiguity to be resolved by a
                // side bit stored elsewhere.  The current encoding of MSVC's __uuidof extension maintains such an
                // ambiguity.
                struct TypeOrExprOperand {
                    union {
                        TypeIndex as_type;
                        ExprIndex as_expr;
                    };

                    void operator=(TypeIndex idx)
                    {
                        as_type = idx;
                    }
                    void operator=(ExprIndex idx)
                    {
                        as_expr = idx;
                    }
                };
                static_assert(sizeof(TypeOrExprOperand) == sizeof(Index));

                // the types of msvc-specific C++ syntax
                enum class Kind : std::uint8_t {
                    Unknown,
                    Declspec,          // __declspec( expression )
                    BuiltinAddressOf,  // __builtin_addressof( expression )
                    UUIDOfTypeID,      // __uuidof( typeid )
                    UUIDOfExpr,        // __uuidof( expression )
                    BuiltinHugeValue,  // __builtin_huge_val()
                    BuiltinHugeValuef, // __builtin_huge_valf()
                    BuiltinNan,        // __builtin_nan
                    BuiltinNanf,       // __builtin_nanf
                    BuiltinNans,       // __builtin_nans
                    BuiltinNansf,      // __builtin_nansf
                    IfExists,          // __if_exists ( id-expression ) brace-enclosed-region
                    Count
                };
                struct Declspec {               // __declspec( expression )
                    SourceLocation left_paren;  // location of '('
                    SourceLocation right_paren; // location of ')'
                    SentenceIndex tokens;       // token expression
                };
                struct BuiltinAddressOf { // __builtin_addressof( expression )
                    ExprIndex expr;       // inner expression
                };
                struct UUIDOf {                 // __uuidof( type-id ) | __uuidof( expression )
                    SourceLocation left_paren;  // location of '('
                    SourceLocation right_paren; // location of ')'
                    TypeOrExprOperand operand;  // kind: UUIDOfTypeId => TypeIndex; UUIDIfExpr => ExprIndex
                };
                struct Intrinsic {
                    SourceLocation left_paren;  // location of '('
                    SourceLocation right_paren; // location of ')'
                    ExprIndex argument;         // expression
                };
                struct IfExists {
                    enum class Kind : std::uint8_t {
                        Statement,
                        Initializer,
                        MemberDeclaration,
                    };
                    enum class Keyword : std::uint8_t {
                        IfExists,
                        IfNotExists,
                    };
                    Kind kind;                  // the declaration context of __if_exists
                    Keyword keyword;            // distinction between __if_exists and __if_not_exists
                    ExprIndex subject;          // the id-expression to be tested
                    SourceLocation left_paren;  // location of '('
                    SourceLocation right_paren; // location of ')'
                    SentenceIndex tokens;       // brace-enclosed region
                };

                struct VendorSyntax : Tag<SyntaxSort::VendorExtension> {
                    VendorSyntax() : ms_declspec{} {}
                    Kind kind = Kind::Unknown;
                    SourceLocation locus{}; // The root source location of the syntax
                    union {
                        Declspec ms_declspec;
                        BuiltinAddressOf ms_builtin_addressof;
                        UUIDOf ms_uuidof;
                        Intrinsic ms_intrinsic;
                        IfExists ms_if_exists;
                    };
                };
            } // namespace microsoft
        }     // namespace syntax

        // Location description of a source file + line pair
        struct FileAndLine {
            NameIndex file{};
            LineNumber line{};
            bool operator==(const FileAndLine&) const = default;
        };

        // Almost any C++ declaration can be parameterized, either directly (as a template),
        // or indirectly (when declared at the lexical scope of a template) without being template.
        struct ParameterizedEntity {
            DeclIndex decl{};     // The entity being parameterized.
            SentenceIndex head{}; // The sequence of words making up the declarative part of current instantiation.
            SentenceIndex body{}; // The sequence of words making up the body of this template
            SentenceIndex attributes{}; // Attributes of this template.
        };

        // Symbolic representation of a specialization request, whether implicit or explicit.
        struct SpecializationForm {
            DeclIndex template_decl{};
            ExprIndex arguments{};
        };

        // This structure is used to represent a body of a constexpr function.
        // In the future, it will be also used to represent the bodies of inline functions and function templates.
        // It is stored in the traits partition indexed by the DeclIndex of its FunctionDecl,
        // NonStaticMemberFunctionDecl or ConstructorDecl.
        // TODO: initializer is a bit out of place here for a mapping. Fix it or learn to live with it.
        struct MappingDefinition {
            ChartIndex parameters{};  // Function parameter list (if any) in the defining declaration
            ExprIndex initializers{}; // Any base-or-member initializers (if this is for a constructor)
            StmtIndex body{};         // The body of the function
        };

        template<typename T>
        struct Identity {
            T name{};               // The name of the entity (either 'NameIndex' or 'TextOffset')
            SourceLocation locus{}; // Source location of this entity
        };

        // Symbolic representation of a function declaration.
        struct FunctionDecl : Tag<DeclSort::Function> {
            Identity<NameIndex> identity{};     // The name and location of this function
            TypeIndex type{};                   // Sort and index of this decl's type.  Null means no type.
            DeclIndex home_scope{};             // Enclosing scope of this declaration.
            ChartIndex chart{};                 // Function parameter list.
            FunctionTraits traits{};            // Function traits
            BasicSpecifiers basic_spec{};       // Basic declaration specifiers.
            Access access{};                    // Access right to this function.
            ReachableProperties properties{};   // Set of reachable properties of this declaration
        };

        static_assert(offsetof(FunctionDecl, identity) == 0,
                      "The name population code expects 'identity' to be at offset zero");

        struct IntrinsicDecl : Tag<DeclSort::Intrinsic> {
            Identity<TextOffset> identity; // // The name and location of this intrinsic
            TypeIndex type;                // Sort and index of this decl's type.  Null means no type.
            DeclIndex home_scope;          // Enclosing scope of this declaration.
            BasicSpecifiers basic_spec;    // Basic declaration specifiers.
            Access access;                 // Access right to this function.
            FunctionTraits
                traits; // Function traits.  TODO: Move this to the logical location for next major IFC version.
        };

        static_assert(offsetof(IntrinsicDecl, identity) == 0,
                      "The name population code expects 'identity' to be at offset zero");

        // An enumerator declaration.
        struct EnumeratorDecl : Tag<DeclSort::Enumerator> {
            Identity<TextOffset> identity{};    // The name and location of this enumerator
            TypeIndex type{};                   // Sort and index of this decl's type.  Null means no type.
            ExprIndex initializer{};            // Sort and index of this declaration's initializer, if any.  Null means absent.
            BasicSpecifiers basic_spec{};       // Basic declaration specifiers.
            Access access{};                    // Access right to this enumerator.
        };

        // A strongly-typed abstraction of ExprIndex which always points to ExprSort::NamedDecl.
        enum class DefaultIndex : std::uint32_t {
            UnderlyingSort = ifc::to_underlying(ExprSort::NamedDecl),
        };

        inline ExprIndex as_expr_index(DefaultIndex index)
        {
            if (index_like::null(index))
                return {};
            // DefaultIndex == 1 starts us at offset 0, so we retract the index to get the offset.
            const auto n        = index_like::pointed<DefaultIndex>::retract(index);
            using SortType      = ExprIndex::SortType;
            constexpr auto sort = SortType{ifc::to_underlying(DefaultIndex::UnderlyingSort)};
            return index_like::make<ExprIndex>(sort, n);
        }

        inline DefaultIndex as_default_index(ExprIndex index)
        {
            if (null(index))
                return {};
            // Since this is an offset into a known partition, we only need its value.
            return index_like::pointed<DefaultIndex>::inject(ifc::to_underlying(index.index()));
        }

        struct ParameterDecl : Tag<DeclSort::Parameter> {
            Identity<TextOffset> identity{};  // The name and location of this function parameter
            TypeIndex type{};                 // Sort and index of this decl's type.  Null means no type.
            ExprIndex type_constraint{};      // Optional type-constraint on the parameter type.
            DefaultIndex initializer{};       // Default argument. Null means none was provided.
            std::uint32_t level{};                 // The nesting depth of this parameter (template or function).
            std::uint32_t position{};              // The 1-based position of this parameter.
            ParameterSort sort{};             // The kind of parameter.
            ReachableProperties properties{}; // The set of semantic properties reaching to outside importers.
        };

        // A variable declaration, including static data members.
        struct VariableDecl : Tag<DeclSort::Variable> {
            Identity<NameIndex> identity{};     // The name and location of this variable
            TypeIndex type{};                   // Sort and index of this decl's type.  Null means no type.
            DeclIndex home_scope{};             // Enclosing scope of this declaration.
            ExprIndex initializer{};            // Sort and index of this declaration's initializer, if any.  Null means absent.
            ExprIndex alignment{};              // If non-zero, explicit alignment specification, e.g. alignas(N).
            ObjectTraits obj_spec{};            // Object traits associated with this variable.
            BasicSpecifiers basic_spec{};       // Basic declaration specifiers.
            Access access{};                    // Access right to this variable.
            ReachableProperties properties{};   // The set of semantic properties reaching to outside importers.
        };

        static_assert(offsetof(VariableDecl, identity) == 0,
                      "The name population code expects 'identity' to be at offset zero");

        // A non-static data member declaration that is not a bitfield.
        struct FieldDecl : Tag<DeclSort::Field> {
            Identity<TextOffset> identity{};    // The name and location of this non-static data member
            TypeIndex type{};                   // Sort and index of this decl's type.  Null means no type.
            DeclIndex home_scope{};             // Enclosing scope of this declaration.
            ExprIndex initializer{};            // The field initializer (if any)
            ExprIndex alignment{};              // If non-zero, explicit alignment specification, e.g. alignas(N).
            ObjectTraits obj_spec{};            // Object traits associated with this field.
            BasicSpecifiers basic_spec{};       // Basic declaration specifiers.
            Access access{};                    // Access right to this data member.
            ReachableProperties properties{};   // Set of reachable semantic properties of this declaration
        };

        // A bitfield declaration.
        struct BitfieldDecl : Tag<DeclSort::Bitfield> {
            Identity<TextOffset> identity{};    // The name and location of this bitfield
            TypeIndex type{};                   // Sort and index of this decl's type.  Null means no type.
            DeclIndex home_scope{};             // Enclosing scope of this declaration.
            ExprIndex width{};                  // Number of bits requested for this bitfield.
            ExprIndex initializer{};            // Sort and index of this declaration's initializer, if any.  Null means absent.
            ObjectTraits obj_spec{};            // Object traits associated with this field.
            BasicSpecifiers basic_spec{};       // Basic declaration specifiers.
            Access access{};                    // Access right to this bitfield.
            ReachableProperties properties{};   // Set of reachable semantic properties of this declaration
        };

        // A declaration for a named scope, e.g. class, namespaces.
        struct ScopeDecl : Tag<DeclSort::Scope> {
            Identity<NameIndex> identity{};     // The name and location of this scope (class or namespace)
            TypeIndex type{};                   // Sort index of this decl's type.
            TypeIndex base{};                   // Base type(s) in class inheritance.
            ScopeIndex initializer{};           // Type definition (i.e. initializer) of this type declaration.
            DeclIndex home_scope{};             // Enclosing scope of this declaration.
            ExprIndex alignment{};              // If non-zero, explicit alignment specification, e.g. alignas(N).
            PackSize pack_size{};               // The pack size value applied to all members of this class (#pragma pack(n))
            BasicSpecifiers basic_spec{};       // Basic declaration specifiers.
            ScopeTraits scope_spec{};           // Property of this scope type declaration.
            Access access{};                    // Access right to the name of this user-defined type.
            ReachableProperties properties{};   // The set of semantic properties reaching to outside importers.
        };

        // A declaration for an enumeration type.
        struct EnumerationDecl : Tag<DeclSort::Enumeration> {
            Identity<TextOffset> identity{};        // The name and location of this enumeration
            TypeIndex type{};                       // Sort index of this decl's type.
            TypeIndex base{};                       // Enumeration base type.
            Sequence<EnumeratorDecl> initializer{}; // Type definition (i.e. initializer) of this type declaration.
            DeclIndex home_scope{};                 // Enclosing scope of this declaration.
            ExprIndex alignment{};                  // If non-zero, explicit alignment specification, e.g. alignas(N).
            BasicSpecifiers basic_spec{};           // Basic declaration specifiers.
            Access access{};                        // Access right to this enumeration.
            ReachableProperties properties{};       // The set of semantic properties reaching to outside importers.
        };

        static_assert(offsetof(EnumerationDecl, identity) == 0,
                      "The name population code expects 'identity' to be at offset zero");

        // A general alias declaration for a type.
        // E.g.
        //     using T = int;
        //     namespace N = VeryLongCompanyName::NiftyDivision::AwesomeDepartment;
        //     template<typename T>
        //        using Ptr = T*;
        // are all (type) alias declarations in the IFC.
        struct AliasDecl : Tag<DeclSort::Alias> {
            Identity<TextOffset> identity{};// The name and location of this alias
            TypeIndex type{};               // The type of this alias (TypeBasis::Typename for conventional C++ type,
                                            // TypeBasis::Namespace for a namespace, TypeSort::Forall for alias template etc.)
            DeclIndex home_scope{};         // Enclosing scope of this declaration.
            TypeIndex aliasee{};            // The type this declaration is introducing a name for.
            BasicSpecifiers basic_spec{};   // Basic declaration specifiers.
            Access access{};                // Access right to the name of this type alias.
        };

        static_assert(offsetof(AliasDecl, identity) == 0,
                      "The name population code expects 'identity' to be at offset zero");

        // A temploid declaration
        struct TemploidDecl : Tag<DeclSort::Temploid>, ParameterizedEntity {
            ChartIndex chart{};                 // The enclosing set template parameters of this entity.
            ReachableProperties properties{};   // The set of semantic properties reaching to outside importers.
        };

        // The core of a template.
        struct Template {
            Identity<NameIndex> identity{}; // What identifies this template
            DeclIndex home_scope{};         // Enclosing scope of this declaration.
            ChartIndex chart{};             // Template parameter list.
            ParameterizedEntity entity{};   // The core parameterized entity.
        };

        static_assert(offsetof(Template, identity) == 0,
                      "The name population code expects 'identity' to be at offset zero");

        // A template declaration.
        struct TemplateDecl : Tag<DeclSort::Template>, Template {
            TypeIndex type{};                   // The type of the parameterized entity.  FIXME: In some sense this is redundant with
                                                // `entity' but VC's internal representation is currently too irregular.
            BasicSpecifiers basic_spec{};       // Basic declaration specifiers.
            Access access{};                    // Access right to the name of this template declaration.
            ReachableProperties properties{};   // The set of semantic properties reaching to outside importers.
        };

        // A specialization of a template.
        struct PartialSpecializationDecl : Tag<DeclSort::PartialSpecialization>, Template {
            SpecFormIndex specialization_form{}; // The specialization pattern: primary template + argument list.
            BasicSpecifiers basic_spec{};        // Basic declaration specifiers.
            Access access{};                     // Access right to the name of this template specialization.
            ReachableProperties properties{};    // The set of semantic properties reaching to outside importers.
        };

        enum class SpecializationSort : std::uint8_t {
            Implicit,      // An implicit specialization
            Explicit,      // An explicit specialization (user provided)
            Instantiation, // An explicit instantiation (user provided)
        };

        struct SpecializationDecl : Tag<DeclSort::Specialization> {
            SpecFormIndex specialization_form{}; // The specialization pattern: primary template + argument list.
            DeclIndex decl{};                    // The entity declared by this specialization.
            SpecializationSort sort{};           // The specialization category.
            BasicSpecifiers basic_spec{};
            Access access{};
            ReachableProperties properties{};
        };

        struct DefaultArgumentDecl : Tag<DeclSort::DefaultArgument> {
            SourceLocation locus{};     // The location of this default argument decl.
            TypeIndex type{};           // The type of the default argument decl.
            DeclIndex home_scope {};    // Enclosing scope of this declaration.
            ExprIndex initializer{};    // The expression used to initialize the accompanying parameter when this default
                                        // argument is used in a call expression.
            BasicSpecifiers basic_spec{};
            Access access{};
            ReachableProperties properties{};
        };

        // A concept.
        struct ConceptDecl : Tag<DeclSort::Concept> {
            Identity<TextOffset> identity{};    // What identifies this concept.
            DeclIndex home_scope{};             // Enclosing scope of this declaration.
            TypeIndex type{};                   // The type of the parameterized entity.
            ChartIndex chart{};                 // Template parameter list.
            ExprIndex constraint{};             // The associated constraint-expression.
            BasicSpecifiers basic_spec{};       // Basic declaration specifiers.
            Access access{};                    // Access right to the name of this template declaration.
            SentenceIndex head{};               // The sequence of words making up the declarative part of current instantiation.
            SentenceIndex body{};               // The sequence of words making up the body of this concept.
        };

        static_assert(offsetof(Template, identity) == 0,
                      "The name population code expects 'identity' to be at offset zero");

        // A non-static member function declaration.
        struct NonStaticMemberFunctionDecl : Tag<DeclSort::Method> {
            Identity<NameIndex> identity{}; // What identifies this non-static member function
            TypeIndex type{};               // Sort and index of this decl's type.  Null means no type.
            DeclIndex home_scope{};         // Enclosing scope of this declaration.
            ChartIndex chart{};             // Function parameter list.
            FunctionTraits traits{};        // Function traits ('const' etc.)
            BasicSpecifiers basic_spec{};   // Basic declaration specifiers.
            Access access{};                // Access right to this member.
            ReachableProperties properties{}; // Set of reachable semantic properties of this declaration
        };

        // A constructor declaration.
        struct ConstructorDecl : Tag<DeclSort::Constructor> {
            Identity<TextOffset> identity{}; // What identifies this constructor
            TypeIndex type{};                // Type of this constructor declaration.
            DeclIndex home_scope{};          // Enclosing scope of this declaration.
            ChartIndex chart{};              // Function parameter list.
            FunctionTraits traits{};         // Function traits
            BasicSpecifiers basic_spec{};    // Basic declaration specifiers.
            Access access{};                 // Access right to this constructor.
            ReachableProperties properties{}; // Set of reachable semantic properties of this declaration
        };

        // A constructor declaration.
        struct InheritedConstructorDecl : Tag<DeclSort::InheritedConstructor> {
            Identity<TextOffset> identity{}; // What identifies this constructor
            TypeIndex type{};                // Type of this constructor declaration.
            DeclIndex home_scope{};          // Enclosing scope of this declaration.
            ChartIndex chart{};              // Function parameter list.
            FunctionTraits traits{};         // Function traits
            BasicSpecifiers basic_spec{};    // Basic declaration specifiers.
            Access access{};                 // Access right to this constructor.
            DeclIndex base_ctor{};           // The base class ctor this ctor references.
        };

        // A destructor declaration.
        struct DestructorDecl : Tag<DeclSort::Destructor> {
            Identity<TextOffset> identity{};    // What identifies this destructor
            DeclIndex home_scope{};             // Enclosing scope of this declaration.
            NoexceptSpecification eh_spec{};    // Exception specification.
            FunctionTraits traits{};            // Function traits.
            BasicSpecifiers basic_spec{};       // Basic declaration specifiers.
            Access access{};                    // Access right to this destructor.
            CallingConvention convention{};     // Calling convention.
            ReachableProperties properties{};   // Set of reachable semantic properties of this declaration
        };

        // A deduction guide for class template.
        // Note: If the deduction guide was parameterized by a template, then this would be
        // the corresponding parameterized decl.  The template declaration itself has no name.
        struct DeductionGuideDecl : Tag<DeclSort::DeductionGuide> {
            Identity<NameIndex> identity{}; // associated primary template and location of declaration
            DeclIndex home_scope{};         // Enclosing scope of this declaration
            ChartIndex source{};            // Function parameters mentioned in the deduction guide declaration
            ExprIndex target{};             // Pattern for the template-argument list to deduce
            GuideTraits traits{};           // Deduction guide traits.
            BasicSpecifiers basic_spec{};   // Basic declaration specifiers.
        };

        // Declaration introducing no names, e.g. static_assert, asm-declaration, empty-declaration
        struct BarrenDecl : Tag<DeclSort::Barren> {
            DirIndex directive{};
            BasicSpecifiers basic_spec{};
            Access access{};
        };

        // A reference to a declaration from an imported module unit.
        // See also bug https://github.com/microsoft/ifc-spec/issues/69
        struct ReferenceDecl : Tag<DeclSort::Reference> {
            ModuleReference translation_unit{}; // The owning TU of this decl.
            DeclIndex local_index{};            // The core declaration.
        };

        // A property declaration -- VC extensions.
        struct PropertyDecl : Tag<DeclSort::Property> {
            DeclIndex data_member{};      // The pseudo data member that represents this property
            TextOffset get_method_name{}; // The name of the 'get' method
            TextOffset set_method_name{}; // The name of the 'set' method
        };

        struct SegmentDecl : Tag<DeclSort::OutputSegment> {
            TextOffset name{};          // offset of name's text in the string table.
            TextOffset class_id{};      // Class ID of the segment.
            SegmentTraits seg_spec{};   // Attributes collected from #pragmas, for linker use.
            SegmentType type{};         // The type of segment.
        };

        struct UsingDecl : Tag<DeclSort::Using> {
            Identity<TextOffset> identity{};    // What identifies this using declaration
            DeclIndex home_scope{};             // Enclosing scope of this declaration.
            DeclIndex resolution{};             // Designates the used set of declarations.
            ExprIndex parent{};                 // If this is a member using declaration then this is the introducing base-class
            TextOffset name{};                  // If this is a member using declaration then this is the name of member
            BasicSpecifiers basic_spec{};       // Basic declaration specifiers.
            Access access{};                    // If this is a member using declaration then this is its access
            bool is_hidden{};                   // Is this using-declaration hidden?
        };

        struct ProlongationDecl : Tag<DeclSort::Prolongation> {
            Identity<NameIndex> identity{};
            DeclIndex enclosing_scope{};   // Enclosing scope of this declaration.
            DeclIndex home_scope{};        // The scope to which this decl belongs.
            DeclIndex original_decl{};     // The original decl in home_scope.
        };

        struct FriendDecl : Tag<DeclSort::Friend> {
            ExprIndex index{}; // The expression representing a reference to the declaration.
                               // Note: most of the time this is a NamedDeclExpression but it
                               // can also be a TemplateIdExpression in the case of the friend
                               // decl being referenced is some specialization of a template.
        };

        struct ExpansionDecl : Tag<DeclSort::Expansion> {
            SourceLocation locus{};     // Location of the expansion
            DeclIndex operand{};        // The declaration to expand
        };

        struct SyntacticDecl : Tag<DeclSort::SyntaxTree> {
            SyntaxIndex index{};
        };

        // A sequence of zero or more entities
        struct TupleDecl : Tag<DeclSort::Tuple>, Sequence<DeclIndex, HeapSort::Decl> {
            TupleDecl() = default;
            TupleDecl(Index f, Cardinality n) : Sequence{f, n} {}
        };

        static_assert(offsetof(UsingDecl, identity) == 0,
                      "The name population code expects 'identity' to be at offset zero");

        struct VendorDecl : Tag<DeclSort::VendorExtension> {
            VendorIndex index{};
        };

        // A sequence of declaration indices.
        struct Scope : Sequence<Declaration> {};

        struct UnilevelChart : Tag<ChartSort::Unilevel>, Sequence<ParameterDecl> {
            ExprIndex requires_clause = {};
        };

        struct MultiChart : Tag<ChartSort::Multilevel>, Sequence<ChartIndex, HeapSort::Chart> {};

        // Note: Every expression has a source location span, and a type.
        // Note: This class captures that commonality. It is parameterized by any sort,
        //       but that parameterization is more for convenience than fundamental.
        //       If the many isomorphic specializations are demonstrated to induce undue
        //       compile-time memory pressure, then this parameterization can be refactored
        //       into the parameterized empty class Tag + the non-parameterized class.
        template<auto s>
        struct LocationAndType : Tag<s> {
            SourceLocation locus{}; // The source location (span) of this node.
            TypeIndex type{};       // The (possibly generalized) type of this node.
        };

        // Like the structure above, but drops the 'type' field in favor of the node deriving the
        // type from its children.  This can be useful in compressing the on-disk representation
        // due to collapsing redundant semantic information.
        template<auto s>
        struct Location : Tag<s> {
            SourceLocation locus{}; // The source location of this node.
        };

        // Note: In IPR, every statement inherits from Expr which also provides a type and location
        // for the deriving Stmt nodes.

        // A sequence of zero or more statements. Used to represent blocks/compound statements.
        struct BlockStmt : Location<StmtSort::Block>, Sequence<StmtIndex, HeapSort::Stmt> {};

        struct TryStmt : Location<StmtSort::Try>, Sequence<StmtIndex, HeapSort::Stmt> {
            StmtIndex handlers{}; // The handler statement or tuple of handler statements.
        };

        struct ExpressionStmt : Location<StmtSort::Expression> {
            ExprIndex expr{};
        };

        struct IfStmt : Location<StmtSort::If> {
            StmtIndex init{};
            StmtIndex condition{};
            StmtIndex consequence{};
            StmtIndex alternative{};
        };

        struct WhileStmt : Location<StmtSort::While> {
            StmtIndex condition{};
            StmtIndex body{};
        };

        struct DoWhileStmt : Location<StmtSort::DoWhile> {
            ExprIndex condition{}; // Grammatically, this can only ever be an expression.
            StmtIndex body{};
        };

        struct ForStmt : Location<StmtSort::For> {
            StmtIndex init{};
            StmtIndex condition{};
            StmtIndex increment{};
            StmtIndex body{};
        };

        struct BreakStmt : Location<StmtSort::Break> {};

        struct ContinueStmt : Location<StmtSort::Continue> {};

        struct GotoStmt : Location<StmtSort::Goto> {
            ExprIndex target{};
        };

        struct SwitchStmt : Location<StmtSort::Switch> {
            StmtIndex init{};
            ExprIndex control{};
            StmtIndex body{};
        };

        struct LabeledStmt : LocationAndType<StmtSort::Labeled> {
            ExprIndex label{}; // The label is an expression to offer more flexibility in the types of labels allowed.
                               // Note: a null index here indicates that this is the default case label in a switch
                               // statement.
            StmtIndex statement{}; // The statement associated with this label.
        };

        struct DeclStmt : Location<StmtSort::Decl> {
            DeclIndex decl{};
        };

        struct ReturnStmt : LocationAndType<StmtSort::Return> {
            ExprIndex expr{};
            TypeIndex function_type{};
        };

        struct HandlerStmt : Location<StmtSort::Handler> {
            DeclIndex exception{};
            StmtIndex body{};
        };

        struct ExpansionStmt : Location<StmtSort::Expansion> {
            StmtIndex operand{}; // The statement to expand
        };

        struct TupleStmt : LocationAndType<StmtSort::Tuple>, Sequence<StmtIndex, HeapSort::Stmt> {};

        struct DirStmt : Tag<StmtSort::Dir> {
            DirIndex directive{};
        };

        // The location information will be stored in the vendor-specific structure.
        struct VendorStmt : Tag<StmtSort::VendorExtension> {
            VendorIndex index{};
        };

        // Expression trees for integer constants below this threshold are directly represented by their indices.
        inline constexpr auto immediate_upper_bound = std::uint64_t(1) << index_like::index_precision<ExprSort>;

        struct StringLiteral {
            TextOffset start{}; // beginning of the first byte in this string literal
            Cardinality size{}; // The number of bytes in the object representation, including terminator(s).
            TextOffset suffix{}; // Suffix, if any.
        };

        struct TypeExpr : LocationAndType<ExprSort::Type> {
            TypeIndex denotation{};
        };

        struct StringExpr : LocationAndType<ExprSort::String> {
            StringIndex string{}; // The string literal
        };

        struct FunctionStringExpr : LocationAndType<ExprSort::FunctionString> {
            TextOffset macro{}; // The name of the macro identifier (e.g. __FUNCTION__)
        };

        struct CompoundStringExpr : LocationAndType<ExprSort::CompoundString> {
            TextOffset prefix{}; // The name of the prefix identifier (e.g. __LPREFIX)
            ExprIndex string{};  // The parenthesized string literal
        };

        struct StringSequenceExpr : LocationAndType<ExprSort::StringSequence> {
            ExprIndex strings{}; // The string literals in the sequence
        };

        struct UnresolvedIdExpr : LocationAndType<ExprSort::UnresolvedId> {
            NameIndex name{};
        };

        struct TemplateIdExpr : LocationAndType<ExprSort::TemplateId> {
            ExprIndex primary_template{};
            ExprIndex arguments{};
        };

        struct TemplateReferenceExpr : LocationAndType<ExprSort::TemplateReference> {
            DeclIndex member{};
            NameIndex member_name{};
            TypeIndex parent{};             // The enclosing concrete specialization
            ExprIndex template_arguments{}; // Any associated template arguments
        };

        // Use of a declared identifier as an expression.
        struct NamedDeclExpr : LocationAndType<ExprSort::NamedDecl> {
            DeclIndex decl{}; // Declaration referenced by the name.
        };

        struct LiteralExpr : LocationAndType<ExprSort::Literal> {
            LitIndex value{};
        };

        struct EmptyExpr : LocationAndType<ExprSort::Empty> {};

        struct PathExpr : LocationAndType<ExprSort::Path> {
            ExprIndex scope{};
            ExprIndex member{};
        };

        // Read from a symbolic address.  When the address is an id-expression, the read expression
        // symbolizes an lvalue-to-rvalue conversion.
        struct ReadExpr : LocationAndType<ExprSort::Read> {
            enum class Kind : std::uint8_t {
                Unknown,            // Unknown
                Indirection,        // Dereference a pointer, e.g. *p
                RemoveReference,    // Convert a reference into an lvalue
                LvalueToRvalue,     // lvalue-to-rvalue conversion
                IntegralConversion, // FIXME: no longer needed
                Count
            };

            ExprIndex child{};
            Kind kind{Kind::Unknown};
        };

        struct MonadicExpr : LocationAndType<ExprSort::Monad> {
            DeclIndex impl{}; // An associated set of decls with this operator.  This set
                              // is obtained through the first phase of 2-phase name lookup.
            ExprIndex arg[1]{};
            MonadicOperator assort{};
        };

        struct DyadicExpr : LocationAndType<ExprSort::Dyad> {
            DeclIndex impl{}; // An associated set of decls with this operator.  This set
                              // is obtained through the first phase of 2-phase name lookup.
            ExprIndex arg[2]{};
            DyadicOperator assort{};
        };

        struct TriadicExpr : LocationAndType<ExprSort::Triad> {
            DeclIndex impl{}; // An associated set of decls with this operator.  This set
                              // is obtained through the first phase of 2-phase name lookup.
            ExprIndex arg[3]{};
            TriadicOperator assort{};
        };

        struct HierarchyConversionExpr : LocationAndType<ExprSort::HierarchyConversion> {
            ExprIndex source{};
            TypeIndex target{};
            ExprIndex inheritance_path{}; // Path from source class to destination class. For conversion of the 'this'
                                          // argument to a virtual call, this is the path to the introducing base class.
            ExprIndex override_inheritance_path{}; // Path from source class to destination class. Usually the same as
                                                   // inheritance_path, but for conversion of the 'this' argument to
                                                   // a virtual call, this is the path to the base class whose member
                                                   // was selected at compile time, which may be an override of the
                                                   // introducing function.
            DyadicOperator assort{};               // The class hierarchy navigation operation
        };

        struct DestructorCallExpr : LocationAndType<ExprSort::DestructorCall> {
            enum class Kind : std::uint8_t {
                Unknown,
                Destructor,
                Finalizer,
            };
            ExprIndex name{};                 // The name of the object being destructor (if present)
            SyntaxIndex decltype_specifier{}; // The decltype specifier (if present)
            Kind kind = Kind::Unknown;        // Do we have a destructor or a finalizer?
        };

        // A sequence of zero of more expressions.
        struct TupleExpr : LocationAndType<ExprSort::Tuple>, Sequence<ExprIndex, HeapSort::Expr> {};

        struct PlaceholderExpr : LocationAndType<ExprSort::Placeholder> {};

        struct ExpansionExpr : LocationAndType<ExprSort::Expansion> {
            ExprIndex operand{}; // The expression to expand
        };

        // A stream of tokens (used for 'complex' default arguments)
        struct TokenExpr : LocationAndType<ExprSort::Tokens> {
            SentenceIndex tokens{};
        };

        struct CallExpr : LocationAndType<ExprSort::Call> {
            ExprIndex function{};  // The function we are calling
            ExprIndex arguments{}; // The arguments to the function call
        };

        struct TemporaryExpr : LocationAndType<ExprSort::Temporary> {
            std::uint32_t index{}; // The index that uniquely identifiers this temporary object
        };

        struct DynamicDispatchExpr : LocationAndType<ExprSort::DynamicDispatch> {
            ExprIndex postfix_expr{}; // The postfix expression on a call to a virtual function.
        };

        struct VirtualFunctionConversionExpr : LocationAndType<ExprSort::VirtualFunctionConversion> {
            DeclIndex function{}; // The virtual function referenced.
        };

        struct RequiresExpr : LocationAndType<ExprSort::Requires> {
            SyntaxIndex parameters{}; // The (optional) parameters
            SyntaxIndex body{};       // The requirement body
        };

        enum class Associativity : std::uint8_t {
            Unspecified,
            Left,
            Right,
        };

        struct UnaryFoldExpr : LocationAndType<ExprSort::UnaryFold> {
            ExprIndex expr{};    // The associated expression
            DyadicOperator op{}; // The operator kind
            Associativity assoc = Associativity::Unspecified;
        };

        struct BinaryFoldExpr : LocationAndType<ExprSort::BinaryFold> {
            ExprIndex left{};    // The left expression
            ExprIndex right{};   // The right expression (if present)
            DyadicOperator op{}; // The operator kind
            Associativity assoc = Associativity::Unspecified;
        };

        struct StatementExpr : LocationAndType<ExprSort::Statement> {
            StmtIndex stmt{};
        };

        struct TypeTraitIntrinsicExpr : LocationAndType<ExprSort::TypeTraitIntrinsic> {
            TypeIndex arguments{};
            Operator intrinsic{};
        };

        struct MemberInitializerExpr : LocationAndType<ExprSort::MemberInitializer> {
            DeclIndex member{};
            TypeIndex base{};
            ExprIndex expression{};
        };

        struct MemberAccessExpr : LocationAndType<ExprSort::MemberAccess> {
            ExprIndex offset{}; // The offset of the member
            TypeIndex parent{}; // The type of the parent
            TextOffset name{};  // The name of the non-static data member
        };

        struct InheritancePathExpr : LocationAndType<ExprSort::InheritancePath> {
            ExprIndex path{};
        };

        struct InitializerListExpr : LocationAndType<ExprSort::InitializerList> {
            ExprIndex elements{};
        };

        struct InitializerExpr : LocationAndType<ExprSort::Initializer> {
            enum class Kind : std::uint8_t {
                Unknown,
                DirectInitialization,
                CopyInitialization,
            };
            ExprIndex initializer{};   // The initializer itself
            Kind kind = Kind::Unknown; // What kind of initialization is this?
        };

        // Any form of cast expression
        struct CastExpr : LocationAndType<ExprSort::Cast> {
            ExprIndex source{}; // The expression being cast
            TypeIndex target{}; // Target type of this cast-expression
            DyadicOperator assort{};
        };

        // A condition: currently this just supports a simple condition 'if (e)' but we will need to enhance it to
        // support more complex conditions 'if (T v = e)'
        struct ConditionExpr : LocationAndType<ExprSort::Condition> {
            ExprIndex expression{};
        };

        struct SimpleIdentifierExpr : LocationAndType<ExprSort::SimpleIdentifier> {
            NameIndex name{};
        };

        struct PointerExpr : Tag<ExprSort::Pointer> {
            SourceLocation locus{};
        };

        struct UnqualifiedIdExpr : LocationAndType<ExprSort::UnqualifiedId> {
            NameIndex name{};                  // The name of the identifier
            ExprIndex symbol{};                // The bound symbol (if any)
            SourceLocation template_keyword{}; // The source location of the 'template' keyword (if any)
        };

        struct QualifiedNameExpr : LocationAndType<ExprSort::QualifiedName> {
            ExprIndex elements{};              // The elements that make up this qualified-name
            SourceLocation typename_keyword{}; // The source location of the 'typename' keyword (if any)
        };

        struct DesignatedInitializerExpr : LocationAndType<ExprSort::DesignatedInitializer> {
            TextOffset member{};     // The identifier being designated
            ExprIndex initializer{}; // The assign-or-braced-init-expr
        };

        // FIXME: Remove at earliest convenience.
        struct ExpressionListExpr : Tag<ExprSort::ExpressionList> {
            enum class Delimiter : std::uint8_t {
                None,               // No delimiter
                Brace,              // Brace delimiters
                Parenthesis,        // Parenthesis delimiters
            };
            SourceLocation left_delimiter{};  // The source location of the left delimiter
            SourceLocation right_delimiter{}; // The source location of the right delimiter
            ExprIndex expressions{};          // The expressions in the list
            Delimiter delimiter{};            // The delimiter
        };

        struct AssignInitializerExpr : Tag<ExprSort::AssignInitializer> {
            SourceLocation assign{}; // The source location of the '='
            ExprIndex initializer{}; // The initializer
        };

        // The representation of 'sizeof(type-id)': 'sizeof expression' is handled as a regular unary-expression
        struct SizeofTypeExpr : LocationAndType<ExprSort::SizeofType> {
            TypeIndex operand{}; // The type-id argument
        };

        // The representation of 'sizeof(type-id)': 'sizeof expression' is handled as a regular unary-expression
        struct AlignofExpr : LocationAndType<ExprSort::Alignof> {
            TypeIndex type_id{}; // The type-id argument
        };

        // Label-expression.  Appears in goto-statements.
        struct LabelExpr : LocationAndType<ExprSort::Label> {
            ExprIndex designator{}; // The name of this label.  This is an expression in order
                                    // to offer flexibility in what the 'identifier' really is (it
                                    // might be a handle).
        };

        struct NullptrExpr : LocationAndType<ExprSort::Nullptr> {};

        struct ThisExpr : LocationAndType<ExprSort::This> {};

        struct PackedTemplateArgumentsExpr : LocationAndType<ExprSort::PackedTemplateArguments> {
            ExprIndex arguments{};
        };

        struct LambdaExpr : Tag<ExprSort::Lambda> {
            SyntaxIndex introducer{};          // The lambda introducer
            SyntaxIndex template_parameters{}; // The template parameters (if present)
            SyntaxIndex declarator{};          // The lambda declarator (if present)
            SyntaxIndex requires_clause{};     // The requires clause (if present)
            SyntaxIndex body{};                // The body of the lambda
        };

        struct TypeidExpr : LocationAndType<ExprSort::Typeid> {
            TypeIndex operand{}; // TypeSort::Syntax => operand is in parse tree form
        };

        struct SyntaxTreeExpr : Tag<ExprSort::SyntaxTree> {
            SyntaxIndex syntax{}; // The representation of this expression as a parse tree
        };

        struct ProductTypeValueExpr : LocationAndType<ExprSort::ProductTypeValue> {
            TypeIndex structure{};         // The class type which this value is associated
            ExprIndex members{};           // The members (zero or more) which are initialized
            ExprIndex base_class_values{}; // The values (zero or more) of base class members
        };

        enum class ActiveMemberIndex : std::uint32_t {};

        struct SumTypeValueExpr : LocationAndType<ExprSort::SumTypeValue> {
            TypeIndex variant{};               // The union type which this value is associated
            ActiveMemberIndex active_member{}; // The active member index
            ExprIndex value{};                 // The object representing the value of the active member
        };

        struct ArrayValueExpr : LocationAndType<ExprSort::ArrayValue> {
            ExprIndex elements{};     // The tuple containing the element expressions
            TypeIndex element_type{}; // The type of elements within the array
        };

        struct ObjectLikeMacro : Tag<MacroSort::ObjectLike> {
            SourceLocation locus{};
            TextOffset name{};            // macro name
            FormIndex replacement_list{}; // The replacement list of the macro.
        };

        struct FunctionLikeMacro : Tag<MacroSort::FunctionLike> {
            SourceLocation locus{};
            TextOffset name{};            // macro name
            FormIndex parameters{};       // The parameters of the macro.
            FormIndex replacement_list{}; // The replacement list of the macro.
            std::uint32_t arity : 31 {};       // Arity for function-like macros.
            std::uint32_t variadic : 1 {};     // True if this macro is variadic.
        };

        // Note: this class is not meant to be used to create objects -- it is just a traits class.
        template<typename T, LiteralSort s>
        struct constant_traits : Tag<s> {
            using ValueType = T;
        };

        // FIXME: investigate if disabling C4624 is really necessary here
#pragma warning(push)
#pragma warning(disable : 4624)
        struct IntegerLiteral : constant_traits<std::uint64_t, LiteralSort::Integer> {};

#pragma pack(push, 4)
        struct LiteralReal {
            double value{};
            std::uint16_t size{};
        };
#pragma pack(pop)

        static_assert(sizeof(LiteralReal) == 12, "LiteralReal must be packed and aligned to 4 byte boundary");

        struct FloatingPointLiteral : constant_traits<LiteralReal, LiteralSort::FloatingPoint> {};
#pragma warning(pop)

        struct PragmaPushState {
            DeclIndex text_segment;     // Symbol for #pragma code_seg
            DeclIndex const_segment;    // Symbol for #pragma const_seg
            DeclIndex data_segment;     // Symbol for #pragma data_seg
            DeclIndex bss_segment;      // Symbol for #pragma data_seg
            std::uint32_t pack_size : 8;     // value of #pragma pack
            std::uint32_t fp_control : 8;    // value of #pragma float_control
            std::uint32_t exec_charset : 8;  // value of #pragma execution_character_set
            std::uint32_t vtor_disp : 8;     // value of #pragma vtordisp
            std::uint32_t std_for_scope : 1; // value of #pragma conform(forScope)
            std::uint32_t unused : 1;        // unused bit - was previously pure_cil, which was meant to be the captured
                                        //  state of #pragma managed(off/on). This is not actually required as we
            //  don't support module export for /clr, and it's always the case for native code.
            std::uint32_t strict_gs_check : 1; // value of #pragma strict_gs_check
        };

        struct BasicAttr : Tag<AttrSort::Basic> {
            Word word{}; // the token in [[ token ]]
        };

        struct ScopedAttr : Tag<AttrSort::Scoped> {
            Word scope{};  // e.g. 'foo' in [[ foo::attr ]]
            Word member{}; // e.g. 'attr' in [[ foo::attr ]]
        };

        struct LabeledAttr : Tag<AttrSort::Labeled> {
            Word label{};          // 'key' in [[ key : value ]]
            AttrIndex attribute{}; // 'value' in [[ key : value ]]
        };

        struct CalledAttr : Tag<AttrSort::Called> {
            AttrIndex function{};  // Function postfix 'opt' in [[ opt(args) ]]
            AttrIndex arguments{}; // Function argument expression 'args' in [[ opt(args) ]]
        };

        struct ExpandedAttr : Tag<AttrSort::Expanded> {
            AttrIndex operand{}; // Expansion expression 'fun(arg)' in [[ fun(arg) ... ]]
        };

        struct FactoredAttr : Tag<AttrSort::Factored> {
            Word factor{};     // The scope 'msvc' factored out of the attr in [[ using msvc: opt(args), dbg(1) ]]
            AttrIndex terms{}; // The attribute(s) using the factored out scope.
        };

        struct ElaboratedAttr : Tag<AttrSort::Elaborated> {
            ExprIndex expr{}; // A generalized expression in an attribute.
        };

        // Sequence of one or more attributes.
        struct TupleAttr : Tag<AttrSort::Tuple>, Sequence<AttrIndex, HeapSort::Attr> {};

        namespace microsoft {
            enum class PragmaCommentSort : std::uint8_t {
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
                TextOffset comment_text = {};
                PragmaCommentSort sort  = PragmaCommentSort::Unknown;
            };
        } // namespace microsoft

        struct PragmaExpr : Location<PragmaSort::Expr> {
            TextOffset name{};   // The name of the pragma
            ExprIndex operand{}; // The operand of the #pragma expression
        };

        enum class Phases : std::uint32_t {
            Unknown        = 0,
            Reading        = 1 << 0,
            Lexing         = 1 << 1,
            Preprocessing  = 1 << 2,
            Parsing        = 1 << 3,
            Importing      = 1 << 4,
            NameResolution = 1 << 5,
            Typing         = 1 << 6,
            Evaluation     = 1 << 7,
            Instantiation  = 1 << 8,
            Analysis       = 1 << 9,
            CodeGeneration = 1 << 10,
            Linking        = 1 << 11,
            Loading        = 1 << 12,
            Execution      = 1 << 13,
        };

        struct EmptyDir : Location<DirSort::Empty> {};

        struct AttributeDir : Location<DirSort::Attribute> {
            AttrIndex attr{};
        };

        struct PragmaDir : Location<DirSort::Pragma> {
            SentenceIndex words{}; // Sentence index making up the directive.
        };

        struct UsingDir : Location<DirSort::Using> {
            ExprIndex nominated{};  // The in-source name expression designating the namespace.
            DeclIndex resolution{}; // Denotes the namespace after semantics elaboration on 'nominated'.
        };

        struct UsingDeclarationDir : Location<DirSort::DeclUse> {
            ExprIndex path{};   // The path expression to the declaration target of the using-declaration.
            DeclIndex result{}; // Denotes the declaration(s) resulting from executing the using-declaration.
        };

        struct ExprDir : Location<DirSort::Expr> {
            ExprIndex expr{}; // Denotes the expression to evaluate.
            Phases phases{};  // Denotes the set of phases of translation which this directive is to be evaluated.
        };

        struct StmtDir : Location<DirSort::Stmt> {
            StmtIndex stmt{}; // Denotes the statement to evaluate.
            Phases phases{};  // Denotes the set of phases of translation which this directive is to be evaluated.
        };

        struct StructuredBindingDir : Location<DirSort::StructuredBinding> {
            Sequence<DeclIndex> bindings{}; // The set of declarations resulting from the elaboration of a structured
                                            // bindings declaration.
            // DeclSpecifierSeq specifiers;     // TODO: we need to know concretely what this should be.
            Sequence<TextOffset> names{}; // The identifiers which make up the decomposed declarations.
            // BindingMode ref { };             // TODO: we need to know concretely what this should be.
        };

        struct SpecifiersSpreadDir : Location<DirSort::SpecifiersSpread> {
            // DeclSpecifierSequence specifiers { };    // TODO: we need to know concretely what this should be.
            // ProclaimerSequence targets { };          // TODO: we need to know concretely what this should be.
        };

        struct TupleDir : Tag<DirSort::Tuple>, Sequence<DirIndex, HeapSort::Dir> {};
    } // namespace symbolic

    namespace symbolic::preprocessing {
        struct IdentifierForm : Tag<FormSort::Identifier> {
            SourceLocation locus = {};
            TextOffset spelling  = {};
        };

        struct NumberForm : Tag<FormSort::Number> {
            SourceLocation locus = {};
            TextOffset spelling  = {};
        };

        struct CharacterForm : Tag<FormSort::Character> {
            SourceLocation locus = {};
            TextOffset spelling  = {};
        };

        struct StringForm : Tag<FormSort::String> {
            SourceLocation locus = {};
            TextOffset spelling  = {};
        };

        struct OperatorForm : Tag<FormSort::Operator> {
            SourceLocation locus = {};
            TextOffset spelling  = {};
            PPOperator value     = {};
        };

        struct KeywordForm : Tag<FormSort::Keyword> {
            SourceLocation locus{};
            TextOffset spelling{};
        };

        struct WhitespaceForm : Tag<FormSort::Whitespace> {
            SourceLocation locus = {};
        };

        struct ParameterForm : Tag<FormSort::Parameter> {
            SourceLocation locus = {};
            TextOffset spelling  = {};
        };

        struct StringizeForm : Tag<FormSort::Stringize> {
            SourceLocation locus = {};
            FormIndex operand    = {};
        };

        struct CatenateForm : Tag<FormSort::Catenate> {
            SourceLocation locus = {};
            FormIndex first      = {};
            FormIndex second     = {};
        };

        struct PragmaForm : Tag<FormSort::Pragma> {
            SourceLocation locus = {};
            FormIndex operand    = {};
        };

        struct HeaderForm : Tag<FormSort::Header> {
            SourceLocation locus = {};
            TextOffset spelling  = {};
        };

        struct ParenthesizedForm : Tag<FormSort::Parenthesized> {
            SourceLocation locus = {};
            FormIndex operand    = {};
        };

        struct JunkForm : Tag<FormSort::Junk> {
            SourceLocation locus = {};
            TextOffset spelling  = {};
        };

        struct TupleForm : Tag<FormSort::Tuple>, Sequence<FormIndex, HeapSort::Form> {
            TupleForm() = default;
            TupleForm(Index f, Cardinality n) : Sequence{f, n} {}
        };
    } // namespace symbolic::preprocessing

    enum class TraitSort : std::uint8_t {
        MappingExpr,     // function definition associated with a declaration
        AliasTemplate,   // extra information required for alias templates
        Friends,         // sequence of friends
        Specializations, // sequence of specializations
        Requires,        // information for requires clause
        Attributes,      // attributes associated with a declaration
        Deprecated,      // deprecation message.
        DeductionGuides, // Declared deduction guides associated with a class
        Prolongations,   // Prolongation declarations associated with a scope
        Count,
    };

    enum class MsvcTraitSort : std::uint8_t {
        Uuid,                       // uuid associated with a declaration
        Segment,                    // segment associated with a declaration
        SpecializationEncoding,     // template specialization encoding associated with a specialization
        SalAnnotation,              // sal annotation text.
        FunctionParameters,         // a parameter lists for a function (why not encompassed by MappingExpr???)
        InitializerLocus,           // the source location of declaration initializer
        TemplateTemplateParameters, // Deprecated: to be removed in a future update.
        CodegenExpression,          // expression used for codegen
        Vendor,                     // extended vendor traits
        DeclAttributes,             // C++ attributes associated with a declaration
        StmtAttributes,             // C++ attributes associated with a statement
        CodegenMappingExpr,         // definitions of inline functions intended for code generation
        DynamicInitVariable,        // A mapping from a real variable to the variable created as part of a dynamic
                                    // initialization implementation.
        CodegenLabelProperties,     // Unique properties attached to label expressions.  Used for code generation.
        CodegenSwitchType,          // A mapping between a switch statement and its 'control' expression type.
        CodegenDoWhileStmt,         // Captures extra statements associated with a do-while statement condition.
        LexicalScopeIndex, // A decl has an association with a specific local lexical scope index which can impact name
                           // mangling.
        FileBoundary,      // A begin/end pair of line numbers indicating the first and last lines of the file.
        HeaderUnitSourceFile, // A temporary trait to express the mapping between the Header::src_path -> NameIndex
                              // representing the source file information.
                              // FIXME: this goes away once bump the IFC version and have UnitIndex point to a source
                              // file instead of a TextOffset for header units.
        FileHash,             // A file hash associated with an indexed source file after compilation with MSVC.
        DebugRecord,          // A debug record for this decl is.
        Count,
    };

    template<typename T>
    concept TraitIndexType = std::same_as<T, DeclIndex> or std::same_as<T, SyntaxIndex> or std::same_as<T, ExprIndex>
                             or std::same_as<T, StmtIndex> or std::same_as<T, NameIndex> or std::same_as<T, TextOffset>;

    template<typename T>
    concept AnyTraitSort = std::same_as<T, TraitSort> or std::same_as<T, MsvcTraitSort>;

    // Declarations have various extension traits, such as deprecation message, code segment allocation, etc.
    // This is the data type used as entry into the decl-trait association table.
    template<TraitIndexType T, typename U>
    struct AssociatedTrait {
        using KeyType   = T;
        using ValueType = U;
        T entity; // The index of entity that is being associated with a trait
        U trait;  // The index of the associated trait
    };

    template<AnyTraitSort auto Tag>
    struct TraitTag : index_like::SortTag<Tag> {
        static constexpr auto partition_tag = Tag;
    };

    template<typename T>
    concept AnyTrait = std::derived_from<T, AssociatedTrait<typename T::KeyType, typename T::ValueType>>
                       and std::derived_from<T, TraitTag<T::partition_tag>>;

    // Ordering used for traits
    struct TraitOrdering {
        template<AnyTrait T>
        bool operator()(T& x, T& y) const
        {
            return x.entity < y.entity;
        }

        template<AnyTrait T>
        bool operator()(typename T::KeyType x, T& y) const
        {
            return x < y.entity;
        }

        template<AnyTrait T>
        bool operator()(T& x, typename T::KeyType y) const
        {
            return x.entity < y;
        }
    };

    namespace symbolic::trait {
        struct MappingExpr : AssociatedTrait<DeclIndex, MappingDefinition>, TraitTag<TraitSort::MappingExpr> {};
        struct AliasTemplate : AssociatedTrait<DeclIndex, SyntaxIndex>, TraitTag<TraitSort::AliasTemplate> {};
        struct Friends : AssociatedTrait<DeclIndex, Sequence<Declaration>>, TraitTag<TraitSort::Friends> {};
        struct Specializations : AssociatedTrait<DeclIndex, Sequence<Declaration>>,
                                 TraitTag<TraitSort::Specializations> {};
        struct Requires : AssociatedTrait<DeclIndex, SyntaxIndex>, TraitTag<TraitSort::Requires> {};
        struct Attributes : AssociatedTrait<SyntaxIndex, SyntaxIndex>, TraitTag<TraitSort::Attributes> {};
        struct Deprecated : AssociatedTrait<DeclIndex, TextOffset>, TraitTag<TraitSort::Deprecated> {};
        struct DeductionGuides : AssociatedTrait<DeclIndex, DeclIndex>, TraitTag<TraitSort::DeductionGuides> {};
        struct Prolongations : AssociatedTrait<DeclIndex, Sequence<Declaration>>, TraitTag<TraitSort::Prolongations> {};
    } // namespace symbolic::trait

    // Msvc specific traits. Should they be in their own namespace, like symbolic::MsvcTrait?
    namespace symbolic::trait {
        struct LocusSpan {
            SourceLocation begin;
            SourceLocation end;
        };

        enum class MsvcLabelKey : std::uint32_t {};
        enum class MsvcLabelType : std::uint32_t {};
        struct MsvcLabelProperties {
            MsvcLabelKey key;
            MsvcLabelType type;
        };
        enum class MsvcLexicalScopeIndex : std::uint32_t {};

        template<typename I>
        using AttributeAssociation = AssociatedTrait<I, AttrIndex>;

        struct MsvcFileBoundaryProperties {
            LineNumber first; // First line in the file (inclusive).
            LineNumber last;  // The last line in the file (inclusive).
        };

        enum class MsvcFileHashSort : std::uint8_t
        {
            None,
            MD5,
            SHA128,
            SHA256,
        };

        struct alignas(partition_alignment) MsvcFileHashData {
            static constexpr auto msvc_file_hash_size = 32;

            std::array<std::uint8_t, msvc_file_hash_size> bytes{};
            MsvcFileHashSort sort = MsvcFileHashSort::None;
            std::array<std::uint8_t, 3> unused;
        };

        static_assert(sizeof(MsvcFileHashData) == 36);

        // The MSVC warning number.
        enum class MsvcWarningNumber : std::uint16_t {};

        // The state of the warning associated with this pragma.
        enum class MsvcWarningState : std::uint8_t {};

        struct MsvcPragmaWarningRegion {
            SourceLocation start_locus{};
            SourceLocation end_locus{};
            MsvcWarningNumber warning_number{};
            MsvcWarningState warning_state{};
        };

        static_assert(sizeof(MsvcPragmaWarningRegion) == 20);

        enum class MsvcDebugRecordIndex : std::uint32_t {};

        struct MsvcGMFSpecializedTemplate {
            DeclIndex template_decl{}; // The template declaration.
            DeclIndex home_scope{};    // The enclosing scope of the template declaration.
        };

        struct MsvcUuid : AssociatedTrait<DeclIndex, StringIndex>, TraitTag<MsvcTraitSort::Uuid> {};
        struct MsvcSegment : AssociatedTrait<DeclIndex, DeclIndex>, TraitTag<MsvcTraitSort::Segment> {};
        struct MsvcSpecializationEncoding : AssociatedTrait<DeclIndex, TextOffset>,
                                            TraitTag<MsvcTraitSort::SpecializationEncoding> {};
        struct MsvcSalAnnotation : AssociatedTrait<DeclIndex, TextOffset>, TraitTag<MsvcTraitSort::SalAnnotation> {};
        struct MsvcFunctionParameters : AssociatedTrait<DeclIndex, ChartIndex>,
                                        TraitTag<MsvcTraitSort::FunctionParameters> {};
        struct MsvcInitializerLocus : AssociatedTrait<DeclIndex, SourceLocation>,
                                      TraitTag<MsvcTraitSort::InitializerLocus> {};
        struct MsvcCodegenExpression : AssociatedTrait<ExprIndex, ExprIndex>,
                                       TraitTag<MsvcTraitSort::CodegenExpression> {};
        struct DeclAttributes : AttributeAssociation<DeclIndex>, TraitTag<MsvcTraitSort::DeclAttributes> {};
        struct StmtAttributes : AttributeAssociation<StmtIndex>, TraitTag<MsvcTraitSort::StmtAttributes> {};
        struct MsvcVendor : AssociatedTrait<DeclIndex, VendorTraits>, TraitTag<MsvcTraitSort::Vendor> {};
        struct MsvcCodegenMappingExpr : AssociatedTrait<DeclIndex, MappingDefinition>,
                                        TraitTag<MsvcTraitSort::CodegenMappingExpr> {};
        struct MsvcDynamicInitVariable : AssociatedTrait<DeclIndex, DeclIndex>,
                                         TraitTag<MsvcTraitSort::DynamicInitVariable> {};
        struct MsvcCodegenLabelProperties : AssociatedTrait<ExprIndex, MsvcLabelProperties>,
                                            TraitTag<MsvcTraitSort::CodegenLabelProperties> {};
        struct MsvcCodegenSwitchType : AssociatedTrait<StmtIndex, TypeIndex>,
                                       TraitTag<MsvcTraitSort::CodegenSwitchType> {};
        struct MsvcCodegenDoWhileStmt : AssociatedTrait<StmtIndex, StmtIndex>,
                                        TraitTag<MsvcTraitSort::CodegenDoWhileStmt> {};
        struct MsvcLexicalScopeIndices : AssociatedTrait<DeclIndex, MsvcLexicalScopeIndex>,
                                         TraitTag<MsvcTraitSort::LexicalScopeIndex> {};
        struct MsvcFileBoundary : AssociatedTrait<NameIndex, MsvcFileBoundaryProperties>,
                                  TraitTag<MsvcTraitSort::FileBoundary> {};
        struct MsvcHeaderUnitSourceFile : AssociatedTrait<TextOffset, NameIndex>,
                                          TraitTag<MsvcTraitSort::HeaderUnitSourceFile> {};
        struct MsvcFileHash : AssociatedTrait<NameIndex, MsvcFileHashData>,
                                TraitTag<MsvcTraitSort::FileHash> {};
        struct MsvcDebugRecord : AssociatedTrait<DeclIndex, MsvcDebugRecordIndex>,
                                TraitTag<MsvcTraitSort::DebugRecord> {};
    } // namespace symbolic::trait

    namespace symbolic::vendor {
        struct SEHTryStmt : Location<VendorSort::SEHTry>, Sequence<StmtIndex, HeapSort::Stmt> {
            StmtIndex handler{}; // Handler for __try.
        };

        struct SEHFinallyStmt : Location<VendorSort::SEHFinally>, Sequence<StmtIndex, HeapSort::Stmt> { };

        struct SEHExceptStmt : Location<VendorSort::SEHExcept>, Sequence<StmtIndex, HeapSort::Stmt> {
            ExprIndex filter{}; // Expression on the __except filter.
        };

        struct SEHLeaveStmt : Location<VendorSort::SEHLeave> { };
    } // namespace symbolic::vendor

    // Partition summaries for the table of contents.
    struct TableOfContents {
        PartitionSummaryData command_line;                // Command line information
        PartitionSummaryData exported_modules;            // Exported modules.
        PartitionSummaryData imported_modules;            // Set of modules explicitly imported by this interface file.
        PartitionSummaryData u64s;                        // 64-bit unsigned integer constants.
        PartitionSummaryData fps;                         // Floating-point constants.
        PartitionSummaryData string_literals;             // String literals.
        PartitionSummaryData states;                      // States of #pragma push.
        PartitionSummaryData lines;                       // Source line descriptors.
        PartitionSummaryData words;                       // Token table.
        PartitionSummaryData sentences;                   // Token stream table.
        PartitionSummaryData scopes;                      // scope table
        PartitionSummaryData entities;                    // All the indices for the entities associated with a scope
        PartitionSummaryData spec_forms;                  // Specialization forms.
        PartitionSummaryData names[count<NameSort> - 1];  // Table of all sorts of names, except plain identifiers.
        PartitionSummaryData decls[count<DeclSort>];      // Table of declaration partitions.
        PartitionSummaryData types[count<TypeSort>];      // Table of type partitions.
        PartitionSummaryData stmts[count<StmtSort>];      // Table of statement partitions
        PartitionSummaryData exprs[count<ExprSort>];      // Table of expression partitions.
        PartitionSummaryData elements[count<SyntaxSort>]; // Table of syntax trees partitions.
        PartitionSummaryData forms[count<FormSort>];      // Table of form partitions.
        PartitionSummaryData traits[count<TraitSort>];    // Table of traits
        PartitionSummaryData msvc_traits[count<MsvcTraitSort>];    // Table of msvc specific traits
        PartitionSummaryData vendor[count<VendorSort>];            // Table of vendor-specific syntax.
        PartitionSummaryData charts;                               // Sequence of unilevel charts.
        PartitionSummaryData multi_charts;                         // Sequence of multi-level charts.
        PartitionSummaryData heaps[count<HeapSort>];               // Set of various abstract reference sequences.
        PartitionSummaryData pragma_warnings;                      // Association map of pragma warning info.
        PartitionSummaryData macros[count<MacroSort>];             // Table of exported macros.
        PartitionSummaryData pragma_directives[count<PragmaSort>]; // Table of pragma directives.
        PartitionSummaryData attrs[count<AttrSort>];               // Table of all attributes.
        PartitionSummaryData dirs[count<DirSort>];                 // Table of all directives.
        PartitionSummaryData implementation_pragmas; // Sequence of PragmaIndex from the TU which can influence
                                                     // semantics of the current TU.
        PartitionSummaryData debug_records;          // Implementation-specific data for debugging.
        PartitionSummaryData gmf_specializations;    // Decl-reachable template specializations in the global module fragment.
        PartitionSummaryData prolongations;          // An association between scopes and prolongation declarations.

        // Facilities for iterating over the ToC as a sequence of partition summaries.
        // Note: the use of reinterpret_cast below is avoidable. One way uses
        // two static_cast; the other uses no casts but field names.  The whole point
        // of having this facility is separate field names from the abstract structure,
        // so that fields order does not matter.  The two static_casts would be
        // just being overly pedantic.  This construct is well understood and well-defined.
        // Bug: lot of comments for a one-liner.
        auto begin()
        {
            return reinterpret_cast<PartitionSummaryData*>(this);
        }

        auto end()
        {
            return begin() + sizeof(*this) / sizeof(PartitionSummaryData);
        }

        auto begin() const
        {
            return reinterpret_cast<const PartitionSummaryData*>(this);
        }

        auto end() const
        {
            return begin() + sizeof(*this) / sizeof(PartitionSummaryData);
        }

        PartitionSummaryData& operator[](StringSort s)
        {
            IFCVERIFY(s >= StringSort::Ordinary and s < StringSort::Count);
            return string_literals;
        }

        const PartitionSummaryData& operator[](StringSort) const
        {
            return string_literals;
        }

        PartitionSummaryData& operator[](NameSort s)
        {
            IFCVERIFY(s > NameSort::Identifier and s < NameSort::Count);
            return names[ifc::to_underlying(s) - 1];
        }

        const PartitionSummaryData& operator[](NameSort s) const
        {
            IFCVERIFY(s > NameSort::Identifier and s < NameSort::Count);
            return names[ifc::to_underlying(s) - 1];
        }

        PartitionSummaryData& operator[](ChartSort s)
        {
            if (s == ChartSort::Unilevel)
            {
                return charts;
            }
            else
            {
                IFCASSERT(s == ChartSort::Multilevel);
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
                IFCASSERT(s == ChartSort::Multilevel);
                return multi_charts;
            }
        }

        template<typename S, typename T>
        static auto& partition(T& table, S s)
        {
            IFCASSERT(s < S::Count);
            return table[ifc::to_underlying(s)];
        }

        PartitionSummaryData& operator[](DeclSort s)
        {
            return partition(decls, s);
        }

        const PartitionSummaryData& operator[](DeclSort s) const
        {
            return partition(decls, s);
        }

        PartitionSummaryData& operator[](TypeSort s)
        {
            return partition(types, s);
        }

        const PartitionSummaryData& operator[](TypeSort s) const
        {
            return partition(types, s);
        }

        PartitionSummaryData& operator[](StmtSort s)
        {
            return partition(stmts, s);
        }

        const PartitionSummaryData& operator[](StmtSort s) const
        {
            return partition(stmts, s);
        }

        PartitionSummaryData& operator[](ExprSort s)
        {
            return partition(exprs, s);
        }

        const PartitionSummaryData& operator[](ExprSort s) const
        {
            return partition(exprs, s);
        }

        PartitionSummaryData& operator[](SyntaxSort s)
        {
            return partition(elements, s);
        }

        const PartitionSummaryData& operator[](SyntaxSort s) const
        {
            return partition(elements, s);
        }

        PartitionSummaryData& operator[](MacroSort s)
        {
            return partition(macros, s);
        }

        const PartitionSummaryData& operator[](MacroSort s) const
        {
            return partition(macros, s);
        }

        PartitionSummaryData& operator[](PragmaSort s)
        {
            return partition(pragma_directives, s);
        }

        const PartitionSummaryData& operator[](PragmaSort s) const
        {
            return partition(pragma_directives, s);
        }

        PartitionSummaryData& operator[](AttrSort s)
        {
            return partition(attrs, s);
        }

        const PartitionSummaryData& operator[](AttrSort s) const
        {
            return partition(attrs, s);
        }

        PartitionSummaryData& operator[](DirSort s)
        {
            return partition(dirs, s);
        }

        const PartitionSummaryData& operator[](DirSort s) const
        {
            return partition(dirs, s);
        }

        PartitionSummaryData& operator[](HeapSort s)
        {
            return partition(heaps, s);
        }

        const PartitionSummaryData& operator[](HeapSort s) const
        {
            return partition(heaps, s);
        }

        PartitionSummaryData& operator[](FormSort s)
        {
            return partition(forms, s);
        }

        const PartitionSummaryData& operator[](FormSort s) const
        {
            return partition(forms, s);
        }

        PartitionSummaryData& operator[](TraitSort s)
        {
            return partition(traits, s);
        }

        const PartitionSummaryData& operator[](TraitSort s) const
        {
            return partition(traits, s);
        }

        PartitionSummaryData& operator[](MsvcTraitSort s)
        {
            return partition(msvc_traits, s);
        }

        const PartitionSummaryData& operator[](MsvcTraitSort s) const
        {
            return partition(msvc_traits, s);
        }

        const PartitionSummaryData& operator[](VendorSort s) const
        {
            return partition(vendor, s);
        }

        PartitionSummaryData& operator[](VendorSort s)
        {
            return partition(vendor, s);
        }

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
    const char* sort_name(DirSort);
    const char* sort_name(HeapSort);
    const char* sort_name(FormSort);
    const char* sort_name(TraitSort);
    const char* sort_name(MsvcTraitSort);
    const char* sort_name(VendorSort);

} // namespace ifc

#endif // IFC_ABSTRACT_SGRAPH
