#include <concepts>
#include <filesystem>
#include <string_view>
#include <string>
#include <type_traits>

#include <cstdio>

#include <gsl/gsl>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "ifc/file.hxx"
#include "ifc/reader.hxx"

#ifndef IFC_FILE
#error this test must be provided an IFC file
#endif // IFC_FILE

using namespace std::literals;
using namespace ifc;

// We're also going to provide our own ifc_assert since it's only provided by the printer (which we don't need).
void ifc_assert(const char* text, const char* file, int line)
{
    fprintf(stderr, "assertion failure: ``%s'' in file ``%s'' at line %d\n", text, file, line);
    // Use the doctest internal assertion framework.
    REQUIRE(false);
}

// Some basic enum utilities.
namespace
{
    template <typename T>
    concept Enum = std::is_enum_v<T>;

    template <Enum E>
    using PrimitiveType = std::underlying_type_t<E>;

    template <Enum E>
    constexpr auto rep(E e) { return PrimitiveType<E>(e); }

    template <Enum E>
    constexpr E operator&(E a, E b) { return E(rep(a) & rep(b)); }

    template <Enum E>
    constexpr E operator|(E a, E b) { return E(rep(a) | rep(b)); }

    template <Enum E>
    constexpr E& operator&=(E& a, E b) { return a = a & b; }

    template <Enum E>
    constexpr E& operator|=(E& a, E b) { return a = a | b; }

    template <Enum E>
    constexpr auto retract(E e, PrimitiveType<E> x = 1) { return E(rep(e) - x); }

    template <Enum E>
    constexpr auto extend(E e, PrimitiveType<E> x = 1) { return E(rep(e) + x); }

    template <Enum E>
    constexpr bool implies(E a, E b) { return (a & b) == b; }

    template <Enum E>
    constexpr E unit = { };

    template <typename E>
    concept YesNoEnum = Enum<E>
                        && std::same_as<PrimitiveType<E>, bool>
                        && requires {
                            { E::Yes };
                            { E::No };
                        }
                        && rep(E::Yes) == true
                        && rep(E::No) == false;

    template <YesNoEnum E>
    constexpr bool is_yes(E e)
    {
        return rep(e);
    }

    template <YesNoEnum E>
    constexpr bool is_no(E e)
    {
        return not rep(e);
    }

    template <YesNoEnum T>
    constexpr T make_yes_no(bool value) noexcept
    {
        return static_cast<T>(value);
    }

    template <typename T>
    concept Countable = requires(T) {
        T::Count;
    };

    template <Countable T>
    constexpr bool last_of(T t)
    {
        return extend(t) == T::Count;
    }

    template <Countable T>
    constexpr auto count_of = rep(T::Count);
} // namespace [anon]

namespace
{
    bool file_exists(std::string_view filepath)
    {
        return std::filesystem::exists(filepath);
    }

    enum class Errno : int { OK };

    Errno file_size(FILE* file, size_t* size)
    {
        long pos = ftell(file);
        if (pos < 0)
            return Errno{ errno };
        if (fseek(file, 0, SEEK_END) < 0)
            return Errno{ errno};
        long result = ftell(file);
        if (result < 0)
            return Errno{ errno};
        if (fseek(file, pos, SEEK_SET) < 0)
            return Errno{ errno};
        *size = (size_t) result;
        return { };
    }

    Errno read_file(std::string_view path, std::string* buf)
    {
        FILE* file = nullptr;

        auto error = Errno{ fopen_s(&file, path.data(), "rb") };
        if (error != Errno::OK)
            return error;

        auto guard = gsl::finally([&]{ fclose(file); });

        size_t size = 0;
        Errno err = file_size(file, &size);
        if (err != Errno::OK)
            return err;

        buf->resize(size);

        fread(buf->data(), size, 1, file);
        if (ferror(file))
            return Errno{ errno };

        return Errno::OK;
    }

    void create_input_ifc(InputIfc* ifc, std::string_view buf)
    {
        // Hold your breath
        // Make a wish
        // Count to three...
        auto* bytes = reinterpret_cast<const std::byte*>(buf.data());

        *ifc = { gsl::span{ bytes, buf.size() } };
    }

    using ModuleName = Pathname;

