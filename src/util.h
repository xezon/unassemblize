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

#include <string>
#include <string_view>
#include <vector>

namespace util
{
std::string to_utf8(const wchar_t *utf16);
std::string to_utf8(const std::wstring &utf16);
std::wstring to_utf16(const char *utf8);
std::wstring to_utf16(const std::string &utf8);
void strip_inplace(std::string &str, std::string_view chars);
std::string strip(const std::string &str, std::string_view chars);
std::string get_file_ext(const std::string &file_name);
std::string to_hex_string(const std::vector<uint8_t> &data);
std::string abs_path(const std::string &path);

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
    Container::iterator it = std::find(container.begin(), container.end(), value);
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
    Container::iterator it = std::find_if(container.begin(), container.end(), pred);
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
