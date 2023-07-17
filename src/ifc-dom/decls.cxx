#include "common.hxx"
#include "ifc/util.hxx"

namespace ifc::util
{
    void load(Loader& ctx, Node& node, ScopeIndex index)
    {
        if (auto* scope = ctx.reader.try_get(index))
        {
            const auto seq = ctx.reader.sequence(*scope);
            node.id = "scope-" + std::to_string((int)index);
            node.children.reserve(seq.size());

            for (auto& decl : seq)
                node.children.push_back(&ctx.get(decl.index));
        }
    }

    std::string to_string(Loader& ctx, SpecFormIndex spec_form_index)
    {
        const auto& spec_form = ctx.reader.get(spec_form_index);
        std::string result{ctx.ref(spec_form.template_decl)};
        result.append(expr_list(ctx, spec_form.arguments, "<>"));
        return result;
    }

    namespace
    {
        symbolic::TypeBasis get_type_basis(Loader& pp, TypeIndex index)
        {
            // Must be a fundamental type.
            // variable template => TypeBasis::VariableTemplate.
            // function template => TypeBasis::Function
            // class template => TypeBasis::Struct or TypeBasis::Class
            // alias template => TypeBasis::Typename
            // enum => TypeBasis::Enum (legacy) or TypeBasis::Struct or TypeBasis::Class
            auto& fundamental = pp.reader.get<symbolic::FundamentalType>(index);
            return fundamental.basis;
        }
    }  // namespace [anon]

    template <index_like::Algebra Index>
    void load_initializer(Loader& ctx, Node& n, Index index)
    {
        if (not index_like::null(index))
            n.children.push_back(&ctx.get(index));
    }

    template <typename T, auto... Tags>
    void load_initializer(Loader& ctx, Node& n, const Sequence<T, Tags...>& seq)
    {
        for (const auto& item: ctx.reader.sequence(seq))
            n.children.push_back(&ctx.get(item));
    }

    void load_specializations(Loader& ctx, DeclIndex decl_index)
    {
        if (auto* specializations =
                ctx.reader.try_find<symbolic::trait::Specializations>(decl_index))
            for (auto& decl : ctx.reader.sequence(specializations->trait))
                ctx.referenced_nodes.insert(decl.index);
    }

    // clang-format off
    template <class T> concept HasAccess = requires(T x) { x.access; };
    template <class T> concept HasAlignment = requires(T x) { x.alignment; };
    template <class T> concept HasPackSize = requires(T x) { x.pack_size; };
    template <class T> concept HasBase = requires(T x) { x.base; };
    template <class T> concept HasBaseCtor = requires(T x) { x.base_ctor; };
    template <class T> concept HasBasicSpec = requires(T x) { x.basic_spec; };
    template <class T> concept HasIdentity = requires(T x) { x.identity; };
    template <class T> concept HasScopeSpec = requires(T x) { x.scope_spec; };
    template <class T> concept HasTraits = requires(T x) { x.traits; };
    template <class T> concept HasObjectSpec = requires(T x) { x.obj_spec; };
    template <class T> concept HasProperties = requires(T x) { x.properties; };
    template <class T> concept HasType = requires(T x) { x.type; };
    template <class T> concept HasSourceType = requires(T x) { {x.source} -> std::convertible_to<TypeIndex>; };
    template <class T> concept HasInitializer = requires(T x) { x.initializer; };
    template <class T> concept HasCallingConvention = requires(T x) { x.convention; };
    template <class T> concept HasNoexceptSpecification = requires(T x) { x.eh_spec; };
    template <class T> concept HasHomeScope = requires(T x) { x.home_scope; };
    // clang-format on

    template <typename T>
    void load_common_props(Loader& ctx, Node& n, const T& val)
    {
        if constexpr (HasType<T>)
            n.props.emplace("type", ctx.ref(val.type));

        if constexpr (HasIdentity<T>)
            n.props.emplace("name", ctx.ref(val.identity));

        if constexpr (HasAccess<T>)
            n.props.emplace("access", to_string(val.access));

        if constexpr (HasSourceType<T>)
            n.props.emplace("source", ctx.ref(val.source));

        if constexpr (HasBase<T>)
            if (not null(val.base))
                n.props.emplace("base", ctx.ref(val.base)); // type

        if constexpr (HasBaseCtor<T>) // reusing 'base' key intentionally
            n.props.emplace("base", ctx.ref(val.base_ctor)); // declref

        if constexpr (HasBasicSpec<T>)
            n.props.emplace("basic-specifiers", to_string(val.basic_spec));

        if constexpr (HasScopeSpec<T>)
            n.props.emplace("scope-specifiers", to_string(val.scope_spec));

        if constexpr (HasObjectSpec<T>)
            n.props.emplace("object-specifiers", to_string(val.obj_spec));

        if constexpr (HasTraits<T>)
            n.props.emplace("traits", to_string(val.traits));

        if constexpr (HasProperties<T>)
            n.props.emplace("reachable-properties", to_string(val.properties));

        if constexpr (HasCallingConvention<T>)
            n.props.emplace("calling-convention", to_string(val.convention));

        if constexpr (HasHomeScope<T>)
            if (not null(val.home_scope))
                n.props.emplace("home-scope", to_string(val.home_scope));

        // TODO: revisit eh spec once the tokens are replaced with expressions
        if constexpr (HasNoexceptSpecification<T>)
            if (val.eh_spec.sort != NoexceptSort::None)
                n.props.emplace("noexcept-specificatinos", to_string(val.eh_spec.sort));

        if constexpr (HasInitializer<T>)
            load_initializer(ctx, n, val.initializer);

        if constexpr (HasAlignment<T>)
            if (not null(val.alignment))
                n.props.emplace("alignment", ctx.ref(val.alignment));

        if constexpr (HasPackSize<T>)
            if (auto size = static_cast<unsigned int>(val.pack_size))
                n.props.emplace("pack_size", std::to_string(size));
    }

