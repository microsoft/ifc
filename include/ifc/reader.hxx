// Copyright Microsoft.

#ifndef IFC_READER_LIB_H
#define IFC_READER_LIB_H

#include "gsl/span"
#include "ifc/abstract-sgraph.hxx"
#include "ifc/file.hxx"
// #include "ifc/io.hxx"

namespace Module {
    // error reporting functions

    [[noreturn]] void not_implemented(std::string_view);
    [[noreturn]] void not_implemented(std::string_view, int);
    [[noreturn]] void unexpected(std::string_view);
    [[noreturn]] void unexpected(std::string_view, int);

    template<typename SortTag>
    [[noreturn]] void unexpected(std::string_view message, SortTag tag)
    {
        std::string result(message);
        result.append(sort_name(tag));
        unexpected(result);
    }

    template<typename SortTag>
    [[noreturn]] void not_implemented(std::string_view message, SortTag tag)
    {
        std::string result(message);
        result.append(sort_name(tag));
        not_implemented(result);
    }

    //    class Reader : public MemoryMapped::InputIfcFile
    class Reader {
        template<typename T>
        const T& view_entry_at(ByteOffset offset) const
        {
            const auto byte_offset = ifc::to_underlying(offset);
            const auto& contents   = ifc.contents();
            IFCASSERT(byte_offset < contents.size());

            const auto byte_ptr = &contents[byte_offset];
            const auto ptr      = reinterpret_cast<const T*>(byte_ptr);
            return *ptr;
        }

        TableOfContents toc{};
        void read_table_of_contents();

    public:
        const Module::InputIfc& ifc;
        explicit Reader(const Module::InputIfc& ifc);

        // get(index) -> get a reference to a data structure of the appropriate type
        //               the type is deduced from the type of the index.
        // get<T>(index) -> get a reference to a data to a particular type (one of the possible
        // sorts for this index).
        //    Example: "case ExprSort::Call: return
        //    handle_call(get<Symbolic::CallExpression>(index));
        // get_if<T>(index) -> if the sort of the index matches T::sort, returns the pointer to an
        // element, otherwise nullptr
        //    Example: "if (auto* decl = ctx.reader.get_if<Symbolic::DeclStatement>(stmt)) ... "

        // using MemoryMapped::InputIfcFile::get;

        const char* get(TextOffset offset) const
        {
            return ifc.get(offset);
        }

        template<typename T, typename Index>
        const T& get(Index index) const
        {
            IFCASSERT(T::algebra_sort == index.sort());
            return view_entry_at<T>(toc.offset(index));
        }

        const Symbolic::StringLiteral& get(StringIndex index) const
        {
            return view_entry_at<Symbolic::StringLiteral>(toc.string_literals.tell(index.index()));
        }

        const Symbolic::FileAndLine& get(LineIndex index) const
        {
            return view_entry_at<Symbolic::FileAndLine>(toc.lines.tell(index));
        }

        const Symbolic::SpecializationForm& get(SpecFormIndex index) const
        {
            return view_entry_at<Symbolic::SpecializationForm>(toc.spec_forms.tell(index));
        }

        template<typename T, typename Index>
        const T* get_if(Index index) const
        {
            return (T::algebra_sort == index.sort()) ? &view_entry_at<T>(toc.offset(index)) : nullptr;
        }

        // ScopeIndex has a dedicated value to indicate absence of a scope,
        // this will return a Scope, if present and nullptr otherwise.
        const Symbolic::Scope* try_get(ScopeIndex index) const;

        // partition<T> - returns a span of all items in a partition.

        template<typename E>
        gsl::span<const E> partition() const
        {
            const auto& summary = toc[E::algebra_sort];
            return ifc.view_partition<E>(summary);
        }

        template<AnyTrait E>
        gsl::span<const E> partition() const
        {
            const auto& summary = toc[E::partition_tag];
            return ifc.view_partition<E>(summary);
        }

        // sequence(seq) - will return a span of all elements in the sequence.
        //                 the type is deduced from a seq.

