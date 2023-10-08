// Copyright Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "ifc/util.hxx"

namespace ifc::util {
    using ifc::implies;

    namespace {
        void append(std::string& result, std::string_view suffix, std::string sep = " ")
        {
            if (not result.empty() and not suffix.empty())
                result.append(sep);
            result.append(suffix);
        }
    } // namespace

    std::string to_string(Access access)
    {
        switch (access)
        {
        case Access::None:
            return "";
        case Access::Private:
            return "private";
        case Access::Protected:
            return "protected";
        case Access::Public:
            return "public";
        default:
            return "unknown-access-" + std::to_string(ifc::to_underlying(access));
        }
    }

    std::string to_string(BasicSpecifiers basic)
    {
        std::string result;
        if (implies(basic, BasicSpecifiers::C))
            append(result, "c-linkage");
        if (implies(basic, BasicSpecifiers::Internal))
            append(result, "internal");
        if (implies(basic, BasicSpecifiers::Vague))
            append(result, "vague");
        if (implies(basic, BasicSpecifiers::External))
            append(result, "external");
        if (implies(basic, BasicSpecifiers::Deprecated))
            append(result, "deprecated");
        if (implies(basic, BasicSpecifiers::InitializedInClass))
            append(result, "initialized-in-class");
        if (implies(basic, BasicSpecifiers::IsMemberOfGlobalModule))
            append(result, "member-of-global-module");

        return result;
    }

    std::string to_string(ScopeTraits traits)
    {
        std::string result;
        if (implies(traits, ScopeTraits::Unnamed))
            append(result, "unnamed");
        if (implies(traits, ScopeTraits::Inline))
            append(result, "inline");
        if (implies(traits, ScopeTraits::InitializerExported))
            append(result, "initializer-exported");
        if (implies(traits, ScopeTraits::ClosureType))
            append(result, "closure-type");
        if (implies(traits, ScopeTraits::Vendor))
            append(result, "vendor");

        if (result.empty())
            return result;

        result.insert(0, "scope-traits(");
        result.push_back(')');
        return result;
    }

    std::string to_string(ReachableProperties reachable)
    {
        std::string result;
        if (implies(reachable, ReachableProperties::Initializer))
            append(result, "initializer");
        if (implies(reachable, ReachableProperties::DefaultArguments))
            append(result, "default-args");
        if (implies(reachable, ReachableProperties::Attributes))
            append(result, "attributes");

        if (result.empty())
            return result;

        result.insert(0, "reachable(");
        result.push_back(')');
        return result;
    }

    std::string to_string(ObjectTraits traits)
    {
        std::string result;
        if (implies(traits, ObjectTraits::Constexpr))
            append(result, "constexpr");
        if (implies(traits, ObjectTraits::Mutable))
            append(result, "mutable");
        if (implies(traits, ObjectTraits::ThreadLocal))
            append(result, "thread_local");
        if (implies(traits, ObjectTraits::InitializerExported))
            append(result, "object-initializer-exported");
        if (implies(traits, ObjectTraits::NoUniqueAddress))
            append(result, "no-unique-address");
        if (implies(traits, ObjectTraits::Vendor))
            append(result, "object-vendor-traits");
        return result;
    }

    std::string to_string(FunctionTraits traits)
    {
        std::string result;
        if (implies(traits, FunctionTraits::Inline))
            append(result, "inline");
        if (implies(traits, FunctionTraits::Constexpr))
            append(result, "constexpr");
        if (implies(traits, FunctionTraits::Explicit))
            append(result, "explicit");
        if (implies(traits, FunctionTraits::Virtual))
            append(result, "virtual");
        if (implies(traits, FunctionTraits::NoReturn))
            append(result, "no-return");
        if (implies(traits, FunctionTraits::PureVirtual))
            append(result, "pure-virtual");
        if (implies(traits, FunctionTraits::HiddenFriend))
            append(result, "hidden-friend");
        if (implies(traits, FunctionTraits::Defaulted))
            append(result, "defaulted");
        if (implies(traits, FunctionTraits::Deleted))
            append(result, "deleted");
        if (implies(traits, FunctionTraits::Constrained))
            append(result, "constrained");
        if (implies(traits, FunctionTraits::Immediate))
            append(result, "immediate");
        if (implies(traits, FunctionTraits::Final))
            append(result, "final");
        if (implies(traits, FunctionTraits::Override))
            append(result, "override");
        if (implies(traits, FunctionTraits::Vendor))
            append(result, "function-vendor-traits");
        return result;
    }

