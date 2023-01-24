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

    // -- malformed IFC input
    struct MalformedIfc {
        ModuleName name;
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

            FileProjection() { }                        // See comment for Mapping's default constructor.
            explicit FileProjection(const Pathname&, Filesystem* fs_api);
            FileProjection(FileProjection&&);
            ~FileProjection();

            // Is this file projection in good state?
            explicit operator bool() const { return _origin != nullptr; }
            const std::byte* origin() const { return _origin; }

        private:
            const std::byte* _origin { };
        };

        struct InputIfcFile : public Module::InputIfc {

            InputIfcFile() { }

            const Pathname& path() const
            {
                return file_path;
            }

            template <UnitSort, typename T>
            static InputIfcFile project(const Pathname& path, Architecture arch, const T& ifc_designator, IfcOptions options, Filesystem* fs_api);

        private:
            Pathname file_path;
            FileProjection file_projection;

            explicit InputIfcFile(const Pathname&, Filesystem* fs_api);
        };

        //Helper class for Anonymous Modules
        class AnonymousModuleIfcFileHelper {
        public:
            AnonymousModuleIfcFileHelper(const Pathname& path, Architecture arch, IfcOptions options, Filesystem* fs_api):
                pathname{ path }, arch{ arch }, opts{ options }, fs{ fs_api } {

            }
            // Open in read-only mode, memory-mapped, the IFC file designated by the path.
            InputIfcFile project() const;
        private:
            Pathname pathname;
            Architecture arch;
            IfcOptions opts;
            Filesystem* fs;
        };

        //Helper class for Named Modules
        class NamedModuleIfcFileHelper {
        public:
            NamedModuleIfcFileHelper(const Pathname& path, Architecture arch, const ModuleName& name, IfcOptions options, Filesystem* fs_api) :
                pathname{ path }, arch{ arch }, module_name{ name }, opts{ options }, fs{ fs_api } {

            }
            // Open in read-only mode, memory-mapped, the IFC file designated by the path.
            // Additionally, check that the IFC has for the specified module name.
            InputIfcFile project() const;
        private:
            Pathname pathname;
            Architecture arch;
            ModuleName module_name;
            IfcOptions opts;
            Filesystem* fs;
        };

        //Helper class for Module Partition
        class ModulePartitionIfcFileHelper {
        public:
            //const Pathname& path, Architecture arch, const ModulePartitionName& name, IfcOptions options, Filesystem* fs_api
            ModulePartitionIfcFileHelper(const Pathname& path, Architecture arch, const ModulePartitionName& name, IfcOptions options, Filesystem* fs_api) :
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
            IfcOptions opts;
            Filesystem* fs;
        };

        //Helper class for Header Units
        class HeaderUnitsIfcFileHelper {
        public:
            HeaderUnitsIfcFileHelper(const Pathname& path, Architecture arch, std::u8string_view resolved_header_path, IfcOptions options, Filesystem* fs_api) :
                pathname{ path }, arch{ arch }, header_path{ resolved_header_path }, opts{ options }, fs{ fs_api } {

            }
            // Open in read-only mode, memory-mapped, the IFC file designated by the path.
            // Additionally, check that the IFC has for the specified owning module name and partition name.
            InputIfcFile project() const;
        private:
            Pathname pathname;
            Architecture arch;
            std::u8string_view header_path;
            IfcOptions opts;
            Filesystem* fs;
        };
    }
}


#endif // IFC_IO_INCLUDED