        template<typename T>
        gsl::span<const T> sequence(Sequence<T> seq)
        {
            const auto partition = this->partition<T>();
            // We prefer our IFCASSERT to subspan terminating on out of bounds.
            const auto start       = ifc::to_underlying(seq.start);
            const auto cardinality = ifc::to_underlying(seq.cardinality);
            const auto top         = start + cardinality;
            IFCASSERT(start <= top && top <= partition.size());
            return partition.subspan(start, cardinality);
        }

        template<index_like::MultiSorted E, HeapSort Tag>
        gsl::span<const E> sequence(Sequence<E, Tag> seq) const
        {
            const auto& summary  = toc[Tag];
            const auto partition = ifc.view_partition<E>(summary);
            // We prefer our IFCASSERT to subspan terminating on out of bounds.
            const auto start       = ifc::to_underlying(seq.start);
            const auto cardinality = ifc::to_underlying(seq.cardinality);
            const auto top         = start + cardinality;
            IFCASSERT(start <= top && top <= partition.size());
            return partition.subspan(start, cardinality);
        }

        // Lookup associative traits by key.
        template<AnyTrait E>
        const E* try_find(typename E::KeyType key) const
        {
            auto table = partition<E>();
            auto iter  = std::lower_bound(table.begin(), table.end(), key, TraitOrdering{});
            if (iter != table.end() && iter->entity == key)
                return &*iter;
            return nullptr;
        }

        // Build an abstract index for the item, by calculating
        // its relative position in its partition.
        template<index_like::MultiSorted Index, typename E>
            requires index_like::FiberEmbedding<E, Index>
        Index index_of(const E& item)
        {
            const auto span = partition<E>();
            IFCASSERT(!span.empty() && &item >= &span.front() && &item <= &span.back());

            auto offset = &item - &span.front();
            return {E::algebra_sort, static_cast<uint32_t>(offset)};
        }

        template<typename F>
        decltype(auto) visit(TypeIndex index, F&& f)
        {
            // clang-format off
            switch (index.sort())
            {
            case TypeSort::Fundamental:  return std::forward<F>(f)(get<Symbolic::FundamentalType>(index));
            case TypeSort::Designated:   return std::forward<F>(f)(get<Symbolic::DesignatedType>(index));

            case TypeSort::Syntactic:    return std::forward<F>(f)(get<Symbolic::SyntacticType>(index));
            case TypeSort::Expansion:    return std::forward<F>(f)(get<Symbolic::ExpansionType>(index));
            case TypeSort::Pointer:      return std::forward<F>(f)(get<Symbolic::PointerType>(index));
            case TypeSort::PointerToMember: return std::forward<F>(f)(get<Symbolic::PointerToMemberType>(index));
            case TypeSort::LvalueReference: return std::forward<F>(f)(get<Symbolic::LvalueReferenceType>(index));
            case TypeSort::RvalueReference: return std::forward<F>(f)(get<Symbolic::RvalueReferenceType>(index));
            case TypeSort::Function:     return std::forward<F>(f)(get<Symbolic::FunctionType>(index));
            case TypeSort::Method:       return std::forward<F>(f)(get<Symbolic::MethodType>(index));
            case TypeSort::Array:        return std::forward<F>(f)(get<Symbolic::ArrayType>(index));
            case TypeSort::Typename:     return std::forward<F>(f)(get<Symbolic::TypenameType>(index));
            case TypeSort::Qualified:    return std::forward<F>(f)(get<Symbolic::QualifiedType>(index));
            case TypeSort::Base:         return std::forward<F>(f)(get<Symbolic::BaseType>(index));

            case TypeSort::Decltype:     return std::forward<F>(f)(get<Symbolic::DecltypeType>(index));
            case TypeSort::Placeholder:  return std::forward<F>(f)(get<Symbolic::PlaceholderType>(index));
            case TypeSort::Tuple:        return std::forward<F>(f)(get<Symbolic::TupleType>(index));
            case TypeSort::Forall:       return std::forward<F>(f)(get<Symbolic::ForallType>(index));
            case TypeSort::Unaligned:    return std::forward<F>(f)(get<Symbolic::UnalignedType>(index));
            case TypeSort::SyntaxTree:   return std::forward<F>(f)(get<Symbolic::SyntaxTreeType>(index));
            case TypeSort::Tor:          return std::forward<F>(f)(get<Symbolic::TorType>(index));

            case TypeSort::VendorExtension:
            case TypeSort::Count:
            default:
                unexpected("visit unexpected type: ", (int)index.sort());
            }
            // clang-format on
        }

