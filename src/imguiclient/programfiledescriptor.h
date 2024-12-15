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
#pragma once

#include "options.h"
#include "programfilecommon.h"
#include "utility/imgui_text_filter.h"

namespace unassemblize
{
struct ExeSymbol;
struct PdbSymbolInfo;
struct PdbFunctionInfo;
} // namespace unassemblize

namespace unassemblize::gui
{
struct ProgramFileDescriptor
{
    ProgramFileDescriptor();
    ~ProgramFileDescriptor();

    bool has_async_work() const;
    WorkQueueCommandId get_first_active_command_id() const;

    bool can_load_exe() const;
    bool can_load_pdb() const;
    bool can_load() const;
    bool can_save_exe_config() const;
    bool can_save_pdb_config() const;
    bool can_save_config() const;

    bool exe_loaded() const;
    bool pdb_loaded() const;

    std::string evaluate_exe_filename() const;
    std::string evaluate_exe_config_filename() const;
    std::string evaluate_pdb_config_filename() const;

    std::string create_short_exe_name() const;
    std::string create_descriptor_name() const;
    std::string create_descriptor_name_with_file_info() const;

    ProgramFileRevisionId get_revision_id() const;

    void create_new_revision_descriptor();

    // Note: All members must be modified by UI thread only

    const ProgramFileId m_id = InvalidId;

    // Must be not editable when the WorkQueue thread works on this descriptor.
    std::string m_exeFilename;
    std::string m_exeConfigFilename = auto_str;
    std::string m_pdbFilename;
    std::string m_pdbConfigFilename = auto_str;

    TextFilterDescriptor<const ExeSymbol *> m_exeSymbolsFilter = "exe_symbols_filter";
    TextFilterDescriptor<const PdbSymbolInfo *> m_pdbSymbolsFilter = "pdb_symbols_filter";
    TextFilterDescriptor<const PdbFunctionInfo *> m_pdbFunctionsFilter = "pdb_functions_filter";

    ProgramFileRevisionDescriptorPtr m_revisionDescriptor;

private:
    static ProgramFileId s_id;
};
} // namespace unassemblize::gui
