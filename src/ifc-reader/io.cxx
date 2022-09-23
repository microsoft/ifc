#include <filesystem>
#include <optional>
#include <string>
#include <compare> // Include as workaround for VSO1074724
#include <gsl/gsl_util>

// Since windows.h defines some NTSTATUS codes we need to guard these so the actual
// ntstatus.h can define them
#define WIN32_NO_STATUS
#include <windows.h>
#undef WIN32_NO_STATUS
#include <ntstatus.h>

#if MODULE_ENABLED
#pragma warning(push)
// Disable macro redefinition for now.  This fires because of conflicting macro definitions
// between the windows header inclusion order and the detection of existing macros.
#pragma warning(disable: 4005)
// This is a preprocessor bug interaction with header units where the preprocessor thinks a
// redefinition is not well-formed.  See ADO1218747.
#pragma warning(disable: 5106)
import <ifc/io.hxx>;
#pragma warning(pop)
#else
#include <ifc/io.hxx>
#endif

// TODO: We need to think about potentially moving the UTF8 library c1xx uses into its own library.
extern size_t UTF8ToUTF16Cch(LPCSTR lpSrcStr, size_t cchSrc, LPWSTR lpDestStr, size_t cchDest);

using UTF8ViewType = std::u8string_view;

namespace Module::StdIO {
    // Tags for catch handler.
    struct OpenAlgorithmError         { NTSTATUS result; };
    struct HashLengthPropertyError    { NTSTATUS result; };
    struct ObjectLengthPropertyError  { NTSTATUS result; };
    struct CreateHashError            { NTSTATUS result; };
    struct HashDataError              { NTSTATUS result; };
    struct FinishHashError            { NTSTATUS result; };
    // A simple helper class designed to be an RAII container for the Windows crypto machinery.
    class SHA256Helper {
    public:
        SHA256Helper()
        {
            digest_ntstatus<OpenAlgorithmError>(
                BCryptOpenAlgorithmProvider(&alg_handle_,
                                            BCRYPT_SHA256_ALGORITHM,
                                            /*pszImplementation = */nullptr,
                                            /*dwFlags = */0));

            DWORD cb_result = 0;
            digest_ntstatus<HashLengthPropertyError>(
                BCryptGetProperty(alg_handle_,
                                    BCRYPT_HASH_LENGTH,
                                    reinterpret_cast<PUCHAR>(&hash_byte_length_),
                                    sizeof hash_byte_length_,
                                    &cb_result,
                                    /*dwFlags = */0));

            digest_ntstatus<ObjectLengthPropertyError>(
                BCryptGetProperty(alg_handle_,
                                    BCRYPT_OBJECT_LENGTH,
                                    reinterpret_cast<PUCHAR>(&object_byte_length_),
                                    sizeof object_byte_length_,
                                    &cb_result,
                                    /*dwFlags = */0));
        }

        ~SHA256Helper()
        {
            if (alg_handle_ != nullptr)
            {
                BCryptCloseAlgorithmProvider(alg_handle_, /*dwFlags = */0);
            }
        }

        SHA256Hash hash(uint8_t* first, uint8_t* last)
        {
            BCRYPT_HASH_HANDLE hash_handle = nullptr;

            using ByteVector = std::vector<uint8_t>;
            ByteVector object_buf(object_byte_length_);

            digest_ntstatus<CreateHashError>(
                BCryptCreateHash(alg_handle_,
                                    &hash_handle,
                                    object_buf.data(),
                                    object_byte_length_,
                                    /*pbSecret = */nullptr,
                                    /*cbSecret = */0,
                                    /*dwFlags = */0));
            auto final_act = gsl::finally([&]{
                if (hash_handle != nullptr)
                {
                    BCryptDestroyHash(hash_handle);
                }
            });
            digest_ntstatus<HashDataError>(
                BCryptHashData(hash_handle,
                                first,
                                static_cast<ULONG>(std::distance(first, last)),
                                /*dwFlags = */0));
            SHA256Hash hash = { };
            // uint32_t array should map to a uint8_t[32] array
            DASSERT(hash_byte_length_ == std::size(hash.value) * 4);
            digest_ntstatus<FinishHashError>(
                BCryptFinishHash(hash_handle,
                                    reinterpret_cast<uint8_t*>(hash.value.data()),
                                    hash_byte_length_,
                                    /*dwFlags = */0));
            return hash;
        }