    void load_function_body(Loader& ctx, Node& node, DeclIndex fn_index, ChartIndex chart = {})
    {
        // Constexpr functions
        if (auto* mapping_def = ctx.reader.try_find<symbolic::trait::MappingExpr>(fn_index))
        {
            if (auto* params = ctx.try_get(mapping_def->trait.parameters))
                node.children.push_back(params);

            if (auto init = mapping_def->trait.initializers; not null(init))
                node.children.push_back(&ctx.get(init));

            node.children.push_back(&ctx.get(mapping_def->trait.body));
        }
        // Inline functions (and all other under a switch to be added).
        else if (auto* mapping_def =
                     ctx.reader.try_find<symbolic::trait::MsvcCodegenMappingExpr>(fn_index))
        {
            if (auto* params = ctx.try_get(mapping_def->trait.parameters))
                node.children.push_back(params);

            if (auto init = mapping_def->trait.initializers; not null(init))
                node.children.push_back(&ctx.get(init));

            node.children.push_back(&ctx.get(mapping_def->trait.body));
        }
        else
        {
            if (auto* params = ctx.try_get(chart))
                node.children.push_back(params);
        }
    }

    struct Decl_loader : detail::Loader_visitor_base
    {
        void load_friends(DeclIndex decl_index)
        {
            if (auto* friends = ctx.reader.try_find<symbolic::trait::Friends>(decl_index))
                for (auto& decl : ctx.reader.sequence(friends->trait))
                    add_child(decl.index);
        }

        void load_deduction_guides(DeclIndex decl_index)
        {
            if (auto* guides = ctx.reader.try_find<symbolic::trait::DeductionGuides>(decl_index))
                add_child(guides->trait);
        }

        void operator()(DeclIndex decl_index, const symbolic::ScopeDecl& udt)
        {
            load_common_props(ctx, node, udt);
            load_friends(decl_index);
            load_deduction_guides(decl_index);
        }

        void operator()(DeclIndex, const symbolic::FieldDecl& field)
        {
            load_common_props(ctx, node, field);
        }

        void operator()(DeclIndex, const symbolic::VariableDecl& var)
        {
            load_common_props(ctx, node, var);
        }

        void operator()(DeclIndex, const symbolic::ParameterDecl& param)
        {
            load_common_props(ctx, node, param);

            if (not null(param.type_constraint))
                node.children.push_back(&ctx.get(param.type_constraint));
        }

        void operator()(DeclIndex fn_index, const symbolic::FunctionDecl& fun)
        {
            load_common_props(ctx, node, fun);
            load_function_body(ctx, node, fn_index, fun.chart);
        }


        void operator()(DeclIndex fn_index, const symbolic::ConstructorDecl& ctor)
        {
            load_common_props(ctx, node, ctor);
            load_function_body(ctx, node, fn_index, ctor.chart);
        }

        void operator()(DeclIndex fn_index, const symbolic::DestructorDecl& dtor)
        {
            load_common_props(ctx, node, dtor);
            load_function_body(ctx, node, fn_index);
        }

        void operator()(DeclIndex fn_index, const symbolic::NonStaticMemberFunctionDecl& fun)
        {
            load_common_props(ctx, node, fun);
            load_function_body(ctx, node, fn_index, fun.chart);
        }

        void operator()(DeclIndex fn_index, const symbolic::InheritedConstructorDecl& ctor)
        {
            load_common_props(ctx, node, ctor);
            load_function_body(ctx, node, fn_index, ctor.chart);
        }

        void operator()(DeclIndex, const symbolic::EnumeratorDecl& enumerator)
        {
            load_common_props(ctx, node, enumerator);
        }

        void operator()(DeclIndex, const symbolic::BitfieldDecl& bitfield)
        {
            add_child(bitfield.width); // to appear as a first child before initializer
            load_common_props(ctx, node, bitfield);
        }