    // This will fast track the file reading and input ifc creation.
    // Note: Both the underlying buffer and the InputIfc structure must outlive the function and have the same lifetime as the reader.
    Reader create_ifc_reader(std::string_view ifc_file, std::string* buf, InputIfc* ifc)
    {
        auto result = read_file(ifc_file, buf);
        REQUIRE(result == Errno::OK);
        create_input_ifc(ifc, *buf);
        REQUIRE(ifc->validate<UnitSort::Primary>(ifc_file.data(), Architecture::Unknown, ModuleName{ u8"m" }, IfcOptions::IntegrityCheck));
        return Reader{ *ifc };
    }

    gsl::span<const symbolic::Declaration> sequence(Reader* reader, const symbolic::Scope& scope)
    {
        const auto partition   = reader->partition<symbolic::Declaration>();
        const auto start       = to_underlying(scope.start);
        const auto cardinality = to_underlying(scope.cardinality);
        [[maybe_unused]] const auto top         = start + cardinality;
        const bool valid_range = start <= top and top <= partition.size();
        REQUIRE(valid_range);
        return partition.subspan(start, cardinality);
    }

    struct NameOfVisitor
    {
        NameOfVisitor(Reader* reader):
            reader{ reader } { }

        std::string_view operator()(TextOffset e)
        {
            return reader->get(e);
        }

        std::string_view operator()(const symbolic::OperatorFunctionId& e)
        {
            return (*this)(e.name);
        }

        std::string_view operator()(const symbolic::ConversionFunctionId& e)
        {
            return (*this)(e.name);
        }

        std::string_view operator()(const symbolic::LiteralOperatorId& e)
        {
            return (*this)(e.name_index);
        }

        std::string_view operator()(const symbolic::TemplateName& e)
        {
            return (*this)(e.name);
        }

        std::string_view operator()(const symbolic::SpecializationName& e)
        {
            return (*this)(e.primary_template);
        }

        std::string_view operator()(const symbolic::SourceFileName& e)
        {
            return (*this)(e.name);
        }

        std::string_view operator()(const symbolic::GuideName&)
        {
            // TODO: complicated.
            return { };
        }

        std::string_view operator()(NameIndex idx)
        {
            return reader->visit(idx, *this);
        }

        Reader* reader;
        std::string_view name;
    };

    void extract_name(NameIndex idx, Reader* reader, std::string* name)
    {
        NameOfVisitor visitor{ reader };
        auto extracted_name = visitor(idx);
        *name = extracted_name;
    }

    void extract_name(TextOffset idx, Reader* reader, std::string* name)
    {
        NameOfVisitor visitor{ reader };
        auto extracted_name = visitor(idx);
        *name = extracted_name;
    }
} // namespace [anon]

TEST_CASE("Basic - IFC exists")
{
    CHECK(file_exists(IFC_FILE));
}

TEST_CASE("Basic - Test file read")
{
    std::string buf;
    auto result = read_file(IFC_FILE, &buf);
    CHECK(result == Errno::OK);
    CHECK(not buf.empty());
}

TEST_CASE("Basic - IFC file validation")
{
    std::string buf;
    auto result = read_file(IFC_FILE, &buf);
    // Kind of duplicated from above...
    REQUIRE(result == Errno::OK);
    InputIfc ifc;
    create_input_ifc(&ifc, buf);
    CHECK(ifc.validate<UnitSort::Primary>(IFC_FILE, Architecture::Unknown, ModuleName{ u8"m" }, IfcOptions::IntegrityCheck));
}

