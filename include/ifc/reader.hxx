// Copyright Microsoft.

#ifndef IFC_READER_LIB_H
#define IFC_READER_LIB_H

#include "gsl/span"
#include "ifc/abstract-sgraph.hxx"
#include "ifc/file.hxx"
//#include "ifc/io.hxx"

namespace ifc
{
    // error reporting functions

    [[noreturn]] void not_implemented(std::string_view);
    [[noreturn]] void not_implemented(std::string_view, int);
    [[noreturn]] void unexpected(std::string_view);
    [[noreturn]] void unexpected(std::string_view, int);

    template <typename SortTag>
    [[noreturn]] void unexpected(std::string_view message, SortTag tag)
    {
        std::string result(message);
        result.append(sort_name(tag));
        unexpected(result);
    }

    template <typename SortTag>
    [[noreturn]] void not_implemented(std::string_view message, SortTag tag)
    {
        std::string result(message);
        result.append(sort_name(tag));
        not_implemented(result);
    }

//    class Reader : public MemoryMapped::InputIfcFile
    class Reader
    {
        template <typename T>
        const T& view_entry_at(ByteOffset offset) const
        {
            const auto byte_offset = ifc::to_underlying(offset);
            const auto& contents = ifc.contents();
            IFCASSERT(byte_offset < contents.size());

            const auto byte_ptr = &contents[byte_offset];
            const auto ptr = reinterpret_cast<const T*>(byte_ptr);
            return *ptr;
        }

        TableOfContents toc{};
        void read_table_of_contents();

    public:
        const ifc::InputIfc& ifc;
        explicit Reader(const ifc::InputIfc& ifc);

        // get(index) -> get a reference to a data structure of the appropriate type
        //               the type is deduced from the type of the index.
        // get<T>(index) -> get a reference to a data to a particular type (one of the possible
        // sorts for this index).
        //    Example: "case ExprSort::Call: return
        //    handle_call(get<symbolic::CallExpression>(index));
        // get_if<T>(index) -> if the sort of the index matches T::sort, returns the pointer to an
        // element, otherwise nullptr
        //    Example: "if (auto* decl = ctx.reader.get_if<symbolic::DeclStatement>(stmt)) ... "

        //using MemoryMapped::InputIfcFile::get;

        const char* get(TextOffset offset) const
        {
            return ifc.get(offset);
        }

        template <typename T, typename Index>
        const T& get(Index index) const
        {
            IFCASSERT(T::algebra_sort == index.sort());
            return view_entry_at<T>(toc.offset(index));
        }

        const symbolic::StringLiteral& get(StringIndex index) const
        {
            return view_entry_at<symbolic::StringLiteral>(toc.string_literals.tell(index.index()));
        }

        const symbolic::FileAndLine& get(LineIndex index) const
        {
            return view_entry_at<symbolic::FileAndLine>(toc.lines.tell(index));
        }

        const symbolic::SpecializationForm& get(SpecFormIndex index) const
        {
            return view_entry_at<symbolic::SpecializationForm>(toc.spec_forms.tell(index));
        }

        template <typename T, typename Index>
        const T* get_if(Index index) const
        {
            return (T::algebra_sort == index.sort()) ? &view_entry_at<T>(toc.offset(index)) :
                                                       nullptr;
        }

        // ScopeIndex has a dedicated value to indicate absence of a scope,
        // this will return a Scope, if present and nullptr otherwise.
        const symbolic::Scope* try_get(ScopeIndex index) const;

        // partition<T> - returns a span of all items in a partition.

        template <typename E>
        gsl::span<const E> partition() const
        {
            const auto& summary = toc[E::algebra_sort];
            return ifc.view_partition<E>(summary);
        }

        template <AnyTrait E>
        gsl::span<const E> partition() const
        {
            const auto& summary = toc[E::partition_tag];
            return ifc.view_partition<E>(summary);
        }

        // sequence(seq) - will return a span of all elements in the sequence.
        //                 the type is deduced from a seq.

        template <typename T>
        gsl::span<const T> sequence(Sequence<T> seq)
        {
            const auto partition = this->partition<T>();
            // We prefer our IFCASSERT to subspan terminating on out of bounds.
            const auto start = ifc::to_underlying(seq.start);
            const auto cardinality = ifc::to_underlying(seq.cardinality);
            const auto top = start + cardinality;
            IFCASSERT(start <= top and top <= partition.size());
            return partition.subspan(start, cardinality);
        }

