// Copyright Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// Windows Implementation of SHA256

#include <windows.h>
#include <bcrypt.h>

#include <ifc/file.hxx>

namespace {
    // -- Preserves a BCryptCreateHash failure without imposing a standard exception hierarchy.
    struct CreateHashError {
        NTSTATUS result; // Original bcrypt status retained for a future diagnostic boundary.
    };

    // -- Distinguishes an input-feeding failure from construction and finalization failures.
    struct HashDataError {
        NTSTATUS result; // Original bcrypt status retained for a future diagnostic boundary.
    };

    // -- Identifies failure to materialize the digest after all message bytes were accepted.
    struct FinishHashError {
        NTSTATUS result; // Original bcrypt status retained for a future diagnostic boundary.
    };
}

namespace ifc {
    // Uses the SHA-256 pseudo-algorithm handle, so there is no provider to open or close, and lets
    // bcrypt manage the hash-object memory (pbHashObject == nullptr).
    Sha256::Sha256()
    {
        const NTSTATUS status = BCryptCreateHash(BCRYPT_SHA256_ALG_HANDLE, &hash_handle,
                                                 /*pbHashObject = */ nullptr, /*cbHashObject = */ 0,
                                                 /*pbSecret = */ nullptr, /*cbSecret = */ 0, /*dwFlags = */ 0);
        if (status < 0)
            throw CreateHashError{status};
    }

    Sha256::~Sha256()
    {
        if (hash_handle != nullptr)
            BCryptDestroyHash(hash_handle);
    }

    void Sha256::update(gsl::span<const std::byte> bytes)
    {
        const NTSTATUS status = BCryptHashData(hash_handle,
                                               reinterpret_cast<PUCHAR>(const_cast<std::byte*>(bytes.data())),
                                               static_cast<ULONG>(bytes.size()), /*dwFlags = */ 0);
        if (status < 0)
            throw HashDataError{status};
    }

    SHA256Hash Sha256::finish()
    {
        SHA256Hash hash = {};
        const NTSTATUS status = BCryptFinishHash(hash_handle, reinterpret_cast<PUCHAR>(hash.value.data()),
                                                 static_cast<ULONG>(sizeof hash.value), /*dwFlags = */ 0);
        if (status < 0)
            throw FinishHashError{status};
        return hash;
    }

} // namespace ifc
