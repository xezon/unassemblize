/**
 * @file
 *
 * @brief ImGui App core
 *
 * @copyright Unassemblize is free software: you can redistribute it and/or
 *            modify it under the terms of the GNU General Public License
 *            as published by the Free Software Foundation, either version
 *            3 of the License, or (at your option) any later version.
 *            A full copy of the GNU General Public License can be found in
 *            LICENSE
 */
#pragma once

#include "utility/imgui_misc.h"
#include "utility/imgui_text_filter.h"
#include <imgui.h>

#include "filecontentstorage.h"
#include "runnerasync.h"

#include <chrono>
#include <optional>

struct CommandLineOptions;

namespace unassemblize::gui
{
enum class ImGuiStatus
{
    Ok,
    Error,
};

class ImGuiApp
{
    // clang-format off
    static constexpr ImGuiTableFlags FileManagerInfoTableFlags =
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_Hideable |
        ImGuiTableFlags_ContextMenuInBody |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_BordersOuter |
        ImGuiTableFlags_BordersV |
        ImGuiTableFlags_SizingFixedFit |
        ImGuiTableFlags_NoHostExtendX |
        ImGuiTableFlags_ScrollX |
        ImGuiTableFlags_ScrollY;
    // clang-format on

    static constexpr uint8_t GuiBuildBundleFlags = BuildMatchedFunctionIndices | BuildAllNamedFunctionIndices;
    static constexpr ImU32 RedColor = IM_COL32(255, 0, 0, 255);
    static constexpr ImU32 GreenColor = IM_COL32(0, 255, 0, 255);
    static constexpr std::chrono::system_clock::time_point InvalidTimePoint = std::chrono::system_clock::time_point::min();
    static constexpr uint32_t InvalidId = 0;

    // Helper struct to keep track of items that are scheduled to be processed just once.
    struct ProcessedState
    {
        void init(size_t maxItemsCount);
        span<const IndexT> get_items_for_processing(span<const IndexT> indices);

    private:
        bool set_item_processed(IndexT index);
        size_t get_processed_item_count() const;
        span<const IndexT> get_processed_items(size_t begin, size_t end) const;

        // Items that have been processed.
        std::vector<IndexT> m_processedItems;
        // Array of bits for all items to keep track of which ones have been processed.
        std::unique_ptr<uint8_t[]> m_processedItemStates;
    };

    using ProgramFileId = uint32_t;
    using ProgramFileRevisionId = uint32_t;
    using ProgramComparisonId = uint32_t;

    struct ProgramFileDescriptor;
    struct ProgramFileRevisionDescriptor;
    struct ProgramComparisonDescriptor;

    using ProgramFileDescriptorPtr = std::unique_ptr<ProgramFileDescriptor>;
    using ProgramFileRevisionDescriptorPtr = std::shared_ptr<ProgramFileRevisionDescriptor>;
    using ProgramComparisonDescriptorPtr = std::unique_ptr<ProgramComparisonDescriptor>;

    using ProgramFileDescriptorPair = std::array<ProgramFileDescriptor *, 2>;

    struct ProgramFileDescriptor
    {
        ProgramFileDescriptor();
        ~ProgramFileDescriptor();

        bool has_active_command() const;
        WorkQueueCommandId get_active_command_id() const;

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

    // Note: Pass down a shared pointer of the ProgramFileRevisionDescriptor when chaining async commands.
    struct ProgramFileRevisionDescriptor
    {
        enum class WorkReason
        {
            Load,
            SaveConfig,
            BuildNamedFunctions,
            DisassembleSelectedFunctions,
            BuildSourceLinesForSelectedFunctions,
            LoadSourceFilesForSelectedFunctions,
        };

        ProgramFileRevisionDescriptor();
        ~ProgramFileRevisionDescriptor();

        void invalidate_command_id();
        bool has_active_command() const;
        WorkQueueCommandId get_active_command_id() const;

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

        // Has pending asynchronous command(s) running when not invalid.
        WorkQueueCommandId m_activeCommandId = InvalidWorkQueueCommandId; // #TODO Make vector of chained id's?
        WorkReason m_workReason = {};

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

        // Stores named functions that have been async processed already. Links to NamedFunctions.
        ProcessedState m_processedNamedFunctions;

        FileContentStorage m_fileContentStrorage;

        bool m_namedFunctionsBuilt = false;

    private:
        static ProgramFileRevisionId s_id;
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

                std::string m_label;
                std::optional<int8_t> m_similarity = std::nullopt;
            };

            struct NamedFunctionBundleUiInfo : public ListItemUiInfo
            {
            };

            struct NamedFunctionUiInfo : public ListItemUiInfo
            {
            };

            using ImGuiBundlesSelectionArray = std::array<ImGuiSelectionBasicStorage, size_t(MatchBundleType::Count)>;
            using NamedFunctionBundleUiInfos = std::vector<NamedFunctionBundleUiInfo>;
            using NamedFunctionUiInfos = std::vector<NamedFunctionUiInfo>;

            File();

            void prepare_rebuild();
            void init();

            void invalidate_command_id();
            bool has_active_command() const;
            WorkQueueCommandId get_active_command_id() const;

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
            void update_active_functions(); // Requires prior call to updated selected bundles.
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

            // Has pending asynchronous command(s) running when not invalid.
            WorkQueueCommandId m_activeCommandId = InvalidWorkQueueCommandId; // #TODO Make vector of chained id's?
            WorkReason m_workReason = {};

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

        bool has_active_command() const;

        bool executables_loaded() const;
        bool named_functions_built() const;
        bool matched_functions_built() const;
        bool bundles_ready() const;

        // Call relevant File::update_selected_functions before this one.
        void update_selected_matched_functions();

        void update_all_bundle_ui_infos();

        FunctionsSimilarityReport build_function_similarity_report(span<const IndexT> matchedFunctionIndices);

        void update_matched_named_function_ui_infos(span<const IndexT> matchedFunctionIndices);

        span<const IndexT> get_matched_named_function_indices_for_processing(IndexT side);

        const ProgramComparisonId m_id = InvalidId;

        int m_pendingBuildComparisonRecordsCommands = 0;

        bool m_has_open_window = true;
        bool m_matchedFunctionsBuilt = false;

        std::array<File, 2> m_files;

        MatchedFunctions m_matchedFunctions;

        // Stores matched functions that have been async processed already. Links to MatchedFunctions.
        ProcessedState m_processedMatchedFunctions;

        // Matched Functions that are visible and selected in the ui. Links to MatchedFunctions.
        std::vector<IndexT> m_selectedMatchedFunctionIndices;

    private:
        static std::vector<IndexT> build_named_function_indices(
            const MatchedFunctions &matchedFunctions,
            span<const IndexT> matchedFunctionIndices,
            IndexT side);

        static ProgramFileId s_id;
    };

public:
    ImGuiApp();
    ~ImGuiApp();

    ImGuiStatus init(const CommandLineOptions &clo);

    void prepare_shutdown_wait();
    void prepare_shutdown_nowait();
    bool can_shutdown() const; // Signals that this app can shutdown.
    void shutdown();

    void set_window_pos(ImVec2 pos) { m_windowPos = pos; }
    void set_window_size(ImVec2 size) { m_windowSize = size; }
    ImGuiStatus update();

    const ImVec4 &get_clear_color() const { return m_clearColor; }

private:
    void update_app();

    static WorkQueueCommandPtr create_load_command(ProgramFileRevisionDescriptorPtr &revisionDescriptor);
    static WorkQueueCommandPtr create_load_exe_command(ProgramFileRevisionDescriptorPtr &revisionDescriptor);
    static WorkQueueCommandPtr create_load_pdb_command(ProgramFileRevisionDescriptorPtr &revisionDescriptor);
    static WorkQueueCommandPtr create_load_pdb_and_exe_command(ProgramFileRevisionDescriptorPtr &revisionDescriptor);

    static WorkQueueCommandPtr create_save_exe_config_command(ProgramFileRevisionDescriptorPtr &revisionDescriptor);
    static WorkQueueCommandPtr create_save_pdb_config_command(ProgramFileRevisionDescriptorPtr &revisionDescriptor);

    static WorkQueueCommandPtr create_build_named_functions_command(ProgramFileRevisionDescriptorPtr &revisionDescriptor);
    static WorkQueueCommandPtr create_build_matched_functions_command(ProgramComparisonDescriptor *comparisonDescriptor);
    static WorkQueueCommandPtr create_build_bundles_from_compilands_command(ProgramComparisonDescriptor::File *file);
    static WorkQueueCommandPtr create_build_bundles_from_source_files_command(ProgramComparisonDescriptor::File *file);
    static WorkQueueCommandPtr create_build_single_bundle_command(
        ProgramComparisonDescriptor *comparisonDescriptor,
        size_t bundle_file_idx);

    static WorkQueueCommandPtr create_disassemble_selected_functions_command(
        ProgramFileRevisionDescriptorPtr &revisionDescriptor,
        span<const IndexT> namedFunctionIndices);

    static WorkQueueCommandPtr create_build_source_lines_for_selected_functions_command(
        ProgramFileRevisionDescriptorPtr &revisionDescriptor,
        span<const IndexT> namedFunctionIndices);

    static WorkQueueCommandPtr create_load_source_files_for_selected_functions_command(
        ProgramFileRevisionDescriptorPtr &revisionDescriptor,
        span<const IndexT> namedFunctionIndices);

    static WorkQueueCommandPtr create_process_selected_functions_command(
        ProgramFileRevisionDescriptorPtr &revisionDescriptor,
        span<const IndexT> namedFunctionIndices);

    static WorkQueueCommandPtr create_build_comparison_records_for_selected_functions_command(
        ProgramComparisonDescriptor *comparisonDescriptor,
        span<const IndexT> matchedFunctionIndices);

    ProgramFileDescriptor *get_program_file_descriptor(size_t program_file_idx);

    void load_async(ProgramFileDescriptor *descriptor);

    void save_config_async(ProgramFileDescriptor *descriptor);

    void load_and_init_comparison_async(
        ProgramFileDescriptorPair fileDescriptorPair,
        ProgramComparisonDescriptor *comparisonDescriptor);
    void init_comparison_async(ProgramComparisonDescriptor *comparisonDescriptor);
    void build_named_functions_async(ProgramComparisonDescriptor *comparisonDescriptor);
    void build_matched_functions_async(ProgramComparisonDescriptor *comparisonDescriptor);
    void build_bundled_functions_async(ProgramComparisonDescriptor *comparisonDescriptor);

    void process_named_functions_async(
        ProgramFileRevisionDescriptorPtr &revisionDescriptor,
        span<const IndexT> namedFunctionIndices);

    void process_named_and_matched_functions_async(
        ProgramComparisonDescriptor *comparisonDescriptor,
        span<const IndexT> matchedFunctionIndices);

    void process_matched_functions_async(
        ProgramComparisonDescriptor *comparisonDescriptor,
        span<const IndexT> matchedFunctionIndices);

    void add_file();
    void remove_file(size_t idx);
    void remove_all_files();

    void add_program_comparison();
    void update_closed_program_comparisons();

    void on_functions_interaction(ProgramComparisonDescriptor &descriptor, ProgramComparisonDescriptor::File &file);

    static std::string create_section_string(uint32_t section_index, const ExeSections *sections);
    static std::string create_time_string(std::chrono::time_point<std::chrono::system_clock> time_point);

    void BackgroundWindow();
    void FileManagerWindow(bool *p_open);
    void OutputManagerWindow(bool *p_open);
    void ComparisonManagerWindows();

    void FileManagerBody();
    void FileManagerDescriptor(ProgramFileDescriptor &descriptor, bool &erased);
    void FileManagerDescriptorExeFile(ProgramFileDescriptor &descriptor);
    void FileManagerDescriptorExeConfig(ProgramFileDescriptor &descriptor);
    void FileManagerDescriptorPdbFile(ProgramFileDescriptor &descriptor);
    void FileManagerDescriptorPdbConfig(ProgramFileDescriptor &descriptor);
    void FileManagerDescriptorActions(ProgramFileDescriptor &descriptor, bool &erased);
    void FileManagerDescriptorSaveLoadStatus(const ProgramFileRevisionDescriptor &descriptor);
    void FileManagerDescriptorLoadStatus(const ProgramFileRevisionDescriptor &descriptor);
    void FileManagerDescriptorSaveStatus(const ProgramFileRevisionDescriptor &descriptor);
    void FileManagerGlobalButtons();
    void FileManagerInfo(ProgramFileDescriptor &fileDescriptor, const ProgramFileRevisionDescriptor &revisionDescriptor);
    void FileManagerInfoExeSections(const ProgramFileRevisionDescriptor &descriptor);
    void FileManagerInfoExeSymbols(
        ProgramFileDescriptor &fileDescriptor,
        const ProgramFileRevisionDescriptor &revisionDescriptor);
    void FileManagerInfoPdbCompilands(const ProgramFileRevisionDescriptor &descriptor);
    void FileManagerInfoPdbSourceFiles(const ProgramFileRevisionDescriptor &descriptor);
    void FileManagerInfoPdbSymbols(
        ProgramFileDescriptor &fileDescriptor,
        const ProgramFileRevisionDescriptor &revisionDescriptor);
    void FileManagerInfoPdbFunctions(
        ProgramFileDescriptor &fileDescriptor,
        const ProgramFileRevisionDescriptor &revisionDescriptor);
    void FileManagerInfoPdbExeInfo(const ProgramFileRevisionDescriptor &descriptor);

    void OutputManagerBody();

    void ComparisonManagerBody(ProgramComparisonDescriptor &descriptor);
    void ComparisonManagerProgramFileSelection(ProgramComparisonDescriptor::File &file);
    void ComparisonManagerItemListStyleColor(
        ScopedStyleColor &styleColor,
        const ProgramComparisonDescriptor::File::ListItemUiInfo &uiInfo);

    void ShowRemoveFileConfirmationPopup(bool &erased);

private:
    ImVec2 m_windowPos = ImVec2(0, 0);
    ImVec2 m_windowSize = ImVec2(0, 0);
    ImVec4 m_clearColor = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);

    bool m_showDemoWindow = true;
    bool m_showFileManager = true;
    bool m_showFileManagerWithTabs = false;
    bool m_showFileManagerExeSectionInfo = true;
    bool m_showFileManagerExeSymbolInfo = true;
    bool m_showFileManagerPdbCompilandInfo = true;
    bool m_showFileManagerPdbSourceFileInfo = true;
    bool m_showFileManagerPdbSymbolInfo = true;
    bool m_showFileManagerPdbFunctionInfo = true;
    bool m_showFileManagerPdbExeInfo = true;

    bool m_showOutputManager = true;

    WorkQueue m_workQueue;

    std::vector<ProgramFileDescriptorPtr> m_programFiles;
    std::vector<ProgramComparisonDescriptorPtr> m_programComparisons;
};

} // namespace unassemblize::gui