    private:
        template <typename T>
        void digest_ntstatus(HRESULT result)
        {
            if (result != STATUS_SUCCESS)
            {
                throw T{ result };
            }
        }

        BCRYPT_ALG_HANDLE alg_handle_ = nullptr;
        DWORD hash_byte_length_ = 0;
        DWORD object_byte_length_ = 0;
    };

    static std::string stringify_common(NTSTATUS s)
    {
        switch (s)
        {
        case STATUS_BUFFER_TOO_SMALL:
            return "buffer size not large enough";
        case STATUS_INVALID_HANDLE:
            return "invalid handle";
        case STATUS_INVALID_PARAMETER:
            return "one or more parameters are invalid";
        default:
            return "unknown";
        }
    }

    static std::string to_string(const OpenAlgorithmError& e)
    {
        switch (e.result)
        {
        case STATUS_NOT_FOUND:
            return "algorithm was not found";
        case STATUS_NO_MEMORY:
            return "memory allocation failure";
        default:
            return stringify_common(e.result);
        }
    }

    static std::string to_string(const HashLengthPropertyError& e)
    {
        switch (e.result)
        {
        case STATUS_NOT_SUPPORTED:
            return "object length property not supported";
        default:
            return stringify_common(e.result);
        }
    }

    static std::string to_string(const ObjectLengthPropertyError& e)
    {
        switch (e.result)
        {
        case STATUS_NOT_SUPPORTED:
            return "length property not supported";
        default:
            return stringify_common(e.result);
        }
    }

    static std::string to_string(const CreateHashError& e)
    {
        switch (e.result)
        {
        case STATUS_NOT_SUPPORTED:
            return "create hash not supported";
        default:
            return stringify_common(e.result);
        }
    }

    static std::string to_string(const HashDataError& e)
    {
        return stringify_common(e.result);
    }

    static std::string to_string(const FinishHashError& e)
    {
        return stringify_common(e.result);
    }

    template <typename T>
    std::string format(const char* preface_error, const T& e)
    {
        return preface_error + to_string(e);
    }

    SHA256Hash hash_bytes(const uint8_t* first, const uint8_t* last)
    try {
        SHA256Helper helper;
        // Windows expects no cv-qualifiers on any data even if it is only read
        return helper.hash(const_cast<uint8_t*>(first), const_cast<uint8_t*>(last));
    }
    catch (const OpenAlgorithmError& e)
    {
        throw HashFailed{ format("open algorithm failed: ", e) };
    }
    catch (const HashLengthPropertyError& e)
    {
        throw HashFailed{ format("hash length failed: ", e) };
    }
    catch (const ObjectLengthPropertyError& e)
    {
        throw HashFailed{ format("object length failed: ", e) };
    }
    catch (const CreateHashError& e)
    {
        throw HashFailed{ format("create hash failed: ", e) };
    }
    catch (const HashDataError& e)
    {
        throw HashFailed{ format("hash data failed: ", e) };
    }
    catch (const FinishHashError& e)
    {
        throw HashFailed{ format("finish hash failed: ", e) };
    }
    catch (...)
    {
        throw;
    }

    static SHA256Hash hash_bytes(const std::byte* first, const std::byte* last)
    {
        return hash_bytes(reinterpret_cast<const uint8_t*>(first), reinterpret_cast<const uint8_t*>(last));
    }

    static SHA256Hash bytes_to_hash(const uint8_t* first, const uint8_t* last)
    {
        auto size = std::distance(first, last);
        if (size != sizeof(SHA256Hash))
        {
            return { };
        }
        SHA256Hash hash{};
        uint8_t* alias = reinterpret_cast<uint8_t*>(hash.value.data());
        // std::copy in devcrt  issues a warning whenever you try to copy
        // an unbounded T* so we will just use std::memcpy instead.
        std::memcpy(alias, first, size);
        return hash;
    }
} // namespace Module::StdIO

namespace Module::MemoryMapped {
    namespace {
        // An object of this type holds onto an underlying OS-handle of a file, and
        // is responsible for closing the handle at the end of its useful life.
        struct FileHandle {
            explicit FileHandle(HANDLE h) : handle{ h }
            { }

            FileHandle(FileHandle&& other) : handle(other.handle)
            {
                other.handle = INVALID_HANDLE_VALUE;
            }

            ~FileHandle()
            {
                if (*this)
                    CloseHandle(get());
            }

            // The underlying OS handle when needed for interfacing with the OS.
            HANDLE get() const { return handle; }

            // Is this handle in good state?
            explicit operator bool() const { return get() != INVALID_HANDLE_VALUE; }

