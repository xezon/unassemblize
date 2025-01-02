/**
 * @file
 *
 * @brief Types to store relevant data from executable files
 *
 * @copyright Unassemblize is free software: you can redistribute it and/or
 *            modify it under the terms of the GNU General Public License
 *            as published by the Free Software Foundation, either version
 *            3 of the License, or (at your option) any later version.
 *            A full copy of the GNU General Public License can be found in
 *            LICENSE
 */
#include "executabletypes.h"
#include "util.h"

namespace unassemblize
{
ExeSectionType to_section_type(std::string_view str)
{
    if (util::equals_nocase(str, "code"))
    {
        return ExeSectionType::Code;
    }
    else if (util::equals_nocase(str, "data"))
    {
        return ExeSectionType::Data;
    }
    else
    {
        return ExeSectionType::Unknown;
    }
    static_assert(size_t(ExeSectionType::Unknown) == 2, "Enum was changed. Update conditions.");
}

const char *to_string(ExeSectionType type)
{
    switch (type)
    {
        case ExeSectionType::Code:
            return "code";
        case ExeSectionType::Data:
            return "data";
        case ExeSectionType::Unknown:
        default:
            return "unknown";
    }
    static_assert(size_t(ExeSectionType::Unknown) == 2, "Enum was changed. Update switch case.");
}

} // namespace unassemblize
