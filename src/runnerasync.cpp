/**
 * @file
 *
 * @brief Asynchronous commands to instigate all high level functionality
 *        for unassemblize
 *
 * @copyright Unassemblize is free software: you can redistribute it and/or
 *            modify it under the terms of the GNU General Public License
 *            as published by the Free Software Foundation, either version
 *            3 of the License, or (at your option) any later version.
 *            A full copy of the GNU General Public License can be found in
 *            LICENSE
 */
#include "runnerasync.h"
#include "executable.h"
#include "pdbreader.h"
#include "runner.h"

namespace unassemblize
{
AsyncLoadExeCommand::AsyncLoadExeCommand(LoadExeOptions &&o) : options(std::move(o))
{
    WorkQueueCommand::work = [this]() {
        auto result = std::make_unique<AsyncLoadExeResult>();
        result->executable = Runner::load_exe(options);

        assert(result->executable == nullptr || result->executable->is_loaded());
        return result;
    };
};

AsyncLoadPdbCommand::AsyncLoadPdbCommand(LoadPdbOptions &&o) : options(std::move(o))
{
    WorkQueueCommand::work = [this]() {
        auto result = std::make_unique<AsyncLoadPdbResult>();
        result->pdbReader = Runner::load_pdb(options);

        assert(result->pdbReader == nullptr || result->pdbReader->is_loaded());
        return result;
    };
};

AsyncSaveExeConfigCommand::AsyncSaveExeConfigCommand(SaveExeConfigOptions &&o) : options(std::move(o))
{
    WorkQueueCommand::work = [this]() {
        auto result = std::make_unique<AsyncSaveExeConfigResult>();
        result->success = Runner::save_exe_config(options);
        return result;
    };
};

AsyncSavePdbConfigCommand::AsyncSavePdbConfigCommand(SavePdbConfigOptions &&o) : options(std::move(o))
{
    WorkQueueCommand::work = [this]() {
        auto result = std::make_unique<AsyncSavePdbConfigResult>();
        result->success = Runner::save_pdb_config(options);
        return result;
    };
};

AsyncBuildFunctionsCommand::AsyncBuildFunctionsCommand(BuildFunctionsOptions &&o) : options(std::move(o))
{
    WorkQueueCommand::work = [this]() {
        auto result = std::make_unique<AsyncBuildFunctionsResult>();
        result->named_functions = Runner::build_functions(options);
        return result;
    };
}

AsyncBuildMatchedFunctionsCommand::AsyncBuildMatchedFunctionsCommand(BuildMatchedFunctionsOptions &&o) :
    options(std::move(o))
{
    WorkQueueCommand::work = [this]() {
        auto result = std::make_unique<AsyncBuildMatchedFunctionsResult>();
        result->matchedFunctionsData = Runner::build_matched_functions(options);
        return result;
    };
}

AsyncBuildUnmatchedFunctionsCommand::AsyncBuildUnmatchedFunctionsCommand(BuildUnmatchedFunctionsOptions &&o) :
    options(std::move(o))
{
    WorkQueueCommand::work = [this]() {
        auto result = std::make_unique<AsyncBuildUnmatchedFunctionsResult>();
        result->unmatched_function_indices = Runner::build_unmatched_functions(options);
        return result;
    };
}

AsyncBuildBundlesFromCompilandsCommand::AsyncBuildBundlesFromCompilandsCommand(BuildBundlesFromCompilandsOptions &&o) :
    options(std::move(o))
{
    WorkQueueCommand::work = [this]() {
        auto result = std::make_unique<AsyncBuildBundlesFromCompilandsResult>();
        result->bundles = Runner::build_bundles_from_compilands(options);
        return result;
    };
}

AsyncBuildBundlesFromSourceFilesCommand::AsyncBuildBundlesFromSourceFilesCommand(BuildBundlesFromSourceFilesOptions &&o) :
    options(std::move(o))
{
    WorkQueueCommand::work = [this]() {
        auto result = std::make_unique<AsyncBuildBundlesFromSourceFilesResult>();
        result->bundles = Runner::build_bundles_from_source_files(options);
        return result;
    };
}

AsyncBuildSingleBundleCommand::AsyncBuildSingleBundleCommand(BuildSingleBundleOptions &&o) : options(std::move(o))
{
    WorkQueueCommand::work = [this]() {
        auto result = std::make_unique<AsyncBuildSingleBundleResult>();
        result->bundle = Runner::build_single_bundle(options);
        return result;
    };
}

AsyncDisassembleMatchedFunctionsCommand::AsyncDisassembleMatchedFunctionsCommand(DisassembleMatchedFunctionsOptions &&o) :
    options(std::move(o))
{
    WorkQueueCommand::work = [this]() {
        auto result = std::make_unique<AsyncDisassembleMatchedFunctionsResult>();
        Runner::disassemble_matched_functions(options);
        return result;
    };
}

AsyncDisassembleSelectedFunctionsCommand::AsyncDisassembleSelectedFunctionsCommand(DisassembleSelectedFunctionsOptions &&o) :
    options(std::move(o))
{
    WorkQueueCommand::work = [this]() {
        auto result = std::make_unique<AsyncDisassembleSelectedFunctionsResult>();
        Runner::disassemble_selected_functions(options);
        return result;
    };
}

AsyncDisassembleFunctionsCommand::AsyncDisassembleFunctionsCommand(DisassembleFunctionsOptions &&o) : options(std::move(o))
{
    WorkQueueCommand::work = [this]() {
        auto result = std::make_unique<AsyncDisassembleFunctionsResult>();
        Runner::disassemble_functions(options);
        return result;
    };
}

AsyncBuildSourceLinesForMatchedFunctionsCommand::AsyncBuildSourceLinesForMatchedFunctionsCommand(
    BuildSourceLinesForMatchedFunctionsOptions &&o) :
    options(std::move(o))
{
    WorkQueueCommand::work = [this]() {
        auto result = std::make_unique<AsyncBuildSourceLinesForMatchedFunctionsResult>();
        Runner::build_source_lines_for_matched_functions(options);
        return result;
    };
}

AsyncBuildSourceLinesForSelectedFunctionsCommand::AsyncBuildSourceLinesForSelectedFunctionsCommand(
    BuildSourceLinesForSelectedFunctionsOptions &&o) :
    options(std::move(o))
{
    WorkQueueCommand::work = [this]() {
        auto result = std::make_unique<AsyncBuildSourceLinesForSelectedFunctionsResult>();
        Runner::build_source_lines_for_selected_functions(options);
        return result;
    };
}

AsyncBuildSourceLinesForFunctionsCommand::AsyncBuildSourceLinesForFunctionsCommand(BuildSourceLinesForFunctionsOptions &&o) :
    options(std::move(o))
{
    WorkQueueCommand::work = [this]() {
        auto result = std::make_unique<AsyncBuildSourceLinesForFunctionsResult>();
        Runner::build_source_lines_for_functions(options);
        return result;
    };
}

AsyncLoadSourceFilesForMatchedFunctionsCommand::AsyncLoadSourceFilesForMatchedFunctionsCommand(
    LoadSourceFilesForMatchedFunctionsOptions &&o) :
    options(std::move(o))
{
    WorkQueueCommand::work = [this]() {
        auto result = std::make_unique<AsyncLoadSourceFilesForMatchedFunctionsResult>();
        result->success = Runner::load_source_files_for_matched_functions(options);
        return result;
    };
}

AsyncLoadSourceFilesForSelectedFunctionsCommand::AsyncLoadSourceFilesForSelectedFunctionsCommand(
    LoadSourceFilesForSelectedFunctionsOptions &&o) :
    options(std::move(o))
{
    WorkQueueCommand::work = [this]() {
        auto result = std::make_unique<AsyncLoadSourceFilesForSelectedFunctionsResult>();
        result->success = Runner::load_source_files_for_selected_functions(options);
        return result;
    };
}

AsyncLoadSourceFilesForFunctionsCommand::AsyncLoadSourceFilesForFunctionsCommand(LoadSourceFilesForFunctionsOptions &&o) :
    options(std::move(o))
{
    WorkQueueCommand::work = [this]() {
        auto result = std::make_unique<AsyncLoadSourceFilesForFunctionsResult>();
        result->success = Runner::load_source_files_for_functions(options);
        return result;
    };
}

AsyncBuildComparisonRecordsForMatchedFunctionsCommand::AsyncBuildComparisonRecordsForMatchedFunctionsCommand(
    BuildComparisonRecordsForMatchedFunctionsOptions &&o) :
    options(std::move(o))
{
    WorkQueueCommand::work = [this]() {
        auto result = std::make_unique<AsyncBuildComparisonRecordsForMatchedFunctionsResult>();
        Runner::build_comparison_records_for_matched_functions(options);
        return result;
    };
}

AsyncBuildComparisonRecordsForSelectedFunctionsCommand::AsyncBuildComparisonRecordsForSelectedFunctionsCommand(
    BuildComparisonRecordsForSelectedFunctionsOptions &&o) :
    options(std::move(o))
{
    WorkQueueCommand::work = [this]() {
        auto result = std::make_unique<AsyncBuildComparisonRecordsForSelectedFunctionsResult>();
        Runner::build_comparison_records_for_selected_functions(options);
        return result;
    };
}

AsyncProcessAsmOutputCommand::AsyncProcessAsmOutputCommand(AsmOutputOptions &&o) : options(std::move(o))
{
    WorkQueueCommand::work = [this]() {
        auto result = std::make_unique<AsyncProcessAsmOutputResult>();
        result->success = Runner::process_asm_output(options);
        return result;
    };
};

AsyncProcessAsmComparisonCommand::AsyncProcessAsmComparisonCommand(AsmComparisonOptions &&o) : options(std::move(o))
{
    WorkQueueCommand::work = [this]() {
        auto result = std::make_unique<AsyncProcessAsmComparisonResult>();
        result->success = Runner::process_asm_comparison(options);
        return result;
    };
};

} // namespace unassemblize
