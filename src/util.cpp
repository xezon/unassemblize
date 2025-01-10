/**
 * @file
 *
 * @brief Utility functions
 *
 * @copyright Unassemblize is free software: you can redistribute it and/or
 *            modify it under the terms of the GNU General Public License
 *            as published by the Free Software Foundation, either version
 *            3 of the License, or (at your option) any later version.
 *            A full copy of the GNU General Public License can be found in
 *            LICENSE
 */

#include "util.h"
#include <codecvt>
#include <filesystem>

namespace util
{
std::string to_utf8(std::wstring_view utf16)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> convert;
    return convert.to_bytes(utf16.data(), utf16.data() + utf16.size());
}

std::wstring to_utf16(std::string_view utf8)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> convert;
    return convert.from_bytes(utf8.data(), utf8.data() + utf8.size());
}

std::string get_file_ext(std::string_view file_name)
{
    const size_t pos = file_name.find_last_of(".");
    if (pos != std::string_view::npos)
    {
        return std::string(file_name.substr(pos + 1));
    }
    return {};
}

std::string to_hex_string(const std::vector<uint8_t> &data)
{
    constexpr char hex_chars[] = "0123456789abcdef";

    const size_t count = data.size() * 2;
    std::string str;
    str.resize(count);

    for (size_t i = 0; i < count; ++i)
    {
        uint8_t byte = data[i];
        str[i] = hex_chars[(byte >> 4) & 0x0F];
        str[i + 1] = hex_chars[byte & 0x0F];
    }

    return str;
}

std::string abs_path(std::string_view path)
{
    return std::filesystem::absolute(path).string();
}

} // namespace util
