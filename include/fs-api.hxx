//
// Microsoft (R) C/C++ Optimizing Compiler Front-End
// Copyright (C) Microsoft Corp. All rights reserved.
//
#ifndef UTILITY_FS_API_H
#define UTILITY_FS_API_H

#include <Windows.h>
#include <stdio.h>
#include "runtime-assertions.hxx"

// Specific way to access or pass the arguments to each call.
namespace WindowsAPIEnums
{
    // Since the argument type was DWORD the range syntactically accepted was
    // huge so these enums will help delimitate that.
    enum class Access : uint32_t
    {
        FileReadData = FILE_READ_DATA,
        FileReadAttributes = FILE_READ_ATTRIBUTES,
        GenericRead = GENERIC_READ,
        GenericWrite = GENERIC_WRITE
    };

    enum class ShareMode : uint32_t
    {
        FileShareDelete = FILE_SHARE_DELETE,
        FileShareRead = FILE_SHARE_READ,
        FileShareWrite = FILE_SHARE_WRITE
    };

    enum class FlagAndAttributes : uint32_t
    {
        FileFlagSequentialScan = FILE_FLAG_SEQUENTIAL_SCAN,
        FileAttributeNormal = FILE_ATTRIBUTE_NORMAL
    };
} // namespace WindowsAPIEnums
// This underlying type(uint32_t) match Windows API DWORD type
static_assert(sizeof(DWORD) == sizeof(uint32_t));

namespace CRTAPIEnums
{
    enum class Mode
    {
        Write,
        WriteText,
        ReadWriteText,
        ReadWriteBinary,
        ReadExistingBinary,
        ReadExistingText,
        ReadWriteBinaryTemp,
        ReadWriteExistingBinary
    };

    enum class ShareFlag : int
    {
        ReadWriteAccess = _SH_DENYNO,
        DenyWriteAccess = _SH_DENYWR
    };

    // CRTAPIEnums::Mode to a const wchar_t*
    constexpr const wchar_t* convert(Mode m)
    {
        switch (m)
        {
        case Mode::Write:
            return L"w";
        case Mode::WriteText:
            return L"wt";
        case Mode::ReadWriteText:
            return L"wt+"; // Same as "w+t"
        case Mode::ReadWriteBinary:
            return L"w+b";
        case Mode::ReadExistingBinary:
            return L"rb";
        case Mode::ReadExistingText:
            return L"rt";
        case Mode::ReadWriteBinaryTemp:
            return L"w+bT";
        case Mode::ReadWriteExistingBinary:
            return L"r+b";
        default:
            RASSERT(false);
            return L"";
        }
    }
} // namespace CRTAPIEnumss

struct Filesystem
{
    // AllowFileNameRedirection enforces that the corresponding filesystem API is called directly 
    // and that inputs to these APIs are not modified in any way.
    enum class AllowFileNameRedirection: bool { No, Yes };
    virtual HANDLE create_file(const wchar_t* filename,
                                 WindowsAPIEnums::Access access,
                                 WindowsAPIEnums::ShareMode share_mode,
                                 WindowsAPIEnums::FlagAndAttributes flag_attributes,
                                 AllowFileNameRedirection direct_call) const = 0;
    //This can potentially merge with create_file due to the flag for fetching
    virtual HANDLE open_directory(const wchar_t* path, AllowFileNameRedirection direct_call) const = 0;
    virtual FILE* fsopen(const wchar_t* filename,
                         CRTAPIEnums::Mode modes,
                         CRTAPIEnums::ShareFlag share_flags,
                         AllowFileNameRedirection direct_call) const = 0;
    virtual FILE* fopen(const wchar_t* filename,
                        CRTAPIEnums::Mode modes,
                        AllowFileNameRedirection direct_call) const = 0;
    virtual errno_t sopen_s(int* file_handle, const wchar_t* fileName, AllowFileNameRedirection direct_call) const = 0;
    virtual bool access(const wchar_t* filename, AllowFileNameRedirection direct_cal) const = 0;
    virtual bool check_file_exists(const wchar_t* filename, AllowFileNameRedirection direct_call) const = 0;
};
#endif // UTILITY_FS_API_H