// Copyright Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// We load the ifc into in memory "dom" format to allow simpler processing
// for text printing, graphing in a dot format, etc.
//
// The main abstraction is a node that has a collection of child nodes
// (where tree form if possible) and properties that are simple string.
//
// Loader tries to preserve the tree form where possible and keep track of references
// to be resolved later. Given an index, loader can be asked for a node
// via Node& get(index) which will load and populate the node and all of it children
// and properties or via std::string ref(index) that will either produce
// a reference string to a node (and remember that a node was referenced to
// be dealt with later) or give a shorthand string when possible,
// such as simple literal or a type 'int' or 'int*" .

#ifndef IFC_UTIL_NODE_H
#define IFC_UTIL_NODE_H

#include "ifc/reader.hxx"
#include "ifc/util.hxx"

#include <compare>
#include <iosfwd>
#include <map>
#include <set>
#include <utility>
#include <variant>
#include <stdexcept>
#include <vector>

namespace ifc::util {
    enum class SortKind : uint16_t {
        Expr,
        Decl,
        Type,
        Name,
        Scope,
        Sentence,
        Chart,
        Syntax,
        Stmt,
    };

    inline std::string to_string(SortKind kind)
    {
        switch (kind)
        {
        case SortKind::Expr:
            return "expr";
        case SortKind::Decl:
            return "decl";
        case SortKind::Type:
            return "type";
        case SortKind::Name:
            return "name";
        case SortKind::Scope:
            return "scope";
        case SortKind::Sentence:
            return "sentence";
        case SortKind::Chart:
            return "chart";
        case SortKind::Syntax:
            return "syntax";
        case SortKind::Stmt:
            return "stmt";
        default:
            return "unexpected-sort-kind-" + std::to_string(ifc::to_underlying(kind));
        }
    }

    // clang-format off
    constexpr SortKind sort_kind(ExprIndex) { return SortKind::Expr;}
    constexpr SortKind sort_kind(DeclIndex) { return SortKind::Decl;}
    constexpr SortKind sort_kind(TypeIndex) { return SortKind::Type;}
    constexpr SortKind sort_kind(ScopeIndex) { return SortKind::Scope;}
    constexpr SortKind sort_kind(NameIndex) { return SortKind::Name;}
    constexpr SortKind sort_kind(SentenceIndex) { return SortKind::Sentence;}
    constexpr SortKind sort_kind(ChartIndex) { return SortKind::Chart;}
    constexpr SortKind sort_kind(StmtIndex) { return SortKind::Stmt;}
    constexpr SortKind sort_kind(SyntaxIndex) { return SortKind::Syntax;}
    constexpr SortKind sort_kind(symbolic::DefaultIndex) { return SortKind::Expr; }
    // clang-format on

    // Type-erased abstract index.
    struct NodeKey {
        // implicit construction intentional
        template<index_like::MultiSorted T>
        NodeKey(T value)
          : index_kind(sort_kind(value)), index_sort(ifc::to_underlying(value.sort())),
            index_value(ifc::to_underlying(value.index()))
        {}

        // implicit construction intentional
        template<index_like::Unisorted T>
        NodeKey(T value) : index_kind(sort_kind(value)), index_sort(), index_value(ifc::to_underlying(value))
        {}

        bool operator==(const NodeKey&) const  = default;
        auto operator<=>(const NodeKey&) const = default;

        SortKind kind() const
        {
            return index_kind;
        }

        auto index() const
        {
            return index_value;
        }

        // convert back to abstract indices
        template<typename F>
        decltype(auto) visit(F&& f);

    private:
        const SortKind index_kind;
        const uint16_t index_sort;
        const uint32_t index_value;
    };

    template<typename F>
    inline decltype(auto) NodeKey::visit(F&& f)
    {
        // clang-format off
        switch (index_kind)
        {
        case SortKind::Expr: return std::forward<F>(f)(index_like::make<ExprIndex>(static_cast<ExprSort>(index_sort), index_value));
        case SortKind::Decl: return std::forward<F>(f)(index_like::make<DeclIndex>(static_cast<DeclSort>(index_sort), index_value));
        case SortKind::Type: return std::forward<F>(f)(index_like::make<TypeIndex>(static_cast<TypeSort>(index_sort), index_value));
        case SortKind::Name: return std::forward<F>(f)(index_like::make<NameIndex>(static_cast<NameSort>(index_sort), index_value));
        case SortKind::Scope: return std::forward<F>(f)(static_cast<ScopeIndex>(index_value));
        case SortKind::Sentence: return std::forward<F>(f)(static_cast<SentenceIndex>(index_value));
        case SortKind::Chart: return std::forward<F>(f)(index_like::make<ChartIndex>(static_cast<ChartSort>(index_sort), index_value));
        case SortKind::Syntax: return std::forward<F>(f)(index_like::make<SyntaxIndex>(static_cast<SyntaxSort>(index_sort), index_value));
        case SortKind::Stmt: return std::forward<F>(f)(index_like::make<StmtIndex>(static_cast<StmtSort>(index_sort), index_value));
        default: throw std::logic_error("unexpected SortKind-" + std::to_string(ifc::to_underlying(index_kind)));
        }
        // clang-format on
    }