        template <index_like::MultiSorted E, HeapSort Tag>
        gsl::span<const E> sequence(Sequence<E, Tag> seq) const
        {
            const auto& summary = toc[Tag];
            const auto partition = ifc.view_partition<E>(summary);
            // We prefer our IFCASSERT to subspan terminating on out of bounds.
            const auto start = ifc::to_underlying(seq.start);
            const auto cardinality = ifc::to_underlying(seq.cardinality);
            const auto top = start + cardinality;
            IFCASSERT(start <= top and top <= partition.size());
            return partition.subspan(start, cardinality);
        }

        // Lookup associative traits by key.
        template <AnyTrait E>
        const E* try_find(typename E::KeyType key) const
        {
            auto table = partition<E>();
            auto iter = std::lower_bound(table.begin(), table.end(), key, TraitOrdering{});
            if (iter != table.end() and iter->entity == key)
                return &*iter;
            return nullptr;
        }

        // Build an abstract index for the item, by calculating
        // its relative position in its partition.
        template <index_like::MultiSorted Index, typename E>
        requires index_like::FiberEmbedding<E, Index> Index index_of(const E& item)
        {
            const auto span = partition<E>();
            IFCASSERT(!span.empty() and &item >= &span.front() and &item <= &span.back());

            auto offset = &item - &span.front();
            return {E::algebra_sort, static_cast<uint32_t>(offset)};
        }

        template <typename F>
        decltype(auto) visit(TypeIndex index, F&& f)
        {
            // clang-format off
            switch (index.sort())
            {
            case TypeSort::Fundamental:  return std::forward<F>(f)(get<symbolic::FundamentalType>(index));
            case TypeSort::Designated:   return std::forward<F>(f)(get<symbolic::DesignatedType>(index));

            case TypeSort::Syntactic:    return std::forward<F>(f)(get<symbolic::SyntacticType>(index));
            case TypeSort::Expansion:    return std::forward<F>(f)(get<symbolic::ExpansionType>(index));
            case TypeSort::Pointer:      return std::forward<F>(f)(get<symbolic::PointerType>(index));
            case TypeSort::PointerToMember: return std::forward<F>(f)(get<symbolic::PointerToMemberType>(index));
            case TypeSort::LvalueReference: return std::forward<F>(f)(get<symbolic::LvalueReferenceType>(index));
            case TypeSort::RvalueReference: return std::forward<F>(f)(get<symbolic::RvalueReferenceType>(index));
            case TypeSort::Function:     return std::forward<F>(f)(get<symbolic::FunctionType>(index));
            case TypeSort::Method:       return std::forward<F>(f)(get<symbolic::MethodType>(index));
            case TypeSort::Array:        return std::forward<F>(f)(get<symbolic::ArrayType>(index));
            case TypeSort::Typename:     return std::forward<F>(f)(get<symbolic::TypenameType>(index));
            case TypeSort::Qualified:    return std::forward<F>(f)(get<symbolic::QualifiedType>(index));
            case TypeSort::Base:         return std::forward<F>(f)(get<symbolic::BaseType>(index));

            case TypeSort::Decltype:     return std::forward<F>(f)(get<symbolic::DecltypeType>(index));
            case TypeSort::Placeholder:  return std::forward<F>(f)(get<symbolic::PlaceholderType>(index));
            case TypeSort::Tuple:        return std::forward<F>(f)(get<symbolic::TupleType>(index));
            case TypeSort::Forall:       return std::forward<F>(f)(get<symbolic::ForallType>(index));
            case TypeSort::Unaligned:    return std::forward<F>(f)(get<symbolic::UnalignedType>(index));
            case TypeSort::SyntaxTree:   return std::forward<F>(f)(get<symbolic::SyntaxTreeType>(index));
            case TypeSort::Tor:          return std::forward<F>(f)(get<symbolic::TorType>(index));

            case TypeSort::VendorExtension:
            case TypeSort::Count:
            default:
                unexpected("visit unexpected type: ", (int)index.sort());
            }
            // clang-format on
        }