        template<typename F>
        decltype(auto) visit(StmtIndex index, F&& f)
        {
            // clang-format off
            switch (index.sort())
            {
            case StmtSort::Block:   return std::forward<F>(f)(get<Symbolic::BlockStatement>(index));
            case StmtSort::Handler: return std::forward<F>(f)(get<Symbolic::HandlerStatement>(index));
            case StmtSort::Try:     return std::forward<F>(f)(get<Symbolic::TryStatement>(index));
            case StmtSort::Return:  return std::forward<F>(f)(get<Symbolic::ReturnStatement>(index));
            case StmtSort::If:      return std::forward<F>(f)(get<Symbolic::IfStatement>(index));
            case StmtSort::Switch:  return std::forward<F>(f)(get<Symbolic::SwitchStatement>(index));
            case StmtSort::For:     return std::forward<F>(f)(get<Symbolic::ForStatement>(index));
            case StmtSort::Goto:    return std::forward<F>(f)(get<Symbolic::GotoStatement>(index));
            case StmtSort::Labeled: return std::forward<F>(f)(get<Symbolic::LabeledStatement>(index));
            case StmtSort::While:   return std::forward<F>(f)(get<Symbolic::WhileStatement>(index));
            case StmtSort::DoWhile: return std::forward<F>(f)(get<Symbolic::DoWhileStatement>(index));
            case StmtSort::Break:   return std::forward<F>(f)(get<Symbolic::BreakStatement>(index));
            case StmtSort::Continue:return std::forward<F>(f)(get<Symbolic::ContinueStatement>(index));
            case StmtSort::Expression: return std::forward<F>(f)(get<Symbolic::ExpressionStatement>(index));
            case StmtSort::Decl:    return std::forward<F>(f)(get<Symbolic::DeclStatement>(index));
            case StmtSort::Tuple:   return std::forward<F>(f)(get<Symbolic::TupleStatement>(index));
            default:
                unexpected("visit unexpected statement: ", (int)index.sort());
            }
            // clang-format on
        }

        template<typename F>
        decltype(auto) visit(NameIndex index, F&& f)
        {
            // clang-format off
            switch (index.sort())
            {
                case NameSort::Identifier: return std::forward<F>(f)(TextOffset(index.index()));
                case NameSort::Operator: return std::forward<F>(f)(get<Symbolic::OperatorFunctionId>(index));
                case NameSort::Conversion: return std::forward<F>(f)(get<Symbolic::ConversionFunctionId>(index));
                case NameSort::Literal: return std::forward<F>(f)(get<Symbolic::LiteralOperatorId>(index));
                case NameSort::Template: return std::forward<F>(f)(get<Symbolic::TemplateName>(index));
                case NameSort::Specialization: return std::forward<F>(f)(get<Symbolic::TemplateId>(index));
                case NameSort::SourceFile: return std::forward<F>(f)(get<Symbolic::SourceFileName>(index));
                case NameSort::Guide: return std::forward<F>(f)(get<Symbolic::GuideName>(index));
            default:
                unexpected("visit unexpected name: ", (int)index.sort());
            }
            // clang-format on
        }