        private:
            HANDLE handle;
        };

        // Call onto the hosting OS to give us a read-capable handle for the file
        // designated by the path.  That file is to be mapped to memory, so
        // we don't require filesystem-enabled optimization for sequential read.
        FileHandle open_file_for_readonly_mapping(const Pathname& path, Filesystem* fs_api)
        {
            auto ntbs = reinterpret_cast<const char*>(path.c_str());
            // Since CreateFile only works in two modes (plain ascii or UTF-16) we need to
            // convert the internal representation of UTF-8 to UTF-16.
            const auto length = (path.length() + 1) * sizeof(wchar_t);
            std::vector<wchar_t> buf(length, L'\0');
            UTF8ToUTF16Cch(ntbs, path.length() + 1, buf.data(), length);
            auto access = WindowsAPIEnums::Access::GenericRead;
            auto share_mode = WindowsAPIEnums::ShareMode::FileShareRead;
            auto flags_attributes = WindowsAPIEnums::FlagAndAttributes::FileAttributeNormal;
            const auto remap = Filesystem::AllowFileNameRedirection::Yes;
            auto x = fs_api->create_file(buf.data(), access, share_mode, flags_attributes, remap);

            if (x == INVALID_HANDLE_VALUE) {
                switch (GetLastError()) {
                case ERROR_TOO_MANY_OPEN_FILES:
                    throw TooManyOpenFilesError{ path };
                case ERROR_ACCESS_DENIED:
                    throw InaccessibleFileError{ path };
                default:
                    break;
                }
            }
            return FileHandle{ x };
        }

        // Return the number of bytes in the opened 'file' designated by 'path'.
        uint64_t file_size(const FileHandle& file, const Pathname& path)
        {
            if (!file)
                return { };
            LARGE_INTEGER sz { };
            if (GetFileSizeEx(file.get(), &sz))
                return sz.QuadPart;
            throw ContentSizeError{ path };
        }

        // Given a file handle, returns a handle to the projection of that file to memory.
        HANDLE acquire_mapping_object(const FileHandle& file, const Pathname& path)
        {
            if (!file)
                return nullptr;
            auto handle = CreateFileMapping(file.get(), /* lpAttributes = */ nullptr, /* flProtect = */ PAGE_READONLY, /* dwMaximumSizeHigh = */ 0, /* dwMaximumSizeLow = */ 0, /* lpName = */ nullptr);
            if (handle == nullptr)
                switch (GetLastError()) {
                case ERROR_FILE_INVALID:            // OK, empty file
                    break;
                default:
                    throw FileProjectionError{ path };
                    break;
                }
            return handle;
        }

        const std::byte* project_to_memory(Mapping& mapping, const Pathname& path)
        {
            auto handle = mapping.get();
            if (handle == nullptr)
                return nullptr;
            auto view = MapViewOfFile(handle, FILE_MAP_READ, /* dwFileOffsetHigh = */ 0, /* dwFileOffsetLow = */ 0, /* dwNumberOfBytesToMap = */ 0);
            if (view == nullptr)
                throw WholeContentReadError{ path };
            return static_cast<const std::byte*>(view);
        }


        template<typename T>
        bool has_signature(FileProjection& file, const T& sig)
        {
            auto start = file.tell();
            return file.position(ByteOffset{ sizeof sig }) && memcmp(start, sig, sizeof sig) == 0;
        }

        void validate_content_integrity(FileProjection& file)
        {
            // Verify integrity of ifc.  To do this we know that the header content after the hash
            // starts after the interface signature and the first 256 bits.
            constexpr size_t hash_start = sizeof InterfaceSignature;
            constexpr size_t contents_start = hash_start + sizeof(SHA256Hash);
            static_assert(contents_start == 36); // 4 bytes for Signature + 8*4 bytes for SHA2
            auto result = StdIO::hash_bytes(file.begin() + contents_start, file.end());
            auto actual_first = reinterpret_cast<const uint8_t*>(result.value.data());
            auto actual_last = actual_first + std::size(result.value) * 4;
            auto expected_first = reinterpret_cast<const uint8_t*>(file.begin()) + hash_start;
            auto expected_last = expected_first + sizeof(SHA256Hash);
            if (!std::equal(actual_first,
                            actual_last,
                            expected_first,
                            expected_last))
            {
                throw IntegrityCheckFailed { StdIO::bytes_to_hash(expected_first, expected_last), result };
            }
        }

