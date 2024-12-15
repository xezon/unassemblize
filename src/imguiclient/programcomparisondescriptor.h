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
#pragma once

#include "asyncworkstate.h"
#include "processedstate.h"
#include "programfilecommon.h"
#include "utility/imgui_text_filter.h"
#include <imgui.h>
#include <optional>

namespace unassemblize::gui
{
struct ProgramComparisonDescriptor
{
    struct File
    {
        enum class WorkReason
        {
            BuildMatchedFunctions,
            BuildCompilandBundles,
            BuildSourceFileBundles,
            BuildSingleBundle,
            BuildComparisonRecordsForSelectedFunctions,
        };

        struct ListItemUiInfo
        {
            void update_info(
                const std::string &itemName,
                uint32_t itemId,
                bool hasMatchedFunction,
                std::optional<int8_t> similarity = std::nullopt);

            std::string m_label;
            std::optional<int8_t> m_similarity = std::nullopt;
        };

        struct NamedFunctionBundleUiInfo : public ListItemUiInfo
        {
        };

        struct NamedFunctionUiInfo : public ListItemUiInfo
        {
        };

        using WorkState = AsyncWorkState<WorkReason>;
        using ImGuiBundlesSelectionArray = std::array<ImGuiSelectionBasicStorage, size_t(MatchBundleType::Count)>;
        using NamedFunctionBundleUiInfos = std::vector<NamedFunctionBundleUiInfo>;
        using NamedFunctionUiInfos = std::vector<NamedFunctionUiInfo>;

        File();

        void prepare_rebuild();
        void init();

        void add_async_work_hint(WorkQueueCommandId commandId, WorkReason reason);
        void remove_async_work_hint(WorkQueueCommandId commandId);
        bool has_async_work() const;
        WorkQueueCommandId get_first_active_command_id() const;

        bool exe_loaded() const;
        bool pdb_loaded() const;
        bool named_functions_built() const;
        bool bundles_ready() const; // Bundles can be used when this returns true.

        bool is_matched_function(IndexT namedFunctionIndex) const;

        MatchBundleType get_selected_bundle_type() const;
        span<const NamedFunctionBundle> get_bundles(MatchBundleType type) const;
        span<NamedFunctionBundleUiInfo> get_bundle_ui_infos(MatchBundleType type);
        span<const NamedFunctionBundleUiInfo> get_bundle_ui_infos(MatchBundleType type) const;
        ImGuiSelectionBasicStorage &get_bundles_selection(MatchBundleType type);
        const NamedFunctionBundle &get_filtered_bundle(int index) const;
        const NamedFunctionBundleUiInfo &get_filtered_bundle_ui_info(int index) const;

        void on_bundles_changed();
        void on_bundles_interaction();

        void update_bundle_ui_infos(MatchBundleType type);
        void update_selected_bundles();
        void update_active_functions(); // Requires prior call to update_selected_bundles()
        void update_named_function_ui_infos(span<const IndexT> namedFunctionIndices);

        span<const IndexT> get_active_named_function_indices() const;
        const NamedFunction &get_filtered_named_function(int index) const;
        const NamedFunctionMatchInfo &get_filtered_named_function_match_info(int index) const;
        const NamedFunctionUiInfo &get_filtered_named_function_ui_info(int index) const;

        void update_selected_named_functions();

        // Selected file index in list box. Is not reset on rebuild.
        // Does not necessarily link to current loaded file.
        IndexT m_imguiSelectedFileIdx = 0;

        // Selected bundle type in combo box. Is not reset on rebuild.
        IndexT m_imguiSelectedBundleTypeIdx = 0;

        // Functions list options. Is not reset on rebuild.
        bool m_imguiShowMatchedFunctions = true;
        bool m_imguiShowUnmatchedFunctions = true;

        // Selected bundles in multi select box. Is not reset on rebuild.
        ImGuiBundlesSelectionArray m_imguiBundlesSelectionArray;

        // Selected functions in multi select box. Is not reset on rebuild.
        ImGuiSelectionBasicStorage m_imguiFunctionsSelection;

        TextFilterDescriptor<const NamedFunctionBundle *> m_bundlesFilter = "bundles_filter";
        TextFilterDescriptor<IndexT> m_functionIndicesFilter = "functions_filter";

        WorkState m_asyncWorkState;

        ProgramFileRevisionDescriptorPtr m_revisionDescriptor;

        NamedFunctionMatchInfos m_namedFunctionMatchInfos;
        NamedFunctionBundles m_compilandBundles;
        NamedFunctionBundles m_sourceFileBundles;
        NamedFunctionBundle m_singleBundle;

        NamedFunctionUiInfos m_namedFunctionUiInfos;
        NamedFunctionBundleUiInfos m_compilandBundleUiInfos;
        NamedFunctionBundleUiInfos m_sourceFileBundleUiInfos;
        NamedFunctionBundleUiInfo m_singleBundleUiInfo;

        TriState m_compilandBundlesBuilt = TriState::False;
        TriState m_sourceFileBundlesBuilt = TriState::False;
        bool m_singleBundleBuilt = false;

        // Bundles that are visible and selected in the ui.
        std::vector<const NamedFunctionBundle *> m_selectedBundles;

        // Named function indices that have been assembled from multiple bundles. Links to NamedFunctions.
        std::vector<IndexT> m_activeNamedFunctionIndices;

        // Functions that are visible and selected in the ui. Links to NamedFunctions.
        std::vector<IndexT> m_selectedNamedFunctionIndices;
    };

    struct FunctionsSimilarityReport
    {
        bool has_result() const { return totalSimilarity.has_value(); }

        std::optional<uint32_t> totalSimilarity = std::nullopt; // Accumulative similarity value of matched functions.
    };

    ProgramComparisonDescriptor();
    ~ProgramComparisonDescriptor();

    void prepare_rebuild();
    void init();

    bool has_async_work() const;

    bool executables_loaded() const;
    bool named_functions_built() const;
    bool matched_functions_built() const;
    bool bundles_ready() const;

    bool matched_functions_disassembled(span<const IndexT> matchedFunctionIndices) const;

    // Requires prior call(s) to File::update_selected_functions()
    void update_selected_matched_functions();

    void update_all_bundle_ui_infos();

    FunctionsSimilarityReport build_function_similarity_report(span<const IndexT> matchedFunctionIndices);

    void update_matched_named_function_ui_infos(span<const IndexT> matchedFunctionIndices);

    span<const IndexT> get_matched_named_function_indices_for_processing(IndexT side);

    const ProgramComparisonId m_id = InvalidId;

    int m_pendingBuildComparisonRecordsCommands = 0;

    bool m_imguiHasOpenWindow = true;
    bool m_matchedFunctionsBuilt = false;

    MatchedFunctions m_matchedFunctions;

    // Stores matched functions that have been prepared for async processing already. Links to MatchedFunctions.
    ProcessedState m_processedMatchedFunctions;

    // Matched Functions that are visible and selected in the ui. Links to MatchedFunctions.
    std::vector<IndexT> m_selectedMatchedFunctionIndices;

    std::array<File, 2> m_files;

private:
    static std::vector<IndexT> build_named_function_indices(
        const MatchedFunctions &matchedFunctions,
        span<const IndexT> matchedFunctionIndices,
        IndexT side);

    static ProgramComparisonId s_id;
};
} // namespace unassemblize::gui