TEST_CASE("IFC spec - Test header")
{
    // Since IFC file validation already validated the checksum, we just need to look at the header contents below that.
    // Section 2.8 : IFC File Header
    // checksum           : SHA256
    // major_version      : Version
    // minor_version      : Version
    // abi                : Abi
    // arch               : Architecture
    // dialect            : LanguageVersion
    // string_table_bytes : ByteOffset
    // string_table_size  : Cardinality
    // unit               : UnitIndex
    // src_path           : TextOffset
    // global_scope       : ScopeIndex
    // toc                : ByteOffset
    // partition_count    : Cardinality
    // internal           : bool

    std::string buf;
    InputIfc ifc;
    Reader reader = create_ifc_reader(IFC_FILE, &buf, &ifc);

    // The currently documented version of the IFC that the MSVC compiler will emit is 0.43.
    constexpr auto expected_version = FormatVersion{ Version{ 0 }, Version{ 43 } };
    CHECK_MESSAGE(reader.ifc.header()->version == expected_version, "minor/major");
    CHECK_MESSAGE(reader.ifc.header()->abi == Abi{}, "abi - not currently set in MSVC");
    // arch - since we compile this file in multiple modes, let's just ensure that 'arch' matches one of the known types.
    const bool valid_arch = reader.ifc.header()->arch == Architecture::Unknown
        or reader.ifc.header()->arch == Architecture::X86
        or reader.ifc.header()->arch == Architecture::X64
        or reader.ifc.header()->arch == Architecture::ARM32
        or reader.ifc.header()->arch == Architecture::ARM64
        or reader.ifc.header()->arch == Architecture::HybridX86ARM64
        or reader.ifc.header()->arch == Architecture::ARM64EC;
    CHECK_MESSAGE(valid_arch, "arch - since we compile this file in multiple modes, let's just ensure that 'arch' matches one of the known types");
    CHECK_MESSAGE(reader.ifc.header()->cplusplus == CPlusPlus{ 202002 }, "dialect - we compile in C++20 mode which has its __cplusplus macro set to 202002");
    // string_table_bytes and string_table_size don't need to be checked here as their values depend on later file content.
    CHECK_MESSAGE(reader.ifc.header()->unit.sort() == UnitSort::Primary, "unit - we compiled a named module, its unit sort should be 'Primary'");
    CHECK_MESSAGE(reader.get(reader.ifc.header()->unit.module_name()) == "m"sv, "Its unit name should be 'm'");
    std::filesystem::path src_path = reader.get(reader.ifc.header()->src_path);
    CHECK_MESSAGE(src_path.filename() == "m.ixx"sv, "src_path - should have a filename of 'm.ixx'");
    CHECK_MESSAGE(not index_like::null(reader.ifc.header()->global_scope), "global_scope - no granular check yet, we just want to ensure it's not null for now");
    CHECK_MESSAGE(not index_like::null(reader.ifc.header()->toc), "toc - once again, just make sure it's not null");
    // partition_count - arbitrary, we'll check this at a more semantic-level later.
    CHECK_MESSAGE(not reader.ifc.header()->internal_partition, "internal - this is _not_ an internal partition");
}

namespace
{
    symbolic::FundamentalType make_fundamental_type(symbolic::TypeBasis basis, symbolic::TypePrecision precision, symbolic::TypeSign sign)
    {
        return { {}, basis, precision, sign };
    }

    void check_fundamental_type(Reader* reader, TypeIndex type, symbolic::FundamentalType expected)
    {
        REQUIRE(type.sort() == TypeSort::Fundamental);
        auto& fun = reader->get<symbolic::FundamentalType>(type);
        CHECK(fun.basis == expected.basis);
        CHECK(fun.precision == expected.precision);
        CHECK(fun.sign == expected.sign);
    }

    void check_glb_void_void_func(Reader* reader, DeclIndex idx)
    {
        const auto sort = idx.sort();
        REQUIRE(sort == DeclSort::Function);
        auto& func_decl = reader->get<symbolic::FunctionDecl>(idx);
        // This is a global function, the home_scope is null.
        CHECK(null(func_decl.home_scope));
        // No arguments.
        CHECK(null(func_decl.chart));
        CHECK(func_decl.traits == FunctionTraits::None);
        CHECK(func_decl.basic_spec == (BasicSpecifiers::Cxx | BasicSpecifiers::External));
        // Access only applies to class members.
        CHECK(func_decl.access == Access::None);
        // The function is defined in the module.
        CHECK(func_decl.properties == ReachableProperties::Nothing);
        // Type check.
        REQUIRE(func_decl.type.sort() == TypeSort::Function);
        auto& type = reader->get<symbolic::FunctionType>(func_decl.type);
        CHECK(not null(type.target));
        // This function takes no arguments.
        CHECK(null(type.source));
        CHECK(type.eh_spec.sort == NoexceptSort::None);
        // This can change based on architecture compiled for.
        const bool valid_calling = type.convention == CallingConvention::Cdecl or type.convention == CallingConvention::Std;
        CHECK(valid_calling);

        // Check return type.
        check_fundamental_type(reader,
                                type.target,
                                make_fundamental_type(symbolic::TypeBasis::Void,
                                                        symbolic::TypePrecision::Default,
                                                        symbolic::TypeSign::Plain));
    }