        template <typename F>
        decltype(auto) visit(StmtIndex index, F&& f)
        {
            // clang-format off
            switch (index.sort())
            {
            case StmtSort::Block:   return std::forward<F>(f)(get<symbolic::BlockStatement>(index));
            case StmtSort::Handler: return std::forward<F>(f)(get<symbolic::HandlerStatement>(index));
            case StmtSort::Try:     return std::forward<F>(f)(get<symbolic::TryStatement>(index));
            case StmtSort::Return:  return std::forward<F>(f)(get<symbolic::ReturnStatement>(index));
            case StmtSort::If:      return std::forward<F>(f)(get<symbolic::IfStatement>(index));
            case StmtSort::Switch:  return std::forward<F>(f)(get<symbolic::SwitchStatement>(index));
            case StmtSort::For:     return std::forward<F>(f)(get<symbolic::ForStatement>(index));
            case StmtSort::Goto:    return std::forward<F>(f)(get<symbolic::GotoStatement>(index));
            case StmtSort::Labeled: return std::forward<F>(f)(get<symbolic::LabeledStatement>(index));
            case StmtSort::While:   return std::forward<F>(f)(get<symbolic::WhileStatement>(index));
            case StmtSort::DoWhile: return std::forward<F>(f)(get<symbolic::DoWhileStatement>(index));
            case StmtSort::Break:   return std::forward<F>(f)(get<symbolic::BreakStatement>(index));
            case StmtSort::Continue:return std::forward<F>(f)(get<symbolic::ContinueStatement>(index));
            case StmtSort::Expression: return std::forward<F>(f)(get<symbolic::ExpressionStatement>(index));
            case StmtSort::Decl:    return std::forward<F>(f)(get<symbolic::DeclStatement>(index));
            case StmtSort::Tuple:   return std::forward<F>(f)(get<symbolic::TupleStatement>(index));
            default:
                unexpected("visit unexpected statement: ", (int)index.sort());
            }
            // clang-format on
        }

        template <typename F>
        decltype(auto) visit(NameIndex index, F&& f)
        {
            // clang-format off
            switch (index.sort())
            {
                case NameSort::Identifier: return std::forward<F>(f)(TextOffset(index.index()));
                case NameSort::Operator: return std::forward<F>(f)(get<symbolic::OperatorFunctionId>(index));
                case NameSort::Conversion: return std::forward<F>(f)(get<symbolic::ConversionFunctionId>(index));
                case NameSort::Literal: return std::forward<F>(f)(get<symbolic::LiteralOperatorId>(index));
                case NameSort::Template: return std::forward<F>(f)(get<symbolic::TemplateName>(index));
                case NameSort::Specialization: return std::forward<F>(f)(get<symbolic::TemplateId>(index));
                case NameSort::SourceFile: return std::forward<F>(f)(get<symbolic::SourceFileName>(index));
                case NameSort::Guide: return std::forward<F>(f)(get<symbolic::GuideName>(index));
            default:
                unexpected("visit unexpected name: ", (int)index.sort());
            }
            // clang-format on
        }

        template <typename F>
        decltype(auto) visit_with_index(DeclIndex index, F&& f)
        {
            // clang-format off
            switch (index.sort())
            {
            case DeclSort::Enumerator:   return std::forward<F>(f)(index, get<symbolic::EnumeratorDecl>(index));
            case DeclSort::Variable:     return std::forward<F>(f)(index, get<symbolic::VariableDecl>(index));
            case DeclSort::Parameter:    return std::forward<F>(f)(index, get<symbolic::ParameterDecl>(index));
            case DeclSort::Field:        return std::forward<F>(f)(index, get<symbolic::FieldDecl>(index));
            case DeclSort::Bitfield:     return std::forward<F>(f)(index, get<symbolic::BitfieldDecl>(index));
            case DeclSort::Scope:        return std::forward<F>(f)(index, get<symbolic::ScopeDecl>(index));
            case DeclSort::Enumeration:  return std::forward<F>(f)(index, get<symbolic::EnumerationDecl>(index));
            case DeclSort::Alias:        return std::forward<F>(f)(index, get<symbolic::AliasDecl>(index));
            case DeclSort::Temploid:     return std::forward<F>(f)(index, get<symbolic::TemploidDecl>(index));
            case DeclSort::Template:     return std::forward<F>(f)(index, get<symbolic::TemplateDecl>(index));
            case DeclSort::PartialSpecialization:  return std::forward<F>(f)(index, get<symbolic::PartialSpecializationDecl>(index));
            case DeclSort::Specialization: return std::forward<F>(f)(index, get<symbolic::SpecializationDecl>(index));
            case DeclSort::Concept:      return std::forward<F>(f)(index, get<symbolic::Concept>(index));
            case DeclSort::Function:     return std::forward<F>(f)(index, get<symbolic::FunctionDecl>(index));
            case DeclSort::Method:       return std::forward<F>(f)(index, get<symbolic::NonStaticMemberFunctionDecl>(index));
            case DeclSort::Constructor:  return std::forward<F>(f)(index, get<symbolic::ConstructorDecl>(index));
            case DeclSort::InheritedConstructor: return std::forward<F>(f)(index, get<symbolic::InheritedConstructorDecl>(index));
            case DeclSort::Destructor:   return std::forward<F>(f)(index, get<symbolic::DestructorDecl>(index));
            case DeclSort::Reference:    return std::forward<F>(f)(index, get<symbolic::ReferenceDecl>(index));
            case DeclSort::UsingDeclaration: return std::forward<F>(f)(index, get<symbolic::UsingDeclaration>(index));
            case DeclSort::Friend:       return std::forward<F>(f)(index, get<symbolic::FriendDeclaration>(index));
            case DeclSort::Expansion:    return std::forward<F>(f)(index, get<symbolic::ExpansionDecl>(index));
            case DeclSort::DeductionGuide:   return std::forward<F>(f)(index, get<symbolic::DeductionGuideDecl>(index));
            case DeclSort::Tuple:        return std::forward<F>(f)(index, get<symbolic::TupleDecl>(index));
            case DeclSort::Intrinsic:    return std::forward<F>(f)(index, get<symbolic::IntrinsicDecl>(index));
            case DeclSort::Property:     return std::forward<F>(f)(index, get<symbolic::PropertyDeclaration>(index));
            case DeclSort::OutputSegment: return std::forward<F>(f)(index, get<symbolic::SegmentDecl>(index));
            case DeclSort::SyntaxTree:   return std::forward<F>(f)(index, get<symbolic::SyntacticDecl>(index));

            case DeclSort::Barren:         // IFC has no corresponding structure as of 05/12/2022
            case DeclSort::VendorExtension:
            case DeclSort::Count:
            default:
                unexpected("visit unexpected decl: ", (int)index.sort());
            }
            // clang-format on
        }

