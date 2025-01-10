/**
 * @file
 *
 * @brief Class to instigate all high level functionality for unassemblize
 *
 * @copyright Unassemblize is free software: you can redistribute it and/or
 *            modify it under the terms of the GNU General Public License
 *            as published by the Free Software Foundation, either version
 *            3 of the License, or (at your option) any later version.
 *            A full copy of the GNU General Public License can be found in
 *            LICENSE
 */
#pragma once

#include "runneroptions.h"

namespace BS
{
class thread_pool;
}

namespace unassemblize
{
class Runner
{
public:
    Runner() = delete;

    static void set_thread_pool(BS::thread_pool *threadPool);

    static std::unique_ptr<Executable> load_exe(const LoadExeOptions &o);
    static std::unique_ptr<PdbReader> load_pdb(const LoadPdbOptions &o);

    static bool save_exe_config(const SaveExeConfigOptions &o);
    static bool save_pdb_config(const SavePdbConfigOptions &o);

    static NamedFunctions build_functions(const BuildFunctionsOptions &o);
    static MatchedFunctionsData build_matched_functions(const BuildMatchedFunctionsOptions &o);
    static std::vector<IndexT> build_unmatched_functions(const BuildUnmatchedFunctionsOptions &o);

    // Note: requires a prior call to build_matched_functions!
    static NamedFunctionBundles build_bundles_from_compilands(const BuildBundlesFromCompilandsOptions &o);
    // Note: requires a prior call to build_matched_functions!
    static NamedFunctionBundles build_bundles_from_source_files(const BuildBundlesFromSourceFilesOptions &o);
    // Create a single bundle with all functions.
    static NamedFunctionBundle build_single_bundle(const BuildSingleBundleOptions &o);

    static void disassemble_matched_functions(const DisassembleMatchedFunctionsOptions &o);
    static void disassemble_selected_functions(const DisassembleSelectedFunctionsOptions &o);
    // Can be used to disassemble a single function too.
    static void disassemble_functions(const DisassembleFunctionsOptions &o);

    static void build_source_lines_for_matched_functions(const BuildSourceLinesForMatchedFunctionsOptions &o);
    static void build_source_lines_for_selected_functions(const BuildSourceLinesForSelectedFunctionsOptions &o);
    // Can be used to build source lines of a single function too.
    static void build_source_lines_for_functions(const BuildSourceLinesForFunctionsOptions &o);

    // Note: requires a prior call to build_source_lines_for_functions!
    static bool load_source_files_for_matched_functions(const LoadSourceFilesForMatchedFunctionsOptions &o);
    // Note: requires a prior call to build_source_lines_for_functions!
    static bool load_source_files_for_selected_functions(const LoadSourceFilesForSelectedFunctionsOptions &o);
    // Note: requires a prior call to build_source_lines_for_functions!
    static bool load_source_files_for_functions(const LoadSourceFilesForFunctionsOptions &o);

    static void build_comparison_records_for_matched_functions(const BuildComparisonRecordsForMatchedFunctionsOptions &o);
    static void build_comparison_records_for_selected_functions(const BuildComparisonRecordsForSelectedFunctionsOptions &o);

    static bool process_asm_output(const AsmOutputOptions &o);
    static bool process_asm_comparison(const AsmComparisonOptions &o);

    static std::string create_exe_filename(const PdbExeInfo &info);

private:
    static bool in_code_section(const ExeSymbol &symbol, const Executable &executable);

    static MultiStringToIndexMapT build_function_name_to_index_map(const NamedFunctions &named_functions);
    static Address64ToIndexMapT build_function_address_to_index_map(const NamedFunctions &named_functions);

    static NamedFunctions build_functions(const Executable &executable);

    static MatchedFunctionsData build_matched_functions(ConstNamedFunctionsPair named_functions_pair);

    static std::vector<IndexT> build_unmatched_functions(
        const NamedFunctionMatchInfos &named_functions_match_infos,
        const MatchedFunctions &matched_functions);

    static NamedFunctionBundles build_bundles(
        const NamedFunctions &named_functions,
        const NamedFunctionMatchInfos &named_functions_match_infos,
        const MatchedFunctions &matched_functions,
        const PdbReader *bundling_pdb_reader,
        MatchBundleType bundle_type,
        size_t bundle_file_idx,
        BuildBundleFlags flags);

    // Note: requires a prior call to build_matched_functions!
    static NamedFunctionBundles build_bundles_from_compilands(
        const NamedFunctions &named_functions,
        const NamedFunctionMatchInfos &named_functions_match_infos,
        const PdbReader &pdb_reader,
        BuildBundleFlags flags);

    // Note: requires a prior call to build_matched_functions!
    static NamedFunctionBundles build_bundles_from_source_files(
        const NamedFunctions &named_functions,
        const NamedFunctionMatchInfos &named_functions_match_infos,
        const PdbReader &pdb_reader,
        BuildBundleFlags flags);

    // Creates a single bundle with all functions.
    static NamedFunctionBundle build_single_bundle(
        const NamedFunctionMatchInfos &named_functions_match_infos,
        const MatchedFunctions &matched_functions,
        size_t bundle_file_idx,
        BuildBundleFlags flags);

    template<class SourceInfoVectorT>
    static NamedFunctionBundles build_bundles(
        const SourceInfoVectorT &sources,
        const PdbFunctionInfoVector &functions,
        const NamedFunctions &named_functions,
        const NamedFunctionMatchInfos &named_functions_match_infos,
        BuildBundleFlags flags);

    template<class SourceInfoVectorT>
    static NamedFunctionBundle build_bundle(
        const SourceInfoVectorT &sources,
        IndexT source_idx,
        const PdbFunctionInfoVector &functions,
        const NamedFunctionMatchInfos &named_functions_match_infos,
        const Address64ToIndexMapT &named_function_to_index_map,
        BuildBundleFlags flags);

    static void disassemble_function(NamedFunction &named, const FunctionSetup &setup);
    static void disassemble_matched_function(
        NamedFunctionsPair named_functions_pair,
        const MatchedFunction &matched,
        std::array<const FunctionSetup *, 2> setup_pair);

    static void disassemble_matched_functions(
        NamedFunctionsPair named_functions_pair,
        const MatchedFunctions &matched_functions,
        ConstExecutablePair executable_pair,
        AsmFormat format);

    // Can be used to disassemble a complete bundle with two calls.
    static void disassemble_selected_functions(
        NamedFunctions &named_functions,
        span<const IndexT> named_function_indices,
        const Executable &executable,
        AsmFormat format);

    // Can be used to disassemble a single function too.
    static void disassemble_functions(span<NamedFunction> named_functions, const Executable &executable, AsmFormat format);

    static void build_source_lines_for_function(NamedFunction &named, const PdbReader &pdb_reader);

    static void build_source_lines_for_matched_functions(
        NamedFunctionsPair named_functions_pair,
        const MatchedFunctions &matched_functions,
        ConstPdbReaderPair pdb_reader_pair);

    // Can be used to build source lines of a complete bundle with two calls.
    static void build_source_lines_for_selected_functions(
        NamedFunctions &named_functions,
        span<const IndexT> named_function_indices,
        const PdbReader &pdb_reader);

    // Can be used to build source lines of a single function too.
    static void build_source_lines_for_functions(span<NamedFunction> named_functions, const PdbReader &pdb_reader);

    // Note: requires a prior call to build_source_lines_for_functions!
    static bool load_source_file_for_function(FileContentStorage &storage, const NamedFunction &named);
    static bool load_source_files_for_matched_function(
        FileContentStorage &storage,
        ConstNamedFunctionsPair named_functions_pair,
        const MatchedFunction &matched);

    // Note: requires a prior call to build_source_lines_for_functions!
    static bool load_source_files_for_matched_functions(
        FileContentStorage &storage,
        ConstNamedFunctionsPair named_functions_pair,
        const MatchedFunctions &matched_functions);

    // Note: requires a prior call to build_source_lines_for_functions!
    static bool load_source_files_for_selected_functions(
        FileContentStorage &storage,
        const NamedFunctions &named_functions,
        span<const IndexT> named_function_indices);

    // Note: requires a prior call to build_source_lines_for_functions!
    static bool load_source_files_for_functions(FileContentStorage &storage, span<NamedFunction> named_functions);

    static void build_comparison_record(
        MatchedFunction &matched,
        ConstNamedFunctionsPair named_functions_pair,
        uint32_t lookahead_limit);

    static void build_comparison_records_for_matched_functions(
        MatchedFunctions &matched_functions,
        ConstNamedFunctionsPair named_functions_pair,
        uint32_t lookahead_limit);

    static void build_comparison_records_for_selected_functions(
        MatchedFunctions &matched_functions,
        ConstNamedFunctionsPair named_functions_pair,
        span<const IndexT> matched_function_indices,
        uint32_t lookahead_limit);

    static bool output_comparison_results(
        ConstNamedFunctionsPair named_functions_pair,
        const MatchedFunctions &matched_functions,
        const NamedFunctionBundles &bundles,
        const FileContentStorage &source_file_storage,
        const AsmComparisonOptions &o);

    static std::string build_cmp_output_path(
        size_t bundle_idx,
        const std::string &bundle_name,
        const std::string &output_file);

private:
    static BS::thread_pool *s_threadPool;
};

} // namespace unassemblize
