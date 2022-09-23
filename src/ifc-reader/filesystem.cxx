#include <fs-api.hxx>
#include <fcntl.h>

struct MSVCFilesystem final : Filesystem
{
    HANDLE create_file(const wchar_t* filename,
        WindowsAPIEnums::Access access,
        WindowsAPIEnums::ShareMode share_mode,
        WindowsAPIEnums::FlagAndAttributes flag_attributes,
        AllowFileNameRedirection direct_call) const override
    {
        // Default arguments - CreateFileW
        //Note:All the "default" values were taken after doing an abstraction to the caller 
        //and matching those arguments that were the same through all the calls. 
        //The goal is to maintain the usual compiler behavior.
        constexpr LPSECURITY_ATTRIBUTES ip_security_attributes = nullptr;
        constexpr DWORD dw_creation_disposition = OPEN_EXISTING;
        constexpr HANDLE h_template_file = nullptr;

        return CreateFileW(filename,
            bits::rep(access),
            bits::rep(share_mode),
            ip_security_attributes,
            dw_creation_disposition,
            bits::rep(flag_attributes),
            h_template_file);
    }

    HANDLE open_directory(const wchar_t* path, AllowFileNameRedirection direct_call) const override
    {
        // Default arguments - CreateFileW
        constexpr LPSECURITY_ATTRIBUTES ip_security_attributes = nullptr;
        constexpr DWORD dw_creation_disposition = OPEN_EXISTING;
        constexpr HANDLE h_template_file = nullptr;
        return CreateFileW(path,
            GENERIC_READ,
            FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
            ip_security_attributes,
            dw_creation_disposition,
            FILE_FLAG_BACKUP_SEMANTICS,
            h_template_file);
    };

    //Note: There are some callers for the _wfopen in which the default for the share_flag is DENYNO
    //Almost all the callers have the share_flag default value, except of Yu feature in PCH (PCH Use).
    FILE* fsopen(const wchar_t* filename,
        CRTAPIEnums::Mode modes,
        CRTAPIEnums::ShareFlag share_flags,
        AllowFileNameRedirection direct_call) const override
    {
        return _wfsopen(filename, CRTAPIEnums::convert(modes), bits::rep(share_flags));
    }
    //Should I make something else for the pointer to the file pointer that receives the pointer to the opened file?
    FILE* fopen(const wchar_t* filename,
        CRTAPIEnums::Mode modes,
        AllowFileNameRedirection direct_call) const
    {
        // I/O parameter removed for encapsulating the error checking
        FILE* pFile = nullptr;
        auto error = _wfopen_s(&pFile, filename, CRTAPIEnums::convert(modes));
        if (error != 0)
            return nullptr;
        return pFile; // If the call is successful, return the file pointer
    }

    errno_t sopen_s(int* file_handle, const wchar_t* filename, AllowFileNameRedirection direct_call) const override
    {
        // Default arguments - _wsopen_s
        constexpr int open_flags = O_RDONLY | O_BINARY; // Opens as Read-only in binary format
        constexpr int oshare_flag = SH_DENYWR;
        constexpr int permission_flag = 0;
        return _wsopen_s(file_handle, filename, open_flags, oshare_flag, permission_flag);
    }

    bool access(const wchar_t* filename, AllowFileNameRedirection direct_call) const override
    {
        // Default arguments - _waccess_s
        constexpr int access_mode = 0;
        return _waccess_s(filename, access_mode) == 0;//errno_t 0 is success
    }

    bool check_file_exists(const wchar_t* filename, AllowFileNameRedirection direct_call) const override
    {
        return access(filename, direct_call);
    }
};

MSVCFilesystem os_api_impl;

extern Filesystem* os_api = &os_api_impl;