    void check_glb_void_void_func_not_exported(Reader* reader, DeclIndex idx)
    {
        const auto sort = idx.sort();
        REQUIRE(sort == DeclSort::Function);
        auto& func_decl = reader->get<symbolic::FunctionDecl>(idx);
        // This is a global function, the home_scope is null.
        CHECK(null(func_decl.home_scope));
        // No arguments.
        CHECK(null(func_decl.chart));
        CHECK(func_decl.traits == FunctionTraits::None);
        // This is not exported
        CHECK(func_decl.basic_spec == (BasicSpecifiers::Cxx | BasicSpecifiers::External | BasicSpecifiers::NonExported));
        // Access only applies to class members.
        CHECK(func_decl.access == Access::None);
        // The function is defined in the module.
        CHECK(func_decl.properties == ReachableProperties::Nothing);
        // Type check.
        REQUIRE(func_decl.type.sort() == TypeSort::Function);
        auto& type = reader->get<symbolic::FunctionType>(func_decl.type);
        CHECK(not null(type.target));
        // This function takes no arguments.
        CHECK(null(type.source));
        CHECK(type.eh_spec.sort == NoexceptSort::None);
        // This can change based on architecture compiled for.
        const bool valid_calling = type.convention == CallingConvention::Cdecl or type.convention == CallingConvention::Std;
        CHECK(valid_calling);

        // Check return type.
        check_fundamental_type(reader,
                                type.target,
                                make_fundamental_type(symbolic::TypeBasis::Void,
                                                        symbolic::TypePrecision::Default,
                                                        symbolic::TypeSign::Plain));
    }

    void check_glb_int_int_func(Reader* reader, DeclIndex idx)
    {
        const auto sort = idx.sort();
        REQUIRE(sort == DeclSort::Function);
        auto& func_decl = reader->get<symbolic::FunctionDecl>(idx);
        // This is a global function, the home_scope is null.
        CHECK(null(func_decl.home_scope));

        REQUIRE(func_decl.chart.sort() == ChartSort::Unilevel);
        auto& chart = reader->get<symbolic::UnilevelChart>(func_decl.chart);
        // No requirement.
        CHECK(null(chart.requires_clause));
        REQUIRE(chart.cardinality == Cardinality{ 1 });
        auto chart_seq = reader->sequence(chart);
        auto& parm = chart_seq[0];
        CHECK(null(as_expr_index(parm.initializer)));
        check_fundamental_type(reader,
                                parm.type,
                                make_fundamental_type(symbolic::TypeBasis::Int,
                                                        symbolic::TypePrecision::Default,
                                                        symbolic::TypeSign::Plain));
        CHECK(parm.level == 0);
        CHECK(parm.position == 0);
        CHECK(parm.sort == ParameterSort::Object);
        CHECK(parm.properties == ReachableProperties::Nothing);

        CHECK(func_decl.traits == FunctionTraits::None);
        CHECK(func_decl.basic_spec == (BasicSpecifiers::Cxx | BasicSpecifiers::External));
        // Access only applies to class members.
        CHECK(func_decl.access == Access::None);
        // The function is defined in the module.
        CHECK(func_decl.properties == ReachableProperties::Nothing);
        // Type check.
        REQUIRE(func_decl.type.sort() == TypeSort::Function);
        auto& type = reader->get<symbolic::FunctionType>(func_decl.type);
        CHECK(not null(type.target));

        // This function has one 'int'.
        check_fundamental_type(reader,
                                type.source,
                                make_fundamental_type(symbolic::TypeBasis::Int,
                                                        symbolic::TypePrecision::Default,
                                                        symbolic::TypeSign::Plain));

        CHECK(type.eh_spec.sort == NoexceptSort::None);
        // This can change based on architecture compiled for.
        const bool valid_calling = type.convention == CallingConvention::Cdecl or type.convention == CallingConvention::Std;
        CHECK(valid_calling);

        // Check return type.
        check_fundamental_type(reader,
                                type.target,
                                make_fundamental_type(symbolic::TypeBasis::Int,
                                                        symbolic::TypePrecision::Default,
                                                        symbolic::TypeSign::Plain));
    }

