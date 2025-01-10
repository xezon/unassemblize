/**
 * @file
 *
 * @brief Program Comparison Descriptor
 *
 * @copyright Unassemblize is free software: you can redistribute it and/or
 *            modify it under the terms of the GNU General Public License
 *            as published by the Free Software Foundation, either version
 *            3 of the License, or (at your option) any later version.
 *            A full copy of the GNU General Public License can be found in
 *            LICENSE
 */
#include "programcomparisondescriptor.h"
#include "programfilerevisiondescriptor.h"
#include "util.h"
#include <fmt/core.h>
#include <unordered_set>

namespace unassemblize::gui
{
ProgramComparisonId ProgramComparisonDescriptor::s_id = 1;

ProgramComparisonDescriptor::ProgramComparisonDescriptor() : m_id(s_id++)
{
}

ProgramComparisonDescriptor::~ProgramComparisonDescriptor()
{
}

ProgramComparisonDescriptor::File::File()
{
    for (ImGuiSelectionBasicStorage &selection : m_imguiBundlesSelectionArray)
    {
        selection.SetItemSelected(ImGuiID(0), true);
    }
}

void ProgramComparisonDescriptor::File::ListItemUiInfo::update_info(
    const std::string &itemName,
    uint32_t itemId,
    bool hasMatchedFunction,
    std::optional<int8_t> similarity)
{
    const bool similarityValueChanged = similarity.has_value() && m_similarity != similarity;

    if (m_label.empty() || similarityValueChanged)
    {
        if (hasMatchedFunction)
        {
            if (similarity.has_value())
            {
                m_label = fmt::format("[M:{:3d}%] {:s}##item{:d}", similarity.value(), itemName, itemId);
            }
            else
            {
                m_label = fmt::format("[M] {:s}##item{:d}", itemName, itemId);
            }
        }
        else
        {
            m_label = fmt::format("{:s}##item{:d}", itemName, itemId);
        }

        m_similarity = similarity;
    }
}

void ProgramComparisonDescriptor::File::prepare_rebuild()
{
    assert(!has_async_work());

    m_bundlesFilter.Reset();
    m_functionIndicesFilter.Reset();

    m_revisionDescriptor.reset();

    util::free_container(m_namedFunctionMatchInfos);
    util::free_container(m_compilandBundles);
    util::free_container(m_sourceFileBundles);
    m_singleBundle = NamedFunctionBundle();

    util::free_container(m_namedFunctionUiInfos);
    util::free_container(m_compilandBundleUiInfos);
    util::free_container(m_sourceFileBundleUiInfos);
    m_singleBundleUiInfo = NamedFunctionBundleUiInfo();

    m_compilandBundlesBuilt = TriState::False;
    m_sourceFileBundlesBuilt = TriState::False;
    m_singleBundleBuilt = false;

    util::free_container(m_selectedBundles);
    util::free_container(m_activeNamedFunctionIndices);
    util::free_container(m_selectedNamedFunctionIndices);
    util::free_container(m_selectedUnmatchedNamedFunctionIndices);
}

void ProgramComparisonDescriptor::File::init()
{
    assert(named_functions_built());

    m_namedFunctionUiInfos.resize(m_revisionDescriptor->m_namedFunctions.size());
    m_compilandBundleUiInfos.resize(m_compilandBundles.size());
    m_sourceFileBundleUiInfos.resize(m_sourceFileBundles.size());

    for (IndexT i = 0; i < IndexT(MatchBundleType::Count); ++i)
    {
        update_bundle_ui_infos(MatchBundleType(i));
    }
}

void ProgramComparisonDescriptor::File::add_async_work_hint(WorkQueueCommandId commandId, WorkReason reason)
{
    m_asyncWorkState.add({commandId, reason});
}

void ProgramComparisonDescriptor::File::remove_async_work_hint(WorkQueueCommandId commandId)
{
    m_asyncWorkState.remove(commandId);
}

bool ProgramComparisonDescriptor::File::has_async_work() const
{
    return get_first_active_command_id() != InvalidWorkQueueCommandId;
}

WorkQueueCommandId ProgramComparisonDescriptor::File::get_first_active_command_id() const
{
    if (m_revisionDescriptor != nullptr && m_revisionDescriptor->has_async_work())
    {
        using WorkState = typename ProgramFileRevisionDescriptor::WorkState;
        using WorkReason = typename WorkState::WorkReason;

        constexpr uint64_t reasonMask = WorkState::get_reason_mask(
            {WorkReason::Load,
             WorkReason::BuildNamedFunctions,
             WorkReason::DisassembleSelectedFunctions,
             WorkReason::BuildSourceLinesForSelectedFunctions,
             WorkReason::LoadSourceFilesForSelectedFunctions});

        WorkState::WorkQueueCommandIdArray<1> commandIdArray =
            m_revisionDescriptor->m_asyncWorkState.get_command_id_array<1>(reasonMask);

        if (commandIdArray.size() >= 1)
            return commandIdArray[0];
    }

    if (!m_asyncWorkState.empty())
    {
        return m_asyncWorkState.get()[0].commandId;
    }

    return InvalidWorkQueueCommandId;
}

bool ProgramComparisonDescriptor::File::exe_loaded() const
{
    return m_revisionDescriptor != nullptr && m_revisionDescriptor->exe_loaded();
}

bool ProgramComparisonDescriptor::File::pdb_loaded() const
{
    return m_revisionDescriptor != nullptr && m_revisionDescriptor->pdb_loaded();
}

bool ProgramComparisonDescriptor::File::named_functions_built() const
{
    return m_revisionDescriptor != nullptr && m_revisionDescriptor->named_functions_built();
}

bool ProgramComparisonDescriptor::File::bundles_ready() const
{
    return m_compilandBundlesBuilt != TriState::False && m_sourceFileBundlesBuilt != TriState::False && m_singleBundleBuilt;
}

span<const IndexT> ProgramComparisonDescriptor::File::get_matched_function_indices() const
{
    assert(m_singleBundle.flags & BuildMatchedFunctionIndices);
    return {m_singleBundle.matchedFunctionIndices};
}

span<const IndexT> ProgramComparisonDescriptor::File::get_unmatched_named_function_indices() const
{
    assert(m_singleBundle.flags & BuildUnmatchedNamedFunctionIndices);
    return {m_singleBundle.unmatchedNamedFunctionIndices};
}

bool ProgramComparisonDescriptor::File::is_matched_function(IndexT namedFunctionIndex) const
{
    assert(namedFunctionIndex < m_namedFunctionMatchInfos.size());
    return m_namedFunctionMatchInfos[namedFunctionIndex].is_matched();
}

MatchBundleType ProgramComparisonDescriptor::File::get_selected_bundle_type() const
{
    static_assert(MatchBundleType::Compiland == MatchBundleType(0), "Unexpected value");
    static_assert(MatchBundleType::SourceFile == MatchBundleType(1), "Unexpected value");

    int index = 0;

    if (m_compilandBundlesBuilt == TriState::True)
    {
        if (index++ == m_imguiSelectedBundleTypeIdx)
            return MatchBundleType::Compiland;
    }

    if (m_sourceFileBundlesBuilt == TriState::True)
    {
        if (index++ == m_imguiSelectedBundleTypeIdx)
            return MatchBundleType::SourceFile;
    }

    return MatchBundleType::None;
}

span<const NamedFunctionBundle> ProgramComparisonDescriptor::File::get_bundles(MatchBundleType type) const
{
    span<const NamedFunctionBundle> bundles;
    switch (type)
    {
        case MatchBundleType::Compiland:
            bundles = {m_compilandBundles};
            break;
        case MatchBundleType::SourceFile:
            bundles = {m_sourceFileBundles};
            break;
        case MatchBundleType::None:
            bundles = {&m_singleBundle, 1};
            break;
    }
    static_assert(size_t(MatchBundleType::Count) == 3, "Enum was changed. Update switch case.");

    return bundles;
}

span<ProgramComparisonDescriptor::File::NamedFunctionBundleUiInfo> ProgramComparisonDescriptor::File::get_bundle_ui_infos(
    MatchBundleType type)
{
    span<NamedFunctionBundleUiInfo> bundle_ui_infos;
    switch (type)
    {
        case MatchBundleType::Compiland:
            bundle_ui_infos = {m_compilandBundleUiInfos};
            break;
        case MatchBundleType::SourceFile:
            bundle_ui_infos = {m_sourceFileBundleUiInfos};
            break;
        case MatchBundleType::None:
            bundle_ui_infos = {&m_singleBundleUiInfo, 1};
            break;
    }
    static_assert(size_t(MatchBundleType::Count) == 3, "Enum was changed. Update switch case.");

    return bundle_ui_infos;
}

span<const ProgramComparisonDescriptor::File::NamedFunctionBundleUiInfo> ProgramComparisonDescriptor::File::
    get_bundle_ui_infos(MatchBundleType type) const
{
    return const_cast<ProgramComparisonDescriptor::File *>(this)->get_bundle_ui_infos(type);
}

ImGuiSelectionBasicStorage &ProgramComparisonDescriptor::File::get_bundles_selection(MatchBundleType type)
{
    return m_imguiBundlesSelectionArray[size_t(type)];
}

const NamedFunctionBundle &ProgramComparisonDescriptor::File::get_filtered_bundle(int index) const
{
    const auto filtered = m_bundlesFilter.Filtered();
    assert(index < filtered.size());
    return *filtered[index];
}

const ProgramComparisonDescriptor::File::NamedFunctionBundleUiInfo &ProgramComparisonDescriptor::File::
    get_filtered_bundle_ui_info(int index) const
{
    const span<const NamedFunctionBundleUiInfo> bundleUiInfos = get_bundle_ui_infos(get_selected_bundle_type());
    const IndexT bundleIndex = get_filtered_bundle(index).id;
    assert(bundleIndex < bundleUiInfos.size());
    return bundleUiInfos[bundleIndex];
}

void ProgramComparisonDescriptor::File::on_bundles_changed()
{
    m_bundlesFilter.Reset();
}

void ProgramComparisonDescriptor::File::on_bundles_interaction()
{
    m_functionIndicesFilter.Reset();

    update_selected_bundles();
    update_active_named_functions();
    // Perhaps the ui infos should be build once earlier and not on every bundle interaction?
    update_active_named_function_ui_infos();
}

void ProgramComparisonDescriptor::File::update_bundle_ui_infos(MatchBundleType type)
{
    const NamedFunctions &namedFunctions = m_revisionDescriptor->m_namedFunctions;
    const span<const NamedFunctionBundle> bundles = get_bundles(type);
    const span<NamedFunctionBundleUiInfo> bundleUiInfos = get_bundle_ui_infos(type);
    const IndexT bundleCount = bundles.size();
    assert(bundleUiInfos.size() == bundleCount);

    for (IndexT i = 0; i < bundleCount; ++i)
    {
        const NamedFunctionBundle &bundle = bundles[i];
        NamedFunctionBundleUiInfo &uiInfo = bundleUiInfos[i];
        const bool hasMatchedFunction = !bundle.matchedFunctionIndices.empty();

        uiInfo.update_info(bundle.name, bundle.id, hasMatchedFunction);
    }
}

void ProgramComparisonDescriptor::File::update_selected_bundles()
{
    const MatchBundleType type = get_selected_bundle_type();
    span<const NamedFunctionBundle> activeBundles = get_bundles(type);
    ImGuiSelectionBasicStorage &selection = get_bundles_selection(type);

    std::vector<const NamedFunctionBundle *> selectedBundles;
    selectedBundles.reserve(selection.Size);

    // Uses lookup set. Is much faster than linear search over elements.
    const auto filtered = m_bundlesFilter.Filtered();
    const std::unordered_set<const NamedFunctionBundle *> filteredSet(filtered.begin(), filtered.end());

    void *it = nullptr;
    ImGuiID id;
    while (selection.GetNextSelectedItem(&it, &id))
    {
        assert(IndexT(id) < activeBundles.size());
        const NamedFunctionBundle *bundle = &activeBundles[IndexT(id)];

        if (filteredSet.count(bundle) == 0)
            continue;

        selectedBundles.push_back(bundle);
    }

    m_selectedBundles = std::move(selectedBundles);
}

void ProgramComparisonDescriptor::File::update_active_named_functions()
{
    std::vector<IndexT> activeAllNamedFunctions;

    if (m_selectedBundles.size() > 1)
    {
#ifndef RELEASE
        std::unordered_set<IndexT> activeAllNamedFunctionsSet;
#endif
        {
            size_t activeAllNamedFunctionsCount = 0;

            for (const NamedFunctionBundle *bundle : m_selectedBundles)
            {
                activeAllNamedFunctionsCount += bundle->allNamedFunctionIndices.size();
            }
            activeAllNamedFunctions.reserve(activeAllNamedFunctionsCount);
#ifndef RELEASE
            activeAllNamedFunctionsSet.reserve(activeAllNamedFunctionsCount);
#endif
        }

        for (const NamedFunctionBundle *bundle : m_selectedBundles)
        {
            for (IndexT index : bundle->allNamedFunctionIndices)
            {
#ifndef RELEASE
                assert(activeAllNamedFunctionsSet.count(index) == 0);
                activeAllNamedFunctionsSet.emplace(index);
#endif
                activeAllNamedFunctions.push_back(index);
            }
        }
    }
    m_activeNamedFunctionIndices = std::move(activeAllNamedFunctions);
}

void ProgramComparisonDescriptor::File::update_active_named_function_ui_infos()
{
    update_named_function_ui_infos(get_active_named_function_indices());
}

void ProgramComparisonDescriptor::File::update_named_function_ui_infos(span<const IndexT> namedFunctionIndices)
{
    for (IndexT functionIndex : namedFunctionIndices)
    {
        const NamedFunction &namedFunction = m_revisionDescriptor->m_namedFunctions[functionIndex];
        const NamedFunctionMatchInfo &matchInfo = m_namedFunctionMatchInfos[functionIndex];
        ListItemUiInfo &uiInfo = m_namedFunctionUiInfos[functionIndex];

        uiInfo.update_info(namedFunction.name, namedFunction.id, matchInfo.is_matched());
    }
}

void ProgramComparisonDescriptor::File::update_selected_named_functions()
{
    assert(named_functions_built());

    std::vector<IndexT> selectedAllNamedFunctionIndices;
    std::vector<IndexT> selectedUnmatchedNamedFunctionIndices;

    // Reservation size typically matches selection size, but could be larger.
    selectedAllNamedFunctionIndices.reserve(m_imguiFunctionsSelection.Size);
    selectedUnmatchedNamedFunctionIndices.reserve(m_imguiFunctionsSelection.Size);

    // Uses lookup set. Is much faster than linear search over elements.
    const auto filtered = m_functionIndicesFilter.Filtered();
    const std::unordered_set<IndexT> filteredSet(filtered.begin(), filtered.end());

    void *it = nullptr;
    ImGuiID id;
    while (m_imguiFunctionsSelection.GetNextSelectedItem(&it, &id))
    {
        if (filteredSet.count(IndexT(id)) == 0)
            continue;

        selectedAllNamedFunctionIndices.push_back(IndexT(id));

        if (!is_matched_function(IndexT(id)))
        {
            selectedUnmatchedNamedFunctionIndices.push_back(IndexT(id));
        }
    }

    m_selectedNamedFunctionIndices = std::move(selectedAllNamedFunctionIndices);
    m_selectedUnmatchedNamedFunctionIndices = std::move(selectedUnmatchedNamedFunctionIndices);
    m_selectedUnmatchedNamedFunctionIndices.shrink_to_fit();
}

span<const IndexT> ProgramComparisonDescriptor::File::get_active_named_function_indices() const
{
    if (m_selectedBundles.size() == 1)
    {
        return span<const IndexT>{m_selectedBundles[0]->allNamedFunctionIndices};
    }
    else if (m_selectedBundles.size() > 1)
    {
        return span<const IndexT>{m_activeNamedFunctionIndices};
    }
    else
    {
        return span<const IndexT>{};
    }
}

const NamedFunction &ProgramComparisonDescriptor::File::get_filtered_named_function(int index) const
{
    const auto filtered = m_functionIndicesFilter.Filtered();
    assert(index < filtered.size());
    return m_revisionDescriptor->m_namedFunctions[filtered[index]];
}

const NamedFunctionMatchInfo &ProgramComparisonDescriptor::File::get_filtered_named_function_match_info(int index) const
{
    const auto filtered = m_functionIndicesFilter.Filtered();
    assert(index < filtered.size());
    return m_namedFunctionMatchInfos[filtered[index]];
}

const ProgramComparisonDescriptor::File::NamedFunctionUiInfo &ProgramComparisonDescriptor::File::
    get_filtered_named_function_ui_info(int index) const
{
    const auto filtered = m_functionIndicesFilter.Filtered();
    assert(index < filtered.size());
    return m_namedFunctionUiInfos[filtered[index]];
}

void ProgramComparisonDescriptor::prepare_rebuild()
{
    assert(m_pendingBuildComparisonRecordsCommands == 0);

    m_matchedFunctionsBuilt = false;
    util::free_container(m_matchedFunctions);
    m_processedMatchedFunctions = ProcessedState();
    util::free_container(m_selectedMatchedFunctionIndices);

    for (File &file : m_files)
    {
        file.prepare_rebuild();
    }
}

void ProgramComparisonDescriptor::init()
{
    for (File &file : m_files)
    {
        file.init();
    }
}

bool ProgramComparisonDescriptor::has_async_work() const
{
    for (const File &file : m_files)
    {
        if (file.has_async_work())
            return true;
    }
    return false;
}

bool ProgramComparisonDescriptor::executables_loaded() const
{
    int count = 0;

    for (const File &file : m_files)
    {
        if (file.exe_loaded())
            ++count;
    }
    return count == m_files.size();
}

bool ProgramComparisonDescriptor::named_functions_built() const
{
    int count = 0;

    for (const File &file : m_files)
    {
        if (file.named_functions_built())
            ++count;
    }
    return count == m_files.size();
}

bool ProgramComparisonDescriptor::matched_functions_built() const
{
    return m_matchedFunctionsBuilt;
}

bool ProgramComparisonDescriptor::bundles_ready() const
{
    IndexT count = 0;

    for (const File &file : m_files)
    {
        if (file.bundles_ready())
            ++count;
    }
    return count == m_files.size();
}

void ProgramComparisonDescriptor::on_match_strictness_changed()
{
    if (bundles_ready())
    {
        update_matched_named_function_ui_infos(get_matched_function_indices());
        update_all_bundle_ui_infos();
    }
}

const NamedFunction &ProgramComparisonDescriptor::get_named_function(Side side, IndexT index) const
{
    return m_files[side].m_revisionDescriptor->m_namedFunctions[index];
}

span<const IndexT> ProgramComparisonDescriptor::get_matched_function_indices() const
{
    assert(m_files[0].get_matched_function_indices().size() == m_files[1].get_matched_function_indices().size());
    return m_files[0].get_matched_function_indices();
}

bool ProgramComparisonDescriptor::matched_functions_disassembled(span<const IndexT> matchedFunctionIndices) const
{
    for (IndexT matchedFunctionIndex : matchedFunctionIndices)
    {
        const MatchedFunction &matchedFunction = m_matchedFunctions[matchedFunctionIndex];

        for (IndexT i = 0; i < 2; ++i)
        {
            const IndexT namedFunctionIndex = matchedFunction.named_idx_pair[i];
            const NamedFunction &namedFunction = m_files[i].m_revisionDescriptor->m_namedFunctions[namedFunctionIndex];

            if (!namedFunction.is_disassembled())
                return false;
        }
    }
    return true;
}

void ProgramComparisonDescriptor::update_selected_matched_functions()
{
    assert(named_functions_built());

    std::vector<IndexT> selectedMatchedFunctionIndices;

    const std::vector<IndexT> &selected0 = m_files[0].m_selectedNamedFunctionIndices;
    const std::vector<IndexT> &selected1 = m_files[1].m_selectedNamedFunctionIndices;

    {
        const size_t maxSize0 = m_files[0].m_namedFunctionMatchInfos.size();
        const size_t maxSize1 = m_files[1].m_namedFunctionMatchInfos.size();
        const size_t maxSize = std::min(maxSize0, maxSize1);
        size_t size = selected0.size() + selected1.size();
        // There can be no more matched functions than the smallest max function count.
        if (size > maxSize)
            size = maxSize;
        selectedMatchedFunctionIndices.reserve(size);
    }

    const Side lessSide = selected0.size() < selected1.size() ? LeftSide : RightSide;
    const Side moreSide = get_opposite_side(lessSide);

    for (IndexT functionIndex : m_files[moreSide].m_selectedNamedFunctionIndices)
    {
        const NamedFunctionMatchInfo &matchInfo = m_files[moreSide].m_namedFunctionMatchInfos[functionIndex];
        if (matchInfo.is_matched())
        {
            selectedMatchedFunctionIndices.push_back(matchInfo.matched_index);
        }
    }

    // Uses lookup set. Is much faster than linear search over elements.
    const std::unordered_set<IndexT> priorSelectedMatchedFunctionIndicesSet(
        selectedMatchedFunctionIndices.begin(),
        selectedMatchedFunctionIndices.end());

    for (IndexT functionIndex : m_files[lessSide].m_selectedNamedFunctionIndices)
    {
        const NamedFunctionMatchInfo &matchInfo = m_files[lessSide].m_namedFunctionMatchInfos[functionIndex];
        if (matchInfo.is_matched())
        {
            if (priorSelectedMatchedFunctionIndicesSet.count(matchInfo.matched_index) != 0)
                continue;

            selectedMatchedFunctionIndices.push_back(matchInfo.matched_index);
        }
    }

    m_selectedMatchedFunctionIndices = std::move(selectedMatchedFunctionIndices);
    m_selectedMatchedFunctionIndices.shrink_to_fit();
}

void ProgramComparisonDescriptor::update_all_bundle_ui_infos()
{
    // Updates ui infos for all bundles to avoid missing any.
    // Potentially is more expensive than we would like it to be.
    // We try to keep calls to a minimum.

    const bool strictnessChanged = m_strictnessUsedForUpdateBundleUiInfos != m_imguiStrictness;
    m_strictnessUsedForUpdateBundleUiInfos = m_imguiStrictness;

    for (IndexT fileIdx = 0; fileIdx < 2; ++fileIdx)
    {
        File &file = m_files[fileIdx];

        for (IndexT bundleType = 0; bundleType < IndexT(MatchBundleType::Count); ++bundleType)
        {
            const span<const NamedFunctionBundle> bundles = file.get_bundles(MatchBundleType(bundleType));
            const span<File::NamedFunctionBundleUiInfo> bundleUiInfos =
                file.get_bundle_ui_infos(MatchBundleType(bundleType));
            const IndexT bundleCount = bundles.size();
            assert(bundleUiInfos.size() == bundleCount);

            for (IndexT bundleIdx = 0; bundleIdx < bundleCount; ++bundleIdx)
            {
                const NamedFunctionBundle &bundle = bundles[bundleIdx];
                if (bundle.matchedFunctionIndices.empty())
                    continue;

                File::NamedFunctionBundleUiInfo &uiInfo = bundleUiInfos[bundleIdx];
                if (!strictnessChanged && uiInfo.m_similarity.has_value())
                    continue;

                const FunctionsSimilarityReport report = build_function_similarity_report({bundle.matchedFunctionIndices});

                if (!report.has_result())
                    continue;

                const uint32_t avgSimilarity = report.totalSimilarity.value() / bundle.allNamedFunctionIndices.size();
                uiInfo.update_info(bundle.name, bundle.id, true, avgSimilarity);
            }
        }
    }
}

ProgramComparisonDescriptor::FunctionsSimilarityReport ProgramComparisonDescriptor::build_function_similarity_report(
    span<const IndexT> matchedFunctionIndices)
{
    assert(!matchedFunctionIndices.empty());
    FunctionsSimilarityReport report;
    report.totalSimilarity.emplace(0);

    for (IndexT matchedFunctionIndex : matchedFunctionIndices)
    {
        const MatchedFunction &matchedFunction = m_matchedFunctions[matchedFunctionIndex];

        if (!matchedFunction.is_compared())
        {
            // Is missing comparison. Report is incomplete.
            report.totalSimilarity = std::nullopt;
            break;
        }
        report.totalSimilarity.value() += matchedFunction.comparison.get_similarity_as_int(m_imguiStrictness);
    }
    return report;
}

void ProgramComparisonDescriptor::update_matched_named_function_ui_infos(span<const IndexT> matchedFunctionIndices)
{
    for (IndexT matchedFunctionIndex : matchedFunctionIndices)
    {
        const MatchedFunction &matchedFunction = m_matchedFunctions[matchedFunctionIndex];
        if (!matchedFunction.is_compared())
            continue;

        for (IndexT i = 0; i < 2; ++i)
        {
            const IndexT namedFunctionIndex = matchedFunction.named_idx_pair[i];
            File &file = m_files[i];
            File::NamedFunctionUiInfo &uiInfo = file.m_namedFunctionUiInfos[namedFunctionIndex];
            const NamedFunction &namedFunction = file.m_revisionDescriptor->m_namedFunctions[namedFunctionIndex];
            const int8_t similarity = matchedFunction.comparison.get_similarity_as_int(m_imguiStrictness);
            uiInfo.update_info(namedFunction.name, namedFunction.id, true, similarity);
        }
    }
}

const ProgramComparisonDescriptor::File::NamedFunctionUiInfo *ProgramComparisonDescriptor::
    get_first_valid_named_function_ui_info(const MatchedFunction &matchedFunction) const
{
    for (IndexT i = 0; i < 2; ++i)
    {
        const IndexT namedFunctionIndex = matchedFunction.named_idx_pair[i];
        const File::NamedFunctionUiInfos &uiInfos = m_files[i].m_namedFunctionUiInfos;
        const File::NamedFunctionUiInfo &uiInfo = uiInfos[namedFunctionIndex];
        if (uiInfo.is_valid())
            return &uiInfo;
    }
    return nullptr;
}

span<const IndexT> ProgramComparisonDescriptor::get_matched_named_function_indices_for_processing(
    span<const IndexT> matchedFunctionIndices,
    Side side)
{
    const std::vector<IndexT> matchedNamedFunctionIndices =
        build_named_function_indices(m_matchedFunctions, matchedFunctionIndices, side);

    return m_files[side].m_revisionDescriptor->m_processedNamedFunctions.get_items_for_processing(
        span<const IndexT>{matchedNamedFunctionIndices});
}

int ProgramComparisonDescriptor::get_functions_page_count() const
{
    assert(m_imguiPageSize > 0);
    size_t size = m_selectedMatchedFunctionIndices.size();
    for (IndexT i = 0; i < 2; ++i)
    {
        size += m_files[i].m_selectedUnmatchedNamedFunctionIndices.size();
    }

    const size_t remainder = size % m_imguiPageSize;
    size /= m_imguiPageSize;
    if (remainder != 0)
        size += 1;

    return size;
}

namespace
{
struct FunctionsSubPageData
{
    span<const IndexT> items;

    // Current page index position.
    int leftOverPageIndex = 0;

    // Page item count remainder at current page index position. Needs to be 0 before page index can advance.
    int leftOverPageIndexRemainder = 0;

    // Current maximum page item count.
    int leftOverPageSize = 0;
};

FunctionsSubPageData get_functions_sub_page_data(
    int pageIndex,
    int pageIndexRemainder,
    int pageSize,
    span<const IndexT> items)
{
    FunctionsSubPageData data;

    if (pageIndexRemainder > 0)
    {
        // Process remainder. Turn the page if all remainders can be filled.
        const int availableRemainder = std::min<int>(pageIndexRemainder, items.size());
        data.leftOverPageIndexRemainder = pageIndexRemainder - availableRemainder;
        if (data.leftOverPageIndexRemainder > 0)
        {
            // Not all remainders were filled. Page is incomplete.
            data.leftOverPageIndex = pageIndex;
            data.leftOverPageSize = pageSize;
            return data;
        }
        // Next page has been reached. Skip items accordingly.
        items = {items.data() + availableRemainder, items.size() - availableRemainder};
        if (pageIndex > 0)
        {
            // Skip page index because items towards the new page were skipped as well.
            --pageIndex;
        }
        else
        {
            // The last page is an exception. Cannot skip.
        }
    }

    if (pageIndex == 0)
    {
        if (pageSize > 0)
        {
            // Fill last page with remaining items.
            const int availableItemSize = std::min<int>(pageSize, items.size());
            data.items = {items.data(), items.data() + availableItemSize};
            data.leftOverPageSize = pageSize - availableItemSize;
        }
        else
        {
            // Nothing left to do. Page has been filled.
        }
    }
    else
    {
        assert(pageIndex > 0);
        const int itemIndex = pageIndex * pageSize;
        if (itemIndex < items.size())
        {
            // Items are contained in page.
            const int availableItemSize = items.size() - itemIndex;
            const int usableItemSize = std::min<int>(availableItemSize, pageSize);
            data.items = {items.data() + itemIndex, items.data() + itemIndex + usableItemSize};
            data.leftOverPageSize = pageSize - usableItemSize;
        }
        else
        {
            // Items are not contained in page.
            data.leftOverPageIndex = pageIndex - (items.size() / pageSize);
            data.leftOverPageSize = pageSize;
        }

        const int remainder = items.size() % pageSize;
        if (remainder > 0)
        {
            // Set remainder if so required. It would be ok to set the remainder always,
            // but all that would do is run through the remainder processing logic once.
            data.leftOverPageIndexRemainder = pageSize - remainder;
        }
    }
    return data;
}
} // namespace

ProgramComparisonDescriptor::FunctionsPageData ProgramComparisonDescriptor::get_selected_functions_page_data() const
{
    assert(m_imguiSelectedPage > 0);
    const int pageIndex = m_imguiSelectedPage - 1;
    FunctionsPageData pageData;
    // Page data is collected from multiple sources.
    FunctionsSubPageData subPageData;
    subPageData = get_functions_sub_page_data(pageIndex, 0, m_imguiPageSize, m_selectedMatchedFunctionIndices);
    pageData.matchedFunctionIndices = subPageData.items;
    for (IndexT i = 0; i < 2; ++i)
    {
        subPageData = get_functions_sub_page_data(
            subPageData.leftOverPageIndex,
            subPageData.leftOverPageIndexRemainder,
            subPageData.leftOverPageSize,
            m_files[i].m_selectedUnmatchedNamedFunctionIndices);
        pageData.namedFunctionIndicesArray[i] = subPageData.items;
    }
    return pageData;
}

std::vector<IndexT> ProgramComparisonDescriptor::build_named_function_indices(
    const MatchedFunctions &matchedFunctions,
    span<const IndexT> matchedFunctionIndices,
    Side side)
{
    const size_t count = matchedFunctionIndices.size();
    std::vector<IndexT> namedFunctionIndices(count);
    for (size_t i = 0; i < count; ++i)
    {
        namedFunctionIndices[i] = matchedFunctions[matchedFunctionIndices[i]].named_idx_pair[side];
    }
    return namedFunctionIndices;
}

} // namespace unassemblize::gui
