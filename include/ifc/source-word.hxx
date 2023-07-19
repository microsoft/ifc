//
// Copyright Microsoft.
//

#ifndef IFC_SOURCE_WORD
#define IFC_SOURCE_WORD

#include <concepts>
#include <ifc/underlying.hxx>

namespace ifc {

    // The many sorts of words that make up a sentence
    enum class WordSort : uint8_t {
        Unknown,    // Unclassified word.
        Directive,  // Implementation-defined control-line surviving Phase 4 of translation.
        Punctuator, // punctuators as defined by ISO C++, along with C1xx internal markers
        Literal,    // scalar literals, string literals, along with C1xx internal literals
        Operator,   // ISO C++ operators, along with MSVC extensions
        Keyword,    // ISO C++ keywords, along with MSVC extensions
        Identifier, // ISO C++ identifiers, along with MSVC extensions
        Count
    };

    // C1xx internally stores the initializer (in the IPR sense) of templated definitions
    // as a sequence of its internal notion of tokens (here abstracted as 'words').  As a
    // temporary measure, those c1xx-tokens are abtracted into words of various sorts,
    // shielded from the token-du-jour update vagaries.  These temporary measures will be
    // removed as c1xx gains more a principled internal representation.
    namespace source {
        enum class Directive : uint16_t {
            Unknown = 0x0000,

            Msvc = 0x1FFF,
            MsvcPragmaPush,
            MsvcPragmaPop,
            MsvcDirectiveStart,
            MsvcDirectiveEnd,
            MsvcPragmaAllocText,
            MsvcPragmaAutoInline,
            MsvcPragmaBssSeg,
            MsvcPragmaCheckStack,
            MsvcPragmaCodeSeg,
            MsvcPragmaComment,
            MsvcPragmaComponent,
            MsvcPragmaConform,
            MsvcPragmaConstSeg,
            MsvcPragmaDataSeg,
            MsvcPragmaDeprecated,
            MsvcPragmaDetectMismatch,
            MsvcPragmaEndregion,
            MsvcPragmaExecutionCharacterSet,
            MsvcPragmaFenvAccess,
            MsvcPragmaFileHash,
            MsvcPragmaFloatControl,
            MsvcPragmaFpContract,
            MsvcPragmaFunction,
            MsvcPragmaBGI,
            MsvcPragmaIdent,
            MsvcPragmaImplementationKey,
            MsvcPragmaIncludeAlias,
            MsvcPragmaInitSeg,
            MsvcPragmaInlineDepth,
            MsvcPragmaInlineRecursion,
            MsvcPragmaIntrinsic,
            MsvcPragmaLoop,
            MsvcPragmaMakePublic,
            MsvcPragmaManaged,
            MsvcPragmaMessage,
            MsvcPragmaOMP,
            MsvcPragmaOptimize,
            MsvcPragmaPack,
            MsvcPragmaPointerToMembers,
            MsvcPragmaPopMacro,
            MsvcPragmaPrefast,
            MsvcPragmaPushMacro,
            MsvcPragmaRegion,
            MsvcPragmaRuntimeChecks,
            MsvcPragmaSameSeg,
            MsvcPragmaSection,
            MsvcPragmaSegment,
            MsvcPragmaSetlocale,
            MsvcPragmaStartMapRegion,
            MsvcPragmaStopMapRegion,
            MsvcPragmaStrictGSCheck,
            MsvcPragmaSystemHeader,
            MsvcPragmaUnmanaged,
            MsvcPragmaVtordisp,
            MsvcPragmaWarning,
            MsvcPragmaP0include,
            MsvcPragmaP0line,
        };

        enum class Punctuator : uint16_t {
            Unknown = 0x0000,
            LeftParenthesis,  // "("
            RightParenthesis, // ")"
            LeftBracket,      // "["
            RightBracket,     // "]"
            LeftBrace,        // "{"
            RightBrace,       // "}"
            Colon,            // ":"
            Question,         // "?"
            Semicolon,        // ";"
            ColonColon,       // "::"
            Pound,            // "#"

            Msvc = 0x1FFF, // MSVC curiosities follow
            MsvcZeroWidthSpace,
            MsvcEndOfPhrase,
            MsvcFullStop,
            MsvcNestedTemplateStart,
            MsvcDefaultArgumentStart,
            MsvcAlignasEdictStart,
            MsvcDefaultInitStart,
        };

        enum class Literal : uint16_t {
            Unknown = 0x0000,
            Scalar,        // characters, integers, floating points, pointers
            String,        // string literals
            DefinedString, // UDL string.