    void check_glb_int_int_char_char_ptr_func(Reader* reader, DeclIndex idx)
    {
        const auto sort = idx.sort();
        REQUIRE(sort == DeclSort::Function);
        auto& func_decl = reader->get<symbolic::FunctionDecl>(idx);
        // This is a global function, the home_scope is null.
        CHECK(null(func_decl.home_scope));

        REQUIRE(func_decl.chart.sort() == ChartSort::Unilevel);
        auto& chart = reader->get<symbolic::UnilevelChart>(func_decl.chart);
        // No requirement.
        CHECK(null(chart.requires_clause));
        REQUIRE(chart.cardinality == Cardinality{ 3 });
        auto chart_seq = reader->sequence(chart);
        {
            auto& parm = chart_seq[0];
            CHECK(null(as_expr_index(parm.initializer)));
            check_fundamental_type(reader,
                                    parm.type,
                                    make_fundamental_type(symbolic::TypeBasis::Int,
                                                            symbolic::TypePrecision::Default,
                                                            symbolic::TypeSign::Plain));
            CHECK(parm.level == 0);
            CHECK(parm.position == 0);
            CHECK(parm.sort == ParameterSort::Object);
            CHECK(parm.properties == ReachableProperties::Nothing);
        }
        {
            auto& parm = chart_seq[1];
            CHECK(null(as_expr_index(parm.initializer)));
            check_fundamental_type(reader,
                                    parm.type,
                                    make_fundamental_type(symbolic::TypeBasis::Char,
                                                            symbolic::TypePrecision::Default,
                                                            symbolic::TypeSign::Plain));
            CHECK(parm.level == 0);
            CHECK(parm.position == 0);
            CHECK(parm.sort == ParameterSort::Object);
            CHECK(parm.properties == ReachableProperties::Nothing);
        }
        {
            auto& parm = chart_seq[2];
            CHECK(null(as_expr_index(parm.initializer)));
            // This type is a 'char*' and encoded as Pointer -> Fundamental.
            REQUIRE(parm.type.sort() == TypeSort::Pointer);
            auto& ptr = reader->get<symbolic::PointerType>(parm.type);
            check_fundamental_type(reader,
                                    ptr.pointee,
                                    make_fundamental_type(symbolic::TypeBasis::Char,
                                                            symbolic::TypePrecision::Default,
                                                            symbolic::TypeSign::Plain));
            CHECK(parm.level == 0);
            CHECK(parm.position == 0);
            CHECK(parm.sort == ParameterSort::Object);
            CHECK(parm.properties == ReachableProperties::Nothing);
        }

        CHECK(func_decl.traits == FunctionTraits::None);
        CHECK(func_decl.basic_spec == (BasicSpecifiers::Cxx | BasicSpecifiers::External));
        // Access only applies to class members.
        CHECK(func_decl.access == Access::None);
        // The function is defined in the module.
        CHECK(func_decl.properties == ReachableProperties::Nothing);
        // Type check.
        REQUIRE(func_decl.type.sort() == TypeSort::Function);
        auto& type = reader->get<symbolic::FunctionType>(func_decl.type);
        CHECK(not null(type.target));

        // This function has '(int, char, char*)' as the parameter-type-list.
        REQUIRE(type.source.sort() == TypeSort::Tuple);
        auto& src_tup = reader->get<symbolic::TupleType>(type.source);
        REQUIRE(src_tup.cardinality == Cardinality{ 3 });
        auto src_seq = reader->sequence(src_tup);
        {
            auto src_type = src_seq[0];
            check_fundamental_type(reader,
                                    src_type,
                                    make_fundamental_type(symbolic::TypeBasis::Int,
                                                            symbolic::TypePrecision::Default,
                                                            symbolic::TypeSign::Plain));
        }
        {
            auto src_type = src_seq[1];
            check_fundamental_type(reader,
                                    src_type,
                                    make_fundamental_type(symbolic::TypeBasis::Char,
                                                            symbolic::TypePrecision::Default,
                                                            symbolic::TypeSign::Plain));
        }
        {
            auto src_type = src_seq[2];
            REQUIRE(src_type.sort() == TypeSort::Pointer);
            auto& ptr = reader->get<symbolic::PointerType>(src_type);
            check_fundamental_type(reader,
                                    ptr.pointee,
                                    make_fundamental_type(symbolic::TypeBasis::Char,
                                                            symbolic::TypePrecision::Default,
                                                            symbolic::TypeSign::Plain));
        }

        CHECK(type.eh_spec.sort == NoexceptSort::None);
        // This can change based on architecture compiled for.
        const bool valid_calling = type.convention == CallingConvention::Cdecl or type.convention == CallingConvention::Std;
        CHECK(valid_calling);

        // Check return type.
        check_fundamental_type(reader,
                                type.target,
                                make_fundamental_type(symbolic::TypeBasis::Int,
                                                        symbolic::TypePrecision::Default,
                                                        symbolic::TypeSign::Plain));
    }