        template <typename F>
        decltype(auto) visit(ExprIndex index, F&& f)
        {
            // clang-format off
            switch (index.sort())
            {
                case ExprSort::Empty: return std::forward<F>(f)(get<symbolic::EmptyExpression>(index));
                case ExprSort::Label: return std::forward<F>(f)(get<symbolic::Label>(index));
                case ExprSort::Literal: return std::forward<F>(f)(get<symbolic::LiteralExpr>(index));
                case ExprSort::Lambda: return std::forward<F>(f)(get<symbolic::LambdaExpression>(index));
                case ExprSort::Type: return std::forward<F>(f)(get<symbolic::TypeExpression>(index));
                case ExprSort::NamedDecl: return std::forward<F>(f)(get<symbolic::NamedDeclExpression>(index));
                case ExprSort::UnresolvedId: return std::forward<F>(f)(get<symbolic::UnresolvedIdExpression>(index));
                case ExprSort::TemplateId: return std::forward<F>(f)(get<symbolic::TemplateIdExpression>(index));
                case ExprSort::UnqualifiedId: return std::forward<F>(f)(get<symbolic::UnqualifiedIdExpr>(index));
                case ExprSort::SimpleIdentifier: return std::forward<F>(f)(get<symbolic::SimpleIdentifier>(index));
                case ExprSort::Pointer: return std::forward<F>(f)(get<symbolic::Pointer>(index));
                case ExprSort::QualifiedName: return std::forward<F>(f)(get<symbolic::QualifiedNameExpr>(index));
                case ExprSort::Path: return std::forward<F>(f)(get<symbolic::PathExpression>(index));
                case ExprSort::Read: return std::forward<F>(f)(get<symbolic::ReadExpression>(index));
                case ExprSort::Monad: return std::forward<F>(f)(get<symbolic::MonadicTree>(index));
                case ExprSort::Dyad: return std::forward<F>(f)(get<symbolic::DyadicTree>(index));
                case ExprSort::Triad: return std::forward<F>(f)(get<symbolic::TriadicTree>(index));
                case ExprSort::String: return std::forward<F>(f)(get<symbolic::StringExpression>(index));
                case ExprSort::Temporary: return std::forward<F>(f)(get<symbolic::TemporaryExpression>(index));
                case ExprSort::Call: return std::forward<F>(f)(get<symbolic::CallExpression>(index));
                case ExprSort::MemberInitializer: return std::forward<F>(f)(get<symbolic::MemberInitializer>(index));
                case ExprSort::MemberAccess: return std::forward<F>(f)(get<symbolic::MemberAccess>(index));
                case ExprSort::InheritancePath: return std::forward<F>(f)(get<symbolic::InheritancePath>(index));
                case ExprSort::InitializerList: return std::forward<F>(f)(get<symbolic::InitializerList>(index));
                case ExprSort::Cast: return std::forward<F>(f)(get<symbolic::CastExpr>(index));
                case ExprSort::Condition: return std::forward<F>(f)(get<symbolic::ConditionExpr>(index));
                case ExprSort::ExpressionList: return std::forward<F>(f)(get<symbolic::ExpressionList>(index));
                case ExprSort::SizeofType: return std::forward<F>(f)(get<symbolic::SizeofTypeId>(index));
                case ExprSort::Alignof: return std::forward<F>(f)(get<symbolic::Alignof>(index));
                case ExprSort::Typeid: return std::forward<F>(f)(get<symbolic::TypeidExpression>(index));
                case ExprSort::DestructorCall: return std::forward<F>(f)(get<symbolic::DestructorCall>(index));
                case ExprSort::SyntaxTree: return std::forward<F>(f)(get<symbolic::SyntaxTreeExpr>(index));
                case ExprSort::FunctionString: return std::forward<F>(f)(get<symbolic::FunctionStringExpression>(index));
                case ExprSort::CompoundString: return std::forward<F>(f)(get<symbolic::CompoundStringExpression>(index));
                case ExprSort::StringSequence: return std::forward<F>(f)(get<symbolic::StringSequenceExpression>(index));
                case ExprSort::Initializer: return std::forward<F>(f)(get<symbolic::Initializer>(index));
                case ExprSort::Requires: return std::forward<F>(f)(get<symbolic::RequiresExpression>(index));
                case ExprSort::UnaryFoldExpression: return std::forward<F>(f)(get<symbolic::UnaryFoldExpression>(index));
                case ExprSort::BinaryFoldExpression: return std::forward<F>(f)(get<symbolic::BinaryFoldExpression>(index));
                case ExprSort::HierarchyConversion: return std::forward<F>(f)(get<symbolic::HierarchyConversionExpression>(index));
                case ExprSort::ProductTypeValue: return std::forward<F>(f)(get<symbolic::ProductTypeValue>(index));
                case ExprSort::SumTypeValue: return std::forward<F>(f)(get<symbolic::SumTypeValue>(index));
                case ExprSort::SubobjectValue: return std::forward<F>(f)(get<symbolic::SubobjectValue>(index));
                case ExprSort::ArrayValue: return std::forward<F>(f)(get<symbolic::ArrayValue>(index));
                case ExprSort::DynamicDispatch: return std::forward<F>(f)(get<symbolic::DynamicDispatch>(index));
                case ExprSort::VirtualFunctionConversion: return std::forward<F>(f)(get<symbolic::VirtualFunctionConversion>(index));
                case ExprSort::Placeholder: return std::forward<F>(f)(get<symbolic::PlaceholderExpr>(index));
                case ExprSort::Expansion: return std::forward<F>(f)(get<symbolic::ExpansionExpr>(index));
                case ExprSort::Tuple: return std::forward<F>(f)(get<symbolic::TupleExpression>(index));
                case ExprSort::Nullptr: return std::forward<F>(f)(get<symbolic::Nullptr>(index));
                case ExprSort::This: return std::forward<F>(f)(get<symbolic::This>(index));
                case ExprSort::TemplateReference: return std::forward<F>(f)(get<symbolic::TemplateReference>(index));
                case ExprSort::TypeTraitIntrinsic: return std::forward<F>(f)(get<symbolic::TypeTraitIntrinsic>(index));
                case ExprSort::DesignatedInitializer: return std::forward<F>(f)(get<symbolic::DesignatedInitializer>(index));
                case ExprSort::PackedTemplateArguments: return std::forward<F>(f)(get<symbolic::PackedTemplateArguments>(index));
                case ExprSort::Tokens: return std::forward<F>(f)(get<symbolic::TokenExpression>(index));
                case ExprSort::AssignInitializer: return std::forward<F>(f)(get<symbolic::AssignInitializer>(index));

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
    inline gsl::span<const symbolic::Scope> Reader::partition<symbolic::Scope>() const
    {
        const auto& summary = toc.scopes;
        return ifc.view_partition<symbolic::Scope>(summary);
    }

    template <>
    inline gsl::span<const symbolic::Declaration> Reader::partition<symbolic::Declaration>() const
    {
        const auto& summary = toc.entities;
        return ifc.view_partition<symbolic::Declaration>(summary);
    }

    template <>
    inline gsl::span<const symbolic::ParameterDecl> Reader::partition<symbolic::ParameterDecl>() const
    {
        const auto& summary = toc[symbolic::ParameterDecl::algebra_sort];
        return ifc.view_partition<symbolic::ParameterDecl>(summary);
    }

    inline const symbolic::Scope* Reader::try_get(ScopeIndex index) const
    {
        if (index_like::null(index))
            return nullptr;

        return &partition<symbolic::Scope>()[ifc::to_underlying(index) - 1];
    }
}  // namespace ifc

#endif // IFC_READER_LIB_H
