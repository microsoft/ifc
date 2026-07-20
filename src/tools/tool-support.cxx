// Copyright Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <array>
#include <algorithm>
#include <cstring>
#include <limits>
#include <utility>

#ifdef _WIN32
#   include <windows.h>
#endif

#include "tool-support.hxx"

namespace ifc::tool {
    bool resemble_option(const ifc::tool::StringView& s)
    {
        return s.starts_with(STR("-"));
    }

    std::u8string_view string_at(std::u8string_view table, std::uint32_t offset)
    {
        if (offset >= table.size())
            return {};
        const std::size_t stop = table.find(u8'\0', offset);
        if (stop == std::u8string_view::npos)
            return {};
        return table.substr(offset, stop - offset);
    }

    HeaderRead read_ifc_header(std::ifstream& in, ifc::Header& header)
    {
        std::array<std::uint8_t, sizeof ifc::InterfaceSignature> signature { };
        if (not in.read(reinterpret_cast<char*>(signature.data()), signature.size())
            or std::memcmp(signature.data(), std::begin(ifc::InterfaceSignature), signature.size()) != 0)
            return HeaderRead::NotIfc;
        if (not in.read(reinterpret_cast<char*>(&header), sizeof header))
            return HeaderRead::Truncated;
        return HeaderRead::Ok;
    }

    OptionMatch take_value(const ifc::tool::Arguments& args, std::size_t& i, ifc::tool::StringView short_name,
                           ifc::tool::StringView long_name, ifc::tool::StringView& value)
    {
        const ifc::tool::StringView arg = args[i];
        if ((not short_name.empty() and arg == short_name) or arg == long_name)
        {
            if (i + 1 >= args.size())
                return OptionMatch::Missing;
            value = args[++i];
            return OptionMatch::Value;
        }
        if (arg.size() > long_name.size() and arg.starts_with(long_name) and arg[long_name.size()] == STR("=")[0])
        {
            value = arg.substr(long_name.size() + 1);
            return OptionMatch::Value;
        }
        return OptionMatch::No;
    }

    std::u8string to_utf8(ifc::tool::StringView s)
    {
#ifdef _WIN32
        if (s.empty())
            return {};
        const int count =
            WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0, nullptr, nullptr);
        std::u8string result(static_cast<std::size_t>(count), u8'\0');
        WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()),
                            reinterpret_cast<char*>(result.data()), count, nullptr, nullptr);
        return result;
#else
        return std::u8string(reinterpret_cast<const char8_t*>(s.data()), s.size());
#endif
    }

    bool valid_ifc_image(ifc::tool::InputFile::View bytes)
    {
        return bytes.size() >= sizeof ifc::InterfaceSignature + sizeof(ifc::Header)
            and std::memcmp(bytes.data(), std::begin(ifc::InterfaceSignature), sizeof ifc::InterfaceSignature) == 0;
    }

    const ifc::Header& ifc_header(ifc::tool::InputFile::View bytes)
    {
        return *reinterpret_cast<const ifc::Header*>(bytes.data() + sizeof ifc::InterfaceSignature);
    }

    ifc::fs::path safe_relative(std::u8string_view stored)
    try
    {
        const ifc::fs::path original { std::u8string { stored } };
        ifc::fs::path result;
        for (const ifc::fs::path& component : original.relative_path())
        {
            if (component.native() == STR(".."))
                return {};
            if (component.native() == STR("."))
                continue;
#ifdef _WIN32
            const std::wstring& text = component.native();
            if (text.find(L':') != std::wstring::npos) // drive-relative path or NTFS data stream
                return {};
            if (text.ends_with(L'.') or text.ends_with(L' '))
                return {};
            std::wstring base = text.substr(0, text.find(L'.'));
            for (wchar_t& c : base)
                if (c >= L'a' and c <= L'z')
                    c = static_cast<wchar_t>(c - L'a' + L'A');
            static constexpr const wchar_t* devices[] = { L"CON", L"PRN", L"AUX", L"NUL", L"COM1", L"COM2", L"COM3",
                L"COM4", L"COM5", L"COM6", L"COM7", L"COM8", L"COM9", L"LPT1", L"LPT2", L"LPT3", L"LPT4", L"LPT5",
                L"LPT6", L"LPT7", L"LPT8", L"LPT9" };
            for (const wchar_t* device : devices)
                if (base == device)
                    return {};
#endif
            result /= component;
        }
        return result;
    }
    catch (const ifc::fs::filesystem_error&)
    {
        return {};
    }

    StringTableBuilder::Handle StringTableBuilder::intern(std::u8string_view s)
    {
        if (const auto it = index_map.find(s); it != index_map.end())
            return it->second;
        if (strings.size() > std::numeric_limits<std::uint32_t>::max())
            throw StringTableOverflow { strings.size() };
        const Handle handle = static_cast<Handle>(strings.size());
        const std::u8string& stored = strings.emplace_back(s);
        index_map.emplace(std::u8string_view { stored }, handle);
        return handle;
    }

    void StringTableBuilder::build()
    {
        table.clear();
        table.push_back(std::byte { 0 }); // offset 0: the empty string, also the null TextOffset
        offsets.assign(strings.size(), ifc::TextOffset { 0 });

        // Order the non-empty strings by their reversed bytes, so that a string which is a suffix
        // of another becomes adjacent to it; empty strings already map to offset 0.
        std::vector<std::uint32_t> order;
        order.reserve(strings.size());
        for (std::uint32_t i = 0; i < strings.size(); ++i)
            if (not strings[i].empty())
                order.push_back(i);
        std::ranges::sort(order, [this](std::uint32_t a, std::uint32_t b)
        {
            const std::u8string& x = strings[a];
            const std::u8string& y = strings[b];
            auto xi = x.rbegin();
            auto yi = y.rbegin();
            for (; xi != x.rend() and yi != y.rend(); ++xi, ++yi)
                if (*xi != *yi)
                    return *xi < *yi;
            return x.size() < y.size();
        });

        // Walk from the longest suffix downwards; a string that is a suffix of the previously
        // placed string is overlaid on it, otherwise it is appended with its own terminator.
        const std::u8string* previous = nullptr;
        std::uint32_t previous_offset = 0;
        for (std::size_t k = order.size(); k-- != 0; )
        {
            const std::uint32_t i = order[k];
            const std::u8string& s = strings[i];
            if (previous != nullptr and s.size() <= previous->size()
                and previous->compare(previous->size() - s.size(), s.size(), s) == 0)
            {
                offsets[i] = ifc::TextOffset { previous_offset
                                               + static_cast<std::uint32_t>(previous->size() - s.size()) };
            }
            else
            {
                if (table.size() > std::numeric_limits<std::uint32_t>::max())
                    throw StringTableOverflow { table.size() };
                offsets[i] = ifc::TextOffset { static_cast<std::uint32_t>(table.size()) };
                for (char8_t code_unit : s)
                    table.push_back(static_cast<std::byte>(code_unit));
                table.push_back(std::byte { 0 });
            }
            previous = &s;
            previous_offset = std::to_underlying(offsets[i]);
        }
    }

    ifc::TextOffset StringTableBuilder::offset(Handle h) const
    {
        // build() must have run and assigned this handle an offset; a handle interned after the
        // last build(), or any query before the first, falls outside `offsets` -- a usage error
        // we reject in every build mode (an assert would vanish under NDEBUG).
        if (std::to_underlying(h) >= offsets.size())
            throw UnresolvedStringHandle { h };
        return offsets[std::to_underlying(h)];
    }

    gsl::span<const std::byte> StringTableBuilder::bytes() const
    {
        return table;
    }
}