    template <FunctionTraits Trait>
    void check_function_trait(FunctionTraits traits)
    {
        CHECK(implies(traits, Trait));
    }

    template <typename S>
    void check_scope_decl_function_traits(Reader* reader, const symbolic::ScopeDecl& scope_decl, S* set)
    {
        if (set->empty())
            return;
        std::string name_buf;

        REQUIRE(not index_like::null(scope_decl.initializer));
        auto* scope = reader->try_get(scope_decl.initializer);
        REQUIRE(scope != nullptr);
        auto span = sequence(reader, *scope);
        for (const auto& decl : span)
        {
            switch (decl.index.sort())
            {
            case DeclSort::Method:
                {
                    auto& func_decl = reader->get<symbolic::NonStaticMemberFunctionDecl>(decl.index);
                    extract_name(func_decl.identity.name, reader, &name_buf);
                    auto found = set->find(name_buf);
                    if (found != end(*set))
                    {
                        found->second(func_decl.traits);
                        set->erase(found);
                    }
                }
                break;
            case DeclSort::Constructor:
                {
                    auto& func_decl = reader->get<symbolic::ConstructorDecl>(decl.index);
                    extract_name(func_decl.identity.name, reader, &name_buf);
                    auto found = set->find(name_buf);
                    if (found != end(*set))
                    {
                        found->second(func_decl.traits);
                        set->erase(found);
                    }
                }
                break;
            }
        }
        CHECK(set->empty());
    }

