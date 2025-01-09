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
#pragma once

#include <fmt/core.h>
#include <string>
#include <string_view>
#include <vector>

#define PRINTF_STRING(str) static_cast<int>(str.size()), str.data()

namespace util
{
std::string to_utf8(std::wstring_view utf16);
std::wstring to_utf16(std::string_view utf8);
std::string get_file_ext(std::string_view file_name);
std::string to_hex_string(const std::vector<uint8_t> &data);
std::string abs_path(std::string_view path);

// Efficiently strip characters in place.
template<typename String>
void strip_inplace(String &str, std::string_view chars)
{
    auto pred = [&chars](const typename String::value_type &c) { return chars.find(c) != String::npos; };
    str.erase(std::remove_if(str.begin(), str.end(), pred), str.end());
}

template<typename String>
String strip(const String &str, std::string_view chars)
{
    String s(str);
    strip_inplace(s, chars);
    return s;
}

constexpr char to_lower(char c)
{
    return (c >= 'A' && c <= 'Z') ? (c + ('a' - 'A')) : c;
}

constexpr bool equals_nocase(char c1, char c2)
{
    return to_lower(c1) == to_lower(c2);
}

constexpr bool equals_nocase(std::string_view str1, std::string_view str2)
{
    if (str1.size() != str2.size())
        return false;

    for (size_t i = 0; i < str1.size(); ++i)
        if (!equals_nocase(str1[i], str2[i]))
            return false;

    return true;
}

constexpr int compare_nocase(std::string_view str1, std::string_view str2)
{
    size_t len = (str1.size() < str2.size()) ? str1.size() : str2.size();

    for (size_t i = 0; i < len; ++i)
    {
        char c1 = to_lower(str1[i]);
        char c2 = to_lower(str2[i]);
        if (c1 != c2)
        {
            return c1 - c2;
        }
    }

    // If all characters are equal up to the shortest length, compare lengths
    if (str1.size() == str2.size())
    {
        return 0;
    }
    return (str1.size() < str2.size()) ? -1 : 1;
}

// Efficiently assign format to std like string.
// The output string will always fit the formatted string in its entirety.
template<typename String, typename... Args>
void assign_format(String &output, fmt::format_string<Args...> format, Args &&... args)
{
    const size_t initial_capacity = output.capacity();
    output.resize(initial_capacity); // Does not allocate.
    auto result = fmt::format_to_n(output.begin(), initial_capacity, format, std::forward<Args>(args)...);
    output.resize(result.size); // May allocate if required size is larger than output string capacity.
    if (result.size > initial_capacity)
    {
        // Output buffer was not large enough. Repeat on the resized string.
        result = fmt::format_to_n(output.begin(), output.size(), format, std::forward<Args>(args)...);
        assert(result.size == output.size());
    }
}

// Efficiently append format to std like string.
// The output string will always fit the formatted string in its entirety.
template<typename String, typename... Args>
void append_format(String &output, fmt::format_string<Args...> format, Args &&... args)
{
    const size_t initial_size = output.size();
    const size_t initial_capacity = output.capacity();
    output.resize(initial_capacity); // Does not allocate.
    auto result = fmt::format_to_n(
        output.begin() + initial_size,
        initial_capacity - initial_size,
        format,
        std::forward<Args>(args)...);
    const size_t expected_size = initial_size + result.size;
    output.resize(expected_size); // May allocate if required size is larger than output string capacity.
    if (expected_size > initial_capacity)
    {
        // Output buffer was not large enough. Repeat on the resized string.
        result = fmt::format_to_n(
            output.begin() + initial_size,
            output.size() - initial_size,
            format,
            std::forward<Args>(args)...);
        assert(initial_size + result.size == output.size());
    }
}

// Efficiently assign format to std like string.
// The output string is truncated if the formatted string exceeds the maximum size.
template<typename String, typename... Args>
void assign_format(String &output, size_t max_size, fmt::format_string<Args...> format, Args &&... args)
{
    output.resize(max_size); // May allocate if required size is larger than output string capacity.
    const auto result = fmt::format_to_n(output.begin(), max_size, format, std::forward<Args>(args)...);
    const size_t written = std::distance(output.begin(), result.out);
    assert(written <= output.size());
    output.resize(written); // Does not allocate.
}

// Efficiently append format to std like string.
// The output string is truncated if the previous string plus the appended formatted string exceeds the maximum size.
template<typename String, typename... Args>
void append_format(String &output, size_t max_size, fmt::format_string<Args...> format, Args &&... args)
{
    const size_t initial_size = output.size();
    output.resize(max_size); // May allocate if required size is larger than output string capacity.
    typename String::iterator begin = output.begin() + initial_size;
    const auto result = fmt::format_to_n(begin, max_size - initial_size, format, std::forward<Args>(args)...);
    const size_t written = std::distance(begin, result.out);
    assert(written <= output.size());
    output.resize(initial_size + written); // Does not allocate.
}

// Efficiently clears a container by swapping with an empty container.
// This is more efficient than clear() as it truly releases memory back to the system.
// Works with any container that supports swap with an empty instance of itself.
template<typename T>
void free_container(T &container)
{
    T().swap(container);
}

// Use this for small containers or rare uses only.
// Use hash maps for longer and reoccurring lookups.
template<typename Container, typename Value>
bool has_value(const Container &container, const Value &value)
{
    return std::find(container.begin(), container.end(), value) != container.end();
}

// Use this for small containers or rare uses only.
// Use hash maps for longer and reoccurring lookups.
template<typename Container, typename UnaryPred>
bool has_value_if(const Container &container, UnaryPred pred)
{
    return std::find_if(container.begin(), container.end(), pred) != container.end();
}

// Use this for small containers or rare uses only.
template<typename Container, typename Value>
bool push_back_unique(Container &container, const Value &value)
{
    if (has_value(container, value))
        return false;
    container.push_back(value);
    return true;
}

template<typename Container, typename Value>
bool find_and_erase(Container &container, const Value &value)
{
    typename Container::iterator it = std::find(container.begin(), container.end(), value);
    if (it != container.end())
    {
        container.erase(it);
        return true;
    }
    return false;
}

template<typename Container, typename UnaryPred>
bool find_and_erase_if(Container &container, UnaryPred pred)
{
    typename Container::iterator it = std::find_if(container.begin(), container.end(), pred);
    if (it != container.end())
    {
        container.erase(it);
        return true;
    }
    return false;
}

template<typename SharedMutex>
class shared_lock_guard // Alias read_lock_guard
{
public:
    using mutex_type = SharedMutex;
    explicit shared_lock_guard(SharedMutex &mutex) : m_mutex(mutex) { m_mutex.lock_shared(); }
    ~shared_lock_guard() { m_mutex.unlock_shared(); }

private:
    SharedMutex &m_mutex;
};

} // namespace util
