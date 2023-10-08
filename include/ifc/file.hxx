// Copyright Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef IFC_FILE_INCLUDED
#define IFC_FILE_INCLUDED

#include <array>
#include <string>
#include <cstring>

#include <ifc/assertions.hxx>
#include "ifc/index-utils.hxx"
#include <ifc/pathname.hxx>
#include "ifc/version.hxx"
#include <gsl/span>

namespace ifc {
    using ifc::implies;
    using ifc::to_underlying;

    using index_like::Index;

    // Type for the size of a partition entry
    enum class EntitySize : uint32_t {};

    // Return the number of bytes in the object representation of type T.
    // This is, of course, the same thing as sizeof(T), except that we insist
    // objects must fit in a 32-bit index space.  That is what the braces are for.
    template<typename T>
    constexpr auto byte_length = EntitySize{sizeof(T)};

    // Offset in a byte stream.
    // TODO: abstract over large file support.  For now, we expect module exports to be vastly smaller
    //       than the collection of all declarations from a header file content.
    enum class ByteOffset : uint32_t {};

    constexpr bool zero(ByteOffset x)
    {
        return to_underlying(x) == 0;
    }

    template<typename S>
    using if_byte_offset = std::enable_if_t<std::is_same<S, std::underlying_type_t<ByteOffset>>::value>;

    template<typename T, typename = if_byte_offset<T>>
    constexpr ByteOffset operator+(ByteOffset x, T n)
    {
        // FIXME: Check overflow.
        return ByteOffset(to_underlying(x) + n);
    }

    template<typename T, typename = if_byte_offset<T>>
    inline ByteOffset& operator+=(ByteOffset& x, T n)
    {
        return x = x + n;
    }

    template<index_like::Unisorted T>
    constexpr std::underlying_type_t<T> operator-(T x, T y)
    {
        IFCASSERT(x >= y);
        return to_underlying(x) - to_underlying(y);
    }

    // Data type for assessing the 'size' of a collection.
    enum class Cardinality : uint32_t {};

    constexpr bool zero(Cardinality n)
    {
        return to_underlying(n) == 0;
    }

    inline Cardinality operator+(Cardinality x, Cardinality y)
    {
        return Cardinality{to_underlying(x) + to_underlying(y)}; // FIXME: Check for overflow.
    }

    inline Cardinality& operator+=(Cardinality& x, Cardinality y)
    {
        return x = x + y;
    }

    inline Cardinality& operator++(Cardinality& n)
    {
        auto x = to_underlying(n);
        IFCASSERT(x < index_like::wilderness<std::underlying_type_t<Cardinality>>);
        return n = Cardinality(x + 1);
    }

    // Multiplying a size measure by a certain count.
    constexpr EntitySize operator*(Cardinality n, EntitySize x)
    {
        // FIXME: Check for overflow.
        return EntitySize{to_underlying(n) * to_underlying(x)};
    }

    // Module Interface signature
    inline constexpr uint8_t InterfaceSignature[4] = {0x54, 0x51, 0x45, 0x1A};

    // ABI description.
    enum class Abi : uint8_t {};

    // Architecture description
    enum class Architecture : uint8_t {
        Unknown        = 0x00, // unknown target
        X86            = 0x01, // x86 (32-bit) target
        X64            = 0x02, // x64 (64-bit) target
        ARM32          = 0x03, // ARM (32-bit) target
        ARM64          = 0x04, // ARM (64-bit) target
        HybridX86ARM64 = 0x05, // Hybrid x86 x arm64
        ARM64EC        = 0x06, // ARM64 EC target
    };

    // CplusPlus Version Info : defined by [cpp.predefined]/1
    enum class CPlusPlus : uint32_t {};

    // Names and strings are stored in a global string table.
    // The texts are referenced via offsets.
    enum class TextOffset : uint32_t {};

    // Index into the scope table.
    enum ScopeIndex : uint32_t {};

    struct SHA256Hash {
        std::array<uint32_t, 8> value;
    };