        template<typename F>
        decltype(auto) visit_with_index(DeclIndex index, F&& f)
        {
            // clang-format off
            switch (index.sort())
            {
            case DeclSort::Enumerator:   return std::forward<F>(f)(index, get<Symbolic::EnumeratorDecl>(index));
            case DeclSort::Variable:     return std::forward<F>(f)(index, get<Symbolic::VariableDecl>(index));
            case DeclSort::Parameter:    return std::forward<F>(f)(index, get<Symbolic::ParameterDecl>(index));
            case DeclSort::Field:        return std::forward<F>(f)(index, get<Symbolic::FieldDecl>(index));
            case DeclSort::Bitfield:     return std::forward<F>(f)(index, get<Symbolic::BitfieldDecl>(index));
            case DeclSort::Scope:        return std::forward<F>(f)(index, get<Symbolic::ScopeDecl>(index));
            case DeclSort::Enumeration:  return std::forward<F>(f)(index, get<Symbolic::EnumerationDecl>(index));
            case DeclSort::Alias:        return std::forward<F>(f)(index, get<Symbolic::AliasDecl>(index));
            case DeclSort::Temploid:     return std::forward<F>(f)(index, get<Symbolic::TemploidDecl>(index));
            case DeclSort::Template:     return std::forward<F>(f)(index, get<Symbolic::TemplateDecl>(index));
            case DeclSort::PartialSpecialization:  return std::forward<F>(f)(index, get<Symbolic::PartialSpecializationDecl>(index));
            case DeclSort::Specialization: return std::forward<F>(f)(index, get<Symbolic::SpecializationDecl>(index));
            case DeclSort::Concept:      return std::forward<F>(f)(index, get<Symbolic::Concept>(index));
            case DeclSort::Function:     return std::forward<F>(f)(index, get<Symbolic::FunctionDecl>(index));
            case DeclSort::Method:       return std::forward<F>(f)(index, get<Symbolic::NonStaticMemberFunctionDecl>(index));
            case DeclSort::Constructor:  return std::forward<F>(f)(index, get<Symbolic::ConstructorDecl>(index));
            case DeclSort::InheritedConstructor: return std::forward<F>(f)(index, get<Symbolic::InheritedConstructorDecl>(index));
            case DeclSort::Destructor:   return std::forward<F>(f)(index, get<Symbolic::DestructorDecl>(index));
            case DeclSort::Reference:    return std::forward<F>(f)(index, get<Symbolic::ReferenceDecl>(index));
            case DeclSort::UsingDeclaration: return std::forward<F>(f)(index, get<Symbolic::UsingDeclaration>(index));
            case DeclSort::Friend:       return std::forward<F>(f)(index, get<Symbolic::FriendDeclaration>(index));
            case DeclSort::Expansion:    return std::forward<F>(f)(index, get<Symbolic::ExpansionDecl>(index));
            case DeclSort::DeductionGuide:   return std::forward<F>(f)(index, get<Symbolic::DeductionGuideDecl>(index));
            case DeclSort::Tuple:        return std::forward<F>(f)(index, get<Symbolic::TupleDecl>(index));
            case DeclSort::Intrinsic:    return std::forward<F>(f)(index, get<Symbolic::IntrinsicDecl>(index));
            case DeclSort::Property:     return std::forward<F>(f)(index, get<Symbolic::PropertyDeclaration>(index));
            case DeclSort::OutputSegment: return std::forward<F>(f)(index, get<Symbolic::SegmentDecl>(index));
            case DeclSort::SyntaxTree:   return std::forward<F>(f)(index, get<Symbolic::SyntacticDecl>(index));

            case DeclSort::Barren:         // IFC has no corresponding structure as of 05/12/2022
            case DeclSort::VendorExtension:
            case DeclSort::Count:
            default:
                unexpected("visit unexpected decl: ", (int)index.sort());
            }
            // clang-format on
        }