    std::string to_string(ifc::Qualifier qual)
    {
        std::string result;
        if (implies(qual, Qualifier::Const))
            append(result, "const");
        if (implies(qual, Qualifier::Volatile))
            append(result, "volatile");
        if (implies(qual, Qualifier::Restrict))
            append(result, "restrict");
        return result;
    }

    std::string to_string(ifc::symbolic::ExpansionMode mode)
    {
        if (mode == symbolic::ExpansionMode::Full)
            return "...";
        if (mode == symbolic::ExpansionMode::Partial)
            return "...(partial)";
        return "unknown-expansion-mode-:" + std::to_string(ifc::to_underlying(mode));
    }

    std::string to_string(symbolic::ReadExpr::Kind kind)
    {
        using Kind = symbolic::ReadExpr::Kind;
        switch (kind)
        {
        case Kind::Unknown:
            return "unknown";
        case Kind::Indirection:
            return "indirection";
        case Kind::RemoveReference:
            return "remove-reference";
        case Kind::LvalueToRvalue:
            return "lvalue-to-rvalue";
        case Kind::IntegralConversion:
            return "integral-conversion";
        default:
            return "unknown-read-kind-" + std::to_string(ifc::to_underlying(kind));
        }
    }

    std::string to_string(CallingConvention conv)
    {
        switch (conv)
        {
        case CallingConvention::Cdecl:
            return "__cdecl";
        case CallingConvention::Fast:
            return "__fastcall";
        case CallingConvention::Std:
            return "__stdcall";
        case CallingConvention::This:
            return "__thiscall";
        case CallingConvention::Clr:
            return "__clrcall";
        case CallingConvention::Vector:
            return "__vectorcall";
        case CallingConvention::Eabi:
            return "__eabi";
        default:
            return "calling-conv-" + std::to_string(ifc::to_underlying(conv));
        }
    }

    std::string to_string(NoexceptSort sort)
    {
        switch (sort)
        {
        case NoexceptSort::False:
            return "noexcept(false)";
        case NoexceptSort::True:
            return "noexcept(true)";
        case NoexceptSort::Expression:
            return "noexcept(<expression>)";
        case NoexceptSort::InferredSpecialMember:
            return "noexcept(<inferred-special-member>)";
        case NoexceptSort::Unenforced:
            return "noexcept(<unenforced>)";
        default:
            return "unknown-noexcept-sort-" + std::to_string(ifc::to_underlying(sort));
        }
    }

    std::string to_string(symbolic::ExpressionListExpr::Delimiter delimiter)
    {
        using Kind = symbolic::ExpressionListExpr::Delimiter;
        switch (delimiter)
        {
        case Kind::Unknown:
            return "Unknown";
        case Kind::Brace:
            return "Brace";
        case Kind::Parenthesis:
            return "Parenthesis";
        default:
            return "unknown-delimiter-kind-" + std::to_string(ifc::to_underlying(delimiter));
        }
    }

    std::string to_string(symbolic::DestructorCallExpr::Kind kind)
    {
        using Kind = symbolic::DestructorCallExpr::Kind;
        switch (kind)
        {
        case Kind::Unknown:
            return "UnknownDtorKind";
        case Kind::Destructor:
            return "Destructor";
        case Kind::Finalizer:
            return "Finalizer";
        default:
            return "unknown-dtor-kind-constant-" + std::to_string(ifc::to_underlying(kind));
        }
    }

    std::string to_string(symbolic::InitializerExpr::Kind kind)
    {
        using Kind = symbolic::InitializerExpr::Kind;
        switch (kind)
        {
        case Kind::Unknown:
            return "unknown";
        case Kind::DirectInitialization:
            return "direct";
        case Kind::CopyInitialization:
            return "copy";
        default:
            return "unknown-initializer-kind-constant-" + std::to_string(ifc::to_underlying(kind));
        }
    }

    std::string to_string(symbolic::Associativity kind)
    {
        using Kind = symbolic::Associativity;
        switch (kind)
        {
        case Kind::Unspecified:
            return "unspecified";
        case Kind::Left:
            return "left";
        case Kind::Right:
            return "right";
        default:
            return "unknown-associativity-constant-" + std::to_string(ifc::to_underlying(kind));
        }
    }

