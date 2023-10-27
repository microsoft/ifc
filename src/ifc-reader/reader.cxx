// Copyright Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "ifc/util.hxx"
#include "ifc/reader.hxx"
#include <cassert>

namespace ifc {
    constexpr std::string_view analysis_partition_prefix = ".msvc.code-analysis.";

    Reader::Reader(const ifc::InputIfc& ifc_) : ifc(ifc_)
    {
        assert(ifc.header());
        read_table_of_contents();
    }

    void Reader::read_table_of_contents()
    {
        for (auto& summary : ifc.partition_table())
        {
            IFCASSERT(not index_like::null(summary.name));
            IFCASSERT(not summary.empty());
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

} // namespace ifc