        template<typename F>
        decltype(auto) visit(ExprIndex index, F&& f)
        {
            // clang-format off
            switch (index.sort())
            {
                case ExprSort::Empty: return std::forward<F>(f)(get<Symbolic::EmptyExpression>(index));
                case ExprSort::Label: return std::forward<F>(f)(get<Symbolic::Label>(index));
                case ExprSort::Literal: return std::forward<F>(f)(get<Symbolic::LiteralExpr>(index));
                case ExprSort::Lambda: return std::forward<F>(f)(get<Symbolic::LambdaExpression>(index));
                case ExprSort::Type: return std::forward<F>(f)(get<Symbolic::TypeExpression>(index));
                case ExprSort::NamedDecl: return std::forward<F>(f)(get<Symbolic::NamedDeclExpression>(index));
                case ExprSort::UnresolvedId: return std::forward<F>(f)(get<Symbolic::UnresolvedIdExpression>(index));
                case ExprSort::TemplateId: return std::forward<F>(f)(get<Symbolic::TemplateIdExpression>(index));
                case ExprSort::UnqualifiedId: return std::forward<F>(f)(get<Symbolic::UnqualifiedIdExpr>(index));
                case ExprSort::SimpleIdentifier: return std::forward<F>(f)(get<Symbolic::SimpleIdentifier>(index));
                case ExprSort::Pointer: return std::forward<F>(f)(get<Symbolic::Pointer>(index));
                case ExprSort::QualifiedName: return std::forward<F>(f)(get<Symbolic::QualifiedNameExpr>(index));
                case ExprSort::Path: return std::forward<F>(f)(get<Symbolic::PathExpression>(index));
                case ExprSort::Read: return std::forward<F>(f)(get<Symbolic::ReadExpression>(index));
                case ExprSort::Monad: return std::forward<F>(f)(get<Symbolic::MonadicTree>(index));
                case ExprSort::Dyad: return std::forward<F>(f)(get<Symbolic::DyadicTree>(index));
                case ExprSort::Triad: return std::forward<F>(f)(get<Symbolic::TriadicTree>(index));
                case ExprSort::String: return std::forward<F>(f)(get<Symbolic::StringExpression>(index));
                case ExprSort::Temporary: return std::forward<F>(f)(get<Symbolic::TemporaryExpression>(index));
                case ExprSort::Call: return std::forward<F>(f)(get<Symbolic::CallExpression>(index));
                case ExprSort::MemberInitializer: return std::forward<F>(f)(get<Symbolic::MemberInitializer>(index));
                case ExprSort::MemberAccess: return std::forward<F>(f)(get<Symbolic::MemberAccess>(index));
                case ExprSort::InheritancePath: return std::forward<F>(f)(get<Symbolic::InheritancePath>(index));
                case ExprSort::InitializerList: return std::forward<F>(f)(get<Symbolic::InitializerList>(index));
                case ExprSort::Cast: return std::forward<F>(f)(get<Symbolic::CastExpr>(index));
                case ExprSort::Condition: return std::forward<F>(f)(get<Symbolic::ConditionExpr>(index));
                case ExprSort::ExpressionList: return std::forward<F>(f)(get<Symbolic::ExpressionList>(index));
                case ExprSort::SizeofType: return std::forward<F>(f)(get<Symbolic::SizeofTypeId>(index));
                case ExprSort::Alignof: return std::forward<F>(f)(get<Symbolic::Alignof>(index));
                case ExprSort::Typeid: return std::forward<F>(f)(get<Symbolic::TypeidExpression>(index));
                case ExprSort::DestructorCall: return std::forward<F>(f)(get<Symbolic::DestructorCall>(index));
                case ExprSort::SyntaxTree: return std::forward<F>(f)(get<Symbolic::SyntaxTreeExpr>(index));
                case ExprSort::FunctionString: return std::forward<F>(f)(get<Symbolic::FunctionStringExpression>(index));
                case ExprSort::CompoundString: return std::forward<F>(f)(get<Symbolic::CompoundStringExpression>(index));
                case ExprSort::StringSequence: return std::forward<F>(f)(get<Symbolic::StringSequenceExpression>(index));
                case ExprSort::Initializer: return std::forward<F>(f)(get<Symbolic::Initializer>(index));
                case ExprSort::Requires: return std::forward<F>(f)(get<Symbolic::RequiresExpression>(index));
                case ExprSort::UnaryFoldExpression: return std::forward<F>(f)(get<Symbolic::UnaryFoldExpression>(index));
                case ExprSort::BinaryFoldExpression: return std::forward<F>(f)(get<Symbolic::BinaryFoldExpression>(index));
                case ExprSort::HierarchyConversion: return std::forward<F>(f)(get<Symbolic::HierarchyConversionExpression>(index));
                case ExprSort::ProductTypeValue: return std::forward<F>(f)(get<Symbolic::ProductTypeValue>(index));
                case ExprSort::SumTypeValue: return std::forward<F>(f)(get<Symbolic::SumTypeValue>(index));
                case ExprSort::SubobjectValue: return std::forward<F>(f)(get<Symbolic::SubobjectValue>(index));
                case ExprSort::ArrayValue: return std::forward<F>(f)(get<Symbolic::ArrayValue>(index));
                case ExprSort::DynamicDispatch: return std::forward<F>(f)(get<Symbolic::DynamicDispatch>(index));
                case ExprSort::VirtualFunctionConversion: return std::forward<F>(f)(get<Symbolic::VirtualFunctionConversion>(index));
                case ExprSort::Placeholder: return std::forward<F>(f)(get<Symbolic::PlaceholderExpr>(index));
                case ExprSort::Expansion: return std::forward<F>(f)(get<Symbolic::ExpansionExpr>(index));
                case ExprSort::Tuple: return std::forward<F>(f)(get<Symbolic::TupleExpression>(index));
                case ExprSort::Nullptr: return std::forward<F>(f)(get<Symbolic::Nullptr>(index));
                case ExprSort::This: return std::forward<F>(f)(get<Symbolic::This>(index));
                case ExprSort::TemplateReference: return std::forward<F>(f)(get<Symbolic::TemplateReference>(index));
                case ExprSort::TypeTraitIntrinsic: return std::forward<F>(f)(get<Symbolic::TypeTraitIntrinsic>(index));
                case ExprSort::DesignatedInitializer: return std::forward<F>(f)(get<Symbolic::DesignatedInitializer>(index));
                case ExprSort::PackedTemplateArguments: return std::forward<F>(f)(get<Symbolic::PackedTemplateArguments>(index));
                case ExprSort::Tokens: return std::forward<F>(f)(get<Symbolic::TokenExpression>(index));
                case ExprSort::AssignInitializer: return std::forward<F>(f)(get<Symbolic::AssignInitializer>(index));

                case ExprSort::VendorExtension:
                case ExprSort::Generic: // C11 generic extension (no IFC structure yet)
                default:
                    unexpected("visit unexpected expr: ", (int)index.sort());
            }
            // clang-format off
        }
    };

