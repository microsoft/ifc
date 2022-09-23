//
// Microsoft (R) C/C++ Optimizing Compiler Front-End
// Copyright (C) Microsoft. All rights reserved.
//

#ifndef IFC_IO_INCLUDED
#define IFC_IO_INCLUDED

#include <stdio.h>

#include <string_view>
#include <type_traits>

// Workaround until https://github.com/microsoft/GSL/pull/1049 is merged into MSVC.
#pragma warning(push)
#pragma warning(disable: 5260)
#include <gsl/span>
#pragma warning(pop)

#include <ifc/file.hxx>
#include <ifc/pathname.hxx>
#include "fs-api.hxx" 

namespace Module {
    using namespace msvc;

    template<typename T>
    bool read_object(T& obj, FILE* f)
    {
        return fread(&obj, bits::rep(byte_length<T>), 1, f) == 1;
    }

    using ModuleName = Pathname;

    struct ModulePartitionName
    {
        ModuleName owner;
        ModuleName partition;
        auto operator<=>(const ModulePartitionName&) const = default;
    };

    // The following types are used for abandonment during IFC IO
    // operations.  They aren't meant as general exception types
    // for IO.  In general, most IO operation failures aren't
    // exceptional.  However, in the specific case of IFC reading,
    // IO failures indicate either logical inconsistency or
    // data structure corruption and the rest of processing for that
    // specific IFC must be abandoned.

    // Exception tag used to signal target architecture mismatch.
    struct IfcArchMismatch {
        ModuleName name;
        Pathname path;
    };

    // Exception tag used to signal an read failure from an IFC file (either invalid or corrupted.)
    struct IfcReadFailure {
        Pathname path;
    };

    // -- Could not determine content size.
    struct ContentSizeError {
        Pathname path;
    };

    // -- could not project file content to memory
    struct FileProjectionError {
        Pathname path;
    };

    // -- could not read entirety of file contents
    struct WholeContentReadError {
        Pathname path;
    };

    // -- too many open files
    struct TooManyOpenFilesError {
        Pathname path;
    };

    // -- inacessible file
    struct InaccessibleFileError {
        Pathname path;
    };

    // -- failed to match ifc integrity check
    struct IntegrityCheckFailed {
        SHA256Hash expected;
        SHA256Hash actual;
    };

    // -- failed to hash
    struct HashFailed {
        std::string reason;
    };

    // -- malformed IFC input
    struct MalformedIfc {
        ModuleName name;
    };

    /// -- unsupported format
    struct UnsupportedFormatVersion {
        FormatVersion version;
    };

    // Facilities for stdio-based file IO
    namespace StdIO {
        SHA256Hash hash_bytes(const uint8_t* first, const uint8_t* last);

        struct FileHandle {
            explicit FileHandle(FILE* file)
                : file_{ file }
            {
            }

            FileHandle(FileHandle&& file_handle)
                : file_(file_handle.file_)
            {
                file_handle.file_ = nullptr;
            }

            ~FileHandle()
            {
                close();
            }

            void close()
            {
                if (file_ != nullptr)
                {
                    fclose(file_);
                    file_ = nullptr;
                }
            }

            FILE* get() const
            {
                return file_;
            }

            explicit operator bool() const
            {
                return file_ != nullptr;
            }

            FILE* release()
            {
                FILE* file = file_;
                file_ = nullptr;
                return file;
            }

            bool position(ByteOffset n)
            {
                return fseek(get(), bits::rep(n), SEEK_SET) == 0;
            }

        protected:
            FILE* file_;
            FileHandle(const FileHandle&) = delete;
        };

        // Handle for input file open in binary mode.  Handles of this type want unique ownership.
        struct InputFileHandle : FileHandle {
            explicit InputFileHandle(const Pathname& path)
                : FileHandle{ nullptr }
            {
                auto ntbs = reinterpret_cast<const char*>(path.c_str());
                fopen_s(&file_, ntbs, "rb");
            }

            explicit InputFileHandle(FILE* file)
                : FileHandle{ file }
            {
            }

