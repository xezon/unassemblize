/**
 * @file
 *
 * @brief Option structures to configure all high level functionality
 *        for unassemblize
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

namespace unassemblize
{
class FileContentStorage;

struct LoadExeOptions
{
    LoadExeOptions(const std::string &input_file) : input_file(input_file) {}

    const std::string input_file;
    std::string config_file;
    const PdbReader *pdb_reader = nullptr;
    bool verbose = false;
};

struct SaveExeConfigOptions
{
    SaveExeConfigOptions(const Executable &executable, const std::string &config_file) :
        executable(executable), config_file(config_file)
    {
    }

    const Executable &executable;
    const std::string config_file;
};

struct LoadPdbOptions
{
    LoadPdbOptions(const std::string &input_file) : input_file(input_file) {}

    const std::string input_file;
    bool verbose = false;
};

struct SavePdbConfigOptions
{
    SavePdbConfigOptions(const PdbReader &pdb_reader, const std::string &config_file) :
        pdb_reader(pdb_reader), config_file(config_file)
    {
    }

    const PdbReader &pdb_reader;
    const std::string config_file;
    bool overwrite_sections = false;
};

struct AsmOutputOptions
{
    AsmOutputOptions(const Executable &executable, const std::string &output_file, uint64_t start_addr, uint64_t end_addr) :
        executable(executable), output_file(output_file), start_addr(start_addr), end_addr(end_addr)
    {
    }

    const Executable &executable;
    const std::string output_file;
    const uint64_t start_addr;
    const uint64_t end_addr;
    AsmFormat format = AsmFormat::IGAS;
    uint32_t print_indent_len = 4;
};

struct AsmComparisonOptions
{
    AsmComparisonOptions(
        ConstExecutablePair executable_pair,
        ConstPdbReaderPair pdb_reader_pair,
        const std::string &output_file) :
        executable_pair(executable_pair), pdb_reader_pair(pdb_reader_pair), output_file(output_file)
    {
    }

    const Executable &get_executable(size_t index) const { return *executable_pair[index]; }

    const PdbReader *bundling_pdb_reader() const
    {
        if (bundle_file_idx < pdb_reader_pair.size())
            return pdb_reader_pair[bundle_file_idx];
        else
            return nullptr;
    }

    const ConstExecutablePair executable_pair;
    const ConstPdbReaderPair pdb_reader_pair;
    const std::string output_file;
    AsmFormat format = AsmFormat::IGAS;
    MatchBundleType bundle_type = MatchBundleType::None; // The method to group symbols with.
    size_t bundle_file_idx = 0;
    uint32_t print_indent_len = 4;
    uint32_t print_asm_len = 80;
    uint32_t print_byte_count = 11;
    uint32_t print_sourcecode_len = 80;
    uint32_t print_sourceline_len = 5;
    uint32_t lookahead_limit = 20;
    AsmMatchStrictness match_strictness = AsmMatchStrictness::Undecided;
};

struct BuildFunctionsOptions
{
    BuildFunctionsOptions(const Executable &executable) : executable(executable) {}

    const Executable &executable;
};

struct BuildMatchedFunctionsOptions
{
    BuildMatchedFunctionsOptions(ConstNamedFunctionsPair pair) : named_functions_pair(pair) {}

    const ConstNamedFunctionsPair named_functions_pair;
};

struct BuildUnmatchedFunctionsOptions
{
    BuildUnmatchedFunctionsOptions(
        const NamedFunctionMatchInfos &named_functions_match_infos,
        const MatchedFunctions &matched_functions) :
        named_functions_match_infos(named_functions_match_infos), matched_functions(matched_functions)
    {
    }

    const NamedFunctionMatchInfos &named_functions_match_infos;
    const MatchedFunctions &matched_functions;
};

enum BuildBundleFlags : uint8_t
{
    BuildMatchedFunctionIndices = 1 << 0,
    BuildMatchedNamedFunctionIndices = 1 << 1,
    BuildUnmatchedNamedFunctionIndices = 1 << 2,
    BuildAllNamedFunctionIndices = 1 << 3,

    BuildBundleFlagsAll = 255u,
};

struct BuildBundlesFromCompilandsOptions
{
    BuildBundlesFromCompilandsOptions(
        const NamedFunctions &named_functions,
        const NamedFunctionMatchInfos &named_functions_match_infos,
        const PdbReader &pdb_reader) :
        named_functions(named_functions), named_functions_match_infos(named_functions_match_infos), pdb_reader(pdb_reader)
    {
    }

    const NamedFunctions &named_functions;
    const NamedFunctionMatchInfos &named_functions_match_infos;
    const PdbReader &pdb_reader;
    uint8_t flags = BuildBundleFlagsAll;
};

struct BuildBundlesFromSourceFilesOptions
{
    BuildBundlesFromSourceFilesOptions(
        const NamedFunctions &named_functions,
        const NamedFunctionMatchInfos &named_functions_match_infos,
        const PdbReader &pdb_reader) :
        named_functions(named_functions), named_functions_match_infos(named_functions_match_infos), pdb_reader(pdb_reader)
    {
    }

    const NamedFunctions &named_functions;
    const NamedFunctionMatchInfos &named_functions_match_infos;
    const PdbReader &pdb_reader;
    uint8_t flags = BuildBundleFlagsAll;
};

struct BuildSingleBundleOptions
{
    BuildSingleBundleOptions(
        const NamedFunctionMatchInfos &named_functions_match_infos,
        const MatchedFunctions &matched_functions,
        size_t bundle_file_idx) :
        named_functions_match_infos(named_functions_match_infos),
        matched_functions(matched_functions),
        bundle_file_idx(bundle_file_idx)
    {
    }

    const NamedFunctionMatchInfos &named_functions_match_infos;
    const MatchedFunctions &matched_functions;
    const size_t bundle_file_idx;
    uint8_t flags = BuildBundleFlagsAll;
};

struct DisassembleMatchedFunctionsOptions
{
    DisassembleMatchedFunctionsOptions(
        NamedFunctionsPair named_functions_pair,
        const MatchedFunctions &matched_functions,
        ConstExecutablePair executable_pair) :
        named_functions_pair(named_functions_pair), matched_functions(matched_functions), executable_pair(executable_pair)
    {
    }

    const NamedFunctionsPair named_functions_pair;
    const MatchedFunctions &matched_functions;
    const ConstExecutablePair executable_pair;
    AsmFormat format = AsmFormat::IGAS;
};

struct DisassembleSelectedFunctionsOptions
{
    DisassembleSelectedFunctionsOptions(
        NamedFunctions &named_functions,
        span<const IndexT> named_function_indices,
        const Executable &executable) :
        named_functions(named_functions), named_function_indices(named_function_indices), executable(executable)
    {
    }

    NamedFunctions &named_functions;
    const span<const IndexT> named_function_indices;
    const Executable &executable;
    AsmFormat format = AsmFormat::IGAS;
};

struct DisassembleFunctionsOptions
{
    DisassembleFunctionsOptions(span<NamedFunction> named_functions, const Executable &executable) :
        named_functions(named_functions), executable(executable)
    {
    }

    const span<NamedFunction> named_functions;
    const Executable &executable;
    AsmFormat format = AsmFormat::IGAS;
};

struct BuildSourceLinesForMatchedFunctionsOptions
{
    BuildSourceLinesForMatchedFunctionsOptions(
        NamedFunctionsPair named_functions_pair,
        const MatchedFunctions &matched_functions,
        ConstPdbReaderPair pdb_reader_pair) :
        named_functions_pair(named_functions_pair), matched_functions(matched_functions), pdb_reader_pair(pdb_reader_pair)
    {
    }

    const NamedFunctionsPair named_functions_pair;
    const MatchedFunctions &matched_functions;
    const ConstPdbReaderPair pdb_reader_pair;
};

struct BuildSourceLinesForSelectedFunctionsOptions
{
    BuildSourceLinesForSelectedFunctionsOptions(
        NamedFunctions &named_functions,
        span<const IndexT> named_function_indices,
        const PdbReader &pdb_reader) :
        named_functions(named_functions), named_function_indices(named_function_indices), pdb_reader(pdb_reader)
    {
    }

    NamedFunctions &named_functions;
    const span<const IndexT> named_function_indices;
    const PdbReader &pdb_reader;
};

struct BuildSourceLinesForFunctionsOptions
{
    BuildSourceLinesForFunctionsOptions(span<NamedFunction> named_functions, const PdbReader &pdb_reader) :
        named_functions(named_functions), pdb_reader(pdb_reader)
    {
    }

    const span<NamedFunction> named_functions;
    const PdbReader &pdb_reader;
};

struct LoadSourceFilesForMatchedFunctionsOptions
{
    LoadSourceFilesForMatchedFunctionsOptions(
        FileContentStorage &storage,
        ConstNamedFunctionsPair named_functions_pair,
        const MatchedFunctions &matched_functions) :
        storage(storage), named_functions_pair(named_functions_pair), matched_functions(matched_functions)
    {
    }

    FileContentStorage &storage;
    const ConstNamedFunctionsPair named_functions_pair;
    const MatchedFunctions &matched_functions;
};

struct LoadSourceFilesForSelectedFunctionsOptions
{
    LoadSourceFilesForSelectedFunctionsOptions(
        FileContentStorage &storage,
        const NamedFunctions &named_functions,
        span<const IndexT> named_function_indices) :
        storage(storage), named_functions(named_functions), named_function_indices(named_function_indices)
    {
    }

    FileContentStorage &storage;
    const NamedFunctions &named_functions;
    const span<const IndexT> named_function_indices;
};

struct LoadSourceFilesForFunctionsOptions
{
    LoadSourceFilesForFunctionsOptions(FileContentStorage &storage, span<NamedFunction> named_functions) :
        storage(storage), named_functions(named_functions)
    {
    }

    FileContentStorage &storage;
    const span<NamedFunction> named_functions;
};

struct BuildComparisonRecordsForMatchedFunctionsOptions
{
    BuildComparisonRecordsForMatchedFunctionsOptions(
        MatchedFunctions &matched_functions,
        ConstNamedFunctionsPair named_functions_pair) :
        matched_functions(matched_functions), named_functions_pair(named_functions_pair)
    {
    }

    MatchedFunctions &matched_functions;
    const ConstNamedFunctionsPair named_functions_pair;
    uint32_t lookahead_limit = 20;
};

struct BuildComparisonRecordsForSelectedFunctionsOptions
{
    BuildComparisonRecordsForSelectedFunctionsOptions(
        MatchedFunctions &matched_functions,
        ConstNamedFunctionsPair named_functions_pair,
        span<const IndexT> matched_function_indices) :
        matched_functions(matched_functions),
        named_functions_pair(named_functions_pair),
        matched_function_indices(matched_function_indices)
    {
    }

    MatchedFunctions &matched_functions;
    const ConstNamedFunctionsPair named_functions_pair;
    const span<const IndexT> matched_function_indices;
    uint32_t lookahead_limit = 20;
};

} // namespace unassemblize