        void operator()(DeclIndex, const symbolic::EnumerationDecl& enumeration)
        {
            load_common_props(ctx, node, enumeration);
        }

        void operator()(DeclIndex, const symbolic::AliasDecl& alias)
        {
            load_common_props(ctx, node, alias);
            node.props.emplace("aliasee", ctx.ref(alias.aliasee));

        }

        void operator()(DeclIndex, const symbolic::TemploidDecl& decl)
        {
            load_common_props(ctx, node, decl);
            add_child_if_not_null(decl.chart);
        }

        void operator()(DeclIndex decl_index, const symbolic::TemplateDecl& decl)
        {
            load_common_props(ctx, node, decl);
            if (auto* params = ctx.try_get(decl.chart))
                node.children.push_back(params);
            add_child(decl.entity.decl);
            if (get_type_basis(ctx, decl.type) == symbolic::TypeBasis::Function)
                load_function_body(ctx, node, decl_index);
            load_specializations(ctx, decl_index);
        }

        void operator()(DeclIndex, const symbolic::PartialSpecializationDecl& decl)
        {
            load_common_props(ctx, node, decl);
            // Indentity stores mangled name for partial specializations,
            // Replace the name property with a proper name.
            node.props["mangled"] = node.props["name"];
            node.props["name"] = to_string(ctx, decl.specialization_form);
            if (auto* params = ctx.try_get(decl.chart))
                node.children.push_back(params);
            add_child(decl.entity.decl);
        }

        void operator()(DeclIndex, const symbolic::SpecializationDecl& decl)
        {
            node.props["name"] = to_string(ctx, decl.specialization_form);
            add_child(decl.decl);
        }

        void operator()(DeclIndex, const symbolic::FriendDeclaration& expr)
        {
            node.props.emplace("type", ctx.ref(expr.index));
        }

        void operator()(DeclIndex, const symbolic::ExpansionDecl& decl)
        {
            add_child(decl.operand);
        }

        void operator()(DeclIndex, const symbolic::Concept& decl)
        {
            load_common_props(ctx, node, decl);
            add_child(decl.constraint);
            add_child(decl.head);
            add_child(decl.body);
        }

        void operator()(DeclIndex, const symbolic::ReferenceDecl& decl)
        {
            std::string name = index_like::null(decl.translation_unit.owner) ?
                "<global>" :
                ctx.reader.get(decl.translation_unit.owner);
            if (not index_like::null(decl.translation_unit.partition))
                name += ":" + std::string(ctx.reader.get(decl.translation_unit.partition));

            node.props.emplace("name", name);
            node.props.emplace("ref", ctx.ref(decl.local_index));
        }

        void operator()(DeclIndex, const symbolic::UsingDeclaration& decl)
        {
            node.props.emplace("ref", ctx.ref(decl.resolution));
            load_common_props(ctx, node, decl);
            add_child_if_not_null(decl.parent);

            if (decl.is_hidden)
                node.props.emplace("is_hidden", "hidden");
            if (not index_like::null(decl.name))
                node.props.emplace("member_name", ctx.reader.get(decl.name));
        }

        void operator()(DeclIndex, const symbolic::DeductionGuideDecl& decl)
        {
            load_common_props(ctx, node, decl);
            add_child_if_not_null(decl.target); // Function parameters mentioned in the deduction guide declaration
        }

        void operator()(DeclIndex, const symbolic::TupleDecl& tuple)
        {
            node.children.reserve((size_t)tuple.cardinality);
            for (auto item : ctx.reader.sequence(tuple))
                add_child(item);
        }

        void operator()(DeclIndex, const symbolic::IntrinsicDecl& decl)
        {
            load_common_props(ctx, node, decl);
        }

        void operator()(DeclIndex, const symbolic::PropertyDeclaration& prop)
        {
            node.props.emplace("ref", ctx.ref(prop.data_member));
            node.props.emplace("get", ctx.reader.get(prop.get_method_name));
            node.props.emplace("set", ctx.reader.get(prop.set_method_name));
        }

        void operator()(DeclIndex, const symbolic::SegmentDecl& segment)
        {
            node.props.emplace("name", ctx.reader.get(segment.name));
            node.props.emplace("class_id", ctx.reader.get(segment.class_id));
            node.props.emplace("seg_spec", std::to_string((int)segment.class_id));
            node.props.emplace("seg_type", std::to_string((int)segment.type));
        }

        void operator()(DeclIndex, const symbolic::SyntacticDecl& decl)
        {
            node.props.emplace("syntax", ctx.ref(decl.index));
        }
    };

    void load(Loader& ctx, Node& node, DeclIndex index)
    {
        if (null(index))
        {
            node.id = "no-decl";
            return;
        }

        node.id = to_string(index);
        Decl_loader loader{ctx, node};
        ctx.reader.visit_with_index(index, loader);
    }

}  // namespace ifc::dom
