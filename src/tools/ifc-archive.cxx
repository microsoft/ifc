// Copyright Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <array>
#include <cerrno>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <random>
#include <set>
#include <string>
#include <system_error>
#include <utility>
#include <vector>
#include <algorithm>
#include <iostream>

#ifdef _WIN32
#   include <windows.h>
#else
#   include <fcntl.h>
#   include <sys/stat.h>
#   include <unistd.h>
#endif

#include "ifc/file.hxx"
#include "ifc/tooling.hxx"
#include "tool-support.hxx"
#include "ifc-archive.hxx"

namespace {
    // Bring the shared subcommand helpers (string_at, read_ifc_header, to_utf8, valid_ifc_image,
    // ...) into scope so the archive internals below can name them directly.
    using namespace ifc::tool;

    // -- Round `offset` up to the next multiple of `alignment`, which must be a power of two.
    inline std::uint64_t align_up(std::uint64_t offset, std::uint64_t alignment)
    {
        return (offset + (alignment - 1)) & ~(alignment - 1);
    }

    // -- The IFC on-disk scalar types have at most 4-byte alignment.  Placing each embedded
    // -- IFC file and the table of contents on a 4-byte boundary keeps them readable in place,
    // -- without copying, on targets that forbid unaligned access.
    constexpr std::uint32_t ifc_alignment = 4;

    // -- The archive table-of-contents entry is the SDK's ArchiveMember record (ifc/file.hxx).
    using ifc::ArchiveMember;

    // -- One representation of the format's canonical-name-first, UnitSort-second member key.
    struct ArchiveMemberKey {
        std::u8string_view canonical; // Canonical name bytes in the relevant string table.
        ifc::UnitSort sort; // Distinguishes translation units that legitimately share a name.

        // -- Default member order is the archive's required ordering.
        std::strong_ordering operator<=>(const ArchiveMemberKey&) const = default;
    };

    // -- A staged archive member while the archive is being written.
    struct ArchiveEntry {
        std::u8string canonical; // Canonical name; with `sort`, the member's identity for sort/dedup.
        ifc::UnitSort sort { }; // The member's translation-unit sort; part of its identity.
        StringTableBuilder::Handle name_handle { }; // Canonical name, in the archive string table.
        StringTableBuilder::Handle path_handle { }; // Filepath, in the archive string table.
        ifc::ByteOffset offset { }; // Member IFC location within the archive.
        ifc::EntitySize size { }; // Member IFC size in bytes.

        // -- Writer sorting uses the same key abstraction as reader validation.
        ArchiveMemberKey key() const
        {
            return { canonical, sort };
        }
    };

    // -- Read the canonical name (and unit sort) recorded in the member IFC mapped at `bytes`.
    // -- Return false if the member records no canonical name (its unit index is null) or the
    // -- header is malformed.  `bytes` must already satisfy valid_ifc_image.
    bool member_canonical_name(ifc::tool::InputFile::View bytes, ifc::UnitSort& sort, std::u8string& name)
    {
        const ifc::Header& header = ifc_header(bytes);
        sort = header.unit.sort();
        const std::uint64_t name_offset = std::to_underlying(header.unit.index());
        if (name_offset == 0) // A null TextOffset: the member records no canonical name.
            return false;
        const std::uint64_t table_offset = std::to_underlying(header.string_table_bytes);
        const std::uint64_t table_size = std::to_underlying(header.string_table_size);
        if (table_offset + table_size > bytes.size())
            return false;
        const std::u8string_view table { reinterpret_cast<const char8_t*>(bytes.data() + table_offset),
                                         static_cast<std::size_t>(table_size) };
        const std::u8string_view canonical = string_at(table, static_cast<std::uint32_t>(name_offset));
        if (canonical.empty())
            return false;
        name.assign(canonical);
        return true;
    }

    // -- The largest byte offset representable in the 32-bit IFC on-disk fields.
    constexpr std::uint64_t archive_offset_limit = std::numeric_limits<std::uint32_t>::max();

    // -- Every archive byte must be addressable by ByteOffset; this check precedes hash processing.
    inline constexpr bool archive_size_fits(std::uint64_t size)
    {
        return size <= archive_offset_limit;
    }
    static_assert(archive_size_fits(archive_offset_limit));
    static_assert(not archive_size_fits(archive_offset_limit + 1));

    // -- A member's canonical name is addressed by the index field of a UnitIndex, which is only
    // -- index_precision<UnitSort> bits wide -- narrower than a full TextOffset.  An offset beyond
    // -- this range would be silently truncated by the UnitIndex constructor, so the archive writer
    // -- rejects it rather than mis-key a member.
    constexpr unsigned unit_name_offset_bits = index_like::index_precision<ifc::UnitSort>;
    constexpr std::uint64_t max_unit_name_offset = (std::uint64_t { 1 } << unit_name_offset_bits) - 1;

    // -- Return true if `offset` fits in the index field of a member's UnitIndex name.
    inline bool name_offset_fits(ifc::TextOffset offset)
    {
        return std::to_underlying(offset) <= max_unit_name_offset;
    }

    // -- Distinguishes harmless name collisions (which may be retried) from creation failures.
    enum class CreateFileResult {
        Ok,     // The file was created and opened.
        Exists, // A filesystem entry already has the candidate name.
        Error,  // Creation failed for another reason.
    };

    // -- Carries a just-created native resource until NativeFile assumes unique ownership.
    struct FileCreation {
#ifdef _WIN32
        HANDLE handle; // Valid only when result is Ok.
#else
        int descriptor; // Valid only when result is Ok.
#endif
        CreateFileResult result; // Allows candidate collisions to be retried without masking hard failures.
    };

    // -- CREATE_NEW/O_EXCL makes the filesystem, rather than randomness, arbitrate name ownership.
    inline FileCreation create_native_file(const ifc::fs::path& path)
    {
#ifdef _WIN32
        const HANDLE handle = CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                                          FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
        if (handle != INVALID_HANDLE_VALUE)
            return { handle, CreateFileResult::Ok };
        const DWORD error = GetLastError();
        return { INVALID_HANDLE_VALUE,
                 error == ERROR_FILE_EXISTS or error == ERROR_ALREADY_EXISTS
                     ? CreateFileResult::Exists : CreateFileResult::Error };
#else
        const int descriptor = ::open(path.c_str(), O_RDWR | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600);
        if (descriptor >= 0)
            return { descriptor, CreateFileResult::Ok };
        return { -1, errno == EEXIST ? CreateFileResult::Exists : CreateFileResult::Error };
#endif
    }

#ifndef _WIN32
    // -- Directory-relative creation preserves containment after the path components were checked.
    inline FileCreation create_native_file_at(int directory, const ifc::fs::path& name)
    {
        const int descriptor = ::openat(directory, name.c_str(),
                                        O_RDWR | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600);
        if (descriptor >= 0)
            return { descriptor, CreateFileResult::Ok };
        return { -1, errno == EEXIST ? CreateFileResult::Exists : CreateFileResult::Error };
    }
#endif

    // -- Native I/O failures unwind through RAII; the surrounding operation supplies diagnostic context.
    struct NativeFileError {
    };

    // -- One move-only owner for CloseHandle/close resources; file and directory code differ in
    // -- operations, not in lifetime semantics.
    struct NativeResource {
#ifdef _WIN32
        // -- Ownership starts only with a valid Windows handle.
        explicit NativeResource(HANDLE value) : handle { value }
        {
        }
#else
        // -- Ownership starts only with a valid POSIX descriptor.
        explicit NativeResource(int value) : descriptor { value }
        {
        }
#endif

        // -- Moving transfers the sole close obligation.
        NativeResource(NativeResource&& source) noexcept
#ifdef _WIN32
            : handle { source.handle }
#else
            : descriptor { source.descriptor }
#endif
        {
#ifdef _WIN32
            source.handle = INVALID_HANDLE_VALUE;
#else
            source.descriptor = -1;
#endif
        }

