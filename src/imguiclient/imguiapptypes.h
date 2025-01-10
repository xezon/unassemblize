/**
 * @file
 *
 * @brief ImGui App types
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
#include <cstdint>

namespace unassemblize::gui
{
enum class AssemblerTableColumn : uint8_t
{
    SourceLine,
    SourceCode,
    Bytes,
    Address,
    Jumps,
    Assembler,
};
inline constexpr size_t AssemblerTableColumnCount = 6;

inline constexpr std::array<const char *, AssemblerTableColumnCount> s_assemblerTableColumnNames =
    {"Source Line", "Source Code", "Bytes", "Address", "Jumps", "Assembler"};

inline constexpr const char *to_string(AssemblerTableColumn column)
{
    return s_assemblerTableColumnNames[static_cast<size_t>(column)];
}

} // namespace unassemblize::gui
