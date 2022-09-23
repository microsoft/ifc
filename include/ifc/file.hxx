//
// Microsoft (R) C/C++ Optimizing Compiler Front-End
// Copyright (C) Microsoft. All rights reserved.
//

#ifndef IFC_FILE_INCLUDED
#define IFC_FILE_INCLUDED

#include <array>

#include "runtime-assertions.hxx"
#include "ifc/index-utils.hxx"
#include "ifc/version.hxx"

namespace Module {
    using index_like::Index;

    // Type for the size of a partition entry
    enum class EntitySize : uint32_t { };

    // Return the number of bytes in the object representation of type T.
    // This is, of course, the same thing as sizeof(T), except that we insist
    // objects must fit in a 32-bit index space.  That is what the braces are for.
    template<typename T>
    constexpr auto byte_length = EntitySize{ sizeof(T) };

    // Offset in a byte stream.
    // TODO: abstract over large file support.  For now, we expect module exports to be vastly smaller
    //       than the collection of all declarations from a header file content.
    enum class ByteOffset : uint32_t { };

    constexpr bool zero(ByteOffset x) { return bits::rep(x) == 0; }

    template<typename S>
    using if_byte_offset = std::enable_if_t<std::is_same<S, bits::raw<ByteOffset>>::value>;

    template<typename T, typename = if_byte_offset<T>>
    constexpr ByteOffset operator+(ByteOffset x, T n)
    {
        // FIXME: Check overflow.
        return ByteOffset(bits::rep(x) + n);
    }

    template<typename T, typename = if_byte_offset<T>>
    inline ByteOffset& operator+=(ByteOffset& x, T n)
    {
        return x = x + n;
    }

    template<index_like::Unisorted T>
    constexpr bits::raw<T> operator-(T x, T y)
    {
        DASSERT(x >= y);
        return bits::rep(x) - bits::rep(y);
    }

    // Data type for assessing the 'size' of a collection.
    enum class Cardinality : uint32_t { };

    constexpr bool zero(Cardinality n) { return bits::rep(n) == 0; }

    inline Cardinality operator+(Cardinality x, Cardinality y)
    {
        return Cardinality{ bits::rep(x) + bits::rep(y) };              // FIXME: Check for overflow.
    }

    inline Cardinality& operator+=(Cardinality& x, Cardinality y)
    {
        return x = x + y;
    }

    inline Cardinality& operator++(Cardinality& n)
    {
        auto x = bits::rep(n);
        DASSERT(x < index_like::wilderness<bits::raw<Cardinality>>);
        return n = Cardinality(x + 1);
    }

    // Multiplying a size measure by a certain count.
    constexpr EntitySize operator*(Cardinality n, EntitySize x)
    {
        // FIXME: Check for overflow.
        return EntitySize{ bits::rep(n) * bits::rep(x) };
    }

    // Module Interface signature
    inline constexpr uint8_t InterfaceSignature[4] = { 0x54, 0x51, 0x45, 0x1A };

    // ABI description.
    enum class Abi : uint8_t { };

    // Architecture description
    enum class Architecture : uint8_t {
        Unknown = 0x00,                     // unknown target
        X86 = 0x01,                         // x86 (32-bit) target
        X64 = 0x02,                         // x64 (64-bit) target
        ARM32 = 0x03,                       // ARM (32-bit) target
        ARM64 = 0x04,                       // ARM (64-bit) target
        HybridX86ARM64 = 0x05,              // Hybrid x86 x arm64
        ARM64EC = 0x06,                     // ARM64 EC target
    };

    // CplusPlus Version Info : defined by [cpp.predefined]/1
    enum class CPlusPlus : uint32_t { };

    // Names and strings are stored in a global string table.
    // The texts are referenced via offsets.
    enum class TextOffset : uint32_t { };

    // Index into the scope table.
    enum ScopeIndex : uint32_t { };

    struct SHA256Hash {
        std::array<uint32_t, 8> value;
    };

    // The various sort of translation units that can be represented in an IFC file.
    enum class UnitSort : uint8_t {
        Source,         // General source translation unit.
        Primary,        // Module primary interface unit.
        Partition,      // Module interface partition unit.
        Header,         // Header unit.
        ExportedTU,     // Translation unit where every declaration is exported, scheduled for removal.
        Count
    };

    struct UnitIndex : index_like::Over<UnitSort> {
        using Base = index_like::Over<UnitSort>;

        // Non-module units do not have module names.
        constexpr UnitIndex() : Base{ UnitSort::Header, 0 } { }

        constexpr UnitIndex(TextOffset text, UnitSort unit) : Base{ unit, bits::rep(text) } { }

        TextOffset module_name() const
        {
            return TextOffset(bits::rep(index()));
        }

        TextOffset header_name() const
        {
            DASSERT(sort() == UnitSort::Header);
            return TextOffset(bits::rep(index()));
        }
    };

    // Module interface header
    struct Header {
        SHA256Hash content_hash;            // For verifying the intregity of the .ifc contents below.
        FormatVersion version;              // Version of the IFC file format
        Abi abi;                            // Abi tag.
        Architecture arch;                  // Target machine architecture tag.
        CPlusPlus cplusplus;                // The __cplusplus version this file was built with.
        ByteOffset string_table_bytes;      // String table offset.
        Cardinality string_table_size;      // Number of bytes in the string table.
        UnitIndex unit;                     // Index of this translation unit.
        TextOffset src_path;                // Pathname of the source file containing the interface definition.
        ScopeIndex global_scope;            // Index of the global scope.
        ByteOffset toc;                     // Offset to the table of contents.
        Cardinality partition_count;        // Number of partitions, including the string table partition.
        bool internal_partition;            // Set to true if this TU does not contribute to a module unit external interface. FIXME: Gaby, find a better representataion for this.
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
            return offset + bits::rep(x) * bits::rep(entry_size);
        }

        bool empty() const { return zero(cardinality); }
    };

    template<typename T>
    struct PartitionSummary : PartitionSummaryData {
        PartitionSummary() : PartitionSummaryData{ } { entry_size = byte_length<T>;  }
    };
}


#endif // IFC_FILE_INCLUDED
