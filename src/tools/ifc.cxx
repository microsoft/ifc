// Copyright Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <new>
#include <string>
#include <utility>
#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>

#ifdef WIN32
#   include <windows.h>
#else
#   include <sys/wait.h>
#   include <unistd.h>
#endif

#include "ifc/file.hxx"
#include "ifc/tooling.hxx"
#include "tool-support.hxx"
#include "ifc-archive.hxx"

#ifdef WIN32
#   define IFC_MAIN wmain
#else
#   define IFC_MAIN main
#endif

namespace {
    // The driver and its subcommands share the option-parsing and IFC-reading helpers declared
    // in tool-support.hxx.
    using namespace ifc::tool;

    // -- Print a brief message of how to invoke the ifc tool.
    void print_usage(const ifc::fs::path& prog)
    {
        auto name = prog.stem();
        IFC_ERR << name << STR(" usage:\n\t")
            << name.native() << STR(" <cmd> [options] <ifc-files>\n");
    }

    // -- Subcommand printing the Spec version from an IFC file.
    struct VersionCommand final : ifc::tool::Extension {
        // -- A constexpr registry key lets the builtin table prove its ordering at compile time.
        constexpr ifc::tool::Name name() const final { return STR("version"); }
        // -- Version inspection reads only the fixed prefix, avoiding whole-file address-space use.
        int run_with(const ifc::tool::Arguments& args) const final
        {
            int error_count = 0;
            for (auto& arg : args)
            {
                if (resemble_option(arg))
                {
                    IFC_ERR << STR("invalid option ") << arg
                            << STR(" to ifc subcommand ")
                            << name() << std::endl;
                    ++error_count;
                    continue;
                }
                ifc::fs::path path { arg };
                std::ifstream file { path, std::ios_base::binary };
                if (not file)
                {
                    IFC_ERR << arg << STR(": couldn't open file") << std::endl;
                    ++error_count;
                    continue;
                }
                // Only the signature and header are needed, so read just those bytes rather
                // than mapping (and reserving address space for) the entire file.
                ifc::Header header { };
                const HeaderRead status = read_ifc_header(file, header);
                if (status == HeaderRead::NotIfc)
                {
                    IFC_ERR << arg << STR(" is not an IFC file") << std::endl;
                    ++error_count;
                    continue;
                }
                if (status == HeaderRead::Truncated)
                {
                    IFC_ERR << arg << STR(" is truncated or corrupted") << std::endl;
                    ++error_count;
                    continue;
                }
                IFC_OUT << arg << STR(":\n\tversion: ")
                        << static_cast<int>(std::to_underlying(header.version.major))
                        << STR(".")
                        << static_cast<int>(std::to_underlying(header.version.minor))
                        << std::endl;
            }
            return error_count;
        }
    };

    // -- Static command objects give the constexpr registry stable addresses without dynamic setup.
    constexpr VersionCommand version_cmd { };
    constexpr ifc::tool::ArchiveCommand archive_cmd { };
    constexpr ifc::tool::ExtractCommand extract_cmd { };

    // -- List of all builtin subcommands, sorted by their name.
    constexpr const ifc::tool::Extension* builtin_extensions[] {
        &archive_cmd,
        &extract_cmd,
        &version_cmd,
    };
    static_assert(std::ranges::is_sorted(builtin_extensions, { },
                                         [](const ifc::tool::Extension* ext) { return ext->name(); }));

    // -- Binary lookup relies on the compile-time ordering assertion above.
    const ifc::tool::Extension* builtin_operation(const ifc::tool::Name& cmd)
    {
        auto ext = std::ranges::lower_bound(builtin_extensions, cmd, { }, &ifc::tool::Extension::name);
        if (ext < std::end(builtin_extensions) and (*ext)->name() == cmd)
            return *ext;
        return nullptr;
    }

#ifdef _WIN32
    // -- Quote `arg` for a Windows command line so that CommandLineToArgvW recovers it exactly.
    ifc::tool::String quote_windows(ifc::tool::StringView arg)
    {
        if (not arg.empty() and arg.find_first_of(STR(" \t\n\v\"")) == ifc::tool::StringView::npos)
            return ifc::tool::String { arg };
        ifc::tool::String result;
        result.push_back(L'"');
        for (auto it = arg.begin();; ++it)
        {
            std::size_t backslashes = 0;
            while (it != arg.end() and *it == L'\\')
            {
                ++it;
                ++backslashes;
            }
            if (it == arg.end())
            {
                result.append(backslashes * 2, L'\\');
                break;
            }
            if (*it == L'"')
            {
                result.append(backslashes * 2 + 1, L'\\');
                result.push_back(L'"');
            }
            else
            {
                result.append(backslashes, L'\\');
                result.push_back(*it);
            }
        }
        result.push_back(L'"');
        return result;
    }

    // -- Find `program` with a ".exe" extension in the directories listed in `search_path` (a
    // -- semicolon-separated list; a single directory is fine).  Return the full path, or an empty
    // -- string if it was not found there.
    ifc::tool::String find_in_directories(const wchar_t* search_path, const ifc::tool::String& program)
    {
        std::wstring buffer(32 * 1024, L'\0');
        const DWORD n = SearchPathW(search_path, program.c_str(), L".exe",
                                    static_cast<DWORD>(buffer.size()), buffer.data(), nullptr);
        if (n == 0 or n >= buffer.size())
            return {};
        buffer.resize(n);
        return ifc::tool::String { buffer };
    }