        // -- Traversal advances ownership without leaking the preceding directory resource.
        NativeResource& operator=(NativeResource&& source) noexcept
        {
            if (this != &source)
            {
                close();
#ifdef _WIN32
                handle = source.handle;
                source.handle = INVALID_HANDLE_VALUE;
#else
                descriptor = source.descriptor;
                source.descriptor = -1;
#endif
            }
            return *this;
        }

        // -- Constructor unwinding and ordinary scope exit share one cleanup path.
        ~NativeResource()
        {
            close();
        }

#ifdef _WIN32
        // -- Native operations need the retained Windows handle without sharing ownership.
        HANDLE get() const
        {
            return handle;
        }
#else
        // -- Native operations need the retained POSIX descriptor without sharing ownership.
        int get() const
        {
            return descriptor;
        }
#endif

        // -- Idempotence supports explicit pre-publication close and destructor cleanup.
        void close()
        {
#ifdef _WIN32
            if (handle != INVALID_HANDLE_VALUE)
            {
                CloseHandle(handle);
                handle = INVALID_HANDLE_VALUE;
            }
#else
            if (descriptor >= 0)
            {
                ::close(descriptor);
                descriptor = -1;
            }
#endif
        }

    private:
#ifdef _WIN32
        HANDLE handle; // INVALID_HANDLE_VALUE denotes the moved-from or closed state.
#else
        int descriptor; // -1 denotes the moved-from or closed state.
#endif
    };

    // -- Retains one native file from exclusive creation through flush, closing the path-reopen
    // -- races inherent in using fstreams for temporary output.
    struct NativeFile {
    #ifdef _WIN32
        // -- Ownership begins only after CREATE_NEW returned a valid handle.
        explicit NativeFile(HANDLE value) : resource { value }
        {
        }
    #else
        // -- Ownership begins only after O_EXCL returned a valid descriptor.
        explicit NativeFile(int value) : resource { value }
        {
        }
    #endif

        // -- Transfer permits complete temporary-file values to be returned without duplicating ownership.
        NativeFile(NativeFile&&) noexcept = default;

        // -- Native writes may be partial or interrupted; callers need an all-or-failure operation.
        void write(gsl::span<const std::byte> bytes)
        {
            const std::byte* first = bytes.data();
            std::size_t remaining = bytes.size();
            while (remaining != 0)
            {
#ifdef _WIN32
                const DWORD count = static_cast<DWORD>(std::min<std::size_t>(remaining,
                                                                             std::numeric_limits<DWORD>::max()));
                DWORD written = 0;
                if (not WriteFile(resource.get(), first, count, &written, nullptr) or written == 0)
                    throw NativeFileError { };
#else
                const ssize_t written = ::write(resource.get(), first, remaining);
                if (written < 0 and errno == EINTR)
                    continue;
                if (written <= 0)
                    throw NativeFileError { };
#endif
                first += written;
                remaining -= written;
            }
        }

        // -- Zero denotes EOF; native failures throw, so callers cannot confuse the two.
        std::size_t read(gsl::span<std::byte> bytes)
        {
#ifdef _WIN32
            const DWORD count = static_cast<DWORD>(std::min<std::size_t>(bytes.size(),
                                                                         std::numeric_limits<DWORD>::max()));
            DWORD received = 0;
            if (not ReadFile(resource.get(), bytes.data(), count, &received, nullptr))
                throw NativeFileError { };
            return received;
#else
            for (;;)
            {
                const ssize_t received = ::read(resource.get(), bytes.data(), bytes.size());
                if (received < 0 and errno == EINTR)
                    continue;
                if (received < 0)
                    throw NativeFileError { };
                return static_cast<std::size_t>(received);
            }
#endif
        }

        // -- The archive header and digest are patched only after body offsets and the hash are known.
        void seek(std::uint64_t offset)
        {
#ifdef _WIN32
            LARGE_INTEGER position;
            position.QuadPart = static_cast<LONGLONG>(offset);
            if (not SetFilePointerEx(resource.get(), position, nullptr, FILE_BEGIN))
                throw NativeFileError { };
#else
            if (::lseek(resource.get(), static_cast<off_t>(offset), SEEK_SET) < 0)
                throw NativeFileError { };
#endif
        }

        // -- A temporary file is never published before its completed contents reach the OS.
        void flush()
        {
#ifdef _WIN32
            if (not FlushFileBuffers(resource.get()))
                throw NativeFileError { };
#else
            if (::fsync(resource.get()) != 0)
                throw NativeFileError { };
#endif
        }

        // -- Idempotence keeps early-return cleanup and pre-commit close on the same path.
        void close()
        {
            resource.close();
        }

    private:
        NativeResource resource; // Centralizes move and close semantics shared with directories.
    };

    // -- Signals failure to establish the user-selected extraction root.
    struct OutputDirectoryError {
        ifc::fs::path path; // Requested root supplies the command diagnostic.
    };

    // -- Signals a link, non-directory, or native failure in an archive-controlled descendant.
    struct UnsafeOutputDirectoryError {
        ifc::fs::path path; // Relative member path identifies the rejected traversal.
    };

    // -- A directory reached without traversing archive-controlled links.  Its native handle is
    // -- retained while a member is written so checked path components cannot be replaced.
    struct SecureDirectory {
        // -- Directory pins have unique ownership because they enforce a live containment boundary.
        SecureDirectory(const SecureDirectory&) = delete;
        SecureDirectory& operator=(const SecureDirectory&) = delete;