    // Outside of class due to GCC bug.
    template <>
    inline const int64_t& Reader::get<int64_t, LitIndex>(LitIndex index) const
    {
        IFCASSERT(LiteralSort::Integer == index.sort());
        return view_entry_at<int64_t>(toc.u64s.tell(index.index()));
    }

    template <>
    inline const double& Reader::get<double, LitIndex>(LitIndex index) const
    {
        IFCASSERT(LiteralSort::FloatingPoint == index.sort());
        return view_entry_at<double>(toc.fps.tell(index.index()));
    }

    template <>
    inline gsl::span<const Symbolic::Scope> Reader::partition<Symbolic::Scope>() const
    {
        const auto& summary = toc.scopes;
        return ifc.view_partition<Symbolic::Scope>(summary);
    }

    template <>
    inline gsl::span<const Symbolic::Declaration> Reader::partition<Symbolic::Declaration>() const
    {
        const auto& summary = toc.entities;
        return ifc.view_partition<Symbolic::Declaration>(summary);
    }

    template <>
    inline gsl::span<const Symbolic::ParameterDecl> Reader::partition<Symbolic::ParameterDecl>() const
    {
        const auto& summary = toc[Symbolic::ParameterDecl::algebra_sort];
        return ifc.view_partition<Symbolic::ParameterDecl>(summary);
    }

    inline const Symbolic::Scope* Reader::try_get(ScopeIndex index) const
    {
        if (index_like::null(index))
            return nullptr;

        return &partition<Symbolic::Scope>()[ifc::to_underlying(index) - 1];
    }
}  // namespace Module

#endif // IFC_READER_LIB_H
