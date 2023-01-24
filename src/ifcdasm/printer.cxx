#include "printer.hxx"
#include <iostream>

#include <stack>
#include <set>

namespace Module::util
{
    namespace
    {
        struct Tree_printer
        {
            enum class Child_type : char8_t
            {
                First,
                Last,
                Only_child,
                Regular
            };
            enum class Indent_action : size_t
            {
                Child,         // '|-' mark this item as a child of its parent
                Last_child,    // '`-' mark this item as the last child of its parent
                Recurse,       // '| ' recurse for the elements of a single child
                Recurse_last,  // '  ' recurse for the elements of the last child
                Undo,          // remove the last indent action
            };

            static Child_type compute_child_type(const size_t current, const size_t total)
            {
                if (total == 1)
                    return Child_type::Only_child;
                if (current == 1)
                    return Child_type::First;
                if (current == total)
                    return Child_type::Last;
                return Child_type::Regular;
            }

            // We may switch values of the properties to be a variant<String, NodeRef> in the future.
            // This useless (at the moment) function will be doing the translation to string from NodeRef then.
            const std::string& get_string_prop(const std::string& val)
            {
                return val;
            }

            void dump_node_header(const Node& n, Child_type child_type)
            {
                update_indent(depth, child_type);

                // Handle the well known properties first.
                // We put the interesting ones (name and type, etc) in a fixed order.
                // Some known properties are too noisy (such as alignment), we don't print it at all.
                static std::set<std::string> known = {
                    "name", "type", "base", "source", "assort", "pack_size", "alignment", "home-scope"};
                stream << indent.c_str();
                {
                    stream << n.id;
                    // Only declarations have an index embedded in the id, but, if a
                    // use requested printing of index, we will print it as requested.
                    if (depth == 0 && implies(options, Print_options::Top_level_index) &&
                        n.key.kind() != SortKind::Decl)
                        stream << "-" << (int)n.key.index();
                }
                if (auto it = n.props.find("type"); it != n.props.end())
                {
                    stream << " '" << get_string_prop(it->second) << "'";
                }
                if (auto it = n.props.find("name"); it != n.props.end())
                {
                    stream << " " << get_string_prop(it->second);
                }
                if (auto it = n.props.find("base"); it != n.props.end())
                {
                    stream << " :" << get_string_prop(it->second);
                }
                if (auto it = n.props.find("assort"); it != n.props.end())
                {
                    stream << " " << get_string_prop(it->second);
                }
                if (auto it = n.props.find("pack_size"); it != n.props.end())
                {
                    stream << " packed-" << get_string_prop(it->second);
                }
                if (auto it = n.props.find("home-scope"); it != n.props.end())
                {
                    stream << " home-scope(" << get_string_prop(it->second) << ")";
                }

                for (auto& entry : n.props)
                    if (not known.contains(entry.first))
                        if (std::string str = get_string_prop(entry.second); !str.empty())
                            stream << ' ' << str;

                stream << std::endl;
            }

            void visit(const Node& n)
            {
                // special case for the topmost node.
                if (depth == 0)
                    dump_node_header(n, Child_type::Only_child);

                ++depth;
                const auto total = n.children.size();
                size_t count = 0;
                for (auto* child : n.children)
                {
                    dump_node_header(*child, compute_child_type(++count, total));
                    visit(*child);
                }
                --depth;
            }

            Tree_printer& operator<<(std::string_view str);
            Tree_printer& operator<<(Indent_action action);

            void update_indent(size_t, Child_type);

            explicit Tree_printer(std::ostream& stream, Print_options options = {}) :
                stream(stream), options(options)
            {
            }

        private:
            size_t depth{0};

            std::ostream& stream;
            Print_options options;

            std::string indent;
            std::stack<Indent_action> indents;
            std::string indent_strs[static_cast<int>(Indent_action::Recurse_last) + 1]{
                "|-", "\\-", "| ", "  "};
        };

        Tree_printer& Tree_printer::operator<<(std::string_view str)
        {
            stream << str;
            return *this;
        }

        Tree_printer& Tree_printer::operator<<(Indent_action action)
        {
            using enum_type = std::underlying_type<Indent_action>::type;
            if (action != Indent_action::Undo)
            {
                indents.push(action);
                indent += indent_strs[static_cast<enum_type>(action)];
            }
            else if (!indents.empty())
            {
                auto indent_size = indent_strs[static_cast<enum_type>(indents.top())].size();
                indent = indent.substr(0, indent.size() - indent_size);
                indents.pop();
            }

            return *this;
        }

        void Tree_printer::update_indent(size_t depth, Child_type child_type)
        {
            if (depth != 0)
            {
                if (!indents.empty() &&
                    (child_type == Child_type::First || child_type == Child_type::Only_child))
                {
                    if (indents.top() == Indent_action::Child)
                    {
                        *this << Indent_action::Undo << Indent_action::Recurse;
                    }
                    else if (indents.top() == Indent_action::Last_child)
                    {
                        *this << Indent_action::Undo << Indent_action::Recurse_last;
                    }
                }

                while (indents.size() >= depth)
                {
                    *this << Indent_action::Undo;
                }

                if (child_type == Child_type::First || child_type == Child_type::Regular)
                {
                    *this << Indent_action::Child;
                }
                else
                {
                    *this << Indent_action::Last_child;
                }
            }
        }
    }  // namespace

    void print(const Node& node, std::ostream& os, Print_options options)
    {
        Tree_printer tree(os, options);
        tree.visit(node);
    }

}  // namespace Module::util
