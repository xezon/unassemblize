/**
 * @file
 *
 * @brief Class to cache file contents for frequent access
 *
 * @copyright Unassemblize is free software: you can redistribute it and/or
 *            modify it under the terms of the GNU General Public License
 *            as published by the Free Software Foundation, either version
 *            3 of the License, or (at your option) any later version.
 *            A full copy of the GNU General Public License can be found in
 *            LICENSE
 */
#pragma once

#include <array>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <vector>

namespace unassemblize
{
struct TextFileContent
{
    std::string filename;
    std::vector<std::string> lines;
};

struct TextFileContentPair
{
    std::array<const TextFileContent *, 2> pair;
};

class FileContentStorage
{
    using FileContentMap = std::map<std::string, TextFileContent>;

public:
    enum class LoadResult
    {
        Failed,
        Loaded,
        AlreadyLoaded,
    };

public:
    FileContentStorage();

    FileContentStorage(FileContentStorage &&) = delete;
    FileContentStorage &operator=(FileContentStorage &&) = delete;
    FileContentStorage(const FileContentStorage &) = delete;
    FileContentStorage &operator=(const FileContentStorage &) = delete;

    const TextFileContent *find_content(const std::string &name) const;
    LoadResult load_content(const std::string &name);
    size_t size() const;
    void clear();

private:
    FileContentMap m_filesMap;
    mutable FileContentMap::const_iterator m_lastFileIt;
    mutable std::shared_mutex m_filesMapMutex; // Mutex taken when reading or writing the map.
    std::mutex m_loadFileMutex; // Mutex taken before loading a file from disk.
};

} // namespace unassemblize
