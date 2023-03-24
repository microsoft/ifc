#include "ifc/reader.hxx"
#include "ifc/dom/node.hxx"
#include "printer.hxx"
#include <iostream>
#include <filesystem>
#include <fstream>

using ifc::to_underlying;

void translate_exception()
{
    try
    {
        throw;
    }
    catch (std::exception& e)
    {
        std::cout << "caught: " << e.what() << '\n';
    }
    catch (Module::IfcArchMismatch&)
    {
        std::cout << "ifc architecture mismatch\n";
    }
    catch (const char* message)
    {
        std::cout << "caught: " << message;
    }
    catch (...)
    {
        std::cout << "unknown exception caught\n";
    }
}

using namespace Module::util;
using namespace std::literals;

struct Arguments
{
    Print_options options = Print_options::None;

    // Files to process.
    std::vector<std::string> files;
};

void print_help(std::filesystem::path path)
{
    auto name = path.stem().string();
    std::cout << "Usage:\n\n";
    std::cout << name << " ifc-file1 [ifc-file2 ...]\n";
    std::cout << name << " --help/-h\n";
}

Arguments process_args(int argc, char** argv)
{
    Arguments result;
    for (int i = 1; i < argc; ++i)
    {
        if (argv[i] == "--help"sv || argv[i] == "-h"sv)
        {
            print_help(argv[0]);
            exit(0);
        }
        else if (argv[i][0] != '-')
        {
            result.files.push_back(argv[i]);
        }
        else
        {
            std::cout << "Unknown command line argument '" << argv[i] << "'\n";
            print_help(argv[0]);
            std::exit(1);
        }
    }

    if (result.files.empty())
    {
        std::cout << "Specify filepath of an ifc file\n";
        print_help(argv[0]);
        std::exit(1);
    }

    return result;
}

std::vector<std::byte> load_file(const std::string& name)
{
    std::filesystem::path path { name };
    auto size = std::filesystem::file_size(path);
    std::vector<std::byte> v;
    v.resize(size);
    std::ifstream file(name, std::ios::binary);
    file.read(reinterpret_cast<char*>(v.data()), v.size());
    return v;
}

std::ostream& operator << (std::ostream& os, const Module::Version& v)
{
    os << (int)to_underlying(v);
    return os;
}

std::ostream& operator << (std::ostream& os, const Module::Abi& v)
{
    os << (int)to_underlying(v);
    return os;
}

std::ostream& operator << (std::ostream& os, const Module::CPlusPlus& v)
{
    os << to_underlying(v);
    return os;
}

std::ostream& operator << (std::ostream& os, const Module::ByteOffset& v)
{
    os << to_underlying(v);
    return os;
}

std::ostream& operator << (std::ostream& os, const Module::Cardinality& v)
{
    os << to_underlying(v);
    return os;
}

std::ostream& operator << (std::ostream& os, const Module::TextOffset& v)
{
    os << to_underlying(v);
    return os;
}

std::ostream& operator << (std::ostream& os, const Module::EntitySize& v)
{
    os << to_underlying(v);
    return os;
}

std::ostream& operator << (std::ostream& os, const Module::Architecture& v)
{
    switch (v)
    {
    case Module::Architecture::Unknown:
        os << "Unknown";
        break;
    case Module::Architecture::X86:
        os << "X86";
        break;
    case Module::Architecture::X64:
        os << "X64";
        break;
    case Module::Architecture::ARM32:
        os << "ARM32";
        break;
    case Module::Architecture::ARM64:
        os << "ARM64";
        break;
    case Module::Architecture::HybridX86ARM64:
        os << "HybridX86ARM64";
        break;
    case Module::Architecture::ARM64EC:
        os << "ARM64EC";
        break;
    default:
        throw 1;
    }
    return os;
}

std::ostream& operator << (std::ostream& os, const Module::NameIndex& v)
{
    IFCASSERT(v.sort() == Module::NameSort::SourceFile);
    os << to_underlying(v.index());
    return os;
}

std::ostream& operator << (std::ostream& os, const Module::Symbolic::FileAndLine& v)
{
    os << "[" << v.file() << "]:" << v.line();
    return os;
}

std::ostream& operator << (std::ostream& os, const Module::FormatVersion& v)
{
    os << v.major << "." << v.minor;
    return os;
}

