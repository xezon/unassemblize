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
#include "imguiapptypes.h"
#include "processedstate.h"
#include "programfilecommon.h"
#include "utility/imgui_text_filter.h"
#include <imgui.h>
#include <optional>

namespace unassemblize::gui
{
struct AssemblerTableColumnSettings
{
    AssemblerTableColumnSettings()
    {
        m_show[(int)AssemblerTableColumn::SourceLine] = true;
        m_show[(int)AssemblerTableColumn::SourceCode] = true;
        m_show[(int)AssemblerTableColumn::Bytes] = true;
        m_show[(int)AssemblerTableColumn::Address] = true;
        m_show[(int)AssemblerTableColumn::Jumps] = false;
        m_show[(int)AssemblerTableColumn::Assembler] = true;

        m_useCustomWidth[(int)AssemblerTableColumn::SourceLine] = false;
        m_useCustomWidth[(int)AssemblerTableColumn::SourceCode] = true;
        m_useCustomWidth[(int)AssemblerTableColumn::Bytes] = true;
        m_useCustomWidth[(int)AssemblerTableColumn::Address] = false;
        m_useCustomWidth[(int)AssemblerTableColumn::Jumps] = false;
        m_useCustomWidth[(int)AssemblerTableColumn::Assembler] = true;

        m_customWidth[(int)AssemblerTableColumn::SourceLine] = 10;
        m_customWidth[(int)AssemblerTableColumn::SourceCode] = 300;
        m_customWidth[(int)AssemblerTableColumn::Bytes] = 100;
        m_customWidth[(int)AssemblerTableColumn::Address] = 10;
        m_customWidth[(int)AssemblerTableColumn::Jumps] = 10;
        m_customWidth[(int)AssemblerTableColumn::Assembler] = 300;
    }

    bool show(AssemblerTableColumn column) const { return m_show[(int)column]; }

    float custom_width(AssemblerTableColumn column) const
    {
        return static_cast<float>(m_useCustomWidth[(int)column] ? m_customWidth[(int)column] : 0);
    }

    std::array<bool, AssemblerTableColumnCount> m_show;
    std::array<bool, AssemblerTableColumnCount> m_useCustomWidth;
    std::array<int, AssemblerTableColumnCount> m_customWidth;
};

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

            bool is_valid() const { return !m_label.empty(); }

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

        span<const IndexT> get_matched_function_indices() const; // Links to MatchedFunctions.
        span<const IndexT> get_unmatched_named_function_indices() const; // Links to NamedFunctions.

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
        void update_active_named_functions(); // Requires prior call to update_selected_bundles()
        void update_active_named_function_ui_infos();
        void update_named_function_ui_infos(span<const IndexT> namedFunctionIndices);
        void update_selected_named_functions();

        span<const IndexT> get_active_named_function_indices() const;
        const NamedFunction &get_filtered_named_function(int index) const;
        const NamedFunctionMatchInfo &get_filtered_named_function_match_info(int index) const;
        const NamedFunctionUiInfo &get_filtered_named_function_ui_info(int index) const;

        WorkState m_asyncWorkState;

        // UI OPTIONS. IS NOT RESET ON REBUILD.

        // Selected file index in list box.
        // Does not necessarily link to current loaded file.
        IndexT m_imguiSelectedFileIdx = 0;

        // Selected bundle type in combo box.
        IndexT m_imguiSelectedBundleTypeIdx = 0;

        // Files list options.
        bool m_imguiReloadFileRevisionOnCompare = false;

        // Functions list options.
        bool m_imguiShowMatchedFunctions = true;
        bool m_imguiShowUnmatchedFunctions = true;

        // Selected bundles in multi select box.
        ImGuiBundlesSelectionArray m_imguiBundlesSelectionArray;

        // Selected functions in multi select box.
        ImGuiSelectionBasicStorage m_imguiFunctionsSelection;

        // BUILT CONTENTS. IS RESET ON REBUILD.

        TextFilterDescriptor<const NamedFunctionBundle *> m_bundlesFilter = "bundles_filter";
        TextFilterDescriptor<IndexT> m_functionIndicesFilter = "functions_filter";

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
        std::vector<IndexT> m_selectedUnmatchedNamedFunctionIndices;
    };

    struct FunctionsSimilarityReport
    {
        bool has_result() const { return totalSimilarity.has_value(); }

        std::optional<uint32_t> totalSimilarity = std::nullopt; // Accumulative similarity value of matched functions.
    };

    struct FunctionsPageData
    {
        span<const IndexT> matchedFunctionIndices;
        std::array<span<const IndexT>, 2> namedFunctionIndicesArray;
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

    void on_match_strictness_changed();

    const NamedFunction &get_named_function(Side side, IndexT index) const;

    span<const IndexT> get_matched_function_indices() const; // Links to MatchedFunctions.

    bool matched_functions_disassembled(span<const IndexT> matchedFunctionIndices) const;

    // Requires prior call(s) to File::update_selected_functions()
    void update_selected_matched_functions();

    void update_all_bundle_ui_infos();

    FunctionsSimilarityReport build_function_similarity_report(span<const IndexT> matchedFunctionIndices);

    void update_matched_named_function_ui_infos(span<const IndexT> matchedFunctionIndices);

    const File::NamedFunctionUiInfo *get_first_valid_named_function_ui_info(const MatchedFunction &matchedFunction) const;

    span<const IndexT> get_matched_named_function_indices_for_processing(
        span<const IndexT> matchedFunctionIndices,
        Side side);

    int get_functions_page_count() const;
    FunctionsPageData get_selected_functions_page_data() const;

    const ProgramComparisonId m_id = InvalidId;

    int m_pendingBuildComparisonRecordsCommands = 0;

    // UI OPTIONS. IS NOT RESET ON REBUILD.

    // Customize how the assembler table columns show.
    AssemblerTableColumnSettings m_imguiAssemblerTableColumnSettings;

    // Set match strictness for instruction comparisons.
    AsmMatchStrictness m_imguiStrictness = AsmMatchStrictness::Undecided;

    // Selected functions in pages.
    int m_imguiPageSize = 25;
    int m_imguiSelectedPage = 1; // 1..n

    bool m_imguiProcessMatchedFunctionsImmediately = false;
    bool m_imguiProcessUnmatchedFunctionsImmediately = false;
    bool m_imguiSettingsWindowOpened = false;
    bool m_imguiComparisonWindowOpened = true;

    // BUILT CONTENTS. IS RESET ON REBUILD.

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
        Side side);

    AsmMatchStrictness m_strictnessUsedForUpdateBundleUiInfos = AsmMatchStrictness::Undecided;

    static ProgramComparisonId s_id;
};
} // namespace unassemblize::gui
