#include "ifc/reader.hxx"
#include <ipr/util/exceptions.hxx> // to integrate error reporting
#include <Windows.h>

extern Filesystem* os_api;
// ifclib relies on a hand-written utf8 conversion routine which is deeply
// embedded in the c1xx and is difficult to pull out. Use this as a replacement for now.
// User Story: 1367721 tracks more permanent solution.
size_t UTF8ToUTF16Cch(LPCSTR lpSrcStr, size_t cchSrc, LPWSTR lpDestStr, size_t cchDest)
{
    return MultiByteToWideChar(CP_UTF8, 0, lpSrcStr, (int)cchSrc, lpDestStr, (int)cchDest);
}

namespace Module
{
    constexpr std::string_view analysis_partition_prefix = ".msvc.code-analysis.";

    Reader::Reader(const Pathname& path) :
        InputIfcFile(MemoryMapped::AnonymousModuleIfcFileHelper(
            path, Architecture::Unknown, MemoryMapped::FileProjectionOptions::None, os_api).project())
    {
        if (not header())
            throw "file not found";
        read_table_of_contents();
    }

    void Reader::read_table_of_contents()
    {
        for (auto& summary : partition_table())
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

    // Map error functions to the exceptions used in ipr-tools project.
    void not_implemented(std::string_view message)
    {
        throw ipr::Not_yet_implemented(message);
    }

    void not_implemented(std::string_view message, int payload)
    {
        std::string result(message);
        result += std::to_string(payload);
        not_implemented(result);
    }

    void unexpected(std::string_view message)
    {
        throw ipr::Unexpected(message);
    }

    void unexpected(std::string_view message, int payload)
    {
        std::string result(message);
        result += std::to_string(payload);
        unexpected(result);
    }

}  // namespace Module