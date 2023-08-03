// Copyright Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef IFC_UTILS_H
#define IFC_UTILS_H

#include "ifc/abstract-sgraph.hxx"
#include <string>

namespace ifc::util {
    // return a single string for various IFC flags and enums

    std::string to_string(Access access);
    std::string to_string(BasicSpecifiers basic);
    std::string to_string(ReachableProperties reachable);
    std::string to_string(Qualifier reachable);
    std::string to_string(symbolic::ExpansionMode mode);
    std::string to_string(ScopeTraits traits);
    std::string to_string(ObjectTraits traits);
    std::string to_string(FunctionTraits traits);
    std::string to_string(symbolic::ReadExpr::Kind kind);
    std::string to_string(CallingConvention conv);
    std::string to_string(NoexceptSort sort);
    std::string to_string(symbolic::ExpressionListExpr::Delimiter delimiter);
    std::string to_string(symbolic::DestructorCallExpr::Kind kind);
    std::string to_string(symbolic::InitializerExpr::Kind kind);
    std::string to_string(symbolic::Associativity kind);
    std::string to_string(GuideTraits traits);
    std::string to_string(symbolic::BaseClassTraits traits);
    std::string to_string(symbolic::SourceLocation locus);

    // fundamental types and type basis can be converted to string.
    std::string to_string(symbolic::TypeBasis basis);
    std::string to_string(const symbolic::FundamentalType& type);

    // those implemented in their own operators.cxx as there are a lot of them!
    std::string to_string(MonadicOperator assort);
    std::string to_string(DyadicOperator assort);
    std::string to_string(TriadicOperator assort);
    std::string to_string(const Operator& assort);
    std::string to_string(StorageOperator assort);
    std::string to_string(VariadicOperator assort);

    // return a single string of the form "decl.variable-N" for various abstract indices
    template<index_like::MultiSorted Key>
    std::string to_string(Key index)
    {
        return sort_name(index.sort()) + ("-" + std::to_string((int)index.index()));
    }

    inline std::string to_string(SentenceIndex index)
    {
        return "sentence-" + std::to_string((int)index);
    }
} // namespace ifc::util
#endif // IFC_UTILS_H
