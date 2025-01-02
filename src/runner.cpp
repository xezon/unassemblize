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
#include "runner.h"
#include "asmmatcher.h"
#include "asmprinter.h"
#include "executable.h"
#include "filecontentstorage.h"
#include "pdbreader.h"
#include "util.h"
#include <filesystem>
#include <fmt/core.h>
#include <fstream>

namespace unassemblize
{
namespace
{
inline ConstFunctionPair to_const_function_pair(ConstNamedFunctionsPair named_functions_pair, const MatchedFunction &matched)
{
    return ConstFunctionPair{
        &named_functions_pair[0]->at(matched.named_idx_pair[0]).function,
        &named_functions_pair[1]->at(matched.named_idx_pair[1]).function};
}

inline NamedFunctionPair to_named_function_pair(NamedFunctionsPair named_functions_pair, const MatchedFunction &matched)
{
    return NamedFunctionPair{
        &named_functions_pair[0]->at(matched.named_idx_pair[0]),
        &named_functions_pair[1]->at(matched.named_idx_pair[1])};
}

inline ConstNamedFunctionPair to_const_named_function_pair(
    ConstNamedFunctionsPair named_functions_pair,
    const MatchedFunction &matched)
{
    return ConstNamedFunctionPair{
        &named_functions_pair[0]->at(matched.named_idx_pair[0]),
        &named_functions_pair[1]->at(matched.named_idx_pair[1])};
}
} // namespace

std::unique_ptr<Executable> Runner::load_exe(const LoadExeOptions &o)
{
    assert(!o.input_file.empty());

    if (o.verbose)
    {
        printf("Loading executable file '%s'...\n", o.input_file.c_str());
    }

    auto executable = std::make_unique<Executable>();
    executable->set_verbose(o.verbose);

    if (!executable->load(o.input_file))
    {
        executable.reset();
        return executable;
    }

    constexpr bool pdb_symbols_overwrite_exe_symbols = true; // Make configurable?
    constexpr bool cfg_symbols_overwrite_exe_pdb_symbols = true; // Make configurable?

    if (o.pdb_reader != nullptr)
    {
        const PdbSymbolInfoVector &pdb_symbols = o.pdb_reader->get_symbols();

        if (!pdb_symbols.empty())
        {
            executable->add_symbols(pdb_symbols, pdb_symbols_overwrite_exe_symbols);
        }
    }

    if (!o.config_file.empty())
    {
        executable->load_config(o.config_file.c_str(), cfg_symbols_overwrite_exe_pdb_symbols);
    }

    return executable;
}

std::unique_ptr<PdbReader> Runner::load_pdb(const LoadPdbOptions &o)
{
    assert(!o.input_file.empty());

    if (o.verbose)
    {
        printf("Loading Pdb file '%s'...\n", o.input_file.c_str());
    }

    auto pdb_reader = std::make_unique<PdbReader>();
    pdb_reader->set_verbose(o.verbose);

    // Currently does not read back config file here.

    if (!pdb_reader->load(o.input_file))
    {
        pdb_reader.reset();
    }

    return pdb_reader;
}

bool Runner::save_exe_config(const SaveExeConfigOptions &o)
{
    assert(!o.config_file.empty());

    return o.executable.save_config(o.config_file.c_str());
}

bool Runner::save_pdb_config(const SavePdbConfigOptions &o)
{
    assert(!o.config_file.empty());

    return o.pdb_reader.save_config(o.config_file, o.overwrite_sections);
}

NamedFunctions Runner::build_functions(const BuildFunctionsOptions &o)
{
    return build_functions(o.executable);
}

MatchedFunctionsData Runner::build_matched_functions(const BuildMatchedFunctionsOptions &o)
{
    return build_matched_functions(o.named_functions_pair);
}

std::vector<IndexT> Runner::build_unmatched_functions(const BuildUnmatchedFunctionsOptions &o)
{
    return build_unmatched_functions(o.named_functions_match_infos, o.matched_functions);
}

NamedFunctionBundles Runner::build_bundles_from_compilands(const BuildBundlesFromCompilandsOptions &o)
{
    return build_bundles_from_compilands(o.named_functions, o.named_functions_match_infos, o.pdb_reader, o.flags);
}

NamedFunctionBundles Runner::build_bundles_from_source_files(const BuildBundlesFromSourceFilesOptions &o)
{
    return build_bundles_from_source_files(o.named_functions, o.named_functions_match_infos, o.pdb_reader, o.flags);
}

NamedFunctionBundle Runner::build_single_bundle(const BuildSingleBundleOptions &o)
{
    return build_single_bundle(o.named_functions_match_infos, o.matched_functions, o.bundle_file_idx, o.flags);
}

void Runner::disassemble_matched_functions(const DisassembleMatchedFunctionsOptions &o)
{
    disassemble_matched_functions(o.named_functions_pair, o.matched_functions, o.executable_pair, o.format);
}

void Runner::disassemble_selected_functions(const DisassembleSelectedFunctionsOptions &o)
{
    disassemble_selected_functions(o.named_functions, o.named_function_indices, o.executable, o.format);
}

void Runner::disassemble_functions(const DisassembleFunctionsOptions &o)
{
    disassemble_functions(o.named_functions, o.executable, o.format);
}

void Runner::build_source_lines_for_matched_functions(const BuildSourceLinesForMatchedFunctionsOptions &o)
{
    build_source_lines_for_matched_functions(o.named_functions_pair, o.matched_functions, o.pdb_reader_pair);
}

void Runner::build_source_lines_for_selected_functions(const BuildSourceLinesForSelectedFunctionsOptions &o)
{
    build_source_lines_for_selected_functions(o.named_functions, o.named_function_indices, o.pdb_reader);
}

void Runner::build_source_lines_for_functions(const BuildSourceLinesForFunctionsOptions &o)
{
    build_source_lines_for_functions(o.named_functions, o.pdb_reader);
}

bool Runner::load_source_files_for_matched_functions(const LoadSourceFilesForMatchedFunctionsOptions &o)
{
    return load_source_files_for_matched_functions(o.storage, o.named_functions_pair, o.matched_functions);
}

bool Runner::load_source_files_for_selected_functions(const LoadSourceFilesForSelectedFunctionsOptions &o)
{
    return load_source_files_for_selected_functions(o.storage, o.named_functions, o.named_function_indices);
}

bool Runner::load_source_files_for_functions(const LoadSourceFilesForFunctionsOptions &o)
{
    return load_source_files_for_functions(o.storage, o.named_functions);
}

void Runner::build_comparison_records_for_matched_functions(const BuildComparisonRecordsForMatchedFunctionsOptions &o)
{
    build_comparison_records_for_matched_functions(o.matched_functions, o.named_functions_pair, o.lookahead_limit);
}

void Runner::build_comparison_records_for_selected_functions(const BuildComparisonRecordsForSelectedFunctionsOptions &o)
{
    build_comparison_records_for_selected_functions(
        o.matched_functions,
        o.named_functions_pair,
        o.matched_function_indices,
        o.lookahead_limit);
}

bool Runner::process_asm_output(const AsmOutputOptions &o)
{
    if (!(o.start_addr < o.end_addr))
    {
        return false;
    }

    if (o.format == AsmFormat::MASM)
    {
        return false;
    }

    if (!o.executable.is_loaded())
    {
        return false;
    }

    std::ofstream fs(o.output_file, std::ofstream::binary);
    if (!fs.is_open())
    {
        return false;
    }

    const FunctionSetup setup(o.executable, o.format);
    Function func;
    func.disassemble(setup, o.start_addr, o.end_addr);

    std::string text;
    AsmPrinter::append_to_string(text, o.executable, func, o.print_indent_len);
    fs.write(text.data(), text.size());

    return true;
}

bool Runner::process_asm_comparison(const AsmComparisonOptions &o)
{
    assert(o.executable_pair[0] != nullptr && o.executable_pair[0]->is_loaded());
    assert(o.executable_pair[1] != nullptr && o.executable_pair[1]->is_loaded());

    bool ok = true;

    std::array<NamedFunctions, 2> named_functions;
    NamedFunctionsPair named_functions_pair = {&named_functions[0], &named_functions[1]};
    ConstNamedFunctionsPair const_named_functions_pair = {&named_functions[0], &named_functions[1]};

    for (size_t i = 0; i < named_functions.size(); ++i)
    {
        named_functions[i] = build_functions(o.get_executable(i));
    }

    MatchedFunctionsData matched_data = build_matched_functions(const_named_functions_pair);

    NamedFunctionBundles bundles = build_bundles(
        named_functions[o.bundle_file_idx],
        matched_data.namedFunctionMatchInfosArray[o.bundle_file_idx],
        matched_data.matchedFunctions,
        o.bundling_pdb_reader(),
        o.bundle_type,
        o.bundle_file_idx,
        BuildMatchedFunctionIndices);

    disassemble_matched_functions(named_functions_pair, matched_data.matchedFunctions, o.executable_pair, o.format);

    FileContentStorage source_file_storage;

    if (o.print_sourceline_len + o.print_sourcecode_len > 0)
    {
        build_source_lines_for_matched_functions(named_functions_pair, matched_data.matchedFunctions, o.pdb_reader_pair);
        load_source_files_for_matched_functions(
            source_file_storage,
            const_named_functions_pair,
            matched_data.matchedFunctions);
    }

    build_comparison_records_for_matched_functions(
        matched_data.matchedFunctions,
        const_named_functions_pair,
        o.lookahead_limit);

    ok = output_comparison_results(
        const_named_functions_pair,
        matched_data.matchedFunctions,
        bundles,
        source_file_storage,
        o);

    return ok;
}

bool Runner::in_code_section(const ExeSymbol &symbol, const Executable &executable)
{
    const ExeSectionInfo *code_section = executable.get_code_section();
    return symbol.address >= code_section->address && symbol.address < code_section->address + code_section->size;
}

MultiStringToIndexMapT Runner::build_function_name_to_index_map(const NamedFunctions &named_functions)
{
    // Using multimap, because there can be multiple symbols sharing the same name.
    MultiStringToIndexMapT map;
    const size_t size = named_functions.size();
    map.reserve(size);

    for (IndexT i = 0; i < size; ++i)
    {
        map.emplace(named_functions[i].name, i);
    }
    return map;
}

Address64ToIndexMapT Runner::build_function_address_to_index_map(const NamedFunctions &named_functions)
{
    const size_t size = named_functions.size();
    Address64ToIndexMapT map;
    map.reserve(size);

    for (IndexT i = 0; i < size; ++i)
    {
        const Address64T address = named_functions[i].function.get_begin_address();
        [[maybe_unused]] auto [_, added] = map.try_emplace(address, i);
        assert(added);
    }
    return map;
}

NamedFunctions Runner::build_functions(const Executable &executable)
{
    const ExeSymbols &symbols = executable.get_symbols();

    NamedFunctions named_functions;
    named_functions.reserve(symbols.size());

    for (const ExeSymbol &symbol : symbols)
    {
        if (!in_code_section(symbol, executable))
        {
            continue;
        }

        named_functions.emplace_back();

        NamedFunction &named = named_functions.back();
        named.id = named_functions.size() - 1;
        named.name = symbol.name;
        named.function.set_address_range(symbol.address, symbol.address + symbol.size);
    }

    named_functions.shrink_to_fit();

    return named_functions;
}

MatchedFunctionsData Runner::build_matched_functions(ConstNamedFunctionsPair named_functions_pair)
{
    const Side less_side = named_functions_pair[0]->size() < named_functions_pair[1]->size() ? LeftSide : RightSide;
    const Side more_side = get_opposite_side(less_side);
    const NamedFunctions &less_named_functions = *named_functions_pair[less_side];
    const NamedFunctions &more_named_functions = *named_functions_pair[more_side];
    const MultiStringToIndexMapT less_named_functions_to_index_map = build_function_name_to_index_map(less_named_functions);
    const MultiStringToIndexMapT more_named_functions_to_index_map = build_function_name_to_index_map(more_named_functions);
    const size_t less_named_size = less_named_functions.size();
    const size_t more_named_size = more_named_functions.size();

    MatchedFunctionsData result;
    result.matchedFunctions.reserve(more_named_size);
    NamedFunctionMatchInfos &lessNamedFunctionMatchInfos = result.namedFunctionMatchInfosArray[less_side];
    NamedFunctionMatchInfos &moreNamedFunctionMatchInfos = result.namedFunctionMatchInfosArray[more_side];
    lessNamedFunctionMatchInfos.resize(less_named_size);
    moreNamedFunctionMatchInfos.resize(more_named_size);

    for (size_t less_named_idx = 0; less_named_idx < less_named_size; ++less_named_idx)
    {
        const NamedFunction &less_named_function = less_named_functions[less_named_idx];

        const auto more_pair = more_named_functions_to_index_map.equal_range(less_named_function.name);
        if (std::distance(more_pair.first, more_pair.second) != 1)
        {
            // No symbol or multiple symbols with this name. Skip.
            continue;
        }

        const auto less_pair = less_named_functions_to_index_map.equal_range(less_named_function.name);
        if (std::distance(less_pair.first, less_pair.second) != 1)
        {
            // Multiple symbols with this name. Skip.
            continue;
        }

        const IndexT matched_index = result.matchedFunctions.size();
        result.matchedFunctions.emplace_back();
        MatchedFunction &matched = result.matchedFunctions.back();
        matched.named_idx_pair[less_side] = less_named_idx;
        matched.named_idx_pair[more_side] = more_pair.first->second;

        lessNamedFunctionMatchInfos[less_named_idx].matched_index = matched_index;
        moreNamedFunctionMatchInfos[more_pair.first->second].matched_index = matched_index;
    }

    result.matchedFunctions.shrink_to_fit();

    return result;
}

std::vector<IndexT> Runner::build_unmatched_functions(
    const NamedFunctionMatchInfos &named_functions_match_infos,
    const MatchedFunctions &matched_functions)
{
    const size_t named_size = named_functions_match_infos.size();
    const size_t matched_size = matched_functions.size();
    assert(named_size >= matched_size);
    const size_t unmatched_size = named_size - matched_size;

    std::vector<IndexT> unmatched_functions;
    unmatched_functions.resize(unmatched_size);
    size_t unmatched_idx = 0;

    for (size_t named_idx = 0; named_idx < named_size; ++named_idx)
    {
        if (!named_functions_match_infos[named_idx].is_matched())
        {
            unmatched_functions[unmatched_idx++] = named_idx;
        }
    }

    return unmatched_functions;
}

NamedFunctionBundles Runner::build_bundles(
    const NamedFunctions &named_functions,
    const NamedFunctionMatchInfos &named_functions_match_infos,
    const MatchedFunctions &matched_functions,
    const PdbReader *bundling_pdb_reader,
    MatchBundleType bundle_type,
    size_t bundle_file_idx,
    BuildBundleFlags flags)
{
    NamedFunctionBundles bundles;

    switch (bundle_type)
    {
        case MatchBundleType::Compiland: {
            bundles =
                build_bundles_from_compilands(named_functions, named_functions_match_infos, *bundling_pdb_reader, flags);
            break;
        }
        case MatchBundleType::SourceFile: {
            bundles =
                build_bundles_from_source_files(named_functions, named_functions_match_infos, *bundling_pdb_reader, flags);
            break;
        }
    }

    if (bundles.empty())
    {
        bundles.resize(1);
        bundles[0] = build_single_bundle(named_functions_match_infos, matched_functions, bundle_file_idx, flags);
    }

    return bundles;
}

NamedFunctionBundles Runner::build_bundles_from_compilands(
    const NamedFunctions &named_functions,
    const NamedFunctionMatchInfos &named_functions_match_infos,
    const PdbReader &pdb_reader,
    BuildBundleFlags flags)
{
    const PdbCompilandInfoVector &compilands = pdb_reader.get_compilands();
    const PdbFunctionInfoVector &functions = pdb_reader.get_functions();

    return build_bundles(compilands, functions, named_functions, named_functions_match_infos, flags);
}

NamedFunctionBundles Runner::build_bundles_from_source_files(
    const NamedFunctions &named_functions,
    const NamedFunctionMatchInfos &named_functions_match_infos,
    const PdbReader &pdb_reader,
    BuildBundleFlags flags)
{
    const PdbSourceFileInfoVector &sources = pdb_reader.get_source_files();
    const PdbFunctionInfoVector &functions = pdb_reader.get_functions();

    return build_bundles(sources, functions, named_functions, named_functions_match_infos, flags);
}

NamedFunctionBundle Runner::build_single_bundle(
    const NamedFunctionMatchInfos &named_functions_match_infos,
    const MatchedFunctions &matched_functions,
    size_t bundle_file_idx,
    BuildBundleFlags flags)
{
    assert(bundle_file_idx < 2);

    NamedFunctionBundle bundle;
    bundle.id = 0;
    bundle.name = "all";
    bundle.flags = flags;

    if (flags & BuildMatchedFunctionIndices)
    {
        const size_t count = matched_functions.size();
        bundle.matchedFunctionIndices.resize(count);
        for (size_t i = 0; i < count; ++i)
        {
            bundle.matchedFunctionIndices[i] = i;
        }
    }

    if (flags & BuildMatchedNamedFunctionIndices)
    {
        const size_t count = matched_functions.size();
        bundle.matchedNamedFunctionIndices.resize(count);
        for (size_t i = 0; i < count; ++i)
        {
            bundle.matchedNamedFunctionIndices[i] = matched_functions[i].named_idx_pair[bundle_file_idx];
        }
    }

    if (flags & BuildUnmatchedNamedFunctionIndices)
    {
        bundle.unmatchedNamedFunctionIndices = build_unmatched_functions(named_functions_match_infos, matched_functions);
    }

    if (flags & BuildAllNamedFunctionIndices)
    {
        const size_t count = named_functions_match_infos.size();
        bundle.allNamedFunctionIndices.resize(count);
        for (size_t i = 0; i < count; ++i)
        {
            bundle.allNamedFunctionIndices[i] = i;
        }
    }

    return bundle;
}

template<class SourceInfoVectorT>
NamedFunctionBundles Runner::build_bundles(
    const SourceInfoVectorT &sources,
    const PdbFunctionInfoVector &functions,
    const NamedFunctions &named_functions,
    const NamedFunctionMatchInfos &named_functions_match_infos,
    BuildBundleFlags flags)
{
    const Address64ToIndexMapT named_function_to_index_map = build_function_address_to_index_map(named_functions);
    const IndexT sources_count = sources.size();
    NamedFunctionBundles bundles;
    bundles.resize(sources_count);

    for (IndexT source_idx = 0; source_idx < sources_count; ++source_idx)
    {
        bundles[source_idx] =
            build_bundle(sources, source_idx, functions, named_functions_match_infos, named_function_to_index_map, flags);
    }

    return bundles;
}

template<class SourceInfoVectorT>
NamedFunctionBundle Runner::build_bundle(
    const SourceInfoVectorT &sources,
    IndexT source_idx,
    const PdbFunctionInfoVector &functions,
    const NamedFunctionMatchInfos &named_functions_match_infos,
    const Address64ToIndexMapT &named_function_to_index_map,
    BuildBundleFlags flags)
{
    const typename SourceInfoVectorT::value_type &source = sources[source_idx];
    const IndexT function_count = source.functionIds.size();
    NamedFunctionBundle bundle;
    bundle.id = source_idx;
    bundle.name = source.name;
    bundle.flags = flags;

    constexpr uint8_t buildIndicesFlags = BuildMatchedFunctionIndices | BuildMatchedNamedFunctionIndices
        | BuildUnmatchedNamedFunctionIndices | BuildAllNamedFunctionIndices;

    if (flags & buildIndicesFlags)
    {
        if (flags & BuildMatchedFunctionIndices)
            bundle.matchedFunctionIndices.reserve(function_count);
        if (flags & BuildMatchedNamedFunctionIndices)
            bundle.matchedNamedFunctionIndices.reserve(function_count);
        if (flags & BuildUnmatchedNamedFunctionIndices)
            bundle.unmatchedNamedFunctionIndices.reserve(function_count);
        if (flags & BuildAllNamedFunctionIndices)
            bundle.allNamedFunctionIndices.reserve(function_count);

        for (IndexT function_idx = 0; function_idx < function_count; ++function_idx)
        {
            const PdbFunctionInfo &function_info = functions[source.functionIds[function_idx]];
            const Address64ToIndexMapT::const_iterator it =
                named_function_to_index_map.find(function_info.address.absVirtual);

            if (it != named_function_to_index_map.cend())
            {
                IndexT named_idx = it->second;
                const NamedFunctionMatchInfo &matchInfo = named_functions_match_infos[named_idx];
                if (matchInfo.is_matched())
                {
                    if (flags & BuildMatchedFunctionIndices)
                        bundle.matchedFunctionIndices.push_back(matchInfo.matched_index);
                    if (flags & BuildMatchedNamedFunctionIndices)
                        bundle.matchedNamedFunctionIndices.push_back(named_idx);
                }
                else
                {
                    if (flags & BuildUnmatchedNamedFunctionIndices)
                        bundle.unmatchedNamedFunctionIndices.push_back(named_idx);
                }
                if (flags & BuildAllNamedFunctionIndices)
                    bundle.allNamedFunctionIndices.push_back(named_idx);
            }
            else
            {
                assert(false);
            }
        }

        bundle.matchedFunctionIndices.shrink_to_fit();
        bundle.matchedNamedFunctionIndices.shrink_to_fit();
        bundle.unmatchedNamedFunctionIndices.shrink_to_fit();
    }

    return bundle;
}

void Runner::disassemble_function(NamedFunction &named, const FunctionSetup &setup)
{
    assert(!named.isDisassembled);

    named.function.disassemble(setup);
    named.isDisassembled = true;
}

void Runner::disassemble_matched_functions(
    NamedFunctionsPair named_functions_pair,
    const MatchedFunctions &matched_functions,
    ConstExecutablePair executable_pair,
    AsmFormat format)
{
    const FunctionSetup setup0(*executable_pair[0], format);
    const FunctionSetup setup1(*executable_pair[1], format);

    for (const MatchedFunction &matched : matched_functions)
    {
        NamedFunctionPair named_pair = to_named_function_pair(named_functions_pair, matched);
        disassemble_function(*named_pair[0], setup0);
        disassemble_function(*named_pair[1], setup1);
    }
}

void Runner::disassemble_selected_functions(
    NamedFunctions &named_functions,
    span<const IndexT> named_function_indices,
    const Executable &executable,
    AsmFormat format)
{
    const FunctionSetup setup(executable, format);

    for (IndexT index : named_function_indices)
    {
        disassemble_function(named_functions[index], setup);
    }
}

void Runner::disassemble_functions(span<NamedFunction> named_functions, const Executable &executable, AsmFormat format)
{
    const FunctionSetup setup(executable, format);

    for (NamedFunction &named : named_functions)
    {
        disassemble_function(named, setup);
    }
}

void Runner::build_source_lines_for_function(NamedFunction &named, const PdbReader &pdb_reader)
{
    assert(named.isLinkedToSourceFile == TriState::False);

    const Address64T address = named.function.get_begin_address();
    const PdbFunctionInfo *pdb_function = pdb_reader.find_function_by_address(address);

    if (pdb_function != nullptr && pdb_function->has_valid_source_file_id())
    {
        const PdbSourceFileInfoVector &source_files = pdb_reader.get_source_files();
        const PdbSourceFileInfo &source_file = source_files[pdb_function->sourceFileId];
        named.function.set_source_file(source_file, pdb_function->sourceLines);
        named.isLinkedToSourceFile = TriState::True;
    }
    else
    {
        named.isLinkedToSourceFile = TriState::NotApplicable;
    }
}

void Runner::build_source_lines_for_matched_functions(
    NamedFunctionsPair named_functions_pair,
    const MatchedFunctions &matched_functions,
    ConstPdbReaderPair pdb_reader_pair)
{
    for (size_t i = 0; i < 2; ++i)
    {
        if (const PdbReader *pdb_reader = pdb_reader_pair[i])
        {
            for (const MatchedFunction &matched : matched_functions)
            {
                NamedFunction &named = named_functions_pair[i]->at(matched.named_idx_pair[i]);
                build_source_lines_for_function(named, *pdb_reader);
            }
        }
        else
        {
            for (const MatchedFunction &matched : matched_functions)
            {
                NamedFunction &named = named_functions_pair[i]->at(matched.named_idx_pair[i]);
                named.isLinkedToSourceFile = TriState::NotApplicable;
            }
        }
    }
}

void Runner::build_source_lines_for_selected_functions(
    NamedFunctions &named_functions,
    span<const IndexT> named_function_indices,
    const PdbReader &pdb_reader)
{
    for (IndexT index : named_function_indices)
    {
        build_source_lines_for_function(named_functions[index], pdb_reader);
    }
}

void Runner::build_source_lines_for_functions(span<NamedFunction> named_functions, const PdbReader &pdb_reader)
{
    for (NamedFunction &named : named_functions)
    {
        build_source_lines_for_function(named, pdb_reader);
    }
}

bool Runner::load_source_file_for_function(FileContentStorage &storage, const NamedFunction &named)
{
    if (named.isLinkedToSourceFile == TriState::NotApplicable)
    {
        // Has no source file associated. Treat as success.
        return true;
    }

    assert(named.isLinkedToSourceFile == TriState::True);

    FileContentStorage::LoadResult result = storage.load_content(named.function.get_source_file_name());
    return result != FileContentStorage::LoadResult::Failed;
}

bool Runner::load_source_files_for_matched_functions(
    FileContentStorage &storage,
    ConstNamedFunctionsPair named_functions_pair,
    const MatchedFunctions &matched_functions)
{
    bool success = true;
    for (const MatchedFunction &matched : matched_functions)
    {
        for (size_t i = 0; i < 2; ++i)
        {
            const NamedFunction &named = named_functions_pair[i]->at(matched.named_idx_pair[i]);
            success &= load_source_file_for_function(storage, named);
        }
    }
    return success;
}

bool Runner::load_source_files_for_selected_functions(
    FileContentStorage &storage,
    const NamedFunctions &named_functions,
    span<const IndexT> named_function_indices)
{
    bool success = true;
    for (IndexT index : named_function_indices)
    {
        success &= load_source_file_for_function(storage, named_functions[index]);
    }
    return success;
}

bool Runner::load_source_files_for_functions(FileContentStorage &storage, span<NamedFunction> named_functions)
{
    bool success = true;
    for (NamedFunction &named : named_functions)
    {
        success &= load_source_file_for_function(storage, named);
    }
    return success;
}

void Runner::build_comparison_record(
    MatchedFunction &matched,
    ConstNamedFunctionsPair named_functions_pair,
    uint32_t lookahead_limit)
{
    if (matched.is_compared())
        return;

    ConstFunctionPair function_pair = to_const_function_pair(named_functions_pair, matched);
    matched.comparison = AsmMatcher::run_comparison(function_pair, lookahead_limit);
}

void Runner::build_comparison_records_for_matched_functions(
    MatchedFunctions &matched_functions,
    ConstNamedFunctionsPair named_functions_pair,
    uint32_t lookahead_limit)
{
    for (MatchedFunction &matched : matched_functions)
    {
        build_comparison_record(matched, named_functions_pair, lookahead_limit);
    }
}

void Runner::build_comparison_records_for_selected_functions(
    MatchedFunctions &matched_functions,
    ConstNamedFunctionsPair named_functions_pair,
    span<const IndexT> matched_function_indices,
    uint32_t lookahead_limit)
{
    for (IndexT index : matched_function_indices)
    {
        build_comparison_record(matched_functions[index], named_functions_pair, lookahead_limit);
    }
}

bool Runner::output_comparison_results(
    ConstNamedFunctionsPair named_functions_pair,
    const MatchedFunctions &matched_functions,
    const NamedFunctionBundles &bundles,
    const FileContentStorage &source_file_storage,
    const AsmComparisonOptions &o)
{
    size_t file_write_count = 0;
    size_t bundle_idx = 0;

    for (const NamedFunctionBundle &bundle : bundles)
    {
        std::string output_file_variant = build_cmp_output_path(bundle_idx, bundle.name, o.output_file);

        std::ofstream fs(output_file_variant, std::ofstream::binary);
        if (fs.is_open())
        {
            AsmPrinter printer;
            std::string text;
            text.reserve(1024 * 1024);

            for (IndexT i : bundle.matchedFunctionIndices)
            {
                const MatchedFunction &matched = matched_functions[i];
                ConstNamedFunctionPair named_function_pair = to_const_named_function_pair(named_functions_pair, matched);
                const std::string &source_file0 = named_function_pair[0]->function.get_source_file_name();
                const std::string &source_file1 = named_function_pair[1]->function.get_source_file_name();

                TextFileContentPair source_file_texts;
                source_file_texts.pair[0] = source_file_storage.find_content(source_file0);
                source_file_texts.pair[1] = source_file_storage.find_content(source_file1);

                text.clear();
                printer.append_to_string(
                    text,
                    matched.comparison,
                    named_function_pair,
                    o.executable_pair,
                    source_file_texts,
                    o.match_strictness,
                    o.print_indent_len,
                    o.print_asm_len,
                    o.print_byte_count,
                    o.print_sourcecode_len,
                    o.print_sourceline_len);

                fs.write(text.data(), text.size());
            }
            ++file_write_count;
        }
    }

    return file_write_count == bundles.size();
}

std::string Runner::create_exe_filename(const PdbExeInfo &info)
{
    assert(!info.exeFileName.empty());
    assert(!info.pdbFilePath.empty());

    std::filesystem::path path = info.pdbFilePath;
    path = path.parent_path();
    path /= info.exeFileName;
    if (!path.has_extension())
    {
        path += ".exe";
    }

    return path.string();
}

std::string Runner::build_cmp_output_path(size_t bundle_idx, const std::string &bundle_name, const std::string &output_file)
{
    const std::filesystem::path bundle_path = bundle_name;
    const std::filesystem::path output_path = output_file;

    const std::string filename = fmt::format(
        "{:s}.{:s}.{:d}{:s}",
        output_path.stem().string(),
        bundle_path.filename().string(),
        bundle_idx++,
        output_path.extension().string());

    const std::filesystem::path path = output_path.parent_path() / filename;
    return path.string();
}

} // namespace unassemblize