    // The various sort of translation units that can be represented in an IFC file.
    enum class UnitSort : uint8_t {
        Source,     // General source translation unit.
        Primary,    // Module primary interface unit.
        Partition,  // Module interface partition unit.
        Header,     // Header unit.
        ExportedTU, // Translation unit where every declaration is exported, scheduled for removal.
        Count
    };

    struct UnitIndex : index_like::Over<UnitSort> {
        using Base = index_like::Over<UnitSort>;

        // Non-module units do not have module names.
        constexpr UnitIndex() : Base{UnitSort::Header, 0} {}

        constexpr UnitIndex(TextOffset text, UnitSort sort) : Base{sort, to_underlying(text)} {}

        TextOffset module_name() const
        {
            return TextOffset(to_underlying(index()));
        }

        TextOffset header_name() const
        {
            IFCASSERT(sort() == UnitSort::Header);
            return TextOffset(to_underlying(index()));
        }
    };

    // Module interface header
    struct Header {
        SHA256Hash content_hash;       // For verifying the integrity of the .ifc contents below.
        FormatVersion version;         // Version of the IFC file format
        Abi abi;                       // Abi tag.
        Architecture arch;             // Target machine architecture tag.
        CPlusPlus cplusplus;           // The __cplusplus version this file was built with.
        ByteOffset string_table_bytes; // String table offset.
        Cardinality string_table_size; // Number of bytes in the string table.
        UnitIndex unit;                // Index of this translation unit.
        TextOffset src_path;           // Pathname of the source file containing the interface definition.
        ScopeIndex global_scope;       // Index of the global scope.
        ByteOffset toc;                // Offset to the table of contents.
        Cardinality partition_count;   // Number of partitions, including the string table partition.
        bool internal_partition; // Set to true if this TU does not contribute to a module unit external interface.
                                 // FIXME: Gaby, find a better representation for this.
    };

    // Partition info in the ToC of a module interface.
    struct PartitionSummaryData {
        TextOffset name;
        ByteOffset offset;
        Cardinality cardinality;
        EntitySize entry_size;

        template<index_like::Unisorted T>
        ByteOffset tell(T x) const
        {
            return offset + to_underlying(x) * to_underlying(entry_size);
        }

        bool empty() const
        {
            return zero(cardinality);
        }
    };

    template<typename T>
    struct PartitionSummary : PartitionSummaryData {
        PartitionSummary() : PartitionSummaryData{}
        {
            entry_size = byte_length<T>;
        }
    };

    // Exception tag used to signal target architecture mismatch.
    struct IfcArchMismatch {
        Pathname name;
        Pathname path;
    };

    // Exception tag used to signal an read failure from an IFC file (either invalid or corrupted.)
    struct IfcReadFailure {
        Pathname path;
    };

    // -- failed to match ifc integrity check
    struct IntegrityCheckFailed {
        SHA256Hash expected;
        SHA256Hash actual;
    };

    /// -- unsupported format
    struct UnsupportedFormatVersion {
        FormatVersion version;
    };

    enum class IfcOptions {
        None                     = 0,
        IntegrityCheck           = 1U << 0, // Enable the SHA256 file check.
        AllowAnyPrimaryInterface = 1U << 1, // Allow any primary module interface without checking for a matching name.
    };

    SHA256Hash hash_bytes(const std::byte* first, const std::byte* last);

    inline SHA256Hash bytes_to_hash(const uint8_t* first, const uint8_t* last)
    {
        auto byte_count = std::distance(first, last);
        if (byte_count != sizeof(SHA256Hash))
        {
            return {};
        }
        auto size = static_cast<std::size_t>(byte_count);

        SHA256Hash hash{};
        uint8_t* alias = reinterpret_cast<uint8_t*>(hash.value.data());
        // std::copy in devcrt  issues a warning whenever you try to copy
        // an unbounded T* so we will just use std::memcpy instead.
        std::memcpy(alias, first, size);
        return hash;
    }

    struct InputIfc {
        using StringTable    = gsl::span<const std::byte>;
        using PartitionTable = gsl::span<const PartitionSummaryData>;
        template<typename T>
        using Table    = gsl::span<const T>;
        using SpanType = gsl::span<const std::byte>;