    struct Node;

    using PropertyMap = std::map<std::string, std::string>;
    using Nodes       = std::vector<const Node*>;

    struct Node {
        const NodeKey key;
        std::string id;

        explicit Node(NodeKey key_) : key(key_) {}
        Node(const Node&)            = delete;
        Node& operator=(const Node&) = delete;

        PropertyMap props;
        Nodes children;
    };

    // Enumerators and parameters are represented in their sequences by
    // value, not by index, thus, the getter need to be aware of that.
    template<typename T>
    concept EnumeratorOrParameterDecl =
        std::same_as<T, symbolic::EnumeratorDecl> or std::same_as<T, symbolic::ParameterDecl>;

    // Loader is responsible for loading the nodes and resolving references.
    // It is also hosts the storage for all the nodes.
    struct Loader {
        ifc::Reader& reader;

        explicit Loader(Reader& reader_) : reader(reader_) {}

        template<index_like::Algebra Key>
        const Node& get(Key abstract_index);
        template<EnumeratorOrParameterDecl T>
        const Node& get(const T&);
        const Node& get(NodeKey);
        // Convenience helper that returns nullptr for ChartSort::None
        Node* try_get(ChartIndex);

        template<index_like::MultiSorted T>
        std::string ref(T index);
        std::string ref(const symbolic::Identity<TextOffset>& id);
        std::string ref(const symbolic::Identity<NameIndex>& id);

        std::set<NodeKey> referenced_nodes;
        // Track indices currently being processed to detect cycles
        std::set<TypeIndex> processing_types;

    private:
        using NodeMap = std::map<NodeKey, Node>;
        NodeMap all_nodes;
    };

    // implementation details

    void load(Loader& ctx, Node& node, ScopeIndex index);
    void load(Loader& ctx, Node& node, DeclIndex index);
    void load(Loader& ctx, Node& node, ExprIndex index);
    void load(Loader& ctx, Node& node, NameIndex index);
    void load(Loader& ctx, Node& node, TypeIndex index);
    void load(Loader& ctx, Node& node, SentenceIndex index);
    void load(Loader& ctx, Node& node, SyntaxIndex index);
    void load(Loader& ctx, Node& node, ChartIndex index);
    void load(Loader& ctx, Node& node, StmtIndex index);
    void load(Loader& ctx, Node& node, symbolic::DefaultIndex index);

    // Will return a nice short string for a type, such as "int*" if it can.
    // Empty string otherwise.
    std::string get_string_if_possible(Loader&, TypeIndex);
    std::string get_string_if_possible(Loader&, ExprIndex);

    template<index_like::MultiSorted Index>
    std::string get_string_if_possible(Loader&, Index)
    {
        return "";
    }

    template<index_like::MultiSorted T>
    std::string Loader::ref(T index)
    {
        if (null(index))
            return "no-" + to_string(sort_kind(index));

        if (auto str = get_string_if_possible(*this, index); not str.empty())
            return str;

        if (not all_nodes.contains(index))
            referenced_nodes.insert(index);

        return to_string(index);
    }

    template<index_like::Algebra Key>
    const Node& Loader::get(Key abstract_index)
    {
        NodeKey key(abstract_index);
        auto [it, inserted] = all_nodes.emplace(key, key);
        if (inserted)
        {
            load(*this, it->second, abstract_index);
            // if we referenced the node before we can remove it now.
            referenced_nodes.erase(key);
        }
        return it->second;
    }

    inline const Node& Loader::get(NodeKey key)
    {
        return key.visit([this](auto index) -> const Node& { return get(index); });
    }

    template<EnumeratorOrParameterDecl T>
    inline const Node& Loader::get(const T& decl)
    {
        const auto abstract_index = reader.index_of<DeclIndex>(decl);
        return get(abstract_index);
    }

} // namespace ifc::util

#endif // IFC_UTIL_NODE_H
