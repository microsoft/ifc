//
// Microsoft (R) C/C++ Optimizing Compiler Front-End
// Copyright (C) Microsoft. All rights reserved.
//

#include <ifc/file.hxx>

namespace Module {
    void InputIfc::validate_content_integrity(const InputIfc& file)
    {
        // Verify integrity of ifc.  To do this we know that the header content after the hash
        // starts after the interface signature and the first 256 bits.
        constexpr size_t hash_start = sizeof InterfaceSignature;
        constexpr size_t contents_start = hash_start + sizeof(SHA256Hash);
        static_assert(contents_start == 36); // 4 bytes for Signature + 8*4 bytes for SHA2
        const auto& contents = file.contents();
        auto result = hash_bytes(contents.data() + contents_start, contents.data() + contents.size());
        auto actual_first = reinterpret_cast<const uint8_t*>(result.value.data());
        auto actual_last = actual_first + std::size(result.value) * 4;
        auto expected_first = reinterpret_cast<const uint8_t*>(&contents[hash_start]);
        auto expected_last = expected_first + sizeof(SHA256Hash);
        if (!std::equal(actual_first,
            actual_last,
            expected_first,
            expected_last))
        {
            throw IntegrityCheckFailed{ bytes_to_hash(expected_first, expected_last), result };
        }
    }

}