            Msvc = 0x1FFF,
            MsvcFunctionNameMacro, // "__FUNCTION__" and friends
            MsvcStringPrefixMacro, // "__LPREFIX" and friends
            MsvcBinding,           // Binding of an identifier to a set of declarations
            MsvcResolvedType,      // Binding of a resolved type
            MsvcDefinedConstant,   // UDL constant.  FIXME: Find efficient rep for all UDLs.
            MsvcCastTargetType,    // Binding of a resolve type in a cast expression.  FIXME: to be removed along with
                                   // other YACC oddities.
            MsvcTemplateId,        // A reference to a known template template specialization.
        };

        enum class Operator : uint16_t {
            Unknown = 0x0000,
            Equal,              // "="
            Comma,              // ","
            Exclaim,            // "!"
            Plus,               // "+"
            Dash,               // "-"
            Star,               // "*"
            Slash,              // "/"
            Percent,            // "%"
            LeftChevron,        // "<<"
            RightChevron,       // ">>"
            Tilde,              // "~"
            Caret,              // "^"
            Bar,                // "|"
            Ampersand,          // "&"
            PlusPlus,           // "++"
            DashDash,           // "--"
            Less,               // "<"
            LessEqual,          // "<="
            Greater,            // ">"
            GreaterEqual,       // ">="
            EqualEqual,         // "=="
            ExclaimEqual,       // "!="
            Diamond,            // "<=>"
            PlusEqual,          // "+="
            DashEqual,          // "-="
            StarEqual,          // "*="
            SlashEqual,         // "/="
            PercentEqual,       // "%="
            AmpersandEqual,     // "&="
            BarEqual,           // "|="
            CaretEqual,         // "^="
            LeftChevronEqual,   // "<<="
            RightChevronEqual,  // ">>="
            AmpersandAmpersand, // "&&"
            BarBar,             // "||"
            Ellipsis,           // "..."
            Dot,                // "."
            Arrow,              // "->"
            DotStar,            // ".*"
            ArrowStar,          // "->*"

            Msvc = 0x1FFF,
        };

        enum class Keyword : uint16_t {
            Unknown = 0x0000,
            Alignas,         // "alignas"
            Alignof,         // "alignof"
            Asm,             // "asm"
            Auto,            // "auto"
            Bool,            // "bool"
            Break,           // "break"
            Case,            // "case"
            Catch,           // "catch"
            Char,            // "char"
            Char8T,          // "char8_t"
            Char16T,         // "char16_t"
            Char32T,         // "char32_t"
            Class,           // "class"
            Concept,         // "concept"
            Const,           // "const"
            Consteval,       // "consteval"
            Constexpr,       // "constexpr"
            Constinit,       // "constinit"
            ConstCast,       // "const_Cast"
            Continue,        // "continue"
            CoAwait,         // "co_await"
            CoReturn,        // "co_return"
            CoYield,         // "co_yield"
            Decltype,        // "decltype"
            Default,         // "default"
            Delete,          // "delete"
            Do,              // "do"
            Double,          // "double"
            DynamicCast,     // "dynamic_cast"
            Else,            // "else"
            Enum,            // "enum"
            Explicit,        // "explicit"
            Export,          // "export"
            Extern,          // "extern"
            False,           // "false"
            Float,           // "float"
            For,             // "for"
            Friend,          // "friend"
            Generic,         // "_Generic"
            Goto,            // "goto"
            If,              // "if"
            Inline,          // "inline"
            Int,             // "int"
            Long,            // "long"
            Mutable,         // "mutable"
            Namespace,       // "namespace"
            New,             // "new"
            Noexcept,        // "noexcept"
            Nullptr,         // "nullptr"
            Operator,        // "operator"
            Pragma,          // "_Pragma"
            Private,         // "private"
            Protected,       // "protected"
            Public,          // "public"
            Register,        // "register"
            ReinterpretCast, // "reinterpret_cast"
            Requires,        // "requires"
            Restrict,        // "restrict"   -- C11 invention
            Return,          // "return"
            Short,           // "short"
            Signed,          // "signed"
            Sizeof,          // "sizeof"
            Static,          // "static"
            StaticAssert,    // "static_assert"
            StaticCast,      // "static_cast"
            Struct,          // "struct"
            Switch,          // "switch"
            Template,        // "template"
            This,            // "this"
            ThreadLocal,     // "thread_local"
            Throw,           // "throw"
            True,            // "true"
            Try,             // "try"
            Typedef,         // "typedef"
            Typeid,          // "typeid"
            Typename,        // "typename"
            Union,           // "union"
            Unsigned,        // "unaigned"
            Using,           // "using"
            Virtual,         // "virtual"
            Void,            // "void"
            Volatile,        // "volatile"
            WcharT,          // "wchar_t"
            While,           // "while"

