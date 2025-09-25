// Copyright Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "printer.hxx"

#if defined(IFC_BUILD_USING_STD_MODULE)
    import std;
#else
    #include <stack>
    #include <set>
#endif

namespace ifc::util {
    namespace {
        // Values of enum refer to the escaped console color code '\u001b[<XXX>m'.
        enum class ConsoleColor {
            None  = 0,
            Black = 30,
            Red,
            Green,
            Yellow,
            Blue,
            Magenta,
            Cyan,
            White,
        };

        std::ostream& operator<<(std::ostream& os, ConsoleColor color)
        {
            os << "\u001b[" << static_cast<int>(color) << "m";
            return os;
        }

        class ScopedConsoleColor {
        public:
            ScopedConsoleColor(std::ostream& os_, ConsoleColor color, bool enabled_ = true) : os(os_), enabled(enabled_)
            {
                if (enabled)
                {
                    os << color;
                }
            }

            ~ScopedConsoleColor()
            {
                if (enabled)
                {
                    os << ConsoleColor::None;
                }
            }

        private:
            std::ostream& os;
            bool enabled = true;
        };

        ConsoleColor get_node_color(const SortKind kind)
        {
            switch (kind)
            {
            case SortKind::Scope:
                return ConsoleColor::Yellow;
            case SortKind::Chart:
                return ConsoleColor::Yellow;
            case SortKind::Expr:
                return ConsoleColor::Yellow;

            case SortKind::Decl:
                return ConsoleColor::Magenta;

            case SortKind::Name:
                return ConsoleColor::Cyan;
            case SortKind::Type:
                return ConsoleColor::Green;
            case SortKind::Stmt:
                return ConsoleColor::Red;
            default:
                return ConsoleColor::White;
            }
        }

        struct TreePrinter {
            enum class ChildType : char8_t {
                First,
                Last,
                Only_child,
                Regular,
            };
            enum class IndentAction : size_t {
                Child,        // '|-' mark this item as a child of its parent
                Last_child,   // '`-' mark this item as the last child of its parent
                Recurse,      // '| ' recurse for the elements of a single child
                Recurse_last, // '  ' recurse for the elements of the last child
                Undo,         // remove the last indent action
            };

            static ChildType compute_child_type(const size_t current, const size_t total)
            {
                if (total == 1)
                    return ChildType::Only_child;
                if (current == 1)
                    return ChildType::First;
                if (current == total)
                    return ChildType::Last;
                return ChildType::Regular;
            }

            // We may switch values of the properties to be a variant<String, NodeRef> in the future.
            // This useless (at the moment) function will be doing the translation to string from NodeRef then.
            const std::string& get_string_prop(const std::string& val)
            {
                return val;
            }

            void dump_node_header(const Node& n, ChildType child_type)
            {
                update_indent(depth, child_type);

                // Handle the well known properties first.
                // We put the interesting ones (name and type, etc) in a fixed order and in a pretty color.
                // Some known properties are too noisy (such as alignment), we don't print it at all.
                static std::set<std::string> known = {"name",   "type",      "base",      "source",
                                                      "assort", "pack_size", "alignment", "home-scope"};
                stream << indent.c_str();
                {
                    ColorSetter color(*this, get_node_color(n.key.kind()));
                    stream << n.id;
                    // Only declarations have an index embedded in the id, but, if a
                    // use requested printing of index, we will print it as requested.
                    if (depth == 0 and implies(options, PrintOptions::Top_level_index)
                        and n.key.kind() != SortKind::Decl)
                        stream << "-" << n.key.index();
                }
                if (auto it = n.props.find("type"); it != n.props.end())
                {
                    ColorSetter color(*this, get_node_color(SortKind::Type));
                    stream << " '" << get_string_prop(it->second) << "'";
                }
                if (auto it = n.props.find("name"); it != n.props.end())
                {
                    ColorSetter color(*this, get_node_color(SortKind::Name));
                    stream << " " << get_string_prop(it->second);
                }
                if (auto it = n.props.find("base"); it != n.props.end())
                {
                    ColorSetter color(*this, get_node_color(SortKind::Type));
                    stream << " :" << get_string_prop(it->second);
                }
                if (auto it = n.props.find("assort"); it != n.props.end())
                {
                    ColorSetter color(*this, get_node_color(SortKind::Name));
                    stream << " " << get_string_prop(it->second);
                }
                if (auto it = n.props.find("pack_size"); it != n.props.end())
                {
                    ColorSetter color(*this, get_node_color(SortKind::Name));
                    stream << " packed-" << get_string_prop(it->second);
                }
                if (auto it = n.props.find("home-scope"); it != n.props.end())
                {
                    ColorSetter color(*this, get_node_color(SortKind::Stmt));
                    stream << " home-scope(" << get_string_prop(it->second) << ")";
                }

                // dump the rest of the properties in a boring white.
                // Color_setter color(*this, Console_color::White);

                for (auto& entry : n.props)
                    if (not known.contains(entry.first))
                        if (std::string str = get_string_prop(entry.second); not str.empty())
                            stream << ' ' << str;

                stream << std::endl;
            }

