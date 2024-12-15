/**
 * @file
 *
 * @brief Program File Revision Descriptor
 *
 * @copyright Unassemblize is free software: you can redistribute it and/or
 *            modify it under the terms of the GNU General Public License
 *            as published by the Free Software Foundation, either version
 *            3 of the License, or (at your option) any later version.
 *            A full copy of the GNU General Public License can be found in
 *            LICENSE
 */
#include "programfilerevisiondescriptor.h"
#include "executable.h"
#include "pdbreader.h"
#include <filesystem>
#include <fmt/core.h>

namespace unassemblize::gui
{
ProgramFileRevisionId ProgramFileRevisionDescriptor::s_id = 1;

ProgramFileRevisionDescriptor::ProgramFileRevisionDescriptor() : m_id(s_id++)
{
}

ProgramFileRevisionDescriptor::~ProgramFileRevisionDescriptor()
{
}

void ProgramFileRevisionDescriptor::add_async_work_hint(WorkQueueCommandId commandId, WorkReason reason)
{
    m_asyncWorkState.add({commandId, reason});
}

void ProgramFileRevisionDescriptor::remove_async_work_hint(WorkQueueCommandId commandId)
{
    m_asyncWorkState.remove(commandId);
}

bool ProgramFileRevisionDescriptor::has_async_work() const
{
    return !m_asyncWorkState.empty();
}

bool ProgramFileRevisionDescriptor::can_load_exe() const
{
    return !evaluate_exe_filename().empty();
}

bool ProgramFileRevisionDescriptor::can_load_pdb() const
{
    return !m_pdbFilenameCopy.empty();
}

bool ProgramFileRevisionDescriptor::can_save_exe_config() const
{
    return exe_loaded() && !evaluate_exe_config_filename().empty();
}

bool ProgramFileRevisionDescriptor::can_save_pdb_config() const
{
    return pdb_loaded() && !evaluate_pdb_config_filename().empty();
}

bool ProgramFileRevisionDescriptor::exe_loaded() const
{
    return m_executable != nullptr;
}

bool ProgramFileRevisionDescriptor::pdb_loaded() const
{
    return m_pdbReader != nullptr;
}

bool ProgramFileRevisionDescriptor::named_functions_built() const
{
    return m_namedFunctionsBuilt;
}

std::string ProgramFileRevisionDescriptor::evaluate_exe_filename() const
{
    if (is_auto_str(m_exeFilenameCopy))
    {
        return m_exeFilenameFromPdb;
    }
    else
    {
        return m_exeFilenameCopy;
    }
}

std::string ProgramFileRevisionDescriptor::evaluate_exe_config_filename() const
{
    return ::get_config_file_name(evaluate_exe_filename(), m_exeConfigFilenameCopy);
}

std::string ProgramFileRevisionDescriptor::evaluate_pdb_config_filename() const
{
    return ::get_config_file_name(m_pdbFilenameCopy, m_pdbConfigFilenameCopy);
}

std::string ProgramFileRevisionDescriptor::create_short_exe_name() const
{
    std::string name;
    if (m_executable != nullptr)
    {
        name = m_executable->get_filename();
    }
    else
    {
        name = evaluate_exe_filename();
        if (name.empty())
            name = m_exeFilenameCopy;
    }
    std::filesystem::path path(name);
    return path.filename().string();
}

std::string ProgramFileRevisionDescriptor::create_descriptor_name() const
{
    return fmt::format("Revision:{:d}", m_id);
}

std::string ProgramFileRevisionDescriptor::create_descriptor_name_with_file_info() const
{
    const std::string name = create_short_exe_name();
    if (name.empty())
    {
        return create_descriptor_name();
    }
    else
    {
        return fmt::format("Revision:{:d} - {:s}", m_id, name);
    }
}
} // namespace unassemblize::gui