        // -- The user-selected root is resolved once; only archive-controlled descendants reject links.
        explicit SecureDirectory(const ifc::fs::path& root)
        {
            std::error_code ec;
            ifc::fs::create_directories(root, ec);
            if (ec)
                throw OutputDirectoryError { root };
#ifdef _WIN32
            const HANDLE handle = CreateFileW(root.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ, nullptr,
                                              OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
            if (handle == INVALID_HANDLE_VALUE)
                throw OutputDirectoryError { root };
            NativeResource root_handle { handle };
            BY_HANDLE_FILE_INFORMATION info { };
            if (not GetFileInformationByHandle(handle, &info)
                or not (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                throw OutputDirectoryError { root };
            const DWORD required = GetFinalPathNameByHandleW(handle, nullptr, 0,
                                                              FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
            if (required == 0)
                throw OutputDirectoryError { root };
            std::wstring final_path(required, L'\0');
            const DWORD written = GetFinalPathNameByHandleW(handle, final_path.data(), required,
                                                             FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
            if (written == 0 or written >= required)
                throw OutputDirectoryError { root };
            final_path.resize(written);
            path = ifc::fs::path { final_path };
            handles.push_back(std::move(root_handle));
#else
            path = ifc::fs::canonical(root, ec);
            if (ec)
                throw OutputDirectoryError { root };
            const int opened = ::open(path.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
            if (opened < 0)
                throw OutputDirectoryError { root };
            handles.emplace_back(opened);
#endif
        }

        // -- Retained-handle traversal prevents a checked descendant from becoming a link before commit.
        SecureDirectory(const SecureDirectory& root, const ifc::fs::path& relative) : path { root.path }
        {
#ifdef _WIN32
            HANDLE duplicate = INVALID_HANDLE_VALUE;
            if (not DuplicateHandle(GetCurrentProcess(), root.handles.back().get(), GetCurrentProcess(), &duplicate,
                                    0, FALSE, DUPLICATE_SAME_ACCESS))
                throw UnsafeOutputDirectoryError { relative };
            handles.emplace_back(duplicate);
            for (const ifc::fs::path& component : relative)
            {
                const ifc::fs::path candidate = path / component;
                if (not CreateDirectoryW(candidate.c_str(), nullptr) and GetLastError() != ERROR_ALREADY_EXISTS)
                    throw UnsafeOutputDirectoryError { relative };
                const HANDLE handle = CreateFileW(candidate.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ, nullptr,
                                                  OPEN_EXISTING,
                                                  FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
                if (handle == INVALID_HANDLE_VALUE)
                    throw UnsafeOutputDirectoryError { relative };
                NativeResource child { handle };
                BY_HANDLE_FILE_INFORMATION info { };
                if (not GetFileInformationByHandle(handle, &info)
                    or not (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    or (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
                    throw UnsafeOutputDirectoryError { relative };
                handles.push_back(std::move(child));
                path = candidate;
            }
#else
            const int duplicate = ::fcntl(root.handles.back().get(), F_DUPFD_CLOEXEC, 0);
            if (duplicate < 0)
                throw UnsafeOutputDirectoryError { relative };
            handles.emplace_back(duplicate);
            for (const ifc::fs::path& component : relative)
            {
                const ifc::fs::path candidate = path / component;
                if (::mkdirat(handles.back().get(), component.c_str(), 0700) != 0 and errno != EEXIST)
                    throw UnsafeOutputDirectoryError { relative };
                const int opened = ::openat(handles.back().get(), component.c_str(),
                                            O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
                if (opened < 0)
                    throw UnsafeOutputDirectoryError { relative };
                NativeResource next { opened };
                path = candidate;
                handles.back() = std::move(next);
            }
#endif
        }

        // -- Win32 lacks an openat equivalent, so the pinned handle chain accompanies this resolved path.
        const ifc::fs::path& pathname() const
        {
            return path;
        }

#ifndef _WIN32
        // -- POSIX *at operations keep creation and commit relative to the verified directory inode.
        int native_descriptor() const
        {
            return handles.back().get();
        }
#endif

    private:
        ifc::fs::path path; // Stable Windows spelling and user-facing diagnostic context.
        // Windows retains every pinned component; POSIX advances one verified leaf descriptor.
        std::vector<NativeResource> handles;
    };

    // -- Encodes whether destruction must remove a temporary filesystem entry.
    enum class TemporaryFileState {
        Armed,     // The temporary file exists and must be removed on destruction.
        Committed, // The temporary file was moved to its destination.
    };

    // -- Records whether cleanup must remain relative to a verified directory handle.
    enum class TemporaryFileLocation {
        Path,      // Address the file by its full pathname.
        Directory, // Address the file relative to a retained directory handle.
    };

    // -- Makes overwrite authority explicit at the commit boundary rather than during creation.
    enum class ExistingFile {
        Reject,  // Leave an existing destination unchanged.
        Replace, // Atomically replace an existing destination.
    };

    // -- Header-unit identity includes the C++ header-name delimiter form, which shells cannot
    // -- preserve reliably when it is written literally on a command line.
    enum class HeaderNameForm {
        Quote, // Select the header unit whose canonical name has "..." form.
        Angle, // Select the header unit whose canonical name has <...> form.
    };

    // -- Retains the user's bare spelling for diagnostics and the synthesized IFC key for lookup.
    struct HeaderRequest {
        ifc::tool::StringView spelling; // Undelimited command-line payload.
        std::u8string canonical; // Exact delimited canonical name recorded by the IFC.
        HeaderNameForm form; // Controls delimiter synthesis and form-specific diagnostics.
    };

    // -- Separating delimiter form from the argument avoids shell quote removal and redirection syntax.
    std::u8string canonical_header_name(HeaderNameForm form, ifc::tool::StringView spelling)
    {
        std::u8string canonical = to_utf8(spelling);
        if (canonical.empty() or canonical.find(u8'\n') != std::u8string::npos
            or canonical.find(u8'\r') != std::u8string::npos)
            return { };

        char8_t open = u8'"';
        char8_t close = u8'"';
        if (form == HeaderNameForm::Quote)
        {
            if (canonical.find(u8'"') != std::u8string::npos)
                return { };
        }
        else
        {
            if (canonical.find(u8'>') != std::u8string::npos)
                return { };
            open = u8'<';
            close = u8'>';
        }
        canonical.insert(canonical.begin(), open);
        canonical.push_back(close);
        return canonical;
    }

    // -- Separates the expected no-overwrite collision from actual commit failures.
    enum class CommitFileResult {
        Ok,     // The destination now names the completed file.
        Exists, // Replacement was forbidden and the destination exists.
    };

    // -- Randomness reduces retries; exclusive creation, not the token, provides collision safety.
    ifc::fs::path unique_temp_path(const ifc::fs::path& base)
    {
        std::random_device rng;
        const std::uint64_t token = (static_cast<std::uint64_t>(rng()) << 32) ^ rng();
        ifc::fs::path temp = base;
        temp += STR(".");
        temp += std::to_string(token);
        temp += STR(".tmp");
        return temp;
    }

    // -- Identifies failure to establish an exclusively-created temporary-file invariant.
    struct TemporaryFileCreateError {
        ifc::fs::path path; // Requested destination supplies diagnostic context.
    };

    // -- A successful exclusive creation paired with the candidate name that won the race.
    struct TemporaryFileCreation {
        NativeFile file; // Already owns the exclusively-created native file.
        ifc::fs::path name; // Candidate spelling used for cleanup and publication.
    };

    // -- Candidate collisions are expected; every other creation failure terminates the retry loop.
    template<typename Create>
    TemporaryFileCreation create_unique_temporary_file(const ifc::fs::path& base,
                                                        const ifc::fs::path& diagnostic, Create create)
    {
        for (unsigned attempt = 0; attempt != 128; ++attempt)
        {
            const ifc::fs::path candidate = unique_temp_path(base);
            const FileCreation created = create(candidate);
            if (created.result == CreateFileResult::Ok)
            {
#ifdef _WIN32
                return { NativeFile { created.handle }, candidate };
#else
                return { NativeFile { created.descriptor }, candidate };
#endif
            }
            if (created.result != CreateFileResult::Exists)
                break;
        }
        throw TemporaryFileCreateError { diagnostic };
    }

    // -- Unexpected publication failures unwind while TemporaryFile still owns cleanup responsibility.
    struct TemporaryFileCommitError {
        ifc::fs::path path; // Destination supplies diagnostic context at the command boundary.
    };

    // -- Complete construction state assembled before TemporaryFile assumes cleanup responsibility.
    struct TemporaryFileData {
        NativeFile file; // Already owns a successfully-created native file.
        ifc::fs::path path; // Full pathname required by Win32 and diagnostics.
        ifc::fs::path name; // POSIX name relative to the verified parent.
        TemporaryFileLocation location; // Determines race-free cleanup addressing.
#ifndef _WIN32
        int directory_descriptor; // Non-owning; the source SecureDirectory outlives TemporaryFile.
#endif
    };

    // -- Archive output uses a sibling so the final rename stays on one filesystem.
    TemporaryFileData create_temporary_file(const ifc::fs::path& base)
    {
        TemporaryFileCreation created = create_unique_temporary_file(base, base,
            [](const ifc::fs::path& candidate) { return create_native_file(candidate); });
#ifdef _WIN32
        return { std::move(created.file), std::move(created.name), { }, TemporaryFileLocation::Path };
#else
        return { std::move(created.file), std::move(created.name), { }, TemporaryFileLocation::Path, -1 };
#endif
    }

    // -- Extraction creates relative to the verified parent so pathname races cannot escape it.
    TemporaryFileData create_temporary_file(const SecureDirectory& directory, const ifc::fs::path& base)
    {
        TemporaryFileCreation created = create_unique_temporary_file(base, directory.pathname() / base,
            [&directory](const ifc::fs::path& candidate)
            {
#ifdef _WIN32
                return create_native_file(directory.pathname() / candidate);
#else
                return create_native_file_at(directory.native_descriptor(), candidate);
#endif
            });
#ifdef _WIN32
        return { std::move(created.file), directory.pathname() / created.name, std::move(created.name),
                 TemporaryFileLocation::Directory };
#else
        return { std::move(created.file), directory.pathname() / created.name, std::move(created.name),
                 TemporaryFileLocation::Directory, directory.native_descriptor() };
#endif
    }

    // -- Couples an exclusively-created native file with fail-safe cleanup and atomic publication.
    struct TemporaryFile {
        // -- Construction either establishes exclusive sibling output or throws without an artifact.
        explicit TemporaryFile(const ifc::fs::path& base) : TemporaryFile { create_temporary_file(base) }
        {
        }

        // -- Construction remains relative to the verified parent for the object's entire lifetime.
        TemporaryFile(const SecureDirectory& directory, const ifc::fs::path& base)
            : TemporaryFile { create_temporary_file(directory, base) }
        {
        }

        // -- Every failed write or commit leaves no partial output artifact.
        ~TemporaryFile()
        {
            file.close();
            if (state == TemporaryFileState::Armed)
            {
#ifndef _WIN32
                if (location == TemporaryFileLocation::Directory)
                {
                    ::unlinkat(directory_descriptor, name.c_str(), 0);
                    return;
                }
#endif
                std::error_code ec;
                ifc::fs::remove(path, ec);
            }
        }

        // -- Archive publication occurs only after flush and uses one atomic filesystem operation.
        CommitFileResult commit_to(const ifc::fs::path& destination, ExistingFile existing)
        {
            prepare_commit();
#ifdef _WIN32
            return commit_path(destination, existing);
#else
            if (existing == ExistingFile::Replace)
            {
                if (::rename(path.c_str(), destination.c_str()) != 0)
                    throw TemporaryFileCommitError { destination };
            }
            else
            {
                if (::link(path.c_str(), destination.c_str()) != 0)
                {
                    if (errno == EEXIST)
                        return CommitFileResult::Exists;
                    throw TemporaryFileCommitError { destination };
                }
                if (::unlink(path.c_str()) != 0)
                    throw TemporaryFileCommitError { destination };
            }
#endif
            state = TemporaryFileState::Committed;
            return CommitFileResult::Ok;
        }

        // -- Extraction preserves containment through commit and enforces overwrite authority atomically.
        CommitFileResult commit_in(const SecureDirectory& directory, const ifc::fs::path& destination,
                                   ExistingFile existing)
        {
            prepare_commit();
#ifdef _WIN32
            return commit_path(directory.pathname() / destination, existing);
#else
            if (existing == ExistingFile::Replace)
            {
                if (::renameat(directory.native_descriptor(), name.c_str(), directory.native_descriptor(),
                               destination.c_str()) != 0)
                    throw TemporaryFileCommitError { directory.pathname() / destination };
                state = TemporaryFileState::Committed;
            }
            else
            {
                if (::linkat(directory.native_descriptor(), name.c_str(), directory.native_descriptor(),
                             destination.c_str(), 0) != 0)
                {
                    if (errno == EEXIST)
                        return CommitFileResult::Exists;
                    throw TemporaryFileCommitError { directory.pathname() / destination };
                }
                // The destination is committed.  Leave the temp armed so its destructor removes
                // the now-redundant link through this retained directory handle.
            }
#endif
            return CommitFileResult::Ok;
        }

        // -- Writers receive native I/O without gaining access to cleanup state.
        NativeFile& output()
        {
            return file;
        }

        // -- Diagnostics may name the temp file without gaining mutation authority.
        const ifc::fs::path& pathname() const
        {
            return path;
        }

    private:
        // -- Publication starts only after completed bytes are flushed and the writer releases its handle.
        void prepare_commit()
        {
            file.flush();
            file.close();
        }

#ifdef _WIN32
        // -- Both archive and extraction publication use the same Windows replacement policy.
        CommitFileResult commit_path(const ifc::fs::path& destination, ExistingFile existing)
        {
            const DWORD flags = MOVEFILE_WRITE_THROUGH
                | (existing == ExistingFile::Replace ? MOVEFILE_REPLACE_EXISTING : 0);
            if (not MoveFileExW(path.c_str(), destination.c_str(), flags))
            {
                const DWORD error = GetLastError();
                if (error == ERROR_FILE_EXISTS or error == ERROR_ALREADY_EXISTS)
                    return CommitFileResult::Exists;
                throw TemporaryFileCommitError { destination };
            }
            state = TemporaryFileState::Committed;
            return CommitFileResult::Ok;
        }
#endif

        // -- Assume cleanup responsibility only after every construction field is available.
        explicit TemporaryFile(TemporaryFileData&& data)
            : file { std::move(data.file) }, path { std::move(data.path) }, name { std::move(data.name) },
              location { data.location }
#ifndef _WIN32
            , directory_descriptor { data.directory_descriptor }
#endif
        {
        }

        NativeFile file; // Prevents replacement between exclusive creation and completed write.
        ifc::fs::path path; // Required by Win32 commit APIs and useful in diagnostics.
        ifc::fs::path name; // Keeps POSIX cleanup and commit relative to the verified parent.
        TemporaryFileState state = TemporaryFileState::Armed; // Construction always creates a cleanup obligation.
        TemporaryFileLocation location; // Selects race-free cleanup addressing.
#ifndef _WIN32
        int directory_descriptor = -1; // Non-owning descriptor; SecureDirectory outlives this file.
#endif
    };

    // -- A forward-only cursor over the archive output stream that tracks the running write
    // -- position, so member offsets are discovered as content is streamed out.
    struct WriteCursor {
        NativeFile& out; // Retained file whose pathname cannot be substituted mid-write.
        std::uint64_t position = 0; // 64-bit accounting precedes every narrowing to IFC offsets.

        // -- Native errors unwind immediately; successful writes advance exact layout accounting.
        void write(const std::byte* data, std::uint64_t n)
        {
            out.write({ data, static_cast<std::size_t>(n) });
            position += n;
        }
        // -- Zero filling makes alignment gaps deterministic because the archive hash covers them.
        void pad_to(std::uint64_t target)
        {
            static constexpr std::array<std::byte, ifc_alignment> zero { };
            while (position < target)
                write(zero.data(), std::min<std::uint64_t>(target - position, zero.size()));
        }
        // -- Centralized alignment keeps every in-place IFC structure safely readable.
        void pad_to_alignment(std::uint64_t alignment)
        {
            pad_to(align_up(position, alignment));
        }

    };

    // -- Identifies a member within an archive: its canonical name together with its unit sort.
    // -- A named module and a header unit can share a name, so the sort is part of the identity.
    struct InternedMemberIdentity {
        StringTableBuilder::Handle name; // Canonical name, in the archive string table.
        ifc::UnitSort sort; // The member's translation-unit sort.

        // -- Any stable order suffices here; the set enforces uniqueness before offsets are resolved.
        std::strong_ordering operator<=>(const InternedMemberIdentity&) const = default;
    };

    // -- Records whether the first member has established the archive architecture.
    enum class ArchitectureState {
        Unset, // No member has contributed an architecture.
        Set,   // At least one member has contributed an architecture.
    };

    // -- The accumulating state of an archive under construction: the string table being built,
    // -- the staged table of contents, the member identities already seen
    // -- (for uniqueness), and the common target architecture (Unknown once members disagree).
    struct ArchiveContents {
        StringTableBuilder strings; // Interned member names, filepaths, and the archive name.
        std::vector<ArchiveEntry> entries; // Staged members, sorted by canonical name then UnitSort.
        std::set<InternedMemberIdentity> used_names; // Interned identities already taken, for uniqueness.
        std::set<StringTableBuilder::Handle> used_paths; // Normalized extraction paths already taken.
        ifc::Architecture arch = ifc::Architecture::Unknown; // Common member architecture, or Unknown if they differ.
        ArchitectureState arch_state = ArchitectureState::Unset; // Controls first-member initialization.
    };

    // -- Mapping one input at a time bounds address-space use; identity, extraction path, and IFC
    // -- limits are validated before the member's verbatim bytes enter the archive.
    bool stage_member(ifc::tool::StringView input, WriteCursor& cursor, ArchiveContents& contents)
    {
        ifc::fs::path path { input };
        try
        {
            ifc::tool::InputFile file { path.native() };
            const ifc::tool::InputFile::View bytes = file.contents();

            if (not valid_ifc_image(bytes))
            {
                IFC_ERR << path.native() << STR(" is not an IFC file") << std::endl;
                return false;
            }

            const ifc::Header& header = ifc_header(bytes);
            if (header.unit.sort() == ifc::UnitSort::Archive)
            {
                IFC_ERR << path.native() << STR(" is itself an archive; archives cannot be nested") << std::endl;
                return false;
            }

            ifc::UnitSort sort { };
            std::u8string canonical;
            if (not member_canonical_name(bytes, sort, canonical))
            {
                IFC_ERR << path.native() << STR(": IFC file has no canonical name and cannot be archived")
                        << std::endl;
                return false;
            }
            // A member is identified by its canonical name together with its unit sort -- a named
            // module and a header unit can share a name -- so that pair must be unique in the archive.
            // Interning dedups by bytes, so an already-seen name yields an already-seen handle.
            const StringTableBuilder::Handle name_handle = contents.strings.intern(canonical);
            if (not contents.used_names.insert({ name_handle, sort }).second)
            {
                IFC_ERR << path.native()
                        << STR(": another member already has this canonical name and unit sort") << std::endl;
                return false;
            }

            const ifc::fs::path stored_path = safe_relative(path.generic_u8string());
            if (stored_path.empty())
            {
                IFC_ERR << path.native() << STR(": filepath cannot be represented safely in an archive") << std::endl;
                return false;
            }
            const StringTableBuilder::Handle path_handle = contents.strings.intern(stored_path.generic_u8string());
            if (not contents.used_paths.insert(path_handle).second)
            {
                IFC_ERR << path.native() << STR(": another member extracts to the same filepath") << std::endl;
                return false;
            }

            const std::uint64_t offset = align_up(cursor.position, ifc_alignment);
            if (not archive_size_fits(offset + bytes.size()))
            {
                IFC_ERR << path.native() << STR(": archive would exceed the 32-bit IFC offset limit") << std::endl;
                return false;
            }

            ArchiveEntry entry {
                .canonical = std::move(canonical),
                .sort = sort,
                .name_handle = name_handle,
                .path_handle = path_handle,
                .offset = ifc::ByteOffset { static_cast<std::uint32_t>(offset) },
                .size = ifc::EntitySize { static_cast<std::uint32_t>(bytes.size()) },
            };

            cursor.pad_to(offset);
            cursor.write(bytes.data(), bytes.size());
            contents.entries.push_back(std::move(entry));

            if (contents.arch_state == ArchitectureState::Unset)
            {
                contents.arch = header.arch;
                contents.arch_state = ArchitectureState::Set;
            }
            else if (header.arch != contents.arch)
            {
                contents.arch = ifc::Architecture::Unknown;
            }
            return true;
        }
        catch (const ifc::tool::AccessError&)
        {
            IFC_ERR << path.native() << STR(": couldn't open file") << std::endl;
            return false;
        }
        catch (const ifc::tool::RegularFileError&)
        {
            IFC_ERR << path.native() << STR(": not a regular file") << std::endl;
            return false;
        }
        catch (const ifc::tool::FileMappingError&)
        {
            IFC_ERR << path.native() << STR(": couldn't memory-map file") << std::endl;
            return false;
        }
    }

    // -- Build the (deduplicated, suffix-shared) string table and write it through `cursor`,
    // -- returning the byte offset at which it was written.
    std::uint64_t write_string_table(WriteCursor& cursor, StringTableBuilder& strings)
    {
        strings.build();
        const gsl::span<const std::byte> table = strings.bytes();
        cursor.pad_to_alignment(ifc_alignment);
        const std::uint64_t offset = cursor.position;
        cursor.write(table.data(), table.size());
        return offset;
    }

    // -- Write the table of contents -- one ArchiveMember per staged member, in the given order
    // -- (sorted by canonical name, then unit sort) -- through `cursor`, returning its byte offset.
    std::uint64_t write_table_of_contents(WriteCursor& cursor, const std::vector<ArchiveEntry>& entries,
                                          const StringTableBuilder& strings)
    {
        cursor.pad_to_alignment(ifc_alignment);
        const std::uint64_t offset = cursor.position;
        for (const ArchiveEntry& entry : entries)
        {
            const ArchiveMember member {
                ifc::UnitIndex { strings.offset(entry.name_handle), entry.sort },
                strings.offset(entry.path_handle),
                entry.offset,
                entry.size,
            };
            cursor.write(reinterpret_cast<const std::byte*>(&member), sizeof member);
        }
        return offset;
    }

    // -- Assemble the archive header from the finished contents and the resolved region offsets.
    // -- The content hash is left zero here and patched in later (see finalize_content_hash).
    ifc::Header make_archive_header(const ArchiveContents& contents, StringTableBuilder::Handle archive_name,
                                    std::uint64_t string_table_offset, std::uint64_t toc_offset)
    {
        // The whole struct -- padding included -- is written to the archive and covered by the
        // content hash, so every byte must be deterministic.  Value-initialization would zero the
        // named members but leave padding bytes indeterminate; memset zeroes the padding too.
        ifc::Header header;
        std::memset(&header, 0, sizeof header);
        header.version = ifc::CurrentFormatVersion;
        header.abi = ifc::Abi { };
        header.arch = contents.arch;
        header.cplusplus = ifc::CPlusPlus { };
        header.string_table_bytes = ifc::ByteOffset { static_cast<std::uint32_t>(string_table_offset) };
        header.string_table_size = ifc::Cardinality { static_cast<std::uint32_t>(contents.strings.bytes().size()) };
        header.unit = ifc::UnitIndex { contents.strings.offset(archive_name), ifc::UnitSort::Archive };
        header.src_path = ifc::TextOffset { 0 };
        header.global_scope = ifc::ScopeIndex { 0 };
        header.toc = ifc::ByteOffset { static_cast<std::uint32_t>(toc_offset) };
        header.partition_count = ifc::Cardinality { static_cast<std::uint32_t>(contents.entries.size()) };
        header.internal_partition = false;
        return header;
    }

    // -- Write the body of the archive IFC -- everything but the content hash -- into `out`.  Each
    // -- input IFC is mapped, validated, and streamed straight in (and released) one at a time, so
    // -- the whole archive is never held in memory and its size need not be known in advance.  The
    // -- header, whose fields reference offsets discovered while writing, is filled in last via a
    // -- seek back to the start.  Return true on success.
    bool write_archive_body(NativeFile& out, const ifc::fs::path& temp_path,
                            const std::vector<ifc::tool::StringView>& inputs, ifc::tool::StringView archive_name)
    {
        constexpr std::uint64_t signature_size = sizeof ifc::InterfaceSignature;

        WriteCursor cursor { out };
        // Write the signature and reserve space for the header, which is patched in at the end.
        cursor.write(reinterpret_cast<const std::byte*>(ifc::InterfaceSignature), signature_size);
        cursor.pad_to(signature_size + sizeof(ifc::Header));

        ArchiveContents contents;
        contents.entries.reserve(inputs.size());
        const StringTableBuilder::Handle archive_name_handle = contents.strings.intern(to_utf8(archive_name));

        for (const ifc::tool::StringView& input : inputs)
            if (not stage_member(input, cursor, contents))
                return false;

        std::ranges::sort(contents.entries, { }, &ArchiveEntry::key);

        const std::uint64_t string_table_offset = write_string_table(cursor, contents.strings);

        // A member's canonical-name offset is stored in the index field of its UnitIndex (and the
        // archive's own name in the header's unit index), which is narrower than a full TextOffset.
        // Reject the archive rather than let the UnitIndex constructor silently truncate an offset
        // that doesn't fit and mis-key a member.
        if (not name_offset_fits(contents.strings.offset(archive_name_handle))
            or std::ranges::any_of(contents.entries, [&contents](const ArchiveEntry& entry)
               {
                   return not name_offset_fits(contents.strings.offset(entry.name_handle));
               }))
        {
            IFC_ERR << STR("archive: a canonical-name offset exceeds the ") << unit_name_offset_bits
                    << STR("-bit range addressable by a member entry") << std::endl;
            return false;
        }

        const std::uint64_t toc_offset = write_table_of_contents(cursor, contents.entries, contents.strings);

        if (not archive_size_fits(cursor.position))
        {
            IFC_ERR << STR("archive: total size exceeds the 32-bit IFC offset limit") << std::endl;
            return false;
        }

        const ifc::Header header = make_archive_header(contents, archive_name_handle, string_table_offset, toc_offset);
        out.seek(signature_size);
        out.write({ reinterpret_cast<const std::byte*>(&header), sizeof header });
        out.flush();
        return true;
    }

    // -- Compute the content hash of the finished archive at `path` and patch it into the header,
    // -- through a single read-write handle: the bytes after the hash field are streamed through an
    // -- incremental hasher (never mapping the whole file), then the digest is written back into the
    // -- hash field.  One open, so there is no verify-then-write gap.  Return true on success.
    void finalize_content_hash(NativeFile& file)
    {
        // Read the archive back in bounded chunks so a large file is never held in memory at once.
        constexpr std::size_t hash_chunk_bytes = 64 * 1024;

        file.seek(ifc::hashed_contents_offset);

        ifc::Sha256 hasher;
        std::vector<std::byte> buffer(hash_chunk_bytes);
        for (;;)
        {
            const std::size_t size = file.read(buffer);
            if (size == 0)
                break;
            hasher.update({ buffer.data(), size });
        }
        const ifc::SHA256Hash hash = hasher.finish();

        file.seek(ifc::content_hash_offset);
        file.write({ reinterpret_cast<const std::byte*>(hash.value.data()), sizeof hash.value });
        file.flush();
    }

    // -- Create the archive IFC named `output_path` from the IFC files named by `inputs`.  The
    // -- archive is built in a sibling temporary file and atomically renamed into place, so a
    // -- failure leaves no partial output and the output may even be one of the inputs.  Return
    // -- true on success.
    bool create_archive(const ifc::fs::path& output_path, const std::vector<ifc::tool::StringView>& inputs,
                        ifc::tool::StringView archive_name)
    try
    {
        TemporaryFile temp { output_path };
        if (not write_archive_body(temp.output(), temp.pathname(), inputs, archive_name))
            return false;
        finalize_content_hash(temp.output());
        temp.commit_to(output_path, ExistingFile::Replace);
        return true;
    }
    catch (const TemporaryFileCreateError&)
    {
        IFC_ERR << output_path.native() << STR(": couldn't create archive") << std::endl;
        return false;
    }
    catch (const NativeFileError&)
    {
        IFC_ERR << output_path.native() << STR(": couldn't write archive") << std::endl;
        return false;
    }
    catch (const TemporaryFileCommitError&)
    {
        IFC_ERR << output_path.native() << STR(": couldn't replace the output with the new archive") << std::endl;
        return false;
    }

    // -- Print how to invoke the archive subcommand.
    inline void print_archive_usage()
    {
        IFC_ERR << STR("ifc archive usage:\n")
                << STR("\tifc archive [--name <name>] -o <archive> <ifc-file>...\n")
                << STR("\tifc archive [--name <name>] <archive> <ifc-file>...\n");
    }

    // -- An opened archive IFC: the whole image mapped once (and hash-verified), with the member
    // -- table and string table viewed in place within it.
    struct ArchiveReader {
        ifc::tool::InputFile::View image; // Keeps every validated view tied to the same hashed bytes.
        gsl::span<const ArchiveMember> members; // Formed only after extent and alignment validation.
        std::u8string_view string_table; // Non-owning text remains valid while image stays mapped.

        // -- One decoder enforces the archive string-table bounds and termination policy.
        std::u8string_view text(ifc::TextOffset offset) const
        {
            return string_at(string_table, std::to_underlying(offset));
        }
        // -- Lookup and validation must compare the same canonical-name representation.
        std::u8string_view canonical(const ArchiveMember& member) const
        {
            return text(ifc::TextOffset { std::to_underlying(member.name.index()) });
        }
        // -- Extraction consumes only path bytes validated during open_archive().
        std::u8string_view path(const ArchiveMember& member) const
        {
            return text(member.path);
        }

        // -- Reader ordering and identity validation share the writer's key representation.
        ArchiveMemberKey key(const ArchiveMember& member) const
        {
            return { canonical(member), member.name.sort() };
        }

        // -- Metadata validation establishes these extents before extraction requests a view.
        ifc::tool::InputFile::View member_image(const ArchiveMember& member) const
        {
            return image.subspan(std::to_underlying(member.offset), std::to_underlying(member.size));
        }

        // -- Path normalization is centralized so validation and extraction cannot disagree.
        ifc::fs::path relative_path(const ArchiveMember& member) const
        {
            return safe_relative(path(member));
        }

        // -- The validated ToC ordering makes all equal canonical names one contiguous range.
        auto members_named(std::u8string_view name) const
        {
            return std::ranges::equal_range(members, name, std::ranges::less { },
                [this](const ArchiveMember& member) { return canonical(member); });
        }

        // -- Header selectors require an exact canonical-name and UnitSort match.
        const ArchiveMember* member(const ArchiveMemberKey& wanted) const
        {
            const auto found = std::ranges::lower_bound(members, wanted, { },
                [this](const ArchiveMember& candidate) { return key(candidate); });
            if (found == members.end() or key(*found) != wanted)
                return nullptr;
            return std::addressof(*found);
        }
    };

    // -- Orders archive byte ranges by start so overlap checks and set neighbors share one model.
    struct ArchiveExtent {
        std::uint64_t begin; // First byte occupied by the represented region.
        std::uint64_t end; // One-past-the-last byte occupied by the represented region.

        // -- Start-first ordering makes set neighbors the only possible overlapping ranges.
        std::strong_ordering operator<=>(const ArchiveExtent&) const = default;
    };

    // -- Metadata and member payloads must denote disjoint ranges before views are formed.
    inline bool overlaps(const ArchiveExtent& first, const ArchiveExtent& second)
    {
        return first.begin < second.end and second.begin < first.end;
    }

    // -- Archive recursion and values outside the specified UnitSort domain are never members.
    inline bool valid_member_sort(ifc::UnitSort sort)
    {
        return sort < ifc::UnitSort::Count and sort != ifc::UnitSort::Archive;
    }

    // -- Positional selectors are reserved for C++ named modules; header units carry explicit form.
    inline bool named_module_sort(ifc::UnitSort sort)
    {
        return sort == ifc::UnitSort::Primary or sort == ifc::UnitSort::Partition;
    }

    // -- Verify the mapped archive `bytes` against the content hash stored in `header` -- the
    // -- SHA-256 over the bytes following the hash field, matching how the archive was finalized.
    // -- `label` names the file in diagnostics.  Return true if the hash matches; a mismatch means
    // -- the archive is corrupt or has been tampered with.
    bool verify_content_hash(ifc::tool::InputFile::View bytes, const ifc::Header& header, ifc::tool::StringView label)
    {
        if (bytes.size() < ifc::hashed_contents_offset)
        {
            IFC_ERR << label << STR(" is truncated or corrupted") << std::endl;
            return false;
        }
        const ifc::SHA256Hash computed = ifc::hash_ifc_contents(bytes);
        if (computed.value != header.content_hash.value)
        {
            IFC_ERR << label << STR(": content hash mismatch; the archive is corrupt or has been tampered with")
                    << std::endl;
            return false;
        }
        return true;
    }

    // -- Validate the mapped archive image `bytes`: check the signature and header, verify the
    // -- content hash, and locate the member table and string table within it.  `label` names the
    // -- file in diagnostics.  On success, point `reader` at those regions (all viewing into
    // -- `bytes`) and return true.
    bool open_archive(ifc::tool::InputFile::View bytes, ifc::tool::StringView label, ArchiveReader& reader)
    {
        if (not archive_size_fits(bytes.size()))
        {
            IFC_ERR << label << STR(" exceeds the 32-bit IFC file-size limit") << std::endl;
            return false;
        }
        if (not valid_ifc_image(bytes))
        {
            IFC_ERR << label << STR(" is not an IFC file") << std::endl;
            return false;
        }
        const ifc::Header& header = ifc_header(bytes);
        if (header.unit.sort() != ifc::UnitSort::Archive)
        {
            IFC_ERR << label << STR(" is not an archive IFC file") << std::endl;
            return false;
        }
        // Verify integrity over the very bytes we are about to read, before trusting any offsets.
        if (not verify_content_hash(bytes, header, label))
            return false;
        if (header.version < ifc::MinimumFormatVersion or header.version > ifc::CurrentFormatVersion)
        {
            IFC_ERR << label << STR(" has an unsupported IFC format version") << std::endl;
            return false;
        }

        const std::uint64_t archive_size = bytes.size();
        constexpr std::uint64_t header_end = sizeof ifc::InterfaceSignature + sizeof(ifc::Header);
        const std::uint64_t toc_offset = std::to_underlying(header.toc);
        const std::size_t member_count = std::to_underlying(header.partition_count);
        if (toc_offset % alignof(ArchiveMember) != 0 or toc_offset > archive_size
            or member_count > (archive_size - toc_offset) / sizeof(ArchiveMember))
        {
            IFC_ERR << label << STR(" has a truncated or corrupt table of contents") << std::endl;
            return false;
        }
        const std::uint64_t toc_size = member_count * sizeof(ArchiveMember);
        const std::uint64_t toc_end = toc_offset + toc_size;
        const ArchiveExtent toc_extent { toc_offset, toc_end };
        const std::uint64_t string_offset = std::to_underlying(header.string_table_bytes);
        const std::uint64_t string_size = std::to_underlying(header.string_table_size);
        if (string_offset > archive_size or string_size > archive_size - string_offset)
        {
            IFC_ERR << label << STR(" has a truncated or corrupt string table") << std::endl;
            return false;
        }
        const std::uint64_t string_end = string_offset + string_size;
        const ArchiveExtent string_extent { string_offset, string_end };
        if (toc_offset < header_end or string_offset < header_end
            or overlaps(toc_extent, string_extent))
        {
            IFC_ERR << label << STR(" has overlapping archive metadata") << std::endl;
            return false;
        }

        reader.image = bytes;
        reader.members = { reinterpret_cast<const ArchiveMember*>(bytes.data() + toc_offset), member_count };
        reader.string_table = { reinterpret_cast<const char8_t*>(bytes.data() + string_offset),
                                static_cast<std::size_t>(string_size) };

        if (reader.string_table.empty() or reader.string_table.front() != u8'\0')
        {
            IFC_ERR << label << STR(" has a malformed string table") << std::endl;
            return false;
        }
        const ifc::TextOffset archive_name { std::to_underlying(header.unit.index()) };
        if (not index_like::null(archive_name) and reader.text(archive_name).empty())
        {
            IFC_ERR << label << STR(" has an invalid archive name") << std::endl;
            return false;
        }

        std::set<std::u8string_view> used_paths;
        std::set<ArchiveExtent> member_extents;
        for (std::size_t index = 0; index != reader.members.size(); ++index)
        {
            const ArchiveMember& member = reader.members[index];
            const ArchiveMemberKey key = reader.key(member);
            const std::u8string_view stored_path = reader.path(member);
            if (not valid_member_sort(key.sort) or key.canonical.empty() or stored_path.empty())
            {
                IFC_ERR << label << STR(" has an invalid archive member identity") << std::endl;
                return false;
            }
            if (index != 0 and not (reader.key(reader.members[index - 1]) < key))
            {
                IFC_ERR << label << STR(" has an unsorted or duplicate table of contents") << std::endl;
                return false;
            }

            const ifc::fs::path relative = reader.relative_path(member);
            if (relative.empty() or relative.generic_u8string() != stored_path
                or not used_paths.insert(stored_path).second)
            {
                IFC_ERR << label << STR(" has an invalid or duplicate member filepath") << std::endl;
                return false;
            }

            const std::uint64_t member_offset = std::to_underlying(member.offset);
            const std::uint64_t member_size = std::to_underlying(member.size);
            const ArchiveExtent extent { member_offset, member_offset + member_size };
            if (member_offset % ifc_alignment != 0 or member_offset < header_end
                or member_offset > archive_size or member_size > archive_size - member_offset
                or overlaps(extent, toc_extent) or overlaps(extent, string_extent))
            {
                IFC_ERR << label << STR(" has an invalid archive member extent") << std::endl;
                return false;
            }
            const auto next = member_extents.lower_bound(extent);
            if ((next != member_extents.end() and overlaps(*next, extent))
                or (next != member_extents.begin() and overlaps(*std::prev(next), extent)))
            {
                IFC_ERR << label << STR(" has overlapping archive member extents") << std::endl;
                return false;
            }
            member_extents.insert(next, extent);

            const ifc::tool::InputFile::View member_image = reader.member_image(member);
            if (not valid_ifc_image(member_image))
            {
                IFC_ERR << label << STR(" contains a malformed IFC member") << std::endl;
                return false;
            }
            ifc::UnitSort embedded_sort { };
            std::u8string embedded_name;
            if (not member_canonical_name(member_image, embedded_sort, embedded_name)
                or embedded_sort != key.sort or embedded_name != key.canonical)
            {
                IFC_ERR << label << STR(" has a member identity inconsistent with its IFC content") << std::endl;
                return false;
            }
        }
        return true;
    }

    // -- Print how to invoke the extract subcommand.
    inline void print_extract_usage()
    {
        IFC_ERR << STR("ifc extract usage:\n")
                << STR("\tifc extract [--force] [-o <dir>] <archive> --all\n")
                << STR("\tifc extract [--force] [-o <dir>] <archive> ")
                << STR("(<module-name> | --quote-header <name> | --angle-header <name>)...\n");
    }
}

int ifc::tool::ArchiveCommand::run_with(const ifc::tool::Arguments& args) const
{
    ifc::tool::StringView output;
    ifc::tool::StringView archive_name;
    bool have_output = false;
    std::vector<ifc::tool::StringView> positionals;

    for (std::size_t i = 0; i < args.size(); ++i)
    {
        const ifc::tool::StringView& arg = args[i];
        ifc::tool::StringView value;
        OptionMatch match = take_value(args, i, STR("-o"), STR("--output"), value);
        if (match == OptionMatch::Value)
        {
            output = value;
            have_output = true;
            continue;
        }
        if (match == OptionMatch::Missing)
        {
            IFC_ERR << STR("archive: missing filename after ") << arg << std::endl;
            return 1;
        }
        match = take_value(args, i, STR(""), STR("--name"), value);
        if (match == OptionMatch::Value)
        {
            archive_name = value;
            continue;
        }
        if (match == OptionMatch::Missing)
        {
            IFC_ERR << STR("archive: missing name after ") << arg << std::endl;
            return 1;
        }
        if (resemble_option(arg))
        {
            IFC_ERR << STR("archive: invalid option ") << arg << std::endl;
            return 1;
        }
        positionals.push_back(arg);
    }

    std::vector<ifc::tool::StringView> inputs;
    if (have_output)
    {
        inputs = std::move(positionals);
    }
    else
    {
        // Without an explicit -o, the first positional names the archive to create.
        if (positionals.size() < 2)
        {
            print_archive_usage();
            return 1;
        }
        output = positionals.front();
        inputs.assign(positionals.begin() + 1, positionals.end());
    }

    if (output.empty())
    {
        IFC_ERR << STR("archive: empty output filename") << std::endl;
        return 1;
    }
    if (inputs.empty())
    {
        IFC_ERR << STR("archive: no input IFC files specified") << std::endl;
        return 1;
    }

    ifc::fs::path output_path { output };
    if (not create_archive(output_path, inputs, archive_name))
        return 1;
    return 0;
}

int ifc::tool::ExtractCommand::run_with(const ifc::tool::Arguments& args) const
{
    ifc::tool::StringView output_dir;
    bool extract_all = false;
    ExistingFile existing = ExistingFile::Reject;
    std::vector<ifc::tool::StringView> positionals;
    std::vector<HeaderRequest> headers;

    for (std::size_t i = 0; i < args.size(); ++i)
    {
        const ifc::tool::StringView& arg = args[i];
        if (arg == STR("--all"))
        {
            extract_all = true;
            continue;
        }
        if (arg == STR("--force"))
        {
            existing = ExistingFile::Replace;
            continue;
        }
        ifc::tool::StringView value;
        OptionMatch match = take_value(args, i, STR("-o"), STR("--output-dir"), value);
        if (match == OptionMatch::Value)
        {
            output_dir = value;
            continue;
        }
        if (match == OptionMatch::Missing)
        {
            IFC_ERR << STR("extract: missing directory after ") << arg << std::endl;
            return 1;
        }
        match = take_value(args, i, STR(""), STR("--quote-header"), value);
        if (match == OptionMatch::Value)
        {
            std::u8string canonical = canonical_header_name(HeaderNameForm::Quote, value);
            if (canonical.empty())
            {
                IFC_ERR << STR("extract: invalid quote-form header name ") << value << std::endl;
                return 1;
            }
            headers.push_back({ value, std::move(canonical), HeaderNameForm::Quote });
            continue;
        }
        if (match == OptionMatch::Missing)
        {
            IFC_ERR << STR("extract: missing header name after ") << arg << std::endl;
            return 1;
        }
        match = take_value(args, i, STR(""), STR("--angle-header"), value);
        if (match == OptionMatch::Value)
        {
            std::u8string canonical = canonical_header_name(HeaderNameForm::Angle, value);
            if (canonical.empty())
            {
                IFC_ERR << STR("extract: invalid angle-form header name ") << value << std::endl;
                return 1;
            }
            headers.push_back({ value, std::move(canonical), HeaderNameForm::Angle });
            continue;
        }
        if (match == OptionMatch::Missing)
        {
            IFC_ERR << STR("extract: missing header name after ") << arg << std::endl;
            return 1;
        }
        if (resemble_option(arg))
        {
            IFC_ERR << STR("extract: invalid option ") << arg << std::endl;
            return 1;
        }
        positionals.push_back(arg);
    }

    if (positionals.empty())
    {
        print_extract_usage();
        return 1;
    }
    const ifc::tool::StringView archive = positionals.front();
    const std::vector<ifc::tool::StringView> names(positionals.begin() + 1, positionals.end());

    if (extract_all and (not names.empty() or not headers.empty()))
    {
        IFC_ERR << STR("extract: cannot combine --all with specific member selectors") << std::endl;
        return 1;
    }
    if (not extract_all and names.empty() and headers.empty())
    {
        IFC_ERR << STR("extract: specify --all or one or more members to extract") << std::endl;
        return 1;
    }

    const ifc::fs::path directory = output_dir.empty() ? ifc::fs::path { STR(".") } : ifc::fs::path { output_dir };

    // Map the archive once; every read below -- header, table of contents, string table, and member
    // content -- comes from these same hash-verified bytes, so there is no verify-then-read gap.
    try
    {
        ifc::tool::InputFile image { ifc::fs::path { archive }.native() };
        ArchiveReader reader;
        if (not open_archive(image.contents(), archive, reader))
            return 1;
        SecureDirectory output { directory };

        auto extract_one = [&](const ArchiveMember& member) -> bool
        {
            const ifc::tool::InputFile::View member_image = reader.member_image(member);
            const ifc::fs::path relative = reader.relative_path(member);
            const ifc::fs::path destination = relative.filename();

            // Write to a unique temporary beside the target, then rename, so a failure never leaves a
            // partial or half-written file in its place.
            try
            {
                SecureDirectory parent { output, relative.parent_path() };
                TemporaryFile temp { parent, destination };
                temp.output().write(member_image);
                const CommitFileResult committed = temp.commit_in(parent, destination, existing);
                if (committed == CommitFileResult::Exists)
                {
                    IFC_ERR << relative.native() << STR(": file already exists; use --force to replace it")
                            << std::endl;
                    return false;
                }
            }
            catch (const UnsafeOutputDirectoryError&)
            {
                IFC_ERR << relative.native() << STR(": output path contains an unsafe directory") << std::endl;
                return false;
            }
            catch (const TemporaryFileCreateError&)
            {
                IFC_ERR << relative.native() << STR(": couldn't create temporary output file") << std::endl;
                return false;
            }
            catch (const NativeFileError&)
            {
                IFC_ERR << relative.native() << STR(": couldn't write extracted file") << std::endl;
                return false;
            }
            catch (const TemporaryFileCommitError&)
            {
                IFC_ERR << relative.native() << STR(": couldn't finalize extracted file") << std::endl;
                return false;
            }
            IFC_OUT << STR("extracted ") << (directory / relative).native() << std::endl;
            return true;
        };

        int error_count = 0;
        if (extract_all)
        {
            for (const ArchiveMember& member : reader.members)
                if (not extract_one(member))
                    ++error_count;
        }
        else
        {
            for (const ifc::tool::StringView& requested : names)
            {
                // A positional spelling selects named modules only; header-unit form is explicit.
                const std::u8string want = to_utf8(requested);
                const std::u8string_view want_view { want };
                const auto matches = reader.members_named(want_view);
                bool found = false;
                for (const ArchiveMember& member : matches)
                {
                    if (named_module_sort(member.name.sort()))
                    {
                        found = true;
                        if (not extract_one(member))
                            ++error_count;
                    }
                }
                if (not found)
                {
                    IFC_ERR << requested << STR(": no named module with that name in the archive") << std::endl;
                    ++error_count;
                }
            }
            for (const HeaderRequest& requested : headers)
            {
                const ArchiveMember* member = reader.member({ requested.canonical, ifc::UnitSort::Header });
                if (member == nullptr)
                {
                    IFC_ERR << requested.spelling
                            << (requested.form == HeaderNameForm::Quote
                                    ? STR(": no quote-form header unit with that name in the archive")
                                    : STR(": no angle-form header unit with that name in the archive"))
                            << std::endl;
                    ++error_count;
                    continue;
                }
                if (not extract_one(*member))
                    ++error_count;
            }
        }
        return error_count;
    }
    catch (const ifc::tool::AccessError&)
    {
        IFC_ERR << archive << STR(": couldn't open file") << std::endl;
        return 1;
    }
    catch (const OutputDirectoryError& error)
    {
        IFC_ERR << error.path.native() << STR(": couldn't create or secure the output directory") << std::endl;
        return 1;
    }
    catch (const ifc::tool::RegularFileError&)
    {
        IFC_ERR << archive << STR(": not a regular file") << std::endl;
        return 1;
    }
    catch (const ifc::tool::FileMappingError&)
    {
        IFC_ERR << archive << STR(": couldn't memory-map file") << std::endl;
        return 1;
    }
}