            void visit(const Node& n)
            {
                // special case for the topmost node.
                if (depth == 0)
                    dump_node_header(n, ChildType::Only_child);

                ++depth;
                const auto total   = n.children.size();
                size_t child_count = 0;
                for (auto* child : n.children)
                {
                    dump_node_header(*child, compute_child_type(++child_count, total));
                    visit(*child);
                }
                --depth;
            }

            TreePrinter& operator<<(std::string_view str);
            TreePrinter& operator<<(IndentAction action);

            void update_indent(size_t, ChildType);

            explicit TreePrinter(std::ostream& stream_, PrintOptions options_ = {}) : stream(stream_), options(options_) {}

            struct ColorSetter : ScopedConsoleColor {
                ColorSetter(TreePrinter& pp, ConsoleColor color)
                  : ScopedConsoleColor(pp.stream, color, implies(pp.options, PrintOptions::Use_color))
                {}
            };

        private:
            size_t depth{0};

            std::ostream& stream;
            PrintOptions options;

            std::string indent;
            std::stack<IndentAction> indents;
            std::string indent_strs[static_cast<int>(IndentAction::Recurse_last) + 1]{"|-", "\\-", "| ", "  "};
        };

        TreePrinter& TreePrinter::operator<<(IndentAction action)
        {
            using enum_type = std::underlying_type<IndentAction>::type;
            if (action != IndentAction::Undo)
            {
                indents.push(action);
                indent += indent_strs[static_cast<enum_type>(action)];
            }
            else if (not indents.empty())
            {
                auto indent_size = indent_strs[static_cast<enum_type>(indents.top())].size();
                indent           = indent.substr(0, indent.size() - indent_size);
                indents.pop();
            }

            return *this;
        }

        void TreePrinter::update_indent(size_t indent_depth, ChildType child_type)
        {
            if (indent_depth == 0)
                return;

            if (not indents.empty() and (child_type == ChildType::First or child_type == ChildType::Only_child))
            {
                if (indents.top() == IndentAction::Child)
                {
                    *this << IndentAction::Undo << IndentAction::Recurse;
                }
                else if (indents.top() == IndentAction::Last_child)
                {
                    *this << IndentAction::Undo << IndentAction::Recurse_last;
                }
            }

            while (indents.size() >= indent_depth)
            {
                *this << IndentAction::Undo;
            }

            if (child_type == ChildType::First or child_type == ChildType::Regular)
            {
                *this << IndentAction::Child;
            }
            else
            {
                *this << IndentAction::Last_child;
            }
        }
    } // namespace

    void print(const Node& node, std::ostream& os, PrintOptions options)
    {
        TreePrinter tree(os, options);
        tree.visit(node);
    }

} // namespace ifc::util
