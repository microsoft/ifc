// Copyright Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <iostream>
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include "ifc/tooling.hxx"
#include "ifc/reader.hxx"
#include "ifc/dom/node.hxx"
#include "printer.hxx"

// -- A macro to abstract over differences of host literal string views. 
#ifdef WIN32
#  define WIDEN(S) L ## S ## sv
#  define NSV(S) WIDEN(S)
#else
#  define NSV(S) S ## sv
#endif

// -- Abstract over standard output and error channels.
#ifdef WIN32
auto& std_error = std::wcerr;
#else
auto& std_error = std::cerr;
#endif

using namespace ifc::util;
using namespace std::literals;

struct Arguments {
    PrintOptions options = PrintOptions::None;

    // Files to process.
    std::vector<ifc::fs::path> files;
};


void print_help(const ifc::fs::path& path)
{
    auto name = path.stem().string();
    std::cout << "Usage:\n\n";
    std::cout << name << " ifc-file1 [ifc-file2 ...] [--color/-c]\n";
    std::cout << name << " --help/-h\n";
}

Arguments process_args(int argc, ifc::tool::NativeChar* argv[])
{
    Arguments result;
    for (int i = 1; i < argc; ++i)
    {
        if (argv[i] == NSV("--help"))
        {
            print_help(argv[0]);
            exit(0);
        }
        else if (argv[i] == NSV("--color"))
        {
            result.options |= PrintOptions::Use_color;
        }
        // Future flags to add as needed
        //   --location: print locations
        //   --header: print module header
        // etc.

        else if (argv[i][0] != '-')
        {
            result.files.emplace_back(argv[i]);
        }
        else
        {
            std_error << NSV("Unknown command line argument '") << argv[i] << NSV("'\n");
            print_help(argv[0]);
            std::exit(1);
        }
    }

    if (result.files.empty())
    {
        std_error << NSV("Specify filepath of an ifc file\n");
        print_help(argv[0]);
        std::exit(1);
    }

    return result;
}

std::vector<std::byte> load_file(const ifc::fs::path& path)
{
    auto size = std::filesystem::file_size(path);
    std::vector<std::byte> v;
    v.resize(size);
    std::ifstream file(path.string(), std::ios::binary);
    file.read(reinterpret_cast<char*>(v.data()), static_cast<std::streamsize>(v.size()));
    return v;
}

void process_ifc(const ifc::fs::path& path, PrintOptions options)
{
    auto contents = load_file(path);

    ifc::InputIfc file{gsl::span(contents)};
    ifc::Pathname name{path.u8string().c_str()};
    file.validate<ifc::UnitSort::Primary>(name, ifc::Architecture::Unknown, ifc::Pathname{},
                                          ifc::IfcOptions::IntegrityCheck);

    ifc::Reader reader(file);
    ifc::util::Loader loader(reader);
    auto& gs = loader.get(reader.ifc.header()->global_scope);
    print(gs, std::cout, options);

    // Make sure that we resolve and print all
    // referenced nodes.
    options |= PrintOptions::Top_level_index;
    while (not loader.referenced_nodes.empty())
    {
        auto it             = loader.referenced_nodes.begin();
        const auto node_key = *it;
        loader.referenced_nodes.erase(it);

        auto& item = loader.get(node_key);
        print(item, std::cout, options);
    }
}

#ifdef WIN32
int wmain(int argc, wchar_t* argv[])
#else
int main(int argc, char* argv[])
#endif
{
    Arguments arguments = process_args(argc, argv);

    try
    {
        for (const auto& file : arguments.files)
            process_ifc(file, arguments.options);

        return EXIT_SUCCESS;
    }
    catch (const ifc::IfcArchMismatch&)
    {
        std::cerr << "ifc architecture mismatch\n";
    }
    catch(ifc::error_condition::UnexpectedVisitor& e)
    {
        std::cerr << "visit unexpected " << e.category << ": " 
                  << e.sort << '\n';
    }
    catch(const ifc::InputIfc::MissingIfcHeader&)
    {
        std::cerr << "Missing ifc binary file header\n";
    }
    catch (...)
    {
        std::cerr << "unknown exception caught\n";
    }

    return EXIT_FAILURE;
}