    void check_function_traits(Reader* reader, DeclIndex ns)
    {
        using MatchingTraitSet = std::map<std::string, void(*)(FunctionTraits)>;

        struct ScopeSetData
        {
            using Func = void(*)(Reader*, const symbolic::ScopeDecl&, MatchingTraitSet*);

            MatchingTraitSet* member_set;
            Func func;
        };

        using MatchingScopeSet = std::map<std::string, ScopeSetData>;

        std::string name_buf;

        MatchingTraitSet expected_traits {
            { "trait_none", check_function_trait<FunctionTraits::None> },
            { "trait_inline", check_function_trait<FunctionTraits::Inline> },
            { "trait_constexpr", check_function_trait<FunctionTraits::Constexpr> },
            { "trait_noreturn", check_function_trait<FunctionTraits::NoReturn> },
            { "trait_deleted", check_function_trait<FunctionTraits::Deleted> },
            { "trait_constrained", check_function_trait<FunctionTraits::Constrained> },
            { "trait_immediate", check_function_trait<FunctionTraits::Immediate> },
        };

        MatchingTraitSet ClassScope_expected_traits {
            { "{ctor}", check_function_trait<FunctionTraits::Explicit> },
            { "trait_virtual", check_function_trait<FunctionTraits::Virtual> },
            { "trait_pure_virtual", check_function_trait<FunctionTraits::PureVirtual> },
            { "==", check_function_trait<FunctionTraits::Defaulted> },
        };

        MatchingTraitSet DerivedClassScope_expected_traits {
            { "trait_final", check_function_trait<FunctionTraits::Final> },
            { "trait_override", check_function_trait<FunctionTraits::Override> },
        };

        MatchingScopeSet expected_scopes {
            { "ClassScope", { &ClassScope_expected_traits, check_scope_decl_function_traits<MatchingTraitSet> } },
            { "DerivedClassScope", { &DerivedClassScope_expected_traits, check_scope_decl_function_traits<MatchingTraitSet> } },
        };

        REQUIRE(ns.sort() == DeclSort::Scope);
        auto& scope_decl = reader->get<symbolic::ScopeDecl>(ns);
        check_fundamental_type(reader,
                                scope_decl.type,
                                make_fundamental_type(symbolic::TypeBasis::Namespace,
                                                        symbolic::TypePrecision::Default,
                                                        symbolic::TypeSign::Plain));
        REQUIRE(not index_like::null(scope_decl.initializer));
        auto* scope = reader->try_get(scope_decl.initializer);
        REQUIRE(scope != nullptr);
        auto span = sequence(reader, *scope);

        for (const auto& decl : span)
        {
            switch (decl.index.sort())
            {
            case DeclSort::Function:
                {
                    auto& func_decl = reader->get<symbolic::FunctionDecl>(decl.index);
                    extract_name(func_decl.identity.name, reader, &name_buf);
                    auto found = expected_traits.find(name_buf);
                    REQUIRE(found != end(expected_traits));
                    found->second(func_decl.traits);
                    expected_traits.erase(found);
                }
                break;
            case DeclSort::Template:
                {
                    auto& templ = reader->get<symbolic::TemplateDecl>(decl.index);
                    REQUIRE(templ.entity.decl.sort() == DeclSort::Function);
                    auto& func_decl = reader->get<symbolic::FunctionDecl>(templ.entity.decl);
                    extract_name(func_decl.identity.name, reader, &name_buf);
                    auto found = expected_traits.find(name_buf);
                    REQUIRE(found != end(expected_traits));
                    found->second(func_decl.traits);
                    expected_traits.erase(found);
                }
                break;
            case DeclSort::Scope:
                {
                    auto& inner_scope_decl = reader->get<symbolic::ScopeDecl>(decl.index);
                    extract_name(inner_scope_decl.identity.name, reader, &name_buf);
                    auto found = expected_scopes.find(name_buf);
                    REQUIRE(found != end(expected_scopes));
                    found->second.func(reader, inner_scope_decl, found->second.member_set);
                    expected_scopes.erase(found);
                }
                break;
            }
        }
        CHECK(expected_traits.empty());
        CHECK(expected_scopes.empty());
        CHECK(ClassScope_expected_traits.empty());
        CHECK(DerivedClassScope_expected_traits.empty());
    }
} // namespace [anon]

TEST_CASE("IFC spec - decls")
{
    std::string buf;
    InputIfc ifc;
    Reader reader = create_ifc_reader(IFC_FILE, &buf, &ifc);
    std::string name_buf;

    using MatchingDeclSet = std::map<std::string, void(*)(Reader*, DeclIndex)>;

    MatchingDeclSet expected_decls {
        { "glb_void_void_func", check_glb_void_void_func },
        { "glb_void_void_func_not_exported", check_glb_void_void_func_not_exported },
        { "glb_int_int_func", check_glb_int_int_func },
        { "glb_int_int_char_char_ptr_func", check_glb_int_int_char_char_ptr_func },
        { "FunctionTraits", check_function_traits },
    };

    CHECK_MESSAGE(not index_like::null(reader.ifc.header()->global_scope), "global_scope should be populated for this IFC");

    // Extract the scope.
    using namespace ifc;
    auto* scope = reader.try_get(reader.ifc.header()->global_scope);
    CHECK(to_underlying(scope->cardinality) != 0);
    auto span = sequence(&reader, *scope);
    for (const auto& decl : span)
    {
        const auto sort = decl.index.sort();
        switch (sort)
        {
        case DeclSort::Function:
            {
                auto& func_decl = reader.get<symbolic::FunctionDecl>(decl.index);
                extract_name(func_decl.identity.name, &reader, &name_buf);
            }
            break;
        case DeclSort::Scope:
            {
                auto& scope_decl = reader.get<symbolic::ScopeDecl>(decl.index);
                extract_name(scope_decl.identity.name, &reader, &name_buf);
            }
            break;
        default:
            // So that the sort is printed.
            REQUIRE(sort != sort);
        }
        auto found = expected_decls.find(name_buf);
        REQUIRE(found != end(expected_decls));
        found->second(&reader, decl.index);

        // Remove this from the set so we don't check against it again.
        expected_decls.erase(found);
    }
    CHECK(expected_decls.empty());
}