// Copyright Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "ifc/util.hxx"
#include <map>

namespace ifc::util {
    namespace {
        template<typename T>
        struct OperatorNameMapEntry {
            constexpr OperatorNameMapEntry(T t, const char* n) : name{n}, op{t} {}
            const char* name;
            T op;
        };

        OperatorNameMapEntry<NiladicOperator> niladics[] = {
            {NiladicOperator::Unknown, "Unknown"},
            {NiladicOperator::Phantom, "Phantom"},
            {NiladicOperator::Constant, "Constant"},
            {NiladicOperator::Nil, "Nil"},
            {NiladicOperator::MsvcConstantObject, "MsvcConstantObject"},
            {NiladicOperator::MsvcLambda, "MsvcLambda"},
        };

        OperatorNameMapEntry<DyadicOperator> dyadics[] = {
            {DyadicOperator::Plus, "Plus"},
            {DyadicOperator::Minus, "Minus"},
            {DyadicOperator::Mult, "Mult"},
            {DyadicOperator::Slash, "Slash"},
            {DyadicOperator::Modulo, "Modulo"},
            {DyadicOperator::Remainder, "Remainder"},
            {DyadicOperator::Bitand, "Bitand"},
            {DyadicOperator::Bitor, "Bitor"},
            {DyadicOperator::Bitxor, "Bitxor"},
            {DyadicOperator::Lshift, "Lshift"},
            {DyadicOperator::Rshift, "Rshift"},
            {DyadicOperator::Equal, "Equal"},
            {DyadicOperator::NotEqual, "NotEqual"},
            {DyadicOperator::Less, "Less"},
            {DyadicOperator::LessEqual, "LessEqual"},
            {DyadicOperator::Greater, "Greater"},
            {DyadicOperator::GreaterEqual, "GreaterEqual"},
            {DyadicOperator::Compare, "Compare"},
            {DyadicOperator::LogicAnd, "LogicAnd"},
            {DyadicOperator::LogicOr, "LogicOr"},
            {DyadicOperator::Assign, "Assign"},
            {DyadicOperator::PlusAssign, "PlusAssign"},
            {DyadicOperator::MinusAssign, "MinusAssign"},
            {DyadicOperator::MultAssign, "MultAssign"},
            {DyadicOperator::SlashAssign, "SlashAssign"},
            {DyadicOperator::ModuloAssign, "ModuloAssign"},
            {DyadicOperator::BitandAssign, "BitandAssign"},
            {DyadicOperator::BitorAssign, "BitorAssign"},
            {DyadicOperator::BitxorAssign, "BitxorAssign"},
            {DyadicOperator::LshiftAssign, "LshiftAssign"},
            {DyadicOperator::RshiftAssign, "RshiftAssign"},
            {DyadicOperator::Comma, "Comma"},
            {DyadicOperator::Dot, "Dot"},
            {DyadicOperator::Arrow, "Arrow"},
            {DyadicOperator::DotStar, "DotStar"},
            {DyadicOperator::ArrowStar, "ArrowStar"},
            {DyadicOperator::Curry, "Curry"},
            {DyadicOperator::Apply, "Apply"},
            {DyadicOperator::Index, "Index"},
            {DyadicOperator::DefaultAt, "DefaultAt"},
            {DyadicOperator::New, "New"},
            {DyadicOperator::NewArray, "NewArray"},
            {DyadicOperator::Destruct, "Destruct"},
            {DyadicOperator::DestructAt, "DestructAt"},
            {DyadicOperator::Cleanup, "Cleanup"},
            {DyadicOperator::Qualification, "Qualification"},
            {DyadicOperator::Promote, "Promote"},
            {DyadicOperator::Demote, "Demote"},
            {DyadicOperator::Coerce, "Coerce"},
            {DyadicOperator::Rewrite, "Rewrite"},
            {DyadicOperator::Bless, "Bless"},
            {DyadicOperator::Cast, "Cast"},
            {DyadicOperator::ExplicitConversion, "ExplicitConversion"},
            {DyadicOperator::ReinterpretCast, "ReinterpretCast"},
            {DyadicOperator::StaticCast, "StaticCast"},
            {DyadicOperator::ConstCast, "ConstCast"},
            {DyadicOperator::DynamicCast, "DynamicCast"},
            {DyadicOperator::Narrow, "Narrow"},
            {DyadicOperator::Widen, "Widen"},
            {DyadicOperator::Pretend, "Pretend"},
            {DyadicOperator::Closure, "Closure"},
            {DyadicOperator::ZeroInitialize, "ZeroInitialize"},
            {DyadicOperator::ClearStorage, "ClearStorage"},

            {DyadicOperator::MsvcTryCast, "MsvcTryCast"},
            {DyadicOperator::MsvcCurry, "MsvcCurry"},
            {DyadicOperator::MsvcVirtualCurry, "MsvcVirtualCurry"},
            {DyadicOperator::MsvcAlign, "MsvcAlign"},
            {DyadicOperator::MsvcBitSpan, "MsvcBitSpan"},
            {DyadicOperator::MsvcBitfieldAccess, "MsvcBitfieldAccess"},
            {DyadicOperator::MsvcObscureBitfieldAccess, "MsvcObscureBitfieldAccess"},
            {DyadicOperator::MsvcInitialize, "MsvcInitialize"},
            {DyadicOperator::MsvcBuiltinOffsetOf, "MsvcBuiltinOffsetOf"},
            {DyadicOperator::MsvcIsBaseOf, "MsvcIsBaseOf"},
            {DyadicOperator::MsvcIsConvertibleTo, "MsvcIsConvertibleTo"},
            {DyadicOperator::MsvcIsTriviallyAssignable, "MsvcIsTriviallyAssignable"},
            {DyadicOperator::MsvcIsNothrowAssignable, "MsvcIsNothrowAssignable"},
            {DyadicOperator::MsvcIsAssignable, "MsvcIsAssignable"},
            {DyadicOperator::MsvcIsAssignableNocheck, "MsvcIsAssignableNocheck"},
            {DyadicOperator::MsvcBuiltinBitCast, "MsvcBuiltinBitCast"},
            {DyadicOperator::MsvcBuiltinIsLayoutCompatible, "MsvcBuiltinIsLayoutCompatible"},
            {DyadicOperator::MsvcBuiltinIsCorrespondingMember, "MsvcBuiltinIsCorrespondingMember"},
            {DyadicOperator::MsvcIntrinsic, "MsvcIntrinsic"},
        };

        OperatorNameMapEntry<MonadicOperator> monadics[] = {
            {MonadicOperator::Plus, "Plus"},
            {MonadicOperator::Negate, "Negate"},
            {MonadicOperator::Deref, "Deref"},
            {MonadicOperator::Address, "Address"},
            {MonadicOperator::Complement, "Complement"},
            {MonadicOperator::Not, "Not"},
            {MonadicOperator::PreIncrement, "PreIncrement"},
            {MonadicOperator::PreDecrement, "PreDecrement"},
            {MonadicOperator::PostIncrement, "PostIncrement"},
            {MonadicOperator::PostDecrement, "PostDecrement"},
            {MonadicOperator::Truncate, "Truncate"},
            {MonadicOperator::Ceil, "Ceil"},
            {MonadicOperator::Floor, "Floor"},
            {MonadicOperator::Paren, "Paren"},
            {MonadicOperator::Brace, "Brace"},
            {MonadicOperator::Alignas, "Alignas"},
            {MonadicOperator::Alignof, "Alignof"},
            {MonadicOperator::Sizeof, "Sizeof"},
            {MonadicOperator::Cardinality, "Cardinality"},
            {MonadicOperator::Typeid, "Typeid"},
            {MonadicOperator::Noexcept, "Noexcept"},
            {MonadicOperator::Requires, "Requires"},
            {MonadicOperator::CoReturn, "CoReturn"},
            {MonadicOperator::Await, "Await"},
            {MonadicOperator::Yield, "Yield"},
            {MonadicOperator::Throw, "Throw"},
            {MonadicOperator::New, "New"},
            {MonadicOperator::Delete, "Delete"},
            {MonadicOperator::DeleteArray, "DeleteArray"},
            {MonadicOperator::Expand, "Expand"},
            {MonadicOperator::Read, "Read"},
            {MonadicOperator::Materialize, "Materialize"},
            {MonadicOperator::PseudoDtorCall, "PseudoDtorCall"},

            {MonadicOperator::LookupGlobally, "LookupGlobally"},
            {MonadicOperator::MsvcAssume, "MsvcAssume"},
            {MonadicOperator::MsvcAlignof, "MsvcAlignof"},
            {MonadicOperator::MsvcUuidof, "MsvcUuidof"},
            {MonadicOperator::MsvcIsClass, "MsvcIsClass"},
            {MonadicOperator::MsvcIsUnion, "MsvcIsUnion"},
            {MonadicOperator::MsvcIsEnum, "MsvcIsEnum"},
            {MonadicOperator::MsvcIsPolymorphic, "MsvcIsPolymorphic"},
            {MonadicOperator::MsvcIsEmpty, "MsvcIsEmpty"},
            {MonadicOperator::MsvcIsTriviallyCopyConstructible, "MsvcIsTriviallyCopyConstructible"},
            {MonadicOperator::MsvcIsTriviallyCopyAssignable, "MsvcIsTriviallyCopyAssignable"},
            {MonadicOperator::MsvcIsTriviallyDestructible, "MsvcIsTriviallyDestructible"},
            {MonadicOperator::MsvcHasVirtualDestructor, "MsvcHasVirtualDestructor"},
            {MonadicOperator::MsvcIsNothrowCopyConstructible, "MsvcIsNothrowCopyConstructible"},
            {MonadicOperator::MsvcIsNothrowCopyAssignable, "MsvcIsNothrowCopyAssignable"},
            {MonadicOperator::MsvcIsPod, "MsvcIsPod"},
            {MonadicOperator::MsvcIsAbstract, "MsvcIsAbstract"},
            {MonadicOperator::MsvcIsTrivial, "MsvcIsTrivial"},
            {MonadicOperator::MsvcIsTriviallyCopyable, "MsvcIsTriviallyCopyable"},
            {MonadicOperator::MsvcIsStandardLayout, "MsvcIsStandardLayout"},
            {MonadicOperator::MsvcIsLiteralType, "MsvcIsLiteralType"},
            {MonadicOperator::MsvcIsTriviallyMoveConstructible, "MsvcIsTriviallyMoveConstructible"},
            {MonadicOperator::MsvcHasTrivialMoveAssign, "MsvcHasTrivialMoveAssign"},
            {MonadicOperator::MsvcIsTriviallyMoveAssignable, "MsvcIsTriviallyMoveAssignable"},
            {MonadicOperator::MsvcIsNothrowMoveAssignable, "MsvcIsNothrowMoveAssignable"},
            {MonadicOperator::MsvcUnderlyingType, "MsvcUnderlyingType"},
            {MonadicOperator::MsvcIsDestructible, "MsvcIsDestructible"},
            {MonadicOperator::MsvcIsNothrowDestructible, "MsvcIsNothrowDestructible"},
            {MonadicOperator::MsvcHasUniqueObjectRepresentations, "MsvcHasUniqueObjectRepresentations"},
            {MonadicOperator::MsvcIsAggregate, "MsvcIsAggregate"},
            {MonadicOperator::MsvcBuiltinAddressOf, "MsvcBuiltinAddressOf"},
            {MonadicOperator::MsvcIsRefClass, "MsvcIsRefClass"},
            {MonadicOperator::MsvcIsValueClass, "MsvcIsValueClass"},
            {MonadicOperator::MsvcIsSimpleValueClass, "MsvcIsSimpleValueClass"},
            {MonadicOperator::MsvcIsInterfaceClass, "MsvcIsInterfaceClass"},
            {MonadicOperator::MsvcIsDelegate, "MsvcIsDelegate"},
            {MonadicOperator::MsvcIsFinal, "MsvcIsFinal"},
            {MonadicOperator::MsvcIsSealed, "MsvcIsSealed"},
            {MonadicOperator::MsvcHasFinalizer, "MsvcHasFinalizer"},
            {MonadicOperator::MsvcHasCopy, "MsvcHasCopy"},
            {MonadicOperator::MsvcHasAssign, "MsvcHasAssign"},
            {MonadicOperator::MsvcHasUserDestructor, "MsvcHasUserDestructor"},

            {MonadicOperator::MsvcConfusion, "MsvcConfusion"},
            {MonadicOperator::MsvcConfusedExpand, "MsvcConfusedExpand"},
            {MonadicOperator::MsvcConfusedDependentSizeof, "MsvcConfusedDependentSizeof"},
            {MonadicOperator::MsvcConfusedPopState, "MsvcConfusedPopState"},
            {MonadicOperator::MsvcConfusedDtorAction, "MsvcConfusedDtorAction"},
        };

