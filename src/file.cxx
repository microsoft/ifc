// Copyright Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <ifc/file.hxx>

namespace ifc {
    SHA256Hash hash_bytes(const std::byte* first, const std::byte* last)
    {
        Sha256 hasher;
        hasher.update({ first, static_cast<std::size_t>(last - first) });
        return hasher.finish();
    }

    SHA256Hash hash_ifc_contents(gsl::span<const std::byte> image)
    {
        IFCASSERT(image.size() >= hashed_contents_offset);
        return hash_bytes(image.data() + hashed_contents_offset, image.data() + image.size());
    }

    void InputIfc::validate_content_integrity(const InputIfc& file)
    {
        const auto& contents = file.contents();
        auto result          = hash_ifc_contents(contents);
        auto actual_first    = reinterpret_cast<const std::uint8_t*>(result.value.data());
        auto actual_last     = actual_first + std::size(result.value) * 4;
        auto expected_first  = reinterpret_cast<const std::uint8_t*>(&contents[content_hash_offset]);
        auto expected_last   = expected_first + sizeof(SHA256Hash);
        if (not std::equal(actual_first, actual_last, expected_first, expected_last))
        {
            throw IntegrityCheckFailed{bytes_to_hash(expected_first, expected_last), result};
        }
    }

} // namespace ifc