    // -- Resolve the external extension `program` (e.g. "ifc-foo") to a full executable path,
    // -- searching first alongside this running tool and then the directories on PATH -- but never
    // -- the current directory, so an executable planted in the working directory cannot be run in
    // -- place of a real extension.  Return an empty string if no such extension is found.
    ifc::tool::String resolve_extension(const ifc::tool::String& program)
    {
        std::wstring module_path(32 * 1024, L'\0');
        const DWORD module_length =
            GetModuleFileNameW(nullptr, module_path.data(), static_cast<DWORD>(module_path.size()));
        if (module_length != 0 and module_length < module_path.size())
        {
            module_path.resize(module_length);
            const ifc::fs::path here = ifc::fs::path { module_path }.parent_path();
            ifc::tool::String found = find_in_directories(here.c_str(), program);
            if (not found.empty())
                return found;
        }

        const DWORD needed = GetEnvironmentVariableW(L"PATH", nullptr, 0);
        if (needed != 0)
        {
            std::wstring path_value(needed, L'\0');
            const DWORD written = GetEnvironmentVariableW(L"PATH", path_value.data(), needed);
            path_value.resize(written);
            return find_in_directories(path_value.c_str(), program);
        }
        return {};
    }

    // -- Run the external extension `program` with `args` directly -- no shell, so command-line
    // -- metacharacters in the arguments are inert.  The executable is resolved on a trusted search
    // -- path (never the current directory) and passed explicitly, so CreateProcessW performs no
    // -- search of its own.  Return the process exit code, or -1 if the extension could not be
    // -- started.
    int spawn_extension(const ifc::tool::String& program, const ifc::tool::Arguments& args)
    {
        const ifc::tool::String executable = resolve_extension(program);
        if (executable.empty())
            return -1;

        ifc::tool::String command_line = quote_windows(program);
        for (const auto& arg : args)
        {
            command_line.push_back(L' ');
            command_line += quote_windows(arg);
        }
        STARTUPINFOW startup { };
        startup.cb = sizeof startup;
        PROCESS_INFORMATION process { };
        if (not CreateProcessW(executable.c_str(), command_line.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr,
                               &startup, &process))
            return -1;
        WaitForSingleObject(process.hProcess, INFINITE);
        DWORD code = 0;
        GetExitCodeProcess(process.hProcess, &code);
        CloseHandle(process.hProcess);
        CloseHandle(process.hThread);
        return static_cast<int>(code);
    }
#else
    // -- Run the external extension `program` with `args` directly -- no shell, so command-line
    // -- metacharacters in the arguments are inert.  Return the process exit code, or -1 if the
    // -- extension could not be started.
    int spawn_extension(const ifc::tool::String& program, const ifc::tool::Arguments& args)
    {
        std::vector<std::string> storage;
        storage.reserve(args.size());
        for (const auto& arg : args)
            storage.emplace_back(arg);
        std::vector<char*> argv;
        argv.reserve(storage.size() + 2);
        argv.push_back(const_cast<char*>(program.c_str()));
        for (std::string& s : storage)
            argv.push_back(s.data());
        argv.push_back(nullptr);

        const pid_t pid = fork();
        if (pid < 0)
            return -1;
        if (pid == 0)
        {
            execvp(program.c_str(), argv.data());
            _exit(127); // exec failed, e.g. the extension was not found on PATH
        }
        int status = 0;
        if (waitpid(pid, &status, 0) < 0)
            return -1;
        if (not WIFEXITED(status) or WEXITSTATUS(status) == 127)
            return -1;
        return WEXITSTATUS(status);
    }
#endif

    // -- Run a builtin subcommand, turning an out-of-memory or otherwise unexpected exception
    // -- into a diagnostic that names the subcommand, and a non-zero exit.
    int run_builtin(const ifc::tool::Extension& op, const ifc::tool::Name& cmd, const ifc::tool::Arguments& args)
    try
    {
        return op.run_with(args);
    }
    catch (const std::bad_alloc&)
    {
        IFC_ERR << STR("ifc ") << cmd << STR(": out of memory") << std::endl;
        return 1;
    }
    catch (...)
    {
        IFC_ERR << STR("ifc ") << cmd << STR(": unexpected internal error") << std::endl;
        return 1;
    }
}


// -- Centralizes final exception containment after builtin or external command dispatch.
int IFC_MAIN(int argc, ifc::tool::NativeChar* argv[])
try
{
    // The `ifc` tool itself does not accept any option.
    int idx = 1;
    while (idx < argc)
    {
        ifc::tool::StringView s { argv[idx] };
        if (not s.starts_with(STR("-")))
            break;
        ++idx;
    }

    if (argc < 2 or idx > 1 or idx >= argc)
    {
        print_usage(argv[0]);
        return 1;
    }

    // The subcommand name is next, after possibly invalid options.
    // It shall not contain any path separator character.
    ifc::tool::Name cmd { argv[idx] };
    if (cmd.contains(STR("/")[0]) or cmd.contains(STR("\\")[0]))
    {
        IFC_ERR << STR("ifc subcommand cannot contain pathname separator") << std::endl;
        return 1;
    }

    ifc::tool::Arguments args { argv + idx + 1, argv + argc };

    if (auto op = builtin_operation(cmd))
        return run_builtin(*op, cmd, args);

    // Otherwise dispatch to an external `ifc-<cmd>` extension found on PATH.
    ifc::tool::String tool = STR("ifc-");
    tool += cmd;
    const int status = spawn_extension(tool, args);
    if (status < 0)
    {
        IFC_ERR << STR("ifc: no subcommand named '") << cmd << STR("'") << std::endl;
        return 1;
    }
    return status;
}
catch (const std::bad_alloc&)
{
    IFC_ERR << STR("ifc: out of memory") << std::endl;
    return 1;
}
catch (...)
{
    IFC_ERR << STR("ifc: unexpected internal error") << std::endl;
    return 1;
}