struct dumper
{
    const std::byte* base = nullptr;
    std::ostream& ostr;
    const Module::Header& header;
    std::vector<Module::TextOffset> strings;
    dumper(std::ostream& ostr, const std::byte* base) : 
        base(base),
        ostr(ostr),
        header(*reinterpret_cast<const Module::Header*>(base + 4))
    {

    }

    int string_index(Module::TextOffset offset)
    {
        auto iter = std::lower_bound(strings.begin(), strings.end(), offset);
        if (iter == strings.end())
            return -1;
        return static_cast<int>(iter - strings.begin());
    }

    void output(const Module::Symbolic::FieldDecl& v)
    {
        ostr << "field ";
        output_string_complete(v.identity.name);
        ostr << " {";
        /*
            Identity<TextOffset> identity;  // The name and location of this non-static data member
            TypeIndex type;                 // Sort and index of this decl's type.  Null means no type.
            DeclIndex home_scope;           // Enclosing scope of this declaration.
            ExprIndex initializer;          // The field initializer (if any)
            ExprIndex alignment;            // If non-zero, explicit alignment specification, e.g. alignas(N).
            ObjectTraits obj_spec;          // Object traits associated with this field.
            BasicSpecifiers basic_spec;     // Basic declaration specifiers.
            Access access;                  // Access right to this data member.
            ReachableProperties properties; // Set of reachable semantic properties of this declaration
        */
        ostr << "\n}";
    }

    void output_string(Module::TextOffset offset)
    {
        auto index = string_index(offset);
        if (index != -1)
        {
            const std::byte* p = &base[static_cast<unsigned int>(header.string_table_bytes)];
            const char* string = reinterpret_cast<const char*>(p + to_underlying(offset));
            ostr << string;
        }
        else
            ostr << "[invalid]" << to_underlying(offset);
    }

    std::string get_string(Module::TextOffset offset)
    {
        auto index = string_index(offset);
        if (index != -1)
        {
            const std::byte* p = &base[static_cast<unsigned int>(header.string_table_bytes)];
            const char* string = reinterpret_cast<const char*>(p + to_underlying(offset));
            return string;
        }
        else
            return "[invalid]";
    }

    void output_header()
    {
        ostr << "Version: " << header.version << std::endl;
        ostr << "Abi: " << header.abi << std::endl;
        ostr << "Architecture: " << header.arch << std::endl;
        ostr << "Dialect: " << header.cplusplus << std::endl;
        /* The following fields are dumped for information purposes but aren't used to rebuild. */
        ostr << "/*" << std::endl;
        ostr << "String Table Offset: " << header.string_table_bytes << std::endl;
        ostr << "String Table Size: " << header.string_table_size << std::endl;
        //    ostr << "TU Index: " << header.unit << std::endl; ????????????????????????????
        ostr << "Source Path Offset: " << header.src_path;
        ostr << " /*";
        output_string(header.src_path);
        ostr << "*/\n";
        ostr << "Global Scope Index: " << header.global_scope << std::endl;
        ostr << "TOC Offset: " << header.toc << std::endl;
        ostr << "Partitions: " << header.partition_count << std::endl;
        ostr << "*/" << std::endl;

        ostr << "Internal: " << header.internal_partition << std::endl;
    }

    void calc_string_table()
    {
        const std::byte* p = &base[static_cast<unsigned int>(header.string_table_bytes)];
        Module::Cardinality length = header.string_table_size;

        auto start = p;
        auto end = p + to_underlying(length);
        bool first = true;
        while (start < end)
        {
            if (first)
            {
                auto offset = start - p;
                strings.push_back(Module::TextOffset(offset));
            }
            first = false;
            if (*start == std::byte{ 0 })
            {
                first = true;
            }
            ++start;
        }
    }

    void output_string_table()
    {
        auto p = reinterpret_cast<const char*>(&base[static_cast<unsigned int>(header.string_table_bytes)]);
        ostr << "StringTable\n{\n";
        for (size_t i = 0; i < strings.size(); ++i)
        {
            ostr << "[" << i << "]:";
            ostr << p + to_underlying(strings[i]) << std::endl;
        }
        ostr << "}\n";
    }

