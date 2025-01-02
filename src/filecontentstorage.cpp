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
#include "filecontentstorage.h"
#include "util.h"
#include <assert.h>
#include <fstream>

namespace unassemblize
{
FileContentStorage::FileContentStorage()
{
    m_lastFileIt = m_filesMap.end();
}

const TextFileContent *FileContentStorage::find_content(const std::string &name) const
{
    if (name.empty())
    {
        return nullptr;
    }

    util::shared_lock_guard readLock(m_filesMapMutex);

    // Fast path lookup.
    if (m_lastFileIt != m_filesMap.cend() && name == m_lastFileIt->first)
    {
        return &m_lastFileIt->second;
    }

    // Search map.
    FileContentMap::const_iterator it = m_filesMap.find(name);
    if (it != m_filesMap.cend())
    {
        m_lastFileIt = it;
        return &it->second;
    }

    return nullptr;
}

FileContentStorage::LoadResult FileContentStorage::load_content(const std::string &name)
{
    // Take exclusive mutex to prevent multiple threads potentially loading the same file.
    std::lock_guard writeLock1(m_loadFileMutex);

    FileContentMap::iterator it;
    {
        util::shared_lock_guard readLock(m_filesMapMutex);
        it = m_filesMap.find(name);
        if (it != m_filesMap.end())
        {
            // Is already loaded.
            return LoadResult::AlreadyLoaded;
        }
    }

    std::ifstream fs(name);

    if (!fs.is_open())
    {
        // File open failed.
        return LoadResult::Failed;
    }

    TextFileContent content;
    content.filename = name;
    {
        std::string buf;
        while (std::getline(fs, buf))
        {
            content.lines.emplace_back(std::move(buf));
        }
    }

    if (!fs.eof() && fs.fail())
    {
        // File read failed.
        return LoadResult::Failed;
    }

    {
        std::lock_guard writeLock2(m_filesMapMutex);
        m_lastFileIt = m_filesMap.insert(it, std::make_pair(name, std::move(content)));
    }
    return LoadResult::Loaded;
}

size_t FileContentStorage::size() const
{
    util::shared_lock_guard readLock(m_filesMapMutex);
    return m_filesMap.size();
}

void FileContentStorage::clear()
{
    std::lock_guard writeLock(m_filesMapMutex);
    m_filesMap.clear();
    m_lastFileIt = m_filesMap.cend();
}

} // namespace unassemblize
