// Copyright Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// Windows Implementation of SHA256

#include <windows.h>
#include <bcrypt.h>
#include <gsl/util>

#if defined(IFC_BUILD_USING_STD_MODULE)
    import std;
#else
    #include <vector>
#endif

#include <ifc/file.hxx>
#include <ifc/assertions.hxx>

namespace {
    // Tags for catch handler.
    struct OpenAlgorithmError {
        NTSTATUS result;
    };
    struct HashLengthPropertyError {
        NTSTATUS result;
    };
    struct ObjectLengthPropertyError {
        NTSTATUS result;
    };
    struct CreateHashError {
        NTSTATUS result;
    };
    struct HashDataError {
        NTSTATUS result;
    };
    struct FinishHashError {
        NTSTATUS result;
    };
    // A simple helper class designed to be an RAII container for the Windows crypto machinery.
    class SHA256Helper {
    public:
        SHA256Helper()
        {
            digest_ntstatus<OpenAlgorithmError>(BCryptOpenAlgorithmProvider(&alg_handle_, BCRYPT_SHA256_ALGORITHM,
                                                                            /*pszImplementation = */ nullptr,
                                                                            /*dwFlags = */ 0));

            DWORD cb_result = 0;
            digest_ntstatus<HashLengthPropertyError>(BCryptGetProperty(alg_handle_, BCRYPT_HASH_LENGTH,
                                                                       reinterpret_cast<PUCHAR>(&hash_byte_length_),
                                                                       sizeof hash_byte_length_, &cb_result,
                                                                       /*dwFlags = */ 0));

            digest_ntstatus<ObjectLengthPropertyError>(BCryptGetProperty(alg_handle_, BCRYPT_OBJECT_LENGTH,
                                                                         reinterpret_cast<PUCHAR>(&object_byte_length_),
                                                                         sizeof object_byte_length_, &cb_result,
                                                                         /*dwFlags = */ 0));
        }

        ~SHA256Helper()
        {
            if (alg_handle_ != nullptr)
            {
                BCryptCloseAlgorithmProvider(alg_handle_, /*dwFlags = */ 0);
            }
        }

        ifc::SHA256Hash hash(const std::byte* first, const std::byte* last)
        {
            BCRYPT_HASH_HANDLE hash_handle = nullptr;

            using ByteVector = std::vector<uint8_t>;
            ByteVector object_buf(object_byte_length_);

            digest_ntstatus<CreateHashError>(BCryptCreateHash(alg_handle_, &hash_handle, object_buf.data(),
                                                              object_byte_length_,
                                                              /*pbSecret = */ nullptr,
                                                              /*cbSecret = */ 0,
                                                              /*dwFlags = */ 0));
            auto final_act = gsl::finally([&] {
                if (hash_handle != nullptr)
                {
                    BCryptDestroyHash(hash_handle);
                }
            });
            digest_ntstatus<HashDataError>(BCryptHashData(hash_handle,
                                                          reinterpret_cast<PUCHAR>(const_cast<std::byte*>(first)),
                                                          static_cast<ULONG>(std::distance(first, last)),
                                                          /*dwFlags = */ 0));
            ifc::SHA256Hash hash = {};
            // uint32_t array should map to a uint8_t[32] array
            IFCASSERT(hash_byte_length_ == std::size(hash.value) * 4);
            digest_ntstatus<FinishHashError>(
                BCryptFinishHash(hash_handle, reinterpret_cast<uint8_t*>(hash.value.data()), hash_byte_length_,
                                 /*dwFlags = */ 0));
            return hash;
        }

    private:
        template<typename T>
        void digest_ntstatus(NTSTATUS result)
        {
            // Success for an NTSTATUS is >= 0.
            if (result < 0)
            {
                throw T{result};
            }
        }

        BCRYPT_ALG_HANDLE alg_handle_ = nullptr;
        DWORD hash_byte_length_       = 0;
        DWORD object_byte_length_     = 0;
    };

} // namespace

namespace ifc {
    SHA256Hash hash_bytes(const std::byte* first, const std::byte* last)
    {
        SHA256Helper helper;
        return helper.hash(first, last);
    }
} // namespace ifc