    std::string to_string(GuideTraits traits)
    {
        std::string result;
        if (implies(traits, GuideTraits::Explicit))
            append(result, "explicit");
        return result;
    }

    std::string to_string(symbolic::BaseClassTraits traits)
    {
        using Traits = symbolic::BaseClassTraits;
        std::string result;
        if (implies(traits, Traits::Shared))
            append(result, "Shared");
        if (implies(traits, Traits::Expanded))
            append(result, "Expanded");
        return result;
    }

    std::string to_string(symbolic::SourceLocation locus)
    {
        return std::to_string(ifc::to_underlying(locus.line)) + "-" + std::to_string(ifc::to_underlying(locus.column));
    }

    // simple type printing

    namespace {
        std::string add_type_sign(std::string base, symbolic::TypeSign sign)
        {
            if (sign == symbolic::TypeSign::Signed)
                return "signed" + base;
            if (sign == symbolic::TypeSign::Unsigned)
                return "unsigned" + base;
            return base;
        }

        std::string add_type_sign_prefix(std::string base, symbolic::TypeSign sign)
        {
            if (sign == symbolic::TypeSign::Unsigned)
                base.insert(0, "u");
            return base;
        }

        std::string integer_type(symbolic::TypePrecision precision, symbolic::TypeSign sign)
        {
            // clang-format off
            switch (precision)
            {
            case symbolic::TypePrecision::Default: return add_type_sign("int", sign);
            case symbolic::TypePrecision::Short: return add_type_sign("short", sign);
            case symbolic::TypePrecision::Long: return add_type_sign("long", sign);
            case symbolic::TypePrecision::Bit8: return add_type_sign_prefix("int8", sign);
            case symbolic::TypePrecision::Bit16: return add_type_sign_prefix("int16", sign);
            case symbolic::TypePrecision::Bit32: return add_type_sign_prefix("int32", sign);
            case symbolic::TypePrecision::Bit64: return add_type_sign_prefix("int64", sign);
            case symbolic::TypePrecision::Bit128: return add_type_sign_prefix("int128", sign);
            default: return "unknown-integer-type-precision-" + std::to_string(ifc::to_underlying(precision));
            }
            // clang-format on
        }
    } // namespace

    std::string to_string(const symbolic::FundamentalType& type)
    {
        // clang-format off
    switch (type.basis)
    {
    case symbolic::TypeBasis::Void: return "void";
    case symbolic::TypeBasis::Bool: return "bool";
    case symbolic::TypeBasis::Char: return add_type_sign("char", type.sign);
    case symbolic::TypeBasis::Wchar_t: return add_type_sign("wchar_t", type.sign);
    case symbolic::TypeBasis::Int: return integer_type(type.precision, type.sign);
    case symbolic::TypeBasis::Float: return "float";
    case symbolic::TypeBasis::Double: return "double";
    case symbolic::TypeBasis::Nullptr: return "nullptr_t";
    case symbolic::TypeBasis::Ellipsis: return "...";
    case symbolic::TypeBasis::SegmentType: return "segment";
    case symbolic::TypeBasis::Class: return "class";
    case symbolic::TypeBasis::Struct: return "struct";
    case symbolic::TypeBasis::Union: return "union";
    case symbolic::TypeBasis::Enum: return "enum";
    case symbolic::TypeBasis::Typename: return "typename";
    case symbolic::TypeBasis::Namespace: return "namespace";
    case symbolic::TypeBasis::Interface: return "__interface";
    case symbolic::TypeBasis::Function: return "function-type";
    case symbolic::TypeBasis::Empty: return "empty-pack-expansion-type";
    case symbolic::TypeBasis::VariableTemplate: return "variable-template";
    case symbolic::TypeBasis::Concept: return "concept";
    case symbolic::TypeBasis::Auto: return "auto";
    case symbolic::TypeBasis::DecltypeAuto: return "decltype(auto)";
    default: return "unknown-fundamenta-type-basis-" + std::to_string(ifc::to_underlying(type.basis));
    }
        // clang-format on
    }

    std::string to_string(symbolic::TypeBasis basis)
    {
        auto type  = symbolic::FundamentalType{};
        type.basis = basis;
        return to_string(type);
    }

} // namespace ifc::util
