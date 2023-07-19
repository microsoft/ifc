#include "ifc/reader.hxx"
#include "ifc/dom/node.hxx"
#include "printer.hxx"
#include <iostream>
#include <filesystem>
#include <fstream>

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
    catch (ifc::IfcArchMismatch&)
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

using namespace ifc::util;
using namespace std::literals;

struct Arguments {
    PrintOptions options = PrintOptions::None;

    // Files to process.
    std::vector<std::string> files;
};

void print_help(std::filesystem::path path)
{
    auto name = path.stem().string();
    std::cout << "Usage:\n\n";
    std::cout << name << " ifc-file1 [ifc-file2 ...] [--color/-c]\n";
    std::cout << name << " --help/-h\n";
}

Arguments process_args(int argc, char** argv)
{
    Arguments result;
    for (int i = 1; i < argc; ++i)
    {
        if (argv[i] == "--help"sv or argv[i] == "-h"sv)
        {
            print_help(argv[0]);
            exit(0);
        }
        else if (argv[i] == "--color"sv or argv[i] == "-c"sv)
        {
            result.options |= PrintOptions::Use_color;
        }
        // Future flags to add as needed
        //   -l --location: print locations
        //   -h --header: print module header
        // etc.

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
    std::filesystem::path path{name};
    auto size = std::filesystem::file_size(path);
    std::vector<std::byte> v;
    v.resize(size);
    std::ifstream file(name, std::ios::binary);
    file.read(reinterpret_cast<char*>(v.data()), v.size());
    return v;
}

void process_ifc(const std::string& name, PrintOptions options)
{
    auto contents = load_file(name);

    ifc::InputIfc file{gsl::span(contents)};
    ifc::Pathname path{name.c_str()};
    file.validate<ifc::UnitSort::Primary>(path, ifc::Architecture::Unknown, ifc::Pathname{},
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

int main(int argc, char** argv)
{
    Arguments arguments = process_args(argc, argv);

    try
    {
        for (const auto& file : arguments.files)
            process_ifc(file, arguments.options);
    }
    catch (...)
    {
        translate_exception();
    }
}