    void output_string_complete(Module::TextOffset offset)
    {
        ostr << "\"";
        output_string(offset);
        ostr << "\"";

        ostr << "/*[" << string_index(offset) << "]*/";
    }

    void output_partition(const Module::PartitionSummaryData& summary)
    {
        ostr << "partition ";
        output_string_complete(summary.name);
        ostr << "\n";

        /*
                TextOffset name;
                ByteOffset offset;
                Cardinality cardinality;
                EntitySize entry_size;
        */

        ostr << "/*\n name:" << summary.name << std::endl;
        ostr << " offset:" << summary.offset << std::endl;
        ostr << " cardinality:" << summary.cardinality << std::endl;
        ostr << " entry_size:" << summary.entry_size << std::endl;
        ostr << "*/\n";

        /* Don't know what the partition type is until later. */
        const std::byte* partition = base + to_underlying(summary.offset);
        ostr << "{\n";

        {
            ostr << std::setw(2) << std::setfill('0') << std::hex;
            auto block = partition;
            for (unsigned int i = 0; i < to_underlying(summary.cardinality); ++i)
            {
                for (unsigned int j = 0; j < to_underlying(summary.entry_size); ++j)
                {
                    ostr << std::to_integer<int>(block[j]) << " ";
                }
                ostr << std::endl;
                block += to_underlying(summary.entry_size);
            }
            ostr << std::dec;
        }
        if (get_string(summary.name) == "src.line"sv)
        {
            auto p = reinterpret_cast<const Module::Symbolic::FileAndLine*>(partition);
            for (unsigned int i = 0; i < to_underlying(summary.cardinality); ++i)
            {
                ostr << p[i] << std::endl;
            }
        }
        else if (get_string(summary.name) == "decl.field"sv)
        {
            auto p = reinterpret_cast<const Module::Symbolic::FieldDecl*>(partition);
            for (unsigned int i = 0; i < to_underlying(summary.cardinality); ++i)
            {
                output(p[i]);
                ostr << std::endl;
            }
        }
        else
        {
            ostr << "/* Partition TBD */" << std::endl;
        }

        ostr << "}\n";

    }

    void output_toc()
    {
        const std::byte* p = &base[static_cast<unsigned int>(header.toc)];

    }


    /*
    The table of contents is an array of all partition summaries in the
    IFC. The field toc indicates the offset (in bytes) from the beginning of the offset
    to the first partition descriptor. The number of partition summaries in the table
    of contents is given by the field partition_count.
    */
    void output_partitions()
    {
        Module::Cardinality count = header.partition_count;
        auto partition_summary_base = reinterpret_cast<const Module::PartitionSummaryData*>(base + to_underlying(header.toc));
        for (unsigned int i = 0; i < to_underlying(count); ++i)
        {
            output_partition(partition_summary_base[i]);
        }
    }

};

void process_ifc(const std::string& name, Print_options options)
{
    auto contents = load_file(name);
    /*
    * File Signature (4 bytes)
    * Header
    * Partition 1
    * ...
    * Partition n
    * String Table
    * Table of Contents
    */

    dumper dumper(std::cout, &contents[0]);
    dumper.calc_string_table();
    dumper.output_header();
    dumper.output_partitions();
    dumper.output_string_table();

    //Module::InputIfc file{ gsl::span(contents) };
    //file.validate<Module::UnitSort::Primary>(Module::Architecture::Unknown, Module::Pathname{ }, Module::IfcOptions::IntegrityCheck);

    //Module::Reader reader(file);
    //Module::util::Loader loader(reader);
    //auto& gs = loader.get(reader.ifc.header()->global_scope);
    //print(gs, std::cout, options);

    //// Make sure that we resolve and print all
    //// referenced nodes.
    //options |= Print_options::Top_level_index;
    //while (not loader.referenced_nodes.empty())
    //{
    //    auto it = loader.referenced_nodes.begin();
    //    const auto node_key = *it;
    //    loader.referenced_nodes.erase(it);

    //    auto& item = loader.get(node_key);
    //    print(item, std::cout, options);
    //}
}

int main(int argc, char** argv)
{
    Arguments arguments = process_args(argc, argv);

    try
    {
        for (const auto& file: arguments.files)
            process_ifc(file, arguments.options);
    }
    catch (...)
    {
        translate_exception();
    }
}
