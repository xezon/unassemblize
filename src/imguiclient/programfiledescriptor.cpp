/**
 * @file
 *
 * @brief Program File Descriptor
 *
 * @copyright Unassemblize is free software: you can redistribute it and/or
 *            modify it under the terms of the GNU General Public License
 *            as published by the Free Software Foundation, either version
 *            3 of the License, or (at your option) any later version.
 *            A full copy of the GNU General Public License can be found in
 *            LICENSE
 */
#include "programfiledescriptor.h"
#include "asyncworkstate.h"
#include "programfilerevisiondescriptor.h"
#include "workqueue.h"
#include <filesystem>
#include <fmt/core.h>

namespace unassemblize::gui
{
ProgramFileId ProgramFileDescriptor::s_id = 1;

ProgramFileDescriptor::ProgramFileDescriptor() : m_id(s_id++)
{
}

ProgramFileDescriptor::~ProgramFileDescriptor()
{
}

bool ProgramFileDescriptor::has_async_work() const
{
    return get_first_active_command_id() != InvalidWorkQueueCommandId;
}

WorkQueueCommandId ProgramFileDescriptor::get_first_active_command_id() const
{
    if (m_revisionDescriptor != nullptr && m_revisionDescriptor->has_async_work())
    {
        using WorkState = typename ProgramFileRevisionDescriptor::WorkState;
        using WorkReason = typename WorkState::WorkReason;

        constexpr uint64_t reasonMask = WorkState::get_reason_mask({WorkReason::Load, WorkReason::SaveConfig});

        WorkState::WorkQueueCommandIdArray<1> commandIdArray =
            m_revisionDescriptor->m_asyncWorkState.get_command_id_array<1>(reasonMask);

        if (commandIdArray.size >= 1)
            return commandIdArray.elements[0];
    }

    return InvalidWorkQueueCommandId;
}

bool ProgramFileDescriptor::can_load_exe() const
{
    return !evaluate_exe_filename().empty();
}

bool ProgramFileDescriptor::can_load_pdb() const
{
    return !m_pdbFilename.empty();
}

bool ProgramFileDescriptor::can_load() const
{
    return can_load_exe() || can_load_pdb();
}

bool ProgramFileDescriptor::can_save_exe_config() const
{
    return exe_loaded() && !evaluate_exe_config_filename().empty();
}

bool ProgramFileDescriptor::can_save_pdb_config() const
{
    return pdb_loaded() && !evaluate_pdb_config_filename().empty();
}

bool ProgramFileDescriptor::can_save_config() const
{
    return can_save_exe_config() || can_save_pdb_config();
}

bool ProgramFileDescriptor::exe_loaded() const
{
    return m_revisionDescriptor != nullptr && m_revisionDescriptor->exe_loaded();
}

bool ProgramFileDescriptor::pdb_loaded() const
{
    return m_revisionDescriptor != nullptr && m_revisionDescriptor->pdb_loaded();
}

std::string ProgramFileDescriptor::evaluate_exe_filename() const
{
    if (is_auto_str(m_exeFilename))
    {
        if (m_revisionDescriptor != nullptr)
            return m_revisionDescriptor->m_exeFilenameFromPdb;
        else
            return std::string();
    }
    else
    {
        return m_exeFilename;
    }
}

std::string ProgramFileDescriptor::evaluate_exe_config_filename() const
{
    return ::get_config_file_name(evaluate_exe_filename(), m_exeConfigFilename);
}

std::string ProgramFileDescriptor::evaluate_pdb_config_filename() const
{
    return ::get_config_file_name(m_pdbFilename, m_pdbConfigFilename);
}

std::string ProgramFileDescriptor::create_short_exe_name() const
{
    std::string name;
    if (m_revisionDescriptor != nullptr)
    {
        name = m_revisionDescriptor->create_short_exe_name();
    }
    else
    {
        name = evaluate_exe_filename();
        if (name.empty())
            name = m_exeFilename;
    }
    std::filesystem::path path(name);
    return path.filename().string();
}

std::string ProgramFileDescriptor::create_descriptor_name() const
{
    return fmt::format("File:{:d}", m_id);
}

std::string ProgramFileDescriptor::create_descriptor_name_with_file_info() const
{
    std::string revision;
    ProgramFileRevisionId revisionId = get_revision_id();
    if (revisionId != InvalidId)
    {
        revision = fmt::format(" - Revision:{:d}", revisionId);
    }

    const std::string name = create_short_exe_name();
    if (name.empty())
    {
        return create_descriptor_name();
    }
    else
    {
        return fmt::format("File:{:d}{:s} - {:s}", m_id, revision, name);
    }
}

ProgramFileRevisionId ProgramFileDescriptor::get_revision_id() const
{
    if (m_revisionDescriptor != nullptr)
        return m_revisionDescriptor->m_id;
    else
        return InvalidId;
}

void ProgramFileDescriptor::create_new_revision_descriptor()
{
    m_exeSymbolsFilter.reset();
    m_pdbSymbolsFilter.reset();
    m_pdbFunctionsFilter.reset();

    m_revisionDescriptor = std::make_shared<ProgramFileRevisionDescriptor>();
    m_revisionDescriptor->m_exeFilenameCopy = m_exeFilename;
    m_revisionDescriptor->m_exeConfigFilenameCopy = m_exeConfigFilename;
    m_revisionDescriptor->m_pdbFilenameCopy = m_pdbFilename;
    m_revisionDescriptor->m_pdbConfigFilenameCopy = m_pdbConfigFilename;
}
} // namespace unassemblize::gui