        OperatorNameMapEntry<TriadicOperator> triadics[] = {
            {TriadicOperator::Choice, "Choice"},
            {TriadicOperator::ConstructAt, "ConstructAt"},
            {TriadicOperator::Initialize, "Initialize"},
        };

        OperatorNameMapEntry<StorageOperator> storage_ops[] = {
            {StorageOperator::AllocateSingle, "AllocateSingle"},
            {StorageOperator::AllocateArray, "AllocateArray"},
            {StorageOperator::DeallocateSingle, "DeallocateSingle"},
            {StorageOperator::DeallocateArray, "DeallocateArray"},
        };

        OperatorNameMapEntry<VariadicOperator> variadics[] = {
            {VariadicOperator::Collection, "Collection"},
            {VariadicOperator::Sequence, "Sequence"},
            {VariadicOperator::MsvcHasTrivialConstructor, "MsvcHasTrivialConstructor"},
            {VariadicOperator::MsvcIsConstructible, "MsvcIsConstructible"},
            {VariadicOperator::MsvcIsNothrowConstructible, "MsvcIsNothrowConstructible"},
            {VariadicOperator::MsvcIsTriviallyConstructible, "MsvcIsTriviallyConstructible"},
        };

        template<typename Operator>
        using OperatorNameTable = std::map<Operator, const char*>;

