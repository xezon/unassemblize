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
#pragma once

#include "asyncworkstate.h"
#include "filecontentstorage.h"
#include "processedstate.h"
#include "programfilecommon.h"
#include <chrono>

namespace unassemblize::gui
{
// Note: Pass down a shared pointer of the ProgramFileRevisionDescriptor when chaining async commands.
struct ProgramFileRevisionDescriptor
{
    static constexpr std::chrono::system_clock::time_point InvalidTimePoint = std::chrono::system_clock::time_point::min();

    enum class WorkReason
    {
        Load,
        SaveConfig,
        BuildNamedFunctions,
        DisassembleSelectedFunctions,
        BuildSourceLinesForSelectedFunctions,
        LoadSourceFilesForSelectedFunctions,
    };

    using WorkState = AsyncWorkState<WorkReason>;

    ProgramFileRevisionDescriptor();
    ~ProgramFileRevisionDescriptor();

    void add_async_work_hint(WorkQueueCommandId commandId, WorkReason reason);
    void remove_async_work_hint(WorkQueueCommandId commandId);
    bool has_async_work() const;

    bool can_load_exe() const;
    bool can_load_pdb() const;
    bool can_save_exe_config() const;
    bool can_save_pdb_config() const;

    bool exe_loaded() const;
    bool pdb_loaded() const;

    bool named_functions_built() const;

    std::string evaluate_exe_filename() const;
    std::string evaluate_exe_config_filename() const;
    std::string evaluate_pdb_config_filename() const;

    std::string create_short_exe_name() const;
    std::string create_descriptor_name() const;
    std::string create_descriptor_name_with_file_info() const;

    const ProgramFileRevisionId m_id = InvalidId;

    WorkState m_asyncWorkState;

    // String copies of the file descriptor at the time of async command chain creation.
    // These allows to evaluate async save load operations without a dependency to the file descriptor.
    std::string m_exeFilenameCopy;
    std::string m_exeConfigFilenameCopy;
    std::string m_pdbFilenameCopy;
    std::string m_pdbConfigFilenameCopy;

    std::unique_ptr<Executable> m_executable;
    std::unique_ptr<PdbReader> m_pdbReader;
    std::string m_exeFilenameFromPdb;
    std::string m_exeSaveConfigFilename;
    std::string m_pdbSaveConfigFilename;

    std::chrono::time_point<std::chrono::system_clock> m_exeLoadTimepoint = InvalidTimePoint;
    std::chrono::time_point<std::chrono::system_clock> m_exeSaveConfigTimepoint = InvalidTimePoint;
    std::chrono::time_point<std::chrono::system_clock> m_pdbLoadTimepoint = InvalidTimePoint;
    std::chrono::time_point<std::chrono::system_clock> m_pdbSaveConfigTimepoint = InvalidTimePoint;

    NamedFunctions m_namedFunctions;

    // Stores named functions that have been prepared for async processing already. Links to NamedFunctions.
    ProcessedState m_processedNamedFunctions;

    FileContentStorage m_fileContentStrorage;

    bool m_namedFunctionsBuilt = false;

private:
    static ProgramFileRevisionId s_id;
};
} // namespace unassemblize::gui
