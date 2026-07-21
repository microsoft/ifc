// Copyright Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "ifc/util.hxx"
#include "ifc/reader.hxx"

namespace ifc {
    constexpr std::string_view analysis_partition_prefix = ".msvc.code-analysis.";

    Reader::Reader(const ifc::InputIfc& ifc_) : ifc(ifc_)
    {
        if (not ifc.header())
            throw InputIfc::MissingIfcHeader{};
        read_table_of_contents();
    }

    void Reader::read_table_of_contents()
    {
        const auto file_size = ifc.contents().size();
        
        for (auto& summary : ifc.partition_table())
        {
            IFCASSERT(not index_like::null(summary.name));
            IFCASSERT(not summary.empty());
            
            // Validate partition bounds to prevent buffer overruns
            const auto base_offset = to_underlying(summary.offset);
            const auto entry_size = to_underlying(summary.entry_size);
            const auto cardinality = to_underlying(summary.cardinality);
            
            // Check base offset is within file
            IFCASSERT(base_offset < file_size);
            
            // Check that the partition doesn't extend beyond file bounds
            if (cardinality > 0 && entry_size > 0) {
                // Check for potential overflow in the multiplication
                const auto max_index = static_cast<uint64_t>(cardinality - 1);
                const auto entry_size_64 = static_cast<uint64_t>(entry_size);
                const auto base_offset_64 = static_cast<uint64_t>(base_offset);
                
                // Validate that the last entry fits within the file
                if (max_index <= (UINT64_MAX - entry_size_64) / entry_size_64) {
                    const auto max_index_offset = max_index * entry_size_64;
                    if (base_offset_64 <= UINT64_MAX - max_index_offset - entry_size_64) {
                        const auto partition_end = base_offset_64 + max_index_offset + entry_size_64;
                        IFCASSERT(partition_end <= file_size);
                    } else {
                        // Overflow would occur, partition extends beyond any reasonable bounds
                        IFCASSERT(false);
                    }
                } else {
                    // Overflow in multiplication, partition is impossibly large
                    IFCASSERT(false);
                }
            }
            
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