        template<typename Op, auto N>
        OperatorNameTable<Op> create_op_table(const OperatorNameMapEntry<Op> (&seq)[N])
        {
            OperatorNameTable<Op> map;
            for (auto& entry : seq)
                map.emplace(entry.op, entry.name);
            return map;
        }

        template<auto& table, typename Op>
        std::string retrieve_name(Op op)
        {
            static const auto name_map = create_op_table(table);
            auto it                    = name_map.find(op);
            if (it != name_map.end())
                return it->second;

            auto sort = operator_sort(op);
            return "unknown-op-" + std::to_string(ifc::to_underlying(sort)) + "-"
                   + std::to_string(ifc::to_underlying(op));
        }
    } // namespace

    // clang-format off
    std::string to_string(NiladicOperator assort) { return retrieve_name<niladics>(assort); }
    std::string to_string(MonadicOperator assort) { return retrieve_name<monadics>(assort); }
    std::string to_string(DyadicOperator assort)  { return retrieve_name<dyadics>(assort); }
    std::string to_string(TriadicOperator assort) { return retrieve_name<triadics>(assort); }
    std::string to_string(StorageOperator assort) { return retrieve_name<storage_ops>(assort); }
    std::string to_string(VariadicOperator assort) { return retrieve_name<variadics>(assort); }

    std::string to_string(const Operator& op)
    {
        switch (op.sort())
        {
        case OperatorSort::Niladic: return to_string(NiladicOperator{ifc::to_underlying(op.index())});
        case OperatorSort::Monadic: return to_string(MonadicOperator{ifc::to_underlying(op.index())});
        case OperatorSort::Dyadic: return to_string(DyadicOperator{ifc::to_underlying(op.index())});
        case OperatorSort::Triadic: return to_string(TriadicOperator{ifc::to_underlying(op.index())});
        case OperatorSort::Storage: return to_string(StorageOperator{ifc::to_underlying(op.index())});
        case OperatorSort::Variadic: return to_string(VariadicOperator{ifc::to_underlying(op.index())});
        default: return "unknown-operator-sort-" + std::to_string(ifc::to_underlying(op.sort()));
        }
    }
    // clang-format on

} // namespace ifc::util