        struct OwningModuleAndPartition
        {
            UTF8ViewType owning_module;
            UTF8ViewType partition_name;
        };

        struct IllFormedPartitionName { };

        OwningModuleAndPartition separate_module_name(UTF8ViewType name)
        {
            // The full module name for a partition will be of the form "M:P"
            // where 'M' is the owning module name parts and 'P' is the partition
            // name parts as specified in n4830 [module.unit]/1.
            auto first = std::begin(name);
            auto last  = std::end(name);
            auto colon = std::find(first, last, ':');

            // If there is no ':' at all, this is not a partition name.
            if (colon == last)
            {
                throw IllFormedPartitionName{ };
            }
            // The name cannot start with a ':'.
            if (colon == first)
            {
                throw IllFormedPartitionName{ };
            }
            auto partition_name_start = std::next(colon);
            // The partition name cannot be blank.
            if (partition_name_start == last)
            {
                throw IllFormedPartitionName{ };
            }
            UTF8ViewType module_name = name.substr(0, std::distance(first, colon));
            UTF8ViewType partition_name = name.substr(std::distance(first, partition_name_start), std::distance(partition_name_start, last));
            return OwningModuleAndPartition{ module_name, partition_name };
        }
    }

    Mapping::~Mapping()
    {
        if (handle != nullptr)
            CloseHandle(handle);
    }

    Mapping create_mapping_for_file(const Pathname& path, Filesystem* fs_api)
    {
        FileHandle file { open_file_for_readonly_mapping(path, fs_api) };
        auto byte_count = file_size(file, path);
        Mapping mapping { acquire_mapping_object(file, path), byte_count };
        return mapping;
    }

    FileProjection::FileProjection(const Pathname& path, Filesystem* fs_api)
        : Mapping { create_mapping_for_file(path, fs_api) },
          origin{ project_to_memory(*this, path) },
          cursor{ origin }
    { }

    FileProjection::FileProjection(FileProjection&& other)
        : Mapping(std::move(other)),
          origin(other.origin),
          cursor(other.cursor)
    {
        other.origin = nullptr;
        other.cursor = nullptr;
    }

    FileProjection::~FileProjection()
    {
        if (origin != nullptr)
            UnmapViewOfFile(origin);
    }

    bool FileProjection::position(ByteOffset offset)
    {
        auto n = bits::rep(offset);
        if (n > size())
            return false;
        cursor = begin() + n;
        return true;
    }

    InputIfcFile::InputIfcFile(const Pathname& path, Filesystem* fs_api)
        : file_path{ path}, FileProjection{ path, fs_api }
    { }

    //helper class AnonymousModuleIfcFileHelper
    InputIfcFile AnonymousModuleIfcFileHelper::project() const
    {
        return InputIfcFile::project<UnitSort::Primary>(pathname, arch, ModuleName{ }, opts | FileProjectionOptions::AllowAnyPrimaryInterface, fs);
    }
    //NamedModuleIfcFileHelper
    InputIfcFile NamedModuleIfcFileHelper::project() const
    {
        //const Pathname& path, Architecture arch, const ModuleName& name, FileProjectionOptions options, Filesystem* fs_api
        return InputIfcFile::project<UnitSort::Primary>(pathname, arch, module_name, opts, fs);
    }
    //ModulePartitionIfcFileHelper
    InputIfcFile ModulePartitionIfcFileHelper::project() const
    {
        //const Pathname& path, Architecture arch, const ModulePartitionName& name, FileProjectionOptions options, Filesystem* fs_api
        return InputIfcFile::project<UnitSort::Partition>(pathname, arch, partition_name, opts, fs);
    }
    //HeaderUnitIfcFileHelper
    InputIfcFile HeaderUnitsIfcFileHelper::project() const
    {
        //const Pathname& path, Architecture arch, std::u8string_view resolved_header_path, FileProjectionOptions options, Filesystem* fs_api
        return InputIfcFile::project<UnitSort::Header>(pathname, arch, header_path, opts, fs);
    }

    bool compatible_architectures(Architecture src, Architecture dst)
    {
        if (src == dst)
            return true;

        // FIXME: CHPE runs the compiler twice -- first X86, then HybridX86ARM64. Because of
        // the ordering of these invocations, the compiler can overwrite X86 IFCs with
        // HybridX86ARM64 IFCS. Subsequent use of these IFCs can result in X86 compiler
        // invocations attempting to read HybridX86ARM64 IFCS, causing architecture mismatches.
        // Our definition of compatibility is regrettably weakened to allow this case.
        if (src == Architecture::HybridX86ARM64 and dst == Architecture::X86)
            return true;

        return false;
    }