            Msvc = 0x1FFF,           // MSVC-specific keyword extensions
            MsvcAsm,                 // "__asm"
            MsvcAssume,              // "__assume"
            MsvcAlignof,             // "__alignof"
            MsvcBased,               // "__based"
            MsvcCdecl,               // "__cdecl"
            MsvcClrcall,             // "__clrcall"
            MsvcDeclspec,            // "__declspec"
            MsvcEabi,                // "__eabi"
            MsvcEvent,               // "__event"
            MsvcSehExcept,           // "__except"
            MsvcFastcall,            // "__fastcall"
            MsvcSehFinally,          // "__finally"
            MsvcForceinline,         // "__forceinline"
            MsvcHook,                // "__hook"
            MsvcIdentifier,          // "__identifier"
            MsvcIfExists,            // "__if_exists"
            MsvcIfNotExists,         // "__if_not_exists"
            MsvcInt8,                // "__int8"
            MsvcInt16,               // "__int16"
            MsvcInt32,               // "__int32"
            MsvcInt64,               // "__int64"
            MsvcInt128,              // "__int128"
            MsvcInterface,           // '__interface"
            MsvcLeave,               // "__leave"
            MsvcMultipleInheritance, // "__multiple_inheritance"
            MsvcNullptr,             // "__nullptr"
            MsvcNovtordisp,          // "__novtordisp"
            MsvcPragma,              // "__pragma"
            MsvcPtr32,               // "__ptr32"
            MsvcPtr64,               // "__ptr64"
            MsvcRestrict,            // "__restrict"
            MsvcSingleInheritance,   // "__single_inheritance"
            MsvcSptr,                // "__sptr"
            MsvcStdcall,             // "__stdcall"
            MsvcSuper,               // "__surper"
            MsvcThiscall,            // "__thiscall"
            MsvcSehTry,              // "__try"
            MsvcUptr,                // "__ptr"
            MsvcUuidof,              // "__uuidof"
            MsvcUnaligned,           // "__unaligned"
            MsvcUnhook,              // "__unhook"
            MsvcVectorcall,          // "__vectorcall"
            MsvcVirtualInheritance,  // "__virtual_inheritance"
            MsvcW64,                 // "__w64"

