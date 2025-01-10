/**
 * @file
 *
 * @brief Program File Common
 *
 * @copyright Unassemblize is free software: you can redistribute it and/or
 *            modify it under the terms of the GNU General Public License
 *            as published by the Free Software Foundation, either version
 *            3 of the License, or (at your option) any later version.
 *            A full copy of the GNU General Public License can be found in
 *            LICENSE
 */
#pragma once

#include "commontypes.h"
#include <memory>

namespace unassemblize::gui
{
struct ProgramFileDescriptor;
struct ProgramFileRevisionDescriptor;
struct ProgramComparisonDescriptor;

using WorkQueueCommandId = uint32_t;

using ProgramFileId = uint32_t;
using ProgramFileRevisionId = uint32_t;
using ProgramComparisonId = uint32_t;

using ProgramFileDescriptorPtr = std::unique_ptr<ProgramFileDescriptor>;
using ProgramFileRevisionDescriptorPtr = std::shared_ptr<ProgramFileRevisionDescriptor>;
using ProgramComparisonDescriptorPtr = std::unique_ptr<ProgramComparisonDescriptor>;

inline constexpr uint32_t InvalidId = 0;
} // namespace unassemblize::gui