    template <UnitSort Kind, typename T>
    InputIfcFile InputIfcFile::project(const Pathname& path, Architecture arch, const T& ifc_designator, FileProjectionOptions options, Filesystem* fs_api)
    {
        if (InputIfcFile file { path, fs_api })
        {
            if (!has_signature(file, Module::InterfaceSignature))
                return { };

            if (implies(options, FileProjectionOptions::IntegrityCheck))
            {
                validate_content_integrity(file);
            }

            auto header = file.hdr = file.read<Header>();
            if (header == nullptr)
                return { };

            if (header->version > CurrentFormatVersion
                || (header->version < MinimumFormatVersion && header->version != EDGFormatVersion))
                throw UnsupportedFormatVersion{ header->version };

            // If the user requested an unknown architecture, we do not perform architecture check.
            if (arch != Architecture::Unknown and not compatible_architectures(header->arch, arch))
            {
                if constexpr (Kind != UnitSort::Partition)
                {
                    throw IfcArchMismatch { ifc_designator, path };
                }
                else
                {
                    throw IfcArchMismatch { ifc_designator.partition, path };
                }
            }

            file.position(header->toc);
            file.toc = file.read<PartitionSummaryData>();

            if (!zero(header->string_table_bytes))
            {
                if (!file.position(header->string_table_bytes))
                    return { };
                auto bytes = file.tell();
                auto nbytes = bits::rep(header->string_table_size);
                DASSERT(file.has_room_left_for(EntitySize{ nbytes }));
                file.str_tab = { bytes, static_cast<StringTable::size_type>(nbytes) };
            }

            if constexpr (Kind == UnitSort::Primary || Kind == UnitSort::ExportedTU)
            {
                // If we are reading module to merge then the final module name (which can be provided on the command-line) may not
                // match the name of the module we are loading. So there is no need to check.
                if (!ifc_designator.empty()
                    && (header->unit.sort() == UnitSort::Primary || header->unit.sort() == UnitSort::ExportedTU))
                {
                    auto sz = bits::rep(header->unit.module_name());
                    DASSERT(sz <= bits::rep(header->string_table_size));
                    auto chars = reinterpret_cast<const char8_t*>(file.string_table()->data()) + sz;
                    UTF8ViewType ifc_name { chars };
                    if (ifc_name != ifc_designator)
                        return { };
                }
                // Failed to have a valid designator or the unit sort is a mismatch.  If we do not allow any arbitrary interface through, exit.
                else if (!implies(options, FileProjectionOptions::AllowAnyPrimaryInterface))
                {
                    return { };
                }
            }

            if constexpr (Kind == UnitSort::Partition)
            {
                // If we are reading module to merge then the final module name (which can be provided on the command-line) may not
                // match the name of the module we are loading. So there is no need to check.
                if (!ifc_designator.partition.empty()
                    && (header->unit.sort() == UnitSort::Partition))
                {
                    auto sz = bits::rep(header->unit.module_name());
                    DASSERT(sz <= bits::rep(header->string_table_size));
                    auto chars = reinterpret_cast<const char8_t*>(file.string_table()->data()) + sz;
                    UTF8ViewType ifc_name { chars };
                    try
                    {
                        auto [owning_module_name, partition_name] = separate_module_name(ifc_name);
                        if (owning_module_name != ifc_designator.owner)
                        {
                            return { };
                        }

                        if (partition_name != ifc_designator.partition)
                        {
                            return { };
                        }
                    }
                    catch (const IllFormedPartitionName&)
                    {
                        return { };
                    }
                }
                // Failed to have a valid designator or the unit sort is a mismatch.
                else
                {
                    return { };
                }
            }

            if constexpr (Kind == UnitSort::Header)
            {
                // We do not need to verify this now that the src_path of the header unit matches the one passed in.
                // It is desireable to simply ask the ifc to be loaded without such verification as paths are not
                // reliable once the header unit is made to be distributable.  A higher-level check is needed.
                // The unit sort is a mismatch.
                if (header->unit.sort() != UnitSort::Header)
                {
                    return { };
                }
            }

            if (!file.position(ByteOffset(sizeof InterfaceSignature)))
                throw IfcReadFailure { path };
            return file;
        }
        // FIXME: Should we signal error for a reference file that isn't readable?
        return { };
    }

}