            bool eof() const
            {
                return feof(get()) != 0;
            }

            template<typename T>
            bool read(T& obj)
            {
                return read_object(obj, get());
            }

            template<typename T>
            bool read_buffer(T* buffer, size_t count)
            {
                return fread(buffer, bits::rep(byte_length<T>), count, get()) == count;
            }

            template<typename T>
            bool has_signature(const T& sig)
            {
                uint8_t buffer[sizeof sig]{};
                if (!read(buffer))
                    return false;
                return memcmp(buffer, sig, sizeof sig) == 0;
            }
        };

        struct OutputFileHandle : FileHandle {
            using FileHandle::FileHandle;
        };
    }

    namespace MemoryMapped {
        // An object of this type holds onto the underlying OS-handle for the
        // association between a file opened for read operations and its
        // projection into the address space of the owning process.  The association
        // is undone at the end of the handle's useful life.
        struct Mapping {
            // The default constructor is not really needed from architectural
            // perspective, but it is needed at the moment for implementation quirks.
            Mapping() { }

            Mapping(void* h, uint64_t s)
                : handle{ h },
                  extent{ s }
            { }

            Mapping(Mapping&& other)
                : handle{ other.handle },
                  extent{ other.extent }
            {
                other.handle = nullptr;
                other.extent = 0;
            }

            ~Mapping();

            // The underlying OS handle when needed for interfacing with the OS.
            auto get() const { return handle; }

            // The number of bytes in the file.
            auto size() const { return extent; }

        private:
            void* handle { };
            uint64_t extent { };
        };

        // An object of type represents a read-only projection of the content of a file into the
        // into the address space of the owning process.  The projection is undone at the end
        // of the object's useful life.
        struct FileProjection : Mapping {
            template<typename T>
            using Table = gsl::span<const T>;

            FileProjection() { }                        // See comment for Mapping's default constructor.
            explicit FileProjection(const Pathname&, Filesystem* fs_api);
            FileProjection(FileProjection&&);
            ~FileProjection();

            const std::byte* begin() const { return origin; }
            const std::byte* end() const { return begin() + size(); }

            // Is this file projection in good state?
            explicit operator bool() const { return get() != nullptr && begin() != nullptr; }

            // Set the file stream position appropriately for the next read operation.
            bool position(ByteOffset);

            const std::byte* tell() const { return cursor; }

            bool has_room_left_for(EntitySize amount) const
            {
                return end() - bits::rep(amount) >= cursor;
            }

            template<typename T>
            const T* read()
            {
                constexpr auto sz = byte_length<T>;
                if (!has_room_left_for(sz))
                    return { };
                auto ptr = reinterpret_cast<const T*>(tell());
                cursor += bits::rep(sz);
                return ptr;
            }

            template<typename T>
            Table<T> read_array(Cardinality n)
            {
                const auto sz = n * byte_length<T>;
                if (!has_room_left_for(sz))
                    return { };
                auto ptr = reinterpret_cast<const T*>(tell());
                cursor += bits::rep(sz);
                return { ptr, static_cast<typename Table<T>::size_type>(bits::rep(n)) };
            }

            // View a partition without touching the cursor.
            // PartitionSummaryData is assumed to be validated on read of toc and not checked in this function.
            template <typename T>
            Table<T> view_partition(const PartitionSummaryData& summary) const
            {
                const auto byte_offset = bits::rep(summary.offset);
                DASSERT(byte_offset < size());

                const auto byte_ptr = begin() + byte_offset;
                DASSERT(end() - bits::rep(summary.cardinality * byte_length<T>) >= byte_ptr);

                const auto ptr = reinterpret_cast<const T*>(byte_ptr);
                return { ptr, static_cast<typename Table<T>::size_type>(bits::rep(summary.cardinality)) };
            }

        private:
            const std::byte* origin { };
            const std::byte* cursor { };
        };


        enum class FileProjectionOptions
        {
            None                     = 0,
            IntegrityCheck           = 1U << 0, // Enable the SHA256 file check.
            AllowAnyPrimaryInterface = 1U << 1, // Project any primary module interface without checking for a matching name.
        };

