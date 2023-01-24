#include "ifc/util.hxx"

namespace Module::util
{
    namespace
    {
        void append(std::string& result, std::string_view suffix, std::string sep = " ")
        {
            if (!result.empty() && !suffix.empty())
                result.append(sep);
            result.append(suffix);
        }
    }  // namespace

    std::string to_string(Access access)
    {
        switch (access)
        {
        case Access::None: return "";
        case Access::Private: return "private";
        case Access::Protected: return "protected";
        case Access::Public: return "public";
        default: return "unknown-access-" + std::to_string((int)access);
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

    std::string to_string(Module::Qualifier qual)
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

    std::string to_string(Module::Symbolic::ExpansionMode mode)
    {
        if (mode == Symbolic::ExpansionMode::Full)
            return "...";
        if (mode == Symbolic::ExpansionMode::Partial)
            return "...(partial)";
        return "unknown-expansion-mode-:" + std::to_string((int)mode);
    };

    std::string to_string(Symbolic::ReadExpression::Kind kind)
    {
        using Kind = Symbolic::ReadExpression::Kind;
        switch (kind)
        {
        case Kind::Unknown: return "unknown";
        case Kind::Indirection: return "indirection";
        case Kind::RemoveReference: return "remove-reference";
        case Kind::LvalueToRvalue: return "lvalue-to-rvalue";
        case Kind::IntegralConversion: return "integral-conversion";
        default: return "unknown-read-kind-" + std::to_string((int)kind);
        }
    }

    std::string to_string(CallingConvention conv)
    {
        switch (conv)
        {
        case CallingConvention::Cdecl: return "__cdecl";
        case CallingConvention::Fast: return "__fastcall";
        case CallingConvention::Std: return "__stdcall";
        case CallingConvention::This: return "__thiscall";
        case CallingConvention::Clr: return "__clrcall";
        case CallingConvention::Vector: return "__vectorcall";
        case CallingConvention::Eabi: return "__eabi";
        default: return "calling-conv-" + std::to_string((int)conv);
        }
    }

    std::string to_string(NoexceptSort sort)
    {
        switch (sort)
        {
        case NoexceptSort::False: return "noexcept(false)";
        case NoexceptSort::True: return "noexcept(true)";
        case NoexceptSort::Expression: return "noexcept(<expression>)";
        case NoexceptSort::InferredSpecialMember: return "noexcept(<inferred-special-member>)";
        case NoexceptSort::Unenforced: return "noexcept(<unenforced>)";
        default: return "unknown-noexcept-sort-" + std::to_string((int)sort);
        }
    }

    std::string to_string(Symbolic::ExpressionList::Delimiter delimiter)
    {
        using Kind = Symbolic::ExpressionList::Delimiter;
        switch (delimiter)
        {
        case Kind::Unknown: return "Unknown";
        case Kind::Brace: return "Brace";
        case Kind::Parenthesis: return "Parenthesis";
        default: return "unknown-delimiter-kind-" + std::to_string((int)delimiter);
        }
    }

    std::string to_string(Symbolic::DestructorCall::Kind kind)
    {
        using Kind = Symbolic::DestructorCall::Kind;
        switch (kind)
        {
        case Kind::Unknown: return "UnknownDtorKind";
        case Kind::Destructor: return "Destructor";
        case Kind::Finalizer: return "Finalizer";
        default: return "unknown-dtor-kind-constant-" + std::to_string((int)kind);
        }
    }

    std::string to_string(Symbolic::Initializer::Kind kind)
    {
        using Kind = Symbolic::Initializer::Kind;
        switch (kind)
        {
        case Kind::Unknown: return "unknown";
        case Kind::DirectInitialization: return "direct";
        case Kind::CopyInitialization: return "copy";
        default: return "unknown-initializer-kind-constant-" + std::to_string((int)kind);
        }
    }

    std::string to_string(Symbolic::Associativity kind)
    {
        using Kind = Symbolic::Associativity;
        switch (kind)
        {
        case Kind::Unspecified: return "unspecified";
        case Kind::Left: return "left";
        case Kind::Right: return "right";
        default: return "unknown-associativity-constant-" + std::to_string((int)kind);
        }
    }

    std::string to_string(GuideTraits traits)
    {
        std::string result;
        if (implies(traits, GuideTraits::Explicit))
            append(result, "explicit");
        return result;
    }

    std::string to_string(Symbolic::BaseClassTraits traits)
    {
        using Traits = Symbolic::BaseClassTraits;
        std::string result;
        if (implies(traits, Traits::Shared))
            append(result, "Shared");
        if (implies(traits, Traits::Expanded))
            append(result, "Expanded");
        return result;
    }

    std::string to_string(Symbolic::SourceLocation locus)
    {
        return std::to_string((unsigned)locus.line) + "-" + std::to_string((unsigned)locus.column);
    }

    // simple type printing

    namespace
    {
        std::string add_type_sign(std::string base, Symbolic::TypeSign sign)
        {
            if (sign == Symbolic::TypeSign::Signed)
                return "signed" + base;
            if (sign == Symbolic::TypeSign::Unsigned)
                return "unsigned" + base;
            return std::move(base);
        }

        std::string add_type_sign_prefix(std::string base, Symbolic::TypeSign sign)
        {
            if (sign == Symbolic::TypeSign::Unsigned)
                base.insert(0, "u");
            return std::move(base);
        }

        std::string integer_type(Symbolic::TypePrecision precision, Symbolic::TypeSign sign)
        {
            // clang-format off
            switch (precision)
            {
            case Symbolic::TypePrecision::Default: return add_type_sign("int", sign);
            case Symbolic::TypePrecision::Short: return add_type_sign("short", sign);
            case Symbolic::TypePrecision::Long: return add_type_sign("long", sign);
            case Symbolic::TypePrecision::Bit8: return add_type_sign_prefix("int8", sign);
            case Symbolic::TypePrecision::Bit16: return add_type_sign_prefix("int16", sign);
            case Symbolic::TypePrecision::Bit32: return add_type_sign_prefix("int32", sign);
            case Symbolic::TypePrecision::Bit64: return add_type_sign_prefix("int64", sign);
            case Symbolic::TypePrecision::Bit128: return add_type_sign_prefix("int128", sign);
            default: return "unknown-integer-type-precision-" + std::to_string((int)precision);
            }
            // clang-format on
        }
    }  // namespace

    std::string to_string(const Symbolic::FundamentalType& type)
    {
        // clang-format off
    switch (type.basis)
    {
    case Symbolic::TypeBasis::Void: return "void";
    case Symbolic::TypeBasis::Bool: return "bool";
    case Symbolic::TypeBasis::Char: return add_type_sign("char", type.sign);
    case Symbolic::TypeBasis::Wchar_t: return add_type_sign("wchar_t", type.sign);
    case Symbolic::TypeBasis::Int: return integer_type(type.precision, type.sign);
    case Symbolic::TypeBasis::Float: return "float";
    case Symbolic::TypeBasis::Double: return "double";
    case Symbolic::TypeBasis::Nullptr: return "nullptr_t";
    case Symbolic::TypeBasis::Ellipsis: return "...";
    case Symbolic::TypeBasis::SegmentType: return "segment";
    case Symbolic::TypeBasis::Class: return "class";
    case Symbolic::TypeBasis::Struct: return "struct";
    case Symbolic::TypeBasis::Union: return "union";
    case Symbolic::TypeBasis::Enum: return "enum";
    case Symbolic::TypeBasis::Typename: return "typename";
    case Symbolic::TypeBasis::Namespace: return "namespace";
    case Symbolic::TypeBasis::Interface: return "__interface";
    case Symbolic::TypeBasis::Function: return "function-type";
    case Symbolic::TypeBasis::Empty: return "empty-pack-expansion-type";
    case Symbolic::TypeBasis::VariableTemplate: return "variable-template";
    case Symbolic::TypeBasis::Concept: return "concept";
    case Symbolic::TypeBasis::Auto: return "auto";
    case Symbolic::TypeBasis::DecltypeAuto: return "decltype(auto)";
    default: return "unknown-fundamenta-type-basis-" + std::to_string((int)type.basis);
    }
        // clang-format on
    }

    std::string to_string(Symbolic::TypeBasis basis)
    {
        return to_string(Symbolic::FundamentalType{basis, {}, {}});
    }

}  // namespace Module::util