        const Header* header() const
        {
            return hdr;
        }
        const StringTable* string_table() const
        {
            return &str_tab;
        }
        PartitionTable partition_table() const
        {
            return {toc, static_cast<PartitionTable::size_type>(hdr->partition_count)};
        }

        InputIfc() = default;

        InputIfc(const SpanType& span_) : span(span_)
        {
            cursor = span.begin();
        }

        void init(const SpanType& s)
        {
            span   = s;
            cursor = span.begin();
        }

        auto contents() const
        {
            return span;
        }

        const char* get(TextOffset offset) const
        {
            if (index_like::null(offset))
                return {};
            return reinterpret_cast<const char*>(string_table()->data()) + to_underlying(offset);
        }

        bool position(ByteOffset offset)
        {
            auto n = to_underlying(offset);
            if (n > span.size())
                return false;
            cursor = span.begin() + n;
            return true;
        }

        SpanType::iterator tell() const
        {
            return cursor;
        }

        bool has_room_left_for(EntitySize amount) const
        {
            return span.end() - to_underlying(amount) >= cursor;
        }

        template<typename T>
        const T* read()
        {
            constexpr auto sz = byte_length<T>;
            if (not has_room_left_for(sz))
                return {};
            const std::byte* byte_ptr = &(*tell());
            auto ptr                  = reinterpret_cast<const T*>(byte_ptr);
            cursor += to_underlying(sz);
            return ptr;
        }

        template<typename T>
        Table<T> read_array(Cardinality n)
        {
            const auto sz = n * byte_length<T>;
            if (not has_room_left_for(sz))
                return {};
            const std::byte* byte_ptr = &(*tell());
            auto ptr                  = reinterpret_cast<const T*>(byte_ptr);
            cursor += to_underlying(sz);
            return {ptr, static_cast<typename Table<T>::size_type>(to_underlying(n))};
        }

        // View a partition without touching the cursor.
        // PartitionSummaryData is assumed to be validated on read of toc and not checked in this function.
        template<typename T>
        Table<T> view_partition(const PartitionSummaryData& summary) const
        {
            const auto byte_offset = to_underlying(summary.offset);
            IFCASSERT(byte_offset < span.size());

            const auto byte_ptr = &span[byte_offset];
            const auto ptr      = reinterpret_cast<const T*>(byte_ptr);
            return {ptr, static_cast<typename Table<T>::size_type>(to_underlying(summary.cardinality))};
        }

        template<typename T>
        static bool has_signature(InputIfc& file, const T& sig)
        {
            auto start = &(*file.tell());
            return file.position(ByteOffset{sizeof(sig)}) and memcmp(start, sig, sizeof(sig)) == 0;
        }

        static void validate_content_integrity(const InputIfc& file);

        static bool compatible_architectures(Architecture src, Architecture dst)
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

        using UTF8ViewType = std::u8string_view;
        struct OwningModuleAndPartition {
            UTF8ViewType owning_module;
            UTF8ViewType partition_name;
        };

        struct IllFormedPartitionName {};

