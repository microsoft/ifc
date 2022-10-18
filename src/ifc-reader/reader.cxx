#include "ifc/reader.hxx"
#include <stdexcept>

class Not_yet_implemented : public std::logic_error
{
public:
    Not_yet_implemented(std::string_view what_is_not_implemented) :
        logic_error("'" + std::string(what_is_not_implemented) + "' is not yet implemented")
    {
    }
};

class Unexpected : public std::logic_error
{
public:
    Unexpected(std::string_view what_is_not_expected) :
        logic_error("unexpected: '" + std::string(what_is_not_expected) + "'")
    {
    }
};

namespace Module
{
    constexpr std::string_view analysis_partition_prefix = ".msvc.code-analysis.";

    Reader::Reader(const Module::InputIfc& ifc) : ifc(ifc)
    {
         if (not ifc.header())
            throw "file not found";
        read_table_of_contents();
    }

    void Reader::read_table_of_contents()
    {
        for (auto& summary : ifc.partition_table())
        {
            DASSERT(!index_like::null(summary.name));
            DASSERT(!summary.empty());
            std::string_view name = get(summary.name);
            // Partitions created by the analysis plugins are not members of the usual table of
            // contents structure.
            if (name.starts_with(analysis_partition_prefix))
            {
                // TODO: Add handling for static analysis partitions once we know what they are.
                // supplemental_partitions.push_back({ .name = name, .summary = summary });
            }
            else
            {
                summary_by_partition_name(toc, name) = summary;
            }
        }
    }

    void not_implemented(std::string_view message)
    {
        throw Not_yet_implemented(message);
    }

    void not_implemented(std::string_view message, int payload)
    {
        std::string result(message);
        result += std::to_string(payload);
        not_implemented(result);
    }

    void unexpected(std::string_view message)
    {
        throw Unexpected(message);
    }

    void unexpected(std::string_view message, int payload)
    {
        std::string result(message);
        result += std::to_string(payload);
        unexpected(result);
    }

}  // namespace Module
