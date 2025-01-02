/**
 * @file
 *
 * @brief Version
 *
 * @copyright Unassemblize is free software: you can redistribute it and/or
 *            modify it under the terms of the GNU General Public License
 *            as published by the Free Software Foundation, either version
 *            3 of the License, or (at your option) any later version.
 *            A full copy of the GNU General Public License can be found in
 *            LICENSE
 */
#include "version.h"
#include "gitinfo.h"
#include <fmt/core.h>

std::string create_version_string()
{
    char revision[12] = {0};
    const char *version;

    if (GitTag[0] == 'v')
    {
        version = GitTag;
    }
    else
    {
        version = GitShortSHA1;
        snprintf(revision, sizeof(revision), "r%d ", GitRevision);
    }

    return fmt::format(
        "unassemblize {:s}{:s}{:s} by The Assembly Armada",
        revision,
        GitUncommittedChanges ? "~" : "",
        version);
}