            MsvcIsClass,                                   // "__is_class"
            MsvcIsUnion,                                   // "__is_union"
            MsvcIsEnum,                                    // "__is_enum"
            MsvcIsPolymorphic,                             // "__is_polymorphic"
            MsvcIsEmpty,                                   // "__is_empty"
            MsvcHasTrivialConstructor,                     // "__has_trivial_constructor"
            MsvcIsTriviallyConstructible,                  // "__is_trivially_constructible"
            MsvcIsTriaviallyCopyConstructible,             // "__is_trivially_copy_constructible"
            MsvcIsTriviallyCopyAssignable,                 // "__is_trivially_copy_assignable"
            MsvcIsTriviallyDestructible,                   // "__is_trivially_destructible"
            MsvcHasVirtualDestructor,                      // "__has_virtual_destructor"
            MsvcIsNothrowConstructible,                    // "__is_nothrow_constructible"
            MsvcIsNothrowCopyConstructible,                // "__is_nothrow_copy_constructible"
            MsvcIsNothrowCopyAssignable,                   // "__is_nothrow_copy_assignable"
            MsvcIsPod,                                     // "__is_pod"
            MsvcIsAbstract,                                // "__is_abstract"
            MsvcIsBaseOf,                                  // "__is_base_of"
            MsvcIsConvertibleTo,                           // "__is_convertible_to"
            MsvcIsTrivial,                                 // "__is_trivial"
            MsvcIsTriviallyCopyable,                       // "__is_trivially_copyable"
            MsvcIsStandardLayout,                          // "__is_standard_layout"
            MsvcIsLiteralType,                             // "__is_literal_type"
            MsvcIsTriviallyMoveConstructible,              // "__is_trivially_move_constructible"
            MsvcHasTrivialMoveAssign,                      // "__has_trivial_move_assign"
            MsvcIsTriviallyMoveAssignable,                 // "__is_trivially_move_assignable"
            MsvcIsNothrowMoveAssignable,                   // "__is_nothrow_move_assign"
            MsvcIsConstructible,                           // "__is_constructible"
            MsvcUnderlyingType,                            // "__underlying_type"
            MsvcIsTriviallyAssignable,                     // "__is_trivially_assignable"
            MsvcIsNothrowAssignable,                       // "__is_nothrow_assignable"
            MsvcIsDestructible,                            // "__is_destructible"
            MsvcIsNothrowDestructible,                     // "__is_nothrow_destructible"
            MsvcIsAssignable,                              // "__is_assignable"
            MsvcIsAssignableNocheck,                       // "__is_assignable_no_precondition_check"
            MsvcHasUniqueObjectRepresentations,            // "__has_unique_object_representations"
            MsvcIsAggregate,                               // "__is_aggregate"
            MsvcBuiltinAddressOf,                          // "__builtin_addressof"
            MsvcBuiltinOffsetOf,                           // "__builtin_offsetof"
            MsvcBuiltinBitCast,                            // "__builtin_bit_cast"
            MsvcBuiltinIsLayoutCompatible,                 // "__builtin_is_layout_compatible"
            MsvcBuiltinIsPointerInterconvertibleBaseOf,    // "__builtin_is_pointer_interconvertible_base_of"
            MsvcBuiltinIsPointerInterconvertibleWithClass, // "__builtin_is_pointer_interconvertible_with_class"
            MsvcBuiltinIsCorrespondingMember,              // "__builtin_is_corresponding_member"
            MsvcIsRefClass,                                // "__is_ref_class"
            MsvcIsValueClass,                              // "__is_value_class"
            MsvcIsSimpleValueClass,                        // "__is_simple_value_class"
            MsvcIsInterfaceClass,                          // "__is_interface_class"
            MsvcIsDelegate,                                // "__is_delegate"
            MsvcIsFinal,                                   // "__is_final"
            MsvcIsSealed,                                  // "__is_sealed"
            MsvcHasFinalizer,                              // "__has_finalizer"
            MsvcHasCopy,                                   // "__has_copy"
            MsvcHasAssign,                                 // "__has_assign"
            MsvcHasUserDestructor,                         // "__has_user_destructor"

            MsvcPackCardinality, // MSVC fusion of "sizeof" and "..."
            MsvcConfusedSizeof,  // Confusion in the token gatherer about "sizeof"
            MsvcConfusedAlignas, // Confusion in the token gatherer about "alignas"
        };

        enum class Identifier : uint16_t {
            Plain = 0x0000,

            Msvc = 0x1FFF,
            MsvcBuiltinHugeVal,  // "__builtin_huge_val"
            MsvcBuiltinHugeValf, // "__builtin_huge_valf"
            MsvcBuiltinNan,      // "__builtin_nan"
            MsvcBuiltinNanf,     // "__builtin_nanf"
            MsvcBuiltinNans,     // "__builtin_nans"
            MsvcBuiltinNansf,    // "__builtin_nansf"
        };
    } // namespace source

    template<typename T>
    concept PPOperatorCategory = std::same_as<T, source::Punctuator> or std::same_as<T, source::Operator>;

    constexpr WordSort pp_operator_sort(source::Operator)
    {
        return WordSort::Operator;
    }
    constexpr WordSort pp_operator_sort(source::Punctuator)
    {
        return WordSort::Punctuator;
    }

    // Stores preprocessing operators and punctuators, before any semantic conversion is done.
    struct PPOperator {
        enum class Index : uint16_t {};
        PPOperator() : tag{}, value{} {}
        template<PPOperatorCategory Category>
        PPOperator(Category c) : tag(ifc::to_underlying(pp_operator_sort(c))), value(ifc::to_underlying(c))
        {}
        WordSort sort() const
        {
            return WordSort(tag);
        }
        Index index() const
        {
            return Index(value);
        }

    private:
        uint16_t tag : 3;
        uint16_t value : 13; // MSVC starts at 0x1FFF, so all values prior fit into 13 bits.
        static_assert(ifc::bit_length(ifc::to_underlying(WordSort::Count)) == 3);
        static_assert(ifc::bit_length(ifc::to_underlying(source::Operator::Msvc)) == 13);
        static_assert(ifc::bit_length(ifc::to_underlying(source::Punctuator::Msvc)) == 13);
    };
    static_assert(sizeof(PPOperator) == sizeof(uint16_t));
} // namespace ifc

#endif