        static OwningModuleAndPartition separate_module_name(UTF8ViewType name)
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
                throw IllFormedPartitionName{};
            }
            // The name cannot start with a ':'.
            if (colon == first)
            {
                throw IllFormedPartitionName{};
            }
            auto partition_name_start = std::next(colon);
            // The partition name cannot be blank.
            if (partition_name_start == last)
            {
                throw IllFormedPartitionName{};
            }
            UTF8ViewType module_name = name.substr(0, static_cast<std::size_t>(std::distance(first, colon)));
            UTF8ViewType partition_name =
                name.substr(static_cast<std::size_t>(std::distance(first, partition_name_start)),
                            static_cast<std::size_t>(std::distance(partition_name_start, last)));
            return OwningModuleAndPartition{module_name, partition_name};
        }

        template<UnitSort Kind, typename T>
        bool designator_matches_ifc_unit_sort(const Header* header, const T& ifc_designator, IfcOptions options)
        {
            if constexpr (Kind == UnitSort::Primary || Kind == UnitSort::ExportedTU)
            {
                // If we are reading module to merge then the final module name (which can be provided on the
                // command-line) may not match the name of the module we are loading. So there is no need to check.
                if (!ifc_designator.empty()
                    && (header->unit.sort() == UnitSort::Primary || header->unit.sort() == UnitSort::ExportedTU))
                {
                    auto sz = to_underlying(header->unit.module_name());
                    IFCASSERT(sz <= to_underlying(header->string_table_size));
                    auto chars = reinterpret_cast<const char8_t*>(string_table()->data()) + sz;
                    std::u8string_view ifc_name{chars};
                    if (ifc_name != ifc_designator)
                        return false;
                }
                // Failed to have a valid designator or the unit sort is a mismatch.  If we do not allow any arbitrary
                // interface through, exit.
                else if (!implies(options, IfcOptions::AllowAnyPrimaryInterface))
                {
                    return false;
                }
            }

            if constexpr (Kind == UnitSort::Partition)
            {
                // If we are reading module to merge then the final module name (which can be provided on the
                // command-line) may not match the name of the module we are loading. So there is no need to check.
                if (!ifc_designator.partition.empty() && (header->unit.sort() == UnitSort::Partition))
                {
                    auto sz = to_underlying(header->unit.module_name());
                    IFCASSERT(sz <= to_underlying(header->string_table_size));
                    auto chars = reinterpret_cast<const char8_t*>(string_table()->data()) + sz;
                    std::u8string_view ifc_name{chars};
                    try
                    {
                        auto [owning_module_name, partition_name] = separate_module_name(ifc_name);
                        if (owning_module_name != ifc_designator.owner)
                        {
                            return false;
                        }

                        if (partition_name != ifc_designator.partition)
                        {
                            return false;
                        }
                    }
                    catch (const IllFormedPartitionName&)
                    {
                        return false;
                    }
                }
                // Failed to have a valid designator or the unit sort is a mismatch.
                else
                {
                    return false;
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
                    return false;
                }
            }

            return true;
        }

        template<UnitSort Kind, typename T>
        bool validate(const ifc::Pathname& path, Architecture arch, const T& ifc_designator, IfcOptions options)
        {
            if (!has_signature(*this, ifc::InterfaceSignature))
                return false;

            if (implies(options, IfcOptions::IntegrityCheck))
            {
                validate_content_integrity(*this);
            }

            auto header = hdr = read<Header>();
            if (header == nullptr)
                return false;

            if (header->version > CurrentFormatVersion
                || (header->version < MinimumFormatVersion && header->version != EDGFormatVersion))
                throw UnsupportedFormatVersion{header->version};

            // If the user requested an unknown architecture, we do not perform architecture check.
            if (arch != Architecture::Unknown and not compatible_architectures(header->arch, arch))
            {
                if constexpr (Kind != UnitSort::Partition)
                {
                    throw IfcArchMismatch{ifc_designator, path};
                }
                else
                {
                    throw IfcArchMismatch{ifc_designator.partition, path};
                }
            }

            position(header->toc);
            toc = read<PartitionSummaryData>();

            if (!zero(header->string_table_bytes))
            {
                if (!position(header->string_table_bytes))
                    return false;
                auto bytes  = tell();
                auto nbytes = to_underlying(header->string_table_size);
                IFCASSERT(has_room_left_for(EntitySize{nbytes}));
                str_tab = {&bytes[0], static_cast<StringTable::size_type>(nbytes)};
            }

            if (not designator_matches_ifc_unit_sort<Kind>(header, ifc_designator, options))
                return false;

            if (!position(ByteOffset(sizeof InterfaceSignature)))
                throw IfcReadFailure{path};
            return true;
        }

    protected:
        SpanType span;
        SpanType::iterator cursor{};
        const Header* hdr{};
        const PartitionSummaryData* toc{};
        StringTable str_tab{};
    };
} // namespace ifc


#endif // IFC_FILE_INCLUDED