        struct InputIfcFile : FileProjection {
            using StringTable = gsl::span<const std::byte>;
            using PartitionTable = gsl::span<const PartitionSummaryData>;

            InputIfcFile() { }

            const Header* header() const { return hdr; }
            const StringTable* string_table() const { return &str_tab; }
            PartitionTable partition_table() const
            {
                return { toc, static_cast<PartitionTable::size_type>(hdr->partition_count) };
            }


            const char* get(TextOffset offset) const
            {
                if (index_like::null(offset))
                    return {};
                return reinterpret_cast<const char*>(string_table()->data()) + bits::rep(offset);
            }

            const Pathname& path() const
            {
                return file_path;
            }

            template <UnitSort, typename T>
            static InputIfcFile project(const Pathname& path, Architecture arch, const T& ifc_designator, FileProjectionOptions options, Filesystem* fs_api);

        private:
            Pathname file_path;
            const Header* hdr { };
            const PartitionSummaryData* toc { };
            StringTable str_tab { };

            explicit InputIfcFile(const Pathname&, Filesystem* fs_api);
        };

        //Helper class for Anonymous Modules
        class AnonymousModuleIfcFileHelper {
        public:
            AnonymousModuleIfcFileHelper(const Pathname& path, Architecture arch, FileProjectionOptions options, Filesystem* fs_api):
                pathname{ path }, arch{ arch }, opts{ options }, fs{ fs_api } {

            }
            // Open in read-only mode, memory-mapped, the IFC file designated by the path.
            InputIfcFile project() const;
        private:
            Pathname pathname;
            Architecture arch;
            FileProjectionOptions opts;
            Filesystem* fs;
        };

        //Helper class for Named Modules
        class NamedModuleIfcFileHelper {
        public:
            NamedModuleIfcFileHelper(const Pathname& path, Architecture arch, const ModuleName& name, FileProjectionOptions options, Filesystem* fs_api) :
                pathname{ path }, arch{ arch }, module_name{ name }, opts{ options }, fs{ fs_api } {

            }
            // Open in read-only mode, memory-mapped, the IFC file designated by the path.
            // Additionally, check that the IFC has for the specified module name.
            InputIfcFile project() const;
        private:
            Pathname pathname;
            Architecture arch;
            ModuleName module_name;
            FileProjectionOptions opts;
            Filesystem* fs;
        };

        //Helper class for Module Partition
        class ModulePartitionIfcFileHelper {
        public:
            //const Pathname& path, Architecture arch, const ModulePartitionName& name, FileProjectionOptions options, Filesystem* fs_api
            ModulePartitionIfcFileHelper(const Pathname& path, Architecture arch, const ModulePartitionName& name, FileProjectionOptions options, Filesystem* fs_api) :
                pathname{ path }, arch{ arch }, partition_name{ name }, opts{ options }, fs{ fs_api } {

            }
            // Open in read-only mode, memory-mapped, the IFC file designated by the path.
            // Additionally, check that the IFC is a legacy header unit and that its resolved
            // header path matches the path provided.
            InputIfcFile project() const;
        private:
            Pathname pathname;
            Architecture arch;
            ModulePartitionName partition_name;
            FileProjectionOptions opts;
            Filesystem* fs;
        };

        //Helper class for Header Units
        class HeaderUnitsIfcFileHelper {
        public:
            HeaderUnitsIfcFileHelper(const Pathname& path, Architecture arch, std::u8string_view resolved_header_path, FileProjectionOptions options, Filesystem* fs_api) :
                pathname{ path }, arch{ arch }, header_path{ resolved_header_path }, opts{ options }, fs{ fs_api } {

            }
            // Open in read-only mode, memory-mapped, the IFC file designated by the path.
            // Additionally, check that the IFC has for the specified owning module name and partition name.
            InputIfcFile project() const;
        private:
            Pathname pathname;
            Architecture arch;
            std::u8string_view header_path;
            FileProjectionOptions opts;
            Filesystem* fs;
        };
    }
}


#endif // IFC_IO_INCLUDED
