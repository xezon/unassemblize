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
#include "imguiapp.h"
#include "executable.h"
#include "pdbreader.h"
#include "utility/imgui_scoped.h"
#include <filesystem>
#include <fmt/core.h>
#include <misc/cpp/imgui_stdlib.h>

// #TODO Add tooltip markers to fields and buttons that need it.

namespace unassemblize::gui
{
ImGuiApp::ImGuiApp()
{
}

ImGuiApp::~ImGuiApp()
{
}

ImGuiStatus ImGuiApp::init(const CommandLineOptions &clo)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
#ifdef RELEASE
    io.ConfigDebugHighlightIdConflicts = false;
    io.ConfigDebugIsDebuggerPresent = false;
#else
    io.ConfigDebugHighlightIdConflicts = true;
    io.ConfigDebugIsDebuggerPresent = true;
#endif
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    io.ConfigDockingWithShift = true;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport / Platform Windows
    // io.ConfigViewportsNoAutoMerge = true;
    // io.ConfigViewportsNoTaskBarIcon = true;

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use
    // ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application
    // (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling
    // ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash
    io.Fonts->AddFontDefault();
    // io.Fonts->AddFontDefault();
    // io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    // ImFont *font =
    //    io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr,
    //    io.Fonts->GetGlyphRangesJapanese());
    // IM_ASSERT(font != nullptr);

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();

    // When view ports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle &style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    m_workQueue.start();

    for (size_t i = 0; i < CommandLineOptions::MAX_INPUT_FILES; ++i)
    {
        if (clo.input_file[i].v.empty())
            continue;

        auto descriptor = std::make_unique<ProgramFileDescriptor>();
        InputType input_type = get_input_type(clo.input_file[i].v, clo.input_type[i].v);
        switch (input_type)
        {
            case InputType::Exe:
                descriptor->m_exeFilename = clo.input_file[i];
                descriptor->m_exeConfigFilename = clo.config_file[i];
                break;
            case InputType::Pdb:
                descriptor->m_exeFilename = auto_str;
                descriptor->m_exeConfigFilename = clo.config_file[i];
                descriptor->m_pdbFilename = clo.input_file[i];
                descriptor->m_pdbConfigFilename = clo.config_file[i];
                break;
            default:
                break;
        }

        m_programFiles.emplace_back(std::move(descriptor));
    }

    add_program_comparison();

    return ImGuiStatus::Ok;
}

void ImGuiApp::prepare_shutdown_wait()
{
    m_workQueue.stop(true);
}

void ImGuiApp::prepare_shutdown_nowait()
{
    m_workQueue.stop(false);
}

bool ImGuiApp::can_shutdown() const
{
    return !m_workQueue.is_busy();
}

void ImGuiApp::shutdown()
{
    assert(can_shutdown());
    ImGui::DestroyContext();
}

void ClampImGuiWindowToClientArea(ImVec2 &position, ImVec2 &size, const ImVec2 &clientPos, const ImVec2 &clientSize)
{
    // Clamp the position to ensure it stays within the client area
    if (position.x < clientPos.x)
        position.x = clientPos.x;
    if (position.y < clientPos.y)
        position.y = clientPos.y;

    if ((position.x + size.x) > (clientPos.x + clientSize.x))
        position.x = clientPos.x + clientSize.x - size.x;

    if ((position.y + size.y) > (clientPos.y + clientSize.y))
        position.y = clientPos.y + clientSize.y - size.y;
}

ImGuiStatus ImGuiApp::update()
{
    m_workQueue.update_callbacks();

    ImGui::NewFrame();

    update_app();

#if !RELEASE
    if (m_showDemoWindow)
    {
        // Show the big demo window.
        // Most of the sample code is in ImGui::ShowDemoWindow()!
        // You can browse its code to learn more about Dear ImGui!
        ImGui::ShowDemoWindow(&m_showDemoWindow);
    }
#endif

    ImGui::EndFrame();

    return ImGuiStatus::Ok;
}

void ImGuiApp::update_app()
{
    BackgroundWindow();

    if (m_showFileManager)
        FileManagerWindow(&m_showFileManager);

    if (m_showOutputManager)
        OutputManagerWindow(&m_showOutputManager);

    ComparisonManagerWindows();
}

WorkQueueCommandPtr ImGuiApp::create_load_command(ProgramFileRevisionDescriptorPtr &revisionDescriptor)
{
    if (revisionDescriptor->can_load_pdb())
    {
        return create_load_pdb_and_exe_command(revisionDescriptor);
    }
    else if (revisionDescriptor->can_load_exe())
    {
        return create_load_exe_command(revisionDescriptor);
    }
    else
    {
        // Cannot load undefined file.
        assert(false);
        return nullptr;
    }
}

WorkQueueCommandPtr ImGuiApp::create_load_exe_command(ProgramFileRevisionDescriptorPtr &revisionDescriptor)
{
    assert(revisionDescriptor->can_load_exe());

    if (revisionDescriptor->m_pdbReader == nullptr)
    {
        revisionDescriptor->m_exeFilenameFromPdb.clear();
    }

    const std::string exe_filename = revisionDescriptor->evaluate_exe_filename();
    auto command = std::make_unique<AsyncLoadExeCommand>(LoadExeOptions(exe_filename));

    command->options.config_file = revisionDescriptor->evaluate_exe_config_filename();
    command->options.pdb_reader = revisionDescriptor->m_pdbReader.get();
    command->callback = [revisionDescriptor](WorkQueueResultPtr &result) {
        auto res = static_cast<AsyncLoadExeResult *>(result.get());
        const bool loaded = res->executable != nullptr;
        revisionDescriptor->m_executable = std::move(res->executable);
        revisionDescriptor->m_exeLoaded = loaded ? TriState::True : TriState::False;
        revisionDescriptor->m_exeLoadTimepoint = std::chrono::system_clock::now();
        revisionDescriptor->remove_async_work_hint(result->command->command_id);
    };

    revisionDescriptor->m_executable.reset();
    revisionDescriptor->m_exeLoaded = TriState::NotApplicable;
    revisionDescriptor->m_exeLoadTimepoint = InvalidTimePoint;
    revisionDescriptor->m_exeSaveConfigFilename.clear();
    revisionDescriptor->m_exeSaveConfigTimepoint = InvalidTimePoint;
    revisionDescriptor->add_async_work_hint(command->command_id, ProgramFileRevisionDescriptor::WorkReason::Load);

    return command;
}

WorkQueueCommandPtr ImGuiApp::create_load_pdb_command(ProgramFileRevisionDescriptorPtr &revisionDescriptor)
{
    assert(revisionDescriptor->can_load_pdb());

    auto command = std::make_unique<AsyncLoadPdbCommand>(LoadPdbOptions(revisionDescriptor->m_pdbFilenameCopy));

    command->callback = [revisionDescriptor](WorkQueueResultPtr &result) {
        auto res = static_cast<AsyncLoadPdbResult *>(result.get());
        const bool loaded = res->pdbReader != nullptr;
        revisionDescriptor->m_pdbReader = std::move(res->pdbReader);
        revisionDescriptor->m_pdbLoaded = loaded ? TriState::True : TriState::False;
        revisionDescriptor->m_pdbLoadTimepoint = std::chrono::system_clock::now();
        revisionDescriptor->remove_async_work_hint(result->command->command_id);
    };

    revisionDescriptor->m_pdbReader.reset();
    revisionDescriptor->m_pdbLoaded = TriState::NotApplicable;
    revisionDescriptor->m_pdbLoadTimepoint = InvalidTimePoint;
    revisionDescriptor->m_pdbSaveConfigFilename.clear();
    revisionDescriptor->m_pdbSaveConfigTimepoint = InvalidTimePoint;
    revisionDescriptor->m_exeFilenameFromPdb.clear();
    revisionDescriptor->add_async_work_hint(command->command_id, ProgramFileRevisionDescriptor::WorkReason::Load);

    return command;
}

WorkQueueCommandPtr ImGuiApp::create_load_pdb_and_exe_command(ProgramFileRevisionDescriptorPtr &revisionDescriptor)
{
    auto command = create_load_pdb_command(revisionDescriptor);

    command->chain([revisionDescriptor](WorkQueueResultPtr &result) mutable -> WorkQueueCommandPtr {
        if (revisionDescriptor->m_pdbReader == nullptr)
            return nullptr;

        const unassemblize::PdbExeInfo &exe_info = revisionDescriptor->m_pdbReader->get_exe_info();
        revisionDescriptor->m_exeFilenameFromPdb = unassemblize::Runner::create_exe_filename(exe_info);

        if (!revisionDescriptor->can_load_exe())
            return nullptr;

        return create_load_exe_command(revisionDescriptor);
    });

    return command;
}

WorkQueueCommandPtr ImGuiApp::create_save_exe_config_command(ProgramFileRevisionDescriptorPtr &revisionDescriptor)
{
    assert(revisionDescriptor->can_save_exe_config());

    const std::string config_filename = revisionDescriptor->evaluate_exe_config_filename();
    auto command = std::make_unique<AsyncSaveExeConfigCommand>(
        SaveExeConfigOptions(*revisionDescriptor->m_executable, config_filename));

    command->callback = [revisionDescriptor](WorkQueueResultPtr &result) {
        auto res = static_cast<AsyncSaveExeConfigResult *>(result.get());
        auto com = static_cast<AsyncSaveExeConfigCommand *>(result->command.get());
        revisionDescriptor->m_exeSaveConfigFilename = util::abs_path(com->options.config_file);
        revisionDescriptor->m_exeConfigSaved = res->success ? TriState::True : TriState::False;
        revisionDescriptor->m_exeSaveConfigTimepoint = std::chrono::system_clock::now();
        revisionDescriptor->remove_async_work_hint(result->command->command_id);
    };

    revisionDescriptor->m_exeSaveConfigFilename.clear();
    revisionDescriptor->m_exeConfigSaved = TriState::NotApplicable;
    revisionDescriptor->m_exeSaveConfigTimepoint = InvalidTimePoint;
    revisionDescriptor->add_async_work_hint(command->command_id, ProgramFileRevisionDescriptor::WorkReason::SaveConfig);

    return command;
}

WorkQueueCommandPtr ImGuiApp::create_save_pdb_config_command(ProgramFileRevisionDescriptorPtr &revisionDescriptor)
{
    assert(revisionDescriptor->can_save_pdb_config());

    const std::string config_filename = revisionDescriptor->evaluate_pdb_config_filename();
    auto command =
        std::make_unique<AsyncSavePdbConfigCommand>(SavePdbConfigOptions(*revisionDescriptor->m_pdbReader, config_filename));

    command->callback = [revisionDescriptor](WorkQueueResultPtr &result) {
        auto res = static_cast<AsyncSavePdbConfigResult *>(result.get());
        auto com = static_cast<AsyncSavePdbConfigCommand *>(result->command.get());
        revisionDescriptor->m_pdbSaveConfigFilename = util::abs_path(com->options.config_file);
        revisionDescriptor->m_pdbConfigSaved = res->success ? TriState::True : TriState::False;
        revisionDescriptor->m_pdbSaveConfigTimepoint = std::chrono::system_clock::now();
        revisionDescriptor->remove_async_work_hint(result->command->command_id);
    };

    revisionDescriptor->m_pdbSaveConfigFilename.clear();
    revisionDescriptor->m_pdbConfigSaved = TriState::NotApplicable;
    revisionDescriptor->m_pdbSaveConfigTimepoint = InvalidTimePoint;
    revisionDescriptor->add_async_work_hint(command->command_id, ProgramFileRevisionDescriptor::WorkReason::SaveConfig);

    return command;
}

WorkQueueCommandPtr ImGuiApp::create_build_named_functions_command(ProgramFileRevisionDescriptorPtr &revisionDescriptor)
{
    assert(revisionDescriptor != nullptr);
    assert(revisionDescriptor->exe_loaded());
    assert(!revisionDescriptor->m_namedFunctionsBuilt);

    auto command = std::make_unique<AsyncBuildFunctionsCommand>(BuildFunctionsOptions(*revisionDescriptor->m_executable));

    command->callback = [revisionDescriptor](WorkQueueResultPtr &result) {
        auto res = static_cast<AsyncBuildFunctionsResult *>(result.get());
        revisionDescriptor->m_namedFunctions = std::move(res->named_functions);
        revisionDescriptor->m_processedNamedFunctions.init(revisionDescriptor->m_namedFunctions.size());
        revisionDescriptor->m_namedFunctionsBuilt = true;
        revisionDescriptor->remove_async_work_hint(result->command->command_id);
    };

    revisionDescriptor->m_namedFunctions.clear();
    revisionDescriptor->m_namedFunctionsBuilt = false;
    revisionDescriptor->add_async_work_hint(
        command->command_id,
        ProgramFileRevisionDescriptor::WorkReason::BuildNamedFunctions);

    return command;
}

WorkQueueCommandPtr ImGuiApp::create_build_matched_functions_command(ProgramComparisonDescriptor *comparisonDescriptor)
{
    assert(comparisonDescriptor != nullptr);
    assert(!comparisonDescriptor->m_matchedFunctionsBuilt);
    ProgramFileRevisionDescriptor *revisionDescriptor0 = comparisonDescriptor->m_files[0].m_revisionDescriptor.get();
    ProgramFileRevisionDescriptor *revisionDescriptor1 = comparisonDescriptor->m_files[1].m_revisionDescriptor.get();
    assert(revisionDescriptor0 != nullptr);
    assert(revisionDescriptor1 != nullptr);
    assert(revisionDescriptor0->named_functions_built());
    assert(revisionDescriptor1->named_functions_built());

    auto command = std::make_unique<AsyncBuildMatchedFunctionsCommand>(
        BuildMatchedFunctionsOptions({&revisionDescriptor0->m_namedFunctions, &revisionDescriptor1->m_namedFunctions}));

    command->callback = [comparisonDescriptor](WorkQueueResultPtr &result) {
        auto res = static_cast<AsyncBuildMatchedFunctionsResult *>(result.get());

        comparisonDescriptor->m_matchedFunctions = std::move(res->matchedFunctionsData.matchedFunctions);
        comparisonDescriptor->m_processedMatchedFunctions.init(comparisonDescriptor->m_matchedFunctions.size());
        comparisonDescriptor->m_matchedFunctionsBuilt = true;

        for (int i = 0; i < 2; ++i)
        {
            ProgramComparisonDescriptor::File &file = comparisonDescriptor->m_files[i];

            file.m_namedFunctionMatchInfos = std::move(res->matchedFunctionsData.namedFunctionMatchInfosArray[i]);
            file.remove_async_work_hint(result->command->command_id);
        }
    };

    for (ProgramComparisonDescriptor::File &file : comparisonDescriptor->m_files)
    {
        file.add_async_work_hint(command->command_id, ProgramComparisonDescriptor::File::WorkReason::BuildMatchedFunctions);
    }

    return command;
}

WorkQueueCommandPtr ImGuiApp::create_build_bundles_from_compilands_command(ProgramComparisonDescriptor::File *file)
{
    assert(file != nullptr);
    assert(file->m_compilandBundlesBuilt == TriState::False);
    assert(file->named_functions_built());
    assert(file->m_revisionDescriptor->pdb_loaded());

    auto command = std::make_unique<AsyncBuildBundlesFromCompilandsCommand>(BuildBundlesFromCompilandsOptions(
        file->m_revisionDescriptor->m_namedFunctions,
        file->m_namedFunctionMatchInfos,
        *file->m_revisionDescriptor->m_pdbReader));

    command->options.flags = GuiBuildBundleFlags;

    command->callback = [file](WorkQueueResultPtr &result) {
        auto res = static_cast<AsyncBuildBundlesFromCompilandsResult *>(result.get());
        file->m_compilandBundles = std::move(res->bundles);
        file->m_compilandBundlesBuilt = TriState::True;
        file->remove_async_work_hint(result->command->command_id);
    };

    file->add_async_work_hint(command->command_id, ProgramComparisonDescriptor::File::WorkReason::BuildCompilandBundles);

    return command;
}

WorkQueueCommandPtr ImGuiApp::create_build_bundles_from_source_files_command(ProgramComparisonDescriptor::File *file)
{
    assert(file != nullptr);
    assert(file->m_sourceFileBundlesBuilt == TriState::False);
    assert(file->named_functions_built());
    assert(file->m_revisionDescriptor->pdb_loaded());

    auto command = std::make_unique<AsyncBuildBundlesFromSourceFilesCommand>(BuildBundlesFromSourceFilesOptions(
        file->m_revisionDescriptor->m_namedFunctions,
        file->m_namedFunctionMatchInfos,
        *file->m_revisionDescriptor->m_pdbReader));

    command->options.flags = GuiBuildBundleFlags;

    command->callback = [file](WorkQueueResultPtr &result) {
        auto res = static_cast<AsyncBuildBundlesFromSourceFilesResult *>(result.get());
        file->m_sourceFileBundles = std::move(res->bundles);
        file->m_sourceFileBundlesBuilt = TriState::True;
        file->remove_async_work_hint(result->command->command_id);
    };

    file->add_async_work_hint(command->command_id, ProgramComparisonDescriptor::File::WorkReason::BuildSourceFileBundles);

    return command;
}

WorkQueueCommandPtr ImGuiApp::create_build_single_bundle_command(
    ProgramComparisonDescriptor *comparisonDescriptor,
    size_t bundle_file_idx)
{
    assert(comparisonDescriptor != nullptr);
    assert(comparisonDescriptor->matched_functions_built());
    assert(bundle_file_idx < comparisonDescriptor->m_files.size());

    ProgramComparisonDescriptor::File *file = &comparisonDescriptor->m_files[bundle_file_idx];
    assert(!file->m_singleBundleBuilt);

    auto command = std::make_unique<AsyncBuildSingleBundleCommand>(BuildSingleBundleOptions(
        file->m_namedFunctionMatchInfos,
        comparisonDescriptor->m_matchedFunctions,
        bundle_file_idx));

    command->options.flags = GuiBuildBundleFlags;

    command->callback = [file](WorkQueueResultPtr &result) {
        auto res = static_cast<AsyncBuildSingleBundleResult *>(result.get());
        file->m_singleBundle = std::move(res->bundle);
        file->m_singleBundleBuilt = true;
        file->remove_async_work_hint(result->command->command_id);
    };

    file->add_async_work_hint(command->command_id, ProgramComparisonDescriptor::File::WorkReason::BuildSingleBundle);

    return command;
}

WorkQueueCommandPtr ImGuiApp::create_disassemble_selected_functions_command(
    ProgramFileRevisionDescriptorPtr &revisionDescriptor,
    span<const IndexT> namedFunctionIndices)
{
    assert(revisionDescriptor->named_functions_built());
    assert(revisionDescriptor->exe_loaded());

    auto command = std::make_unique<AsyncDisassembleSelectedFunctionsCommand>(DisassembleSelectedFunctionsOptions(
        revisionDescriptor->m_namedFunctions,
        namedFunctionIndices,
        *revisionDescriptor->m_executable));

    command->callback = [revisionDescriptor](WorkQueueResultPtr &result) {
        revisionDescriptor->remove_async_work_hint(result->command->command_id);
    };

    revisionDescriptor->add_async_work_hint(
        command->command_id,
        ProgramFileRevisionDescriptor::WorkReason::DisassembleSelectedFunctions);

    return command;
}

WorkQueueCommandPtr ImGuiApp::create_build_source_lines_for_selected_functions_command(
    ProgramFileRevisionDescriptorPtr &revisionDescriptor,
    span<const IndexT> namedFunctionIndices)
{
    assert(revisionDescriptor->named_functions_built());
    assert(revisionDescriptor->pdb_loaded());

    auto command =
        std::make_unique<AsyncBuildSourceLinesForSelectedFunctionsCommand>(BuildSourceLinesForSelectedFunctionsOptions(
            revisionDescriptor->m_namedFunctions,
            namedFunctionIndices,
            *revisionDescriptor->m_pdbReader));

    command->callback = [revisionDescriptor](WorkQueueResultPtr &result) {
        revisionDescriptor->remove_async_work_hint(result->command->command_id);
    };

    revisionDescriptor->add_async_work_hint(
        command->command_id,
        ProgramFileRevisionDescriptor::WorkReason::BuildSourceLinesForSelectedFunctions);

    return command;
}

WorkQueueCommandPtr ImGuiApp::create_load_source_files_for_selected_functions_command(
    ProgramFileRevisionDescriptorPtr &revisionDescriptor,
    span<const IndexT> namedFunctionIndices)
{
    assert(revisionDescriptor->named_functions_built());

    auto command =
        std::make_unique<AsyncLoadSourceFilesForSelectedFunctionsCommand>(LoadSourceFilesForSelectedFunctionsOptions(
            revisionDescriptor->m_fileContentStorage,
            revisionDescriptor->m_namedFunctions,
            namedFunctionIndices));

    command->callback = [revisionDescriptor](WorkQueueResultPtr &result) {
        auto res = static_cast<AsyncLoadSourceFilesForSelectedFunctionsResult *>(result.get());
        if (!res->success)
        {
            // Show error?
        }
        revisionDescriptor->remove_async_work_hint(result->command->command_id);
    };

    revisionDescriptor->add_async_work_hint(
        command->command_id,
        ProgramFileRevisionDescriptor::WorkReason::LoadSourceFilesForSelectedFunctions);

    return command;
}

WorkQueueCommandPtr ImGuiApp::create_process_selected_functions_command(
    ProgramFileRevisionDescriptorPtr &revisionDescriptor,
    span<const IndexT> namedFunctionIndices)
{
    auto command = create_disassemble_selected_functions_command(revisionDescriptor, namedFunctionIndices);

    if (revisionDescriptor->pdb_loaded())
    {
        command->chain_to_last([=](WorkQueueResultPtr &result) mutable {
            return create_build_source_lines_for_selected_functions_command(revisionDescriptor, namedFunctionIndices);
        });

        command->chain_to_last([=](WorkQueueResultPtr &result) mutable {
            return create_load_source_files_for_selected_functions_command(revisionDescriptor, namedFunctionIndices);
        });
    }

    return command;
}

WorkQueueCommandPtr ImGuiApp::create_build_comparison_records_for_selected_functions_command(
    ProgramComparisonDescriptor *comparisonDescriptor,
    span<const IndexT> matchedFunctionIndices)
{
    assert(comparisonDescriptor->named_functions_built());
    assert(comparisonDescriptor->matched_functions_built());

    auto namedFunctionsPair = ConstNamedFunctionsPair{
        &comparisonDescriptor->m_files[0].m_revisionDescriptor->m_namedFunctions,
        &comparisonDescriptor->m_files[1].m_revisionDescriptor->m_namedFunctions};

    auto command = std::make_unique<AsyncBuildComparisonRecordsForSelectedFunctionsCommand>(
        BuildComparisonRecordsForSelectedFunctionsOptions(
            comparisonDescriptor->m_matchedFunctions,
            namedFunctionsPair,
            matchedFunctionIndices));

    command->callback = [comparisonDescriptor, matchedFunctionIndices](WorkQueueResultPtr &result) {
        comparisonDescriptor->update_matched_named_function_ui_infos(matchedFunctionIndices);
        if (--comparisonDescriptor->m_pendingBuildComparisonRecordsCommands == 0)
        {
            comparisonDescriptor->update_all_bundle_ui_infos();
        }
        for (ProgramComparisonDescriptor::File &file : comparisonDescriptor->m_files)
        {
            file.remove_async_work_hint(result->command->command_id);
        }
    };

    ++comparisonDescriptor->m_pendingBuildComparisonRecordsCommands;

    for (ProgramComparisonDescriptor::File &file : comparisonDescriptor->m_files)
    {
        file.add_async_work_hint(
            command->command_id,
            ProgramComparisonDescriptor::File::WorkReason::BuildComparisonRecordsForSelectedFunctions);
    }

    return command;
}

ProgramFileDescriptor *ImGuiApp::get_program_file_descriptor(size_t program_file_idx)
{
    if (program_file_idx < m_programFiles.size())
    {
        return m_programFiles[program_file_idx].get();
    }
    return nullptr;
}

void ImGuiApp::load_async(ProgramFileDescriptor *descriptor)
{
    assert(descriptor != nullptr);
    assert(!descriptor->has_async_work());

    descriptor->create_new_revision_descriptor();

    auto command = create_load_command(descriptor->m_revisionDescriptor);

    m_workQueue.enqueue(std::move(command));
}

void ImGuiApp::save_config_async(ProgramFileDescriptor *descriptor)
{
    assert(descriptor != nullptr);
    assert(!descriptor->has_async_work());
    assert(descriptor->m_revisionDescriptor != nullptr);

    WorkQueueDelayedCommand head_command;
    WorkQueueDelayedCommand *next_command = &head_command;

    if (descriptor->can_save_exe_config())
    {
        descriptor->m_revisionDescriptor->m_exeConfigFilenameCopy = descriptor->m_exeConfigFilename;
        next_command = next_command->chain(
            [descriptor, revisionDescriptor = descriptor->m_revisionDescriptor](WorkQueueResultPtr &result) mutable
            -> WorkQueueCommandPtr { return create_save_exe_config_command(revisionDescriptor); });
    }

    if (descriptor->can_save_pdb_config())
    {
        descriptor->m_revisionDescriptor->m_pdbConfigFilenameCopy = descriptor->m_pdbConfigFilename;
        next_command = next_command->chain(
            [descriptor, revisionDescriptor = descriptor->m_revisionDescriptor](WorkQueueResultPtr &result) mutable
            -> WorkQueueCommandPtr { return create_save_pdb_config_command(revisionDescriptor); });
    }

    assert(head_command.next_delayed_command != nullptr);

    m_workQueue.enqueue(head_command);
}

void ImGuiApp::load_and_init_comparison_async(
    ProgramFileDescriptorPair fileDescriptorPair,
    ProgramComparisonDescriptor *comparisonDescriptor)
{
    assert(fileDescriptorPair[0]->can_load() || fileDescriptorPair[0]->exe_loaded());
    assert(fileDescriptorPair[1]->can_load() || fileDescriptorPair[1]->exe_loaded());
    assert(!comparisonDescriptor->has_async_work());

    for (int i = 0; i < 2; ++i)
    {
        ProgramComparisonDescriptor::File &file = comparisonDescriptor->m_files[i];

        if (file.m_revisionDescriptor == nullptr || file.m_revisionDescriptor != fileDescriptorPair[i]->m_revisionDescriptor)
        {
            // Force rebuild matched functions when at least one of the files needs to be loaded first or has changed.
            comparisonDescriptor->prepare_rebuild();
            break;
        }
    }

    bool isAsyncLoading = false;
    const bool isComparingSameFile = fileDescriptorPair[0] == fileDescriptorPair[1];
    const int loadFileCount = isComparingSameFile ? 1 : 2;

    for (int i = 0; i < loadFileCount; ++i)
    {
        ProgramFileDescriptor *fileDescriptor = fileDescriptorPair[i];

        if (fileDescriptor->exe_loaded())
        {
            // Executable is already loaded. Use it.

            if (isComparingSameFile)
            {
                comparisonDescriptor->m_files[0].m_revisionDescriptor = fileDescriptor->m_revisionDescriptor;
                comparisonDescriptor->m_files[1].m_revisionDescriptor = fileDescriptor->m_revisionDescriptor;
            }
            else
            {
                comparisonDescriptor->m_files[i].m_revisionDescriptor = fileDescriptor->m_revisionDescriptor;
            }
        }
        else
        {
            // Executable is not yet loaded. Load it first.

            fileDescriptor->create_new_revision_descriptor();

            if (isComparingSameFile)
            {
                comparisonDescriptor->m_files[0].m_revisionDescriptor = fileDescriptor->m_revisionDescriptor;
                comparisonDescriptor->m_files[1].m_revisionDescriptor = fileDescriptor->m_revisionDescriptor;
            }
            else
            {
                comparisonDescriptor->m_files[i].m_revisionDescriptor = fileDescriptor->m_revisionDescriptor;
            }

            auto command = create_load_command(fileDescriptor->m_revisionDescriptor);

            command->chain_to_last([this, comparisonDescriptor](WorkQueueResultPtr &result) -> WorkQueueCommandPtr {
                if (comparisonDescriptor->executables_loaded())
                {
                    init_comparison_async(comparisonDescriptor);
                }
                return nullptr;
            });

            m_workQueue.enqueue(std::move(command));
            isAsyncLoading = true;
        }
    }

    if (!isAsyncLoading)
    {
        init_comparison_async(comparisonDescriptor);
    }
}

void ImGuiApp::init_comparison_async(ProgramComparisonDescriptor *comparisonDescriptor)
{
    assert(comparisonDescriptor != nullptr);
    assert(!comparisonDescriptor->has_async_work());

    if (!comparisonDescriptor->named_functions_built())
    {
        build_named_functions_async(comparisonDescriptor);
    }
    else if (!comparisonDescriptor->matched_functions_built())
    {
        build_matched_functions_async(comparisonDescriptor);
    }
    else if (!comparisonDescriptor->bundles_ready())
    {
        build_bundled_functions_async(comparisonDescriptor);
    }
    else
    {
        // All async commands have finished. Finalize initialization.

        assert(comparisonDescriptor->executables_loaded());
        assert(comparisonDescriptor->named_functions_built());
        assert(comparisonDescriptor->matched_functions_built());
        assert(comparisonDescriptor->bundles_ready());

        comparisonDescriptor->init();
    }
}

void ImGuiApp::build_named_functions_async(ProgramComparisonDescriptor *comparisonDescriptor)
{
    std::array<ProgramFileRevisionDescriptorPtr, 2> revisionDescriptorPair = {
        comparisonDescriptor->m_files[0].m_revisionDescriptor,
        comparisonDescriptor->m_files[1].m_revisionDescriptor};

    const bool isComparingSameFile = revisionDescriptorPair[0] == revisionDescriptorPair[1];
    const int loadFileCount = isComparingSameFile ? 1 : 2;

    for (size_t i = 0; i < loadFileCount; ++i)
    {
        assert(revisionDescriptorPair[i] != nullptr);

        if (!revisionDescriptorPair[i]->named_functions_built())
        {
            auto command = create_build_named_functions_command(revisionDescriptorPair[i]);

            command->chain_to_last([this, comparisonDescriptor](WorkQueueResultPtr &result) -> WorkQueueCommandPtr {
                if (comparisonDescriptor->named_functions_built())
                {
                    // Go to next step.
                    init_comparison_async(comparisonDescriptor);
                }
                return nullptr;
            });

            m_workQueue.enqueue(std::move(command));
        }
    }
}

void ImGuiApp::build_matched_functions_async(ProgramComparisonDescriptor *comparisonDescriptor)
{
    auto command = create_build_matched_functions_command(comparisonDescriptor);

    command->chain_to_last([this, comparisonDescriptor](WorkQueueResultPtr &result) -> WorkQueueCommandPtr {
        assert(comparisonDescriptor->matched_functions_built());

        // Go to next step.
        init_comparison_async(comparisonDescriptor);

        return nullptr;
    });

    m_workQueue.enqueue(std::move(command));
}

void ImGuiApp::build_bundled_functions_async(ProgramComparisonDescriptor *comparisonDescriptor)
{
    for (size_t i = 0; i < 2; ++i)
    {
        ProgramComparisonDescriptor::File &file = comparisonDescriptor->m_files[i];
        assert(file.m_revisionDescriptor != nullptr);

        auto Callback = [this, comparisonDescriptor](WorkQueueResultPtr &result) -> WorkQueueCommandPtr {
            if (comparisonDescriptor->bundles_ready())
            {
                // Go to next step.
                init_comparison_async(comparisonDescriptor);
            }
            return nullptr;
        };

        if (file.m_compilandBundlesBuilt == TriState::False)
        {
            if (file.pdb_loaded())
            {
                auto command = create_build_bundles_from_compilands_command(&file);
                command->chain_to_last(Callback);
                m_workQueue.enqueue(std::move(command));
            }
            else
            {
                file.m_compilandBundlesBuilt = TriState::NotApplicable;
            }
        }

        if (file.m_sourceFileBundlesBuilt == TriState::False)
        {
            if (file.pdb_loaded())
            {
                auto command = create_build_bundles_from_source_files_command(&file);
                command->chain_to_last(Callback);
                m_workQueue.enqueue(std::move(command));
            }
            else
            {
                file.m_sourceFileBundlesBuilt = TriState::NotApplicable;
            }
        }

        if (!file.m_singleBundleBuilt)
        {
            auto command = create_build_single_bundle_command(comparisonDescriptor, i);
            command->chain_to_last(Callback);
            m_workQueue.enqueue(std::move(command));
        }
    }
}

void ImGuiApp::process_named_functions_async(
    ProgramFileRevisionDescriptorPtr &revisionDescriptor,
    span<const IndexT> namedFunctionIndices)
{
    assert(revisionDescriptor != nullptr);
    assert(!namedFunctionIndices.empty());

    auto command = create_process_selected_functions_command(revisionDescriptor, namedFunctionIndices);
    m_workQueue.enqueue(std::move(command));
}

void ImGuiApp::process_matched_functions_async(
    ProgramComparisonDescriptor *comparisonDescriptor,
    span<const IndexT> matchedFunctionIndices)
{
    assert(comparisonDescriptor != nullptr);
    assert(!matchedFunctionIndices.empty());

    auto command =
        create_build_comparison_records_for_selected_functions_command(comparisonDescriptor, matchedFunctionIndices);
    m_workQueue.enqueue(std::move(command));
}

void ImGuiApp::process_named_and_matched_functions_async(
    ProgramComparisonDescriptor *comparisonDescriptor,
    span<const IndexT> matchedFunctionIndices)
{
    assert(comparisonDescriptor != nullptr);
    assert(!matchedFunctionIndices.empty());

    std::array<span<const IndexT>, 2> analyzeNamedFunctionIndicesArray;
    analyzeNamedFunctionIndicesArray[0] = comparisonDescriptor->get_matched_named_function_indices_for_processing(0);
    analyzeNamedFunctionIndicesArray[1] = comparisonDescriptor->get_matched_named_function_indices_for_processing(1);

    int analyzeNamedFunctionsCount = 0;
    for (IndexT i = 0; i < 2; ++i)
        if (!analyzeNamedFunctionIndicesArray[i].empty())
            ++analyzeNamedFunctionsCount;

    if (analyzeNamedFunctionsCount > 0)
    {
        // Process named functions first.

        // Increment here because the command is delayed and therefore the pending work would be unknown at this time.
        ++comparisonDescriptor->m_pendingBuildComparisonRecordsCommands;

        auto sharedWorkCount = std::make_shared<int>(analyzeNamedFunctionsCount);

        for (IndexT i = 0; i < 2; ++i)
        {
            if (!analyzeNamedFunctionIndicesArray[i].empty())
            {
                ProgramFileRevisionDescriptorPtr &revisionDescriptor = comparisonDescriptor->m_files[i].m_revisionDescriptor;

                auto command =
                    create_process_selected_functions_command(revisionDescriptor, analyzeNamedFunctionIndicesArray[i]);

                command->chain_to_last(
                    [this, sharedWorkCount, comparisonDescriptor, matchedFunctionIndices](
                        WorkQueueResultPtr &result) -> WorkQueueCommandPtr {
                        if (--(*sharedWorkCount) == 0)
                        {
                            --comparisonDescriptor->m_pendingBuildComparisonRecordsCommands;
                            assert(comparisonDescriptor->matched_functions_disassembled(matchedFunctionIndices));
                            process_matched_functions_async(comparisonDescriptor, matchedFunctionIndices);
                        }
                        return nullptr;
                    });

                m_workQueue.enqueue(std::move(command));
            }
        }
    }
    else if (comparisonDescriptor->matched_functions_disassembled(matchedFunctionIndices))
    {
        // Named functions are already processed. Proceed with the matched functions.

        process_matched_functions_async(comparisonDescriptor, matchedFunctionIndices);
    }
    else
    {
        // Something else has started the processing of named function but they are not yet finished.
        // #TODO: Add a scheduler or message or something.
    }
}

void ImGuiApp::add_file()
{
    m_programFiles.emplace_back(std::make_unique<ProgramFileDescriptor>());
}

void ImGuiApp::remove_file(size_t idx)
{
    if (idx < m_programFiles.size())
    {
        m_programFiles.erase(m_programFiles.begin() + idx);
    }
}

void ImGuiApp::remove_all_files()
{
    m_programFiles.clear();
}

void ImGuiApp::add_program_comparison()
{
    m_programComparisons.emplace_back(std::make_unique<ProgramComparisonDescriptor>());
}

void ImGuiApp::update_closed_program_comparisons()
{
    // Remove descriptor when window was closed.
    m_programComparisons.erase(
        std::remove_if(
            m_programComparisons.begin(),
            m_programComparisons.end(),
            [](const ProgramComparisonDescriptorPtr &p) { return !p->m_imguiHasOpenWindow && !p->has_async_work(); }),
        m_programComparisons.end());
}

void ImGuiApp::on_functions_interaction(ProgramComparisonDescriptor &descriptor, ProgramComparisonDescriptor::File &file)
{
    file.update_selected_named_functions();
    descriptor.update_selected_matched_functions();

    // Process matched functions, including named functions from both executables.
    {
        const span<const IndexT> matchedFunctionIndices = descriptor.m_processedMatchedFunctions.get_items_for_processing(
            span<const IndexT>{descriptor.m_selectedMatchedFunctionIndices});

        if (!matchedFunctionIndices.empty())
        {
            process_named_and_matched_functions_async(&descriptor, matchedFunctionIndices);
        }
    }

    // Process remaining named functions. These are unmatched functions that were not already processed before.
    {
        const span<const IndexT> namedFunctionIndices =
            file.m_revisionDescriptor->m_processedNamedFunctions.get_items_for_processing(
                span<const IndexT>{file.m_selectedNamedFunctionIndices});

        if (!namedFunctionIndices.empty())
        {
            process_named_functions_async(file.m_revisionDescriptor, namedFunctionIndices);
        }
    }
}

std::string ImGuiApp::create_section_string(uint32_t section_index, const ExeSections *sections)
{
    if (sections != nullptr && section_index < sections->size())
    {
        return (*sections)[section_index].name;
    }
    else
    {
        return fmt::format("{:d}", section_index + 1);
    }
}

std::string ImGuiApp::create_time_string(std::chrono::time_point<std::chrono::system_clock> time_point)
{
    if (time_point == InvalidTimePoint)
        return std::string();

    std::time_t time = std::chrono::system_clock::to_time_t(time_point);
    std::tm *local_time = std::localtime(&time);

    char buffer[32];
    if (0 == std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", local_time))
        return std::string();

    return std::string(buffer);
}

void ImGuiApp::BackgroundWindow()
{
    // #TODO: Make the background dockable somehow. There is a example in ImGui Demo > Examples > Dockspace

    // clang-format off
    constexpr ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_MenuBar |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoDocking;
    // clang-format on

    bool window_open = true;
    ImScoped::Window window("main", &window_open, window_flags);
    if (window.IsContentVisible)
    {
        ImGui::SetWindowPos("main", m_windowPos);
        ImGui::SetWindowSize("main", ImVec2(m_windowSize.x, 0.f));

        ImScoped::MenuBar menu_bar;
        if (menu_bar.IsOpen)
        {
            {
                ImScoped::Menu menu("File");
                if (menu.IsOpen)
                {
                    if (ImGui::MenuItem("Exit"))
                    {
                        // Will wait for all work to finish and then shutdown the app.
                        prepare_shutdown_nowait();
                    }
                    ImGui::SameLine();
                    TooltipTextUnformattedMarker("Graceful shutdown. Finishes all tasks before exiting.");
                }
            }

            {
                ImScoped::Menu menu("Tools");
                if (menu.IsOpen)
                {
                    ImGui::MenuItem("Program File Manager", nullptr, &m_showFileManager);
                    ImGui::MenuItem("Assembler Output", nullptr, &m_showOutputManager);

                    if (ImGui::MenuItem("New Assembler Comparison"))
                    {
                        add_program_comparison();
                    }
                    ImGui::SameLine();
                    TooltipTextUnformattedMarker("Opens a new Assembler Comparison window.");
                }
            }
        }
    }
}

void ImGuiApp::FileManagerWindow(bool *p_open)
{
    ImScoped::Window window("File Manager", p_open, ImGuiWindowFlags_MenuBar);
    if (window.IsContentVisible)
    {
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Add File"))
                {
                    add_file();
                }
                if (ImGui::MenuItem("Remove All Files"))
                {
                    remove_all_files();
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View"))
            {
                ImGui::MenuItem("Show Tabs", nullptr, &m_showFileManagerWithTabs);
                ImGui::MenuItem("Show Exe Section Info", nullptr, &m_showFileManagerExeSectionInfo);
                ImGui::MenuItem("Show Exe Symbol Info", nullptr, &m_showFileManagerExeSymbolInfo);
                ImGui::MenuItem("Show Pdb Compiland Info", nullptr, &m_showFileManagerPdbCompilandInfo);
                ImGui::MenuItem("Show Pdb Source File Info", nullptr, &m_showFileManagerPdbSourceFileInfo);
                ImGui::MenuItem("Show Pdb Symbol Info", nullptr, &m_showFileManagerPdbSymbolInfo);
                ImGui::MenuItem("Show Pdb Function Info", nullptr, &m_showFileManagerPdbFunctionInfo);
                ImGui::MenuItem("Show Pdb Exe Info", nullptr, &m_showFileManagerPdbExeInfo);
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
        FileManagerBody();
    }
}

void ImGuiApp::OutputManagerWindow(bool *p_open)
{
    ImScoped::Window window("Assembler Output Manager", p_open);
    if (window.IsContentVisible)
    {
        OutputManagerBody();
    }
}

void ImGuiApp::ComparisonManagerWindows()
{
    const size_t count = m_programComparisons.size();
    for (size_t i = 0; i < count; ++i)
    {
        ProgramComparisonDescriptor &descriptor = *m_programComparisons[i];

        const std::string title = fmt::format("Assembler Comparison {:d}", descriptor.m_id);

        ImScoped::Window window(title.c_str(), &descriptor.m_imguiHasOpenWindow);
        ImScoped::ID id(i);
        if (window.IsContentVisible && descriptor.m_imguiHasOpenWindow)
        {
            ComparisonManagerBody(descriptor);
        }
    }

    update_closed_program_comparisons();
}

namespace
{
static const char *const g_browse_file_button_label = "Browse ..";
static const std::string g_select_file_dialog_title = "Select File";
} // namespace

void ImGuiApp::FileManagerBody()
{
    FileManagerGlobalButtons();

    ImGui::SeparatorText("File List");

    size_t erase_idx = size_t(~0);

    bool show_files = !m_programFiles.empty();

    if (show_files && m_showFileManagerWithTabs)
    {
        show_files = ImGui::BeginTabBar("##file_tabs");
    }

    if (show_files)
    {
        for (size_t i = 0; i < m_programFiles.size(); ++i)
        {
            ProgramFileDescriptor &descriptor = *m_programFiles[i];

            // Set a unique ID for each element to avoid ID conflicts
            ImScoped::ID id(i);

            bool is_open = false;

            if (m_showFileManagerWithTabs)
            {
                // Tab items cannot have dynamic labels without bugs. Force consistent names.
                const std::string title = descriptor.create_descriptor_name();
                is_open = ImGui::BeginTabItem(title.c_str());
                // Tooltip on hover tab.
                const std::string exe_name = descriptor.create_short_exe_name();
                if (!exe_name.empty())
                    TooltipTextUnformatted(exe_name);
            }
            else
            {
                const std::string title = descriptor.create_descriptor_name_with_file_info();
                is_open = TreeNodeHeader("##file_tree", ImGuiTreeNodeFlags_DefaultOpen, title.c_str());
            }

            if (is_open)
            {
                bool erased;
                FileManagerDescriptor(descriptor, erased);
                if (erased)
                    erase_idx = i;

                if (m_showFileManagerWithTabs)
                {
                    ImGui::EndTabItem();
                }
            }
        }

        if (m_showFileManagerWithTabs)
        {
            ImGui::EndTabBar();
        }

        // Erase at the end to avoid incomplete elements.
        remove_file(erase_idx);
    }
}

void ImGuiApp::FileManagerDescriptor(ProgramFileDescriptor &descriptor, bool &erased)
{
    {
        ImScoped::Group group;
        ImScoped::Disabled disabled(descriptor.has_async_work());
        ImScoped::ItemWidth item_width(ImGui::GetFontSize() * -12);

        FileManagerDescriptorExeFile(descriptor);

        FileManagerDescriptorExeConfig(descriptor);

        FileManagerDescriptorPdbFile(descriptor);

        FileManagerDescriptorPdbConfig(descriptor);

        FileManagerDescriptorActions(descriptor, erased);
    }

    const ImRect groupRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
    FileManagerDescriptorProgressOverlay(descriptor, groupRect);

    ProgramFileRevisionDescriptor *revisionDescriptor = descriptor.m_revisionDescriptor.get();

    if (revisionDescriptor != nullptr)
    {
        FileManagerDescriptorSaveLoadStatus(*revisionDescriptor);
        FileManagerInfoNode(descriptor, *revisionDescriptor);
    }

    ImGui::Spacing();
}

void ImGuiApp::FileManagerDescriptorExeFile(ProgramFileDescriptor &descriptor)
{
    FileDialogButton(
        g_browse_file_button_label,
        &descriptor.m_exeFilename,
        fmt::format("exe_file_dialog{:d}", descriptor.m_id),
        g_select_file_dialog_title,
        "Program (*.*){((.*))}"); // ((.*)) is regex for all files

    ImGui::SameLine();
    ImGui::InputTextWithHint("Program File", auto_str, &descriptor.m_exeFilename);

    // Tooltip on hover 'auto'
    if (is_auto_str(descriptor.m_exeFilename) && !descriptor.has_async_work())
    {
        const std::string exe_filename = descriptor.evaluate_exe_filename();
        if (exe_filename.empty())
        {
            TooltipText("'%s' evaluates when the Pdb file is read", auto_str);
        }
        else
        {
            TooltipText("'%s' evaluates to '%s'", auto_str, exe_filename.c_str());
        }
    }
}

void ImGuiApp::FileManagerDescriptorExeConfig(ProgramFileDescriptor &descriptor)
{
    FileDialogButton(
        g_browse_file_button_label,
        &descriptor.m_exeConfigFilename,
        fmt::format("exe_config_file_dialog{:d}", descriptor.m_id),
        g_select_file_dialog_title,
        "Config (*.json){.json}");

    ImGui::SameLine();
    ImGui::InputTextWithHint("Program Config File", auto_str, &descriptor.m_exeConfigFilename);

    // Tooltip on hover 'auto'
    if (is_auto_str(descriptor.m_exeConfigFilename) && !descriptor.m_exeFilename.empty() && !descriptor.has_async_work())
    {
        const std::string exe_filename = descriptor.evaluate_exe_filename();
        if (exe_filename.empty())
        {
            TooltipText("'%s' evaluates when the Pdb file is read", auto_str);
        }
        else
        {
            const std::string config_filename = descriptor.evaluate_exe_config_filename();
            TooltipText("'%s' evaluates to '%s'", auto_str, config_filename.c_str());
        }
    }
}

void ImGuiApp::FileManagerDescriptorPdbFile(ProgramFileDescriptor &descriptor)
{
    FileDialogButton(
        g_browse_file_button_label,
        &descriptor.m_pdbFilename,
        fmt::format("pdb_file_dialog{:d}", descriptor.m_id),
        g_select_file_dialog_title,
        "Program Database (*.pdb){.pdb}");

    ImGui::SameLine();
    ImGui::InputText("Pdb File", &descriptor.m_pdbFilename);
}

void ImGuiApp::FileManagerDescriptorPdbConfig(ProgramFileDescriptor &descriptor)
{
    FileDialogButton(
        g_browse_file_button_label,
        &descriptor.m_pdbConfigFilename,
        fmt::format("pdb_config_file_dialog{:d}", descriptor.m_id),
        g_select_file_dialog_title,
        "Config (*.json){.json}");

    ImGui::SameLine();
    ImGui::InputTextWithHint("Pdb Config File", auto_str, &descriptor.m_pdbConfigFilename);

    // Tooltip on hover 'auto'
    if (is_auto_str(descriptor.m_pdbConfigFilename) && !descriptor.m_pdbFilename.empty())
    {
        const std::string config_filename = descriptor.evaluate_pdb_config_filename();
        TooltipText("'%s' evaluates to '%s'", auto_str, config_filename.c_str());
    }
}

void ImGuiApp::FileManagerDescriptorActions(ProgramFileDescriptor &descriptor, bool &erased)
{
    // Action buttons
    {
        bool open;
        {
            // Change button color.
            ImScoped::StyleColor color1(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.0f, 0.2f));
            ImScoped::StyleColor color2(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.0f, 0.0f, 0.3f));
            ImScoped::StyleColor color3(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.0f, 0.0f, 0.4f));
            // Change text color too to make it readable with the light ImGui color theme.
            ImScoped::StyleColor text_color1(ImGuiCol_Text, ImVec4(1.00f, 1.00f, 1.00f, 1.00f));
            ImScoped::StyleColor text_color2(ImGuiCol_TextDisabled, ImVec4(0.50f, 0.50f, 0.50f, 1.00f));

            open = Button("Remove");
        }
        const std::string descriptorName = descriptor.create_descriptor_name();
        const std::string dialogTitle = fmt::format("Remove {:s}?", descriptorName);
        erased = UpdateConfirmationPopup(
            open,
            dialogTitle.c_str(),
            "Are you sure you want to remove this file from the list? It will not be deleted from disk.");
    }

    ImGui::SameLine();
    {
        ImScoped::Disabled disabled(!descriptor.can_load());
        // Change button color.
        ImScoped::StyleColor color1(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.3f, 0.6f, 0.6f));
        ImScoped::StyleColor color2(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.3f, 0.8f, 0.8f));
        ImScoped::StyleColor color3(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.3f, 1.0f, 1.0f));
        // Change text color too to make it readable with the light ImGui color theme.
        ImScoped::StyleColor text_color1(ImGuiCol_Text, ImVec4(1.00f, 1.00f, 1.00f, 1.00f));
        ImScoped::StyleColor text_color2(ImGuiCol_TextDisabled, ImVec4(0.50f, 0.50f, 0.50f, 1.00f));
        if (Button("Load"))
        {
            load_async(&descriptor);
        }
    }

    ImGui::SameLine();
    {
        ImScoped::Disabled disabled(!descriptor.can_save_config());
        // Change button color.
        ImScoped::StyleColor color1(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.6f));
        ImScoped::StyleColor color2(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));
        ImScoped::StyleColor color3(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.0f, 1.0f, 1.0f));
        // Change text color too to make it readable with the light ImGui color theme.
        ImScoped::StyleColor text_color1(ImGuiCol_Text, ImVec4(1.00f, 1.00f, 1.00f, 1.00f));
        ImScoped::StyleColor text_color2(ImGuiCol_TextDisabled, ImVec4(0.50f, 0.50f, 0.50f, 1.00f));
        if (Button("Save Config"))
        {
            save_config_async(&descriptor);
        }
    }
}

void ImGuiApp::FileManagerDescriptorProgressOverlay(const ProgramFileDescriptor &descriptor, const ImRect &rect)
{
    if (descriptor.has_async_work())
    {
        const std::string overlay = fmt::format("Processing command {:d} ..", descriptor.get_first_active_command_id());

        OverlayProgressBar(rect, -1.0f * (float)ImGui::GetTime(), overlay.c_str());
    }
}

void ImGuiApp::FileManagerDescriptorSaveLoadStatus(const ProgramFileRevisionDescriptor &descriptor)
{
    FileManagerDescriptorLoadStatus(descriptor);
    FileManagerDescriptorSaveStatus(descriptor);
}

void ImGuiApp::FileManagerDescriptorLoadStatus(const ProgramFileRevisionDescriptor &descriptor)
{
    switch (descriptor.m_exeLoaded)
    {
        case TriState::True:
            DrawInTextCircle(GreenColor);
            ImGui::Text(
                " Loaded Exe: [Revision:%u] [%s] %s",
                descriptor.m_id,
                create_time_string(descriptor.m_exeLoadTimepoint).c_str(),
                descriptor.m_executable->get_filename().c_str());
            break;

        case TriState::False:
            DrawInTextCircle(RedColor);
            ImGui::Text(
                " Failed to load Exe: [Revision:%u] [%s] %s",
                descriptor.m_id,
                create_time_string(descriptor.m_exeLoadTimepoint).c_str(),
                descriptor.m_exeFilenameCopy);
            break;
    }

    switch (descriptor.m_pdbLoaded)
    {
        case TriState::True:
            DrawInTextCircle(GreenColor);
            ImGui::Text(
                " Loaded Pdb: [Revision:%u] [%s] %s",
                descriptor.m_id,
                create_time_string(descriptor.m_pdbLoadTimepoint).c_str(),
                descriptor.m_pdbReader->get_filename().c_str());
            break;

        case TriState::False:
            DrawInTextCircle(RedColor);
            ImGui::Text(
                " Failed to load Pdb: [Revision:%u] [%s] %s",
                descriptor.m_id,
                create_time_string(descriptor.m_pdbLoadTimepoint).c_str(),
                descriptor.m_pdbFilenameCopy);
            break;
    }
}

void ImGuiApp::FileManagerDescriptorSaveStatus(const ProgramFileRevisionDescriptor &descriptor)
{
    switch (descriptor.m_exeConfigSaved)
    {
        case TriState::True:
            DrawInTextCircle(GreenColor);
            ImGui::Text(
                " Saved Exe Config: [Revision:%u] [%s] %s",
                descriptor.m_id,
                create_time_string(descriptor.m_exeSaveConfigTimepoint).c_str(),
                descriptor.m_exeSaveConfigFilename.c_str());
            break;

        case TriState::False:
            DrawInTextCircle(RedColor);
            ImGui::Text(
                " Failed to save Exe Config: [Revision:%u] [%s] %s",
                descriptor.m_id,
                create_time_string(descriptor.m_exeSaveConfigTimepoint).c_str(),
                descriptor.m_exeSaveConfigFilename.c_str());
            break;
    }

    switch (descriptor.m_pdbConfigSaved)
    {
        case TriState::True:
            DrawInTextCircle(GreenColor);
            ImGui::Text(
                " Saved Pdb Config: [Revision:%u] [%s] %s",
                descriptor.m_id,
                create_time_string(descriptor.m_pdbSaveConfigTimepoint).c_str(),
                descriptor.m_pdbSaveConfigFilename.c_str());
            break;

        case TriState::False:
            DrawInTextCircle(RedColor);
            ImGui::Text(
                " Failed to save Pdb Config: [Revision:%u] [%s] %s",
                descriptor.m_id,
                create_time_string(descriptor.m_pdbSaveConfigTimepoint).c_str(),
                descriptor.m_pdbSaveConfigFilename.c_str());
            break;
    }
}

void ImGuiApp::FileManagerGlobalButtons()
{
    if (Button("Add File"))
    {
        add_file();
    }

    ImGui::SameLine();
    {
        const bool can_load_any =
            std::any_of(m_programFiles.begin(), m_programFiles.end(), [](const ProgramFileDescriptorPtr &descriptor) {
                return descriptor->can_load();
            });
        ImScoped::Disabled disabled(!can_load_any);
        // Change button color.
        ImScoped::StyleColor color1(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.3f, 0.6f, 0.6f));
        ImScoped::StyleColor color2(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.3f, 0.8f, 0.8f));
        ImScoped::StyleColor color3(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.3f, 1.0f, 1.0f));
        // Change text color too to make it readable with the light ImGui color theme.
        ImScoped::StyleColor text_color1(ImGuiCol_Text, ImVec4(1.00f, 1.00f, 1.00f, 1.00f));
        ImScoped::StyleColor text_color2(ImGuiCol_TextDisabled, ImVec4(0.50f, 0.50f, 0.50f, 1.00f));
        if (Button("Load All"))
        {
            for (ProgramFileDescriptorPtr &descriptor : m_programFiles)
            {
                if (!descriptor->can_load())
                    continue;
                load_async(descriptor.get());
            }
        }
    }
}

void ImGuiApp::FileManagerInfoNode(
    ProgramFileDescriptor &fileDescriptor,
    const ProgramFileRevisionDescriptor &revisionDescriptor)
{
    if (revisionDescriptor.m_executable != nullptr || revisionDescriptor.m_pdbReader != nullptr)
    {
        ImScoped::TreeNodeEx tree("Info", ImGuiTreeNodeFlags_SpanAvailWidth);
        if (tree.IsOpen)
        {
            FileManagerInfo(fileDescriptor, revisionDescriptor);
        }
    }
}

void ImGuiApp::FileManagerInfo(
    ProgramFileDescriptor &fileDescriptor,
    const ProgramFileRevisionDescriptor &revisionDescriptor)
{
    ImScoped::ItemWidth itemWidth(ImGui::GetFontSize() * -12);

    if (revisionDescriptor.m_executable != nullptr)
    {
        if (m_showFileManagerExeSectionInfo)
            FileManagerInfoExeSections(revisionDescriptor);

        if (m_showFileManagerExeSymbolInfo)
            FileManagerInfoExeSymbols(fileDescriptor, revisionDescriptor);
    }
    if (revisionDescriptor.m_pdbReader != nullptr)
    {
        if (m_showFileManagerPdbCompilandInfo)
            FileManagerInfoPdbCompilands(revisionDescriptor);

        if (m_showFileManagerPdbSourceFileInfo)
            FileManagerInfoPdbSourceFiles(revisionDescriptor);

        if (m_showFileManagerPdbSymbolInfo)
            FileManagerInfoPdbSymbols(fileDescriptor, revisionDescriptor);

        if (m_showFileManagerPdbFunctionInfo)
            FileManagerInfoPdbFunctions(fileDescriptor, revisionDescriptor);

        if (m_showFileManagerPdbExeInfo)
            FileManagerInfoPdbExeInfo(revisionDescriptor);
    }
}

void ImGuiApp::FileManagerInfoExeSections(const ProgramFileRevisionDescriptor &descriptor)
{
    ImGui::SeparatorText("Exe Sections");

    ImGui::Text("Exe Image base: %08x", down_cast<uint32_t>(descriptor.m_executable->image_base()));

    const ExeSections &sections = descriptor.m_executable->get_sections();

    ImGui::Text("Count: %zu", sections.size());

    const float defaultHeight = GetDefaultTableHeight(sections.size(), 10);
    ImScoped::Child child("exe_sections_container", ImVec2(0.0f, defaultHeight), ImGuiChildFlags_ResizeY);
    if (child.IsContentVisible)
    {
        if (ImGui::BeginTable("exe_sections", 3, FileManagerInfoTableFlags))
        {
            ImGui::TableSetupScrollFreeze(0, 1); // Makes top row always visible.
            ImGui::TableSetupColumn("Address");
            ImGui::TableSetupColumn("Size");
            ImGui::TableSetupColumn("Name");
            ImGui::TableHeadersRow();

            for (const ExeSectionInfo &section : sections)
            {
                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::Text("%08x", down_cast<uint32_t>(section.address));

                ImGui::TableNextColumn();
                ImGui::Text("%08x", down_cast<uint32_t>(section.size));

                ImGui::TableNextColumn();
                TextUnformatted(section.name);
            }
            ImGui::EndTable();
        }
    }
}

void ImGuiApp::FileManagerInfoExeSymbols(
    ProgramFileDescriptor &fileDescriptor,
    const ProgramFileRevisionDescriptor &revisionDescriptor)
{
    ImGui::SeparatorText("Exe Symbols");

    const auto &filtered = fileDescriptor.m_exeSymbolsFilter.filtered;
    {
        const ExeSymbols &symbols = revisionDescriptor.m_executable->get_symbols();

        UpdateFilter(
            fileDescriptor.m_exeSymbolsFilter,
            symbols,
            [](const ImGuiTextFilterEx &filter, const ExeSymbol &symbol) -> bool { return filter.PassFilter(symbol.name); });

        ImGui::Text("Count: %d, Filtered: %d", int(symbols.size()), filtered.size());
    }

    const float defaultHeight = GetDefaultTableHeight(filtered.size(), 10);
    ImScoped::Child child("exe_symbols_container", ImVec2(0.0f, defaultHeight), ImGuiChildFlags_ResizeY);
    if (child.IsContentVisible)
    {
        if (ImGui::BeginTable("exe_symbols", 3, FileManagerInfoTableFlags))
        {
            ImGui::TableSetupScrollFreeze(0, 1); // Makes top row always visible.
            ImGui::TableSetupColumn("Address");
            ImGui::TableSetupColumn("Size");
            ImGui::TableSetupColumn("Name");
            ImGui::TableHeadersRow();

            ImGuiListClipper clipper;
            clipper.Begin(filtered.size());

            while (clipper.Step())
            {
                for (int n = clipper.DisplayStart; n < clipper.DisplayEnd; ++n)
                {
                    const ExeSymbol &symbol = *filtered[n];

                    ImGui::TableNextRow();

                    ImGui::TableNextColumn();
                    ImGui::Text("%08x", down_cast<uint32_t>(symbol.address));

                    ImGui::TableNextColumn();
                    ImGui::Text("%08x", down_cast<uint32_t>(symbol.size));

                    ImGui::TableNextColumn();
                    TextUnformatted(symbol.name);
                }
            }

            ImGui::EndTable();
        }
    }
}

void ImGuiApp::FileManagerInfoPdbCompilands(const ProgramFileRevisionDescriptor &descriptor)
{
    ImGui::SeparatorText("Pdb Compilands");

    const PdbCompilandInfoVector &compilands = descriptor.m_pdbReader->get_compilands();

    ImGui::Text("Count: %zu", compilands.size());

    const float defaultHeight = GetDefaultTableHeight(compilands.size(), 10);
    ImScoped::Child child("pdb_compilands_container", ImVec2(0.0f, defaultHeight), ImGuiChildFlags_ResizeY);
    if (child.IsContentVisible)
    {
        if (ImGui::BeginTable("pdb_compilands", 1, FileManagerInfoTableFlags))
        {
            ImGui::TableSetupScrollFreeze(0, 1); // Makes top row always visible.
            ImGui::TableSetupColumn("Name");
            ImGui::TableHeadersRow();

            ImGuiListClipper clipper;
            clipper.Begin(compilands.size());

            while (clipper.Step())
            {
                for (int n = clipper.DisplayStart; n < clipper.DisplayEnd; ++n)
                {
                    const PdbCompilandInfo &compiland = compilands[n];

                    ImGui::TableNextRow();

                    ImGui::TableNextColumn();
                    TextUnformatted(compiland.name);
                }
            }

            ImGui::EndTable();
        }
    }
}

void ImGuiApp::FileManagerInfoPdbSourceFiles(const ProgramFileRevisionDescriptor &descriptor)
{
    ImGui::SeparatorText("Pdb Source Files");

    const PdbSourceFileInfoVector &source_files = descriptor.m_pdbReader->get_source_files();

    ImGui::Text("Count: %zu", source_files.size());

    const float defaultHeight = GetDefaultTableHeight(source_files.size(), 10);
    ImScoped::Child child("pdb_source_files_container", ImVec2(0.0f, defaultHeight), ImGuiChildFlags_ResizeY);
    if (child.IsContentVisible)
    {
        if (ImGui::BeginTable("pdb_source_files", 3, FileManagerInfoTableFlags))
        {
            ImGui::TableSetupScrollFreeze(0, 1); // Makes top row always visible.
            ImGui::TableSetupColumn("Checksum Type");
            ImGui::TableSetupColumn("Checksum");
            ImGui::TableSetupColumn("Name");
            ImGui::TableHeadersRow();

            ImGuiListClipper clipper;
            clipper.Begin(source_files.size());

            while (clipper.Step())
            {
                for (int n = clipper.DisplayStart; n < clipper.DisplayEnd; ++n)
                {
                    const PdbSourceFileInfo &source_file = source_files[n];

                    ImGui::TableNextRow();

                    ImGui::TableNextColumn();
                    switch (source_file.checksumType)
                    {
                        default:
                        case CV_Chksum::CHKSUM_TYPE_NONE:
                            TextUnformatted("none");
                            break;
                        case CV_Chksum::CHKSUM_TYPE_MD5:
                            TextUnformatted("md5");
                            break;
                        case CV_Chksum::CHKSUM_TYPE_SHA1:
                            TextUnformatted("sha1");
                            break;
                        case CV_Chksum::CHKSUM_TYPE_SHA_256:
                            TextUnformatted("sha256");
                            break;
                    }

                    const std::string checksum = util::to_hex_string(source_file.checksum); // #TODO: Cache this

                    ImGui::TableNextColumn();
                    TextUnformatted(checksum);

                    ImGui::TableNextColumn();
                    TextUnformatted(source_file.name);
                }
            }

            ImGui::EndTable();
        }
    }
}

void ImGuiApp::FileManagerInfoPdbSymbols(
    ProgramFileDescriptor &fileDescriptor,
    const ProgramFileRevisionDescriptor &revisionDescriptor)
{
    ImGui::SeparatorText("Pdb Symbols");

    const auto &filtered = fileDescriptor.m_pdbSymbolsFilter.filtered;
    {
        const PdbSymbolInfoVector &symbols = revisionDescriptor.m_pdbReader->get_symbols();

        UpdateFilter(
            fileDescriptor.m_pdbSymbolsFilter,
            symbols,
            [](const ImGuiTextFilterEx &filter, const PdbSymbolInfo &symbol) -> bool {
                if (filter.PassFilter(symbol.decoratedName))
                    return true;
                if (filter.PassFilter(symbol.undecoratedName))
                    return true;
                if (filter.PassFilter(symbol.globalName))
                    return true;
                return false;
            });

        ImGui::Text("Count: %d, Filtered: %d", int(symbols.size()), filtered.size());
    }

    const ExeSections *sections = nullptr;
    if (revisionDescriptor.m_executable != nullptr)
        sections = &revisionDescriptor.m_executable->get_sections();

    const float defaultHeight = GetDefaultTableHeight(filtered.size(), 10);
    ImScoped::Child child("pdb_symbols_container", ImVec2(0.0f, defaultHeight), ImGuiChildFlags_ResizeY);
    if (child.IsContentVisible)
    {
        if (ImGui::BeginTable("pdb_symbols", 6, FileManagerInfoTableFlags))
        {
            ImGui::TableSetupScrollFreeze(0, 1); // Makes top row always visible.
            ImGui::TableSetupColumn("Address");
            ImGui::TableSetupColumn("Size");
            ImGui::TableSetupColumn("Section");
            ImGui::TableSetupColumn("Decorated Name");
            ImGui::TableSetupColumn("Undecorated Name");
            ImGui::TableSetupColumn("Global Name");
            ImGui::TableHeadersRow();

            ImGuiListClipper clipper;
            clipper.Begin(filtered.size());

            while (clipper.Step())
            {
                for (int n = clipper.DisplayStart; n < clipper.DisplayEnd; ++n)
                {
                    const PdbSymbolInfo &symbol = *filtered[n];

                    ImGui::TableNextRow();

                    ImGui::TableNextColumn();
                    ImGui::Text("%08x", down_cast<uint32_t>(symbol.address.absVirtual));

                    ImGui::TableNextColumn();
                    ImGui::Text("%08x", symbol.length);

                    ImGui::TableNextColumn();
                    std::string section = create_section_string(symbol.address.section_as_index(), sections);
                    TextUnformatted(section);

                    ImGui::TableNextColumn();
                    TextUnformatted(symbol.decoratedName);

                    ImGui::TableNextColumn();
                    TextUnformatted(symbol.undecoratedName);

                    ImGui::TableNextColumn();
                    TextUnformatted(symbol.globalName);
                }
            }

            ImGui::EndTable();
        }
    }
}

void ImGuiApp::FileManagerInfoPdbFunctions(
    ProgramFileDescriptor &fileDescriptor,
    const ProgramFileRevisionDescriptor &revisionDescriptor)
{
    ImGui::SeparatorText("Pdb Functions");

    const auto &filtered = fileDescriptor.m_pdbFunctionsFilter.filtered;
    {
        const PdbFunctionInfoVector &functions = revisionDescriptor.m_pdbReader->get_functions();

        UpdateFilter(
            fileDescriptor.m_pdbFunctionsFilter,
            functions,
            [](const ImGuiTextFilterEx &filter, const PdbFunctionInfo &function) -> bool {
                if (filter.PassFilter(function.decoratedName))
                    return true;
                if (filter.PassFilter(function.undecoratedName))
                    return true;
                if (filter.PassFilter(function.globalName))
                    return true;
                return false;
            });

        ImGui::Text("Count: %d, Filtered: %d", int(functions.size()), filtered.size());
    }

    const float defaultHeight = GetDefaultTableHeight(filtered.size(), 10);
    ImScoped::Child child("pdb_functions_container", ImVec2(0.0f, defaultHeight), ImGuiChildFlags_ResizeY);
    if (child.IsContentVisible)
    {
        if (ImGui::BeginTable("pdb_functions", 5, FileManagerInfoTableFlags))
        {
            ImGui::TableSetupScrollFreeze(0, 1); // Makes top row always visible.
            ImGui::TableSetupColumn("Address");
            ImGui::TableSetupColumn("Size");
            ImGui::TableSetupColumn("Decorated Name");
            ImGui::TableSetupColumn("Undecorated Name");
            ImGui::TableSetupColumn("Global Name");
            ImGui::TableHeadersRow();

            ImGuiListClipper clipper;
            clipper.Begin(filtered.size());

            while (clipper.Step())
            {
                for (int n = clipper.DisplayStart; n < clipper.DisplayEnd; ++n)
                {
                    const PdbFunctionInfo &symbol = *filtered[n];

                    ImGui::TableNextRow();

                    ImGui::TableNextColumn();
                    ImGui::Text("%08x", down_cast<uint32_t>(symbol.address.absVirtual));

                    ImGui::TableNextColumn();
                    ImGui::Text("%08x", symbol.length);

                    ImGui::TableNextColumn();
                    TextUnformatted(symbol.decoratedName);

                    ImGui::TableNextColumn();
                    TextUnformatted(symbol.undecoratedName);

                    ImGui::TableNextColumn();
                    TextUnformatted(symbol.globalName);
                }
            }

            ImGui::EndTable();
        }
    }
}

void ImGuiApp::FileManagerInfoPdbExeInfo(const ProgramFileRevisionDescriptor &descriptor)
{
    ImGui::SeparatorText("Pdb Exe Info");

    const PdbExeInfo &exe_info = descriptor.m_pdbReader->get_exe_info();
    ImGui::Text("Exe File Name: %s", exe_info.exeFileName.c_str());
    ImGui::Text("Pdb File Path: %s", exe_info.pdbFilePath.c_str());
}

void ImGuiApp::OutputManagerBody()
{
    // #TODO implement.

    ImGui::TextUnformatted("Not implemented");
}

void ImGuiApp::ComparisonManagerBody(ProgramComparisonDescriptor &descriptor)
{
    if (TreeNodeHeader("Files", ImGuiTreeNodeFlags_DefaultOpen))
    {
        {
            ComparisonManagerFilesHeaders();

            ImScoped::Group group;
            ImScoped::Disabled disabled(descriptor.has_async_work());

            ComparisonManagerFilesLists(descriptor);
            ComparisonManagerFilesActions(descriptor);
        }
        const ImRect groupRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
        ComparisonManagerFilesProgressOverlay(descriptor, groupRect);
        ComparisonManagerFilesStatus(descriptor);
    }

    if (descriptor.bundles_ready())
    {
        if (TreeNodeHeader("Bundles", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ComparisonManagerBundlesSettings(descriptor);
            ComparisonManagerBundlesLists(descriptor);
        }

        if (TreeNodeHeader("Functions", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ComparisonManagerFunctionsSettings(descriptor);
            ComparisonManagerFunctionsLists(descriptor);
        }

        if (TreeNodeHeader("Assembler", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ComparisonManagerFunctionEntries(descriptor);
        }
    }
}

void ImGuiApp::ComparisonManagerFilesHeaders()
{
    ImScoped::Table table("##files_header_table", 2, ComparisonSplitTableFlags);
    if (table.IsContentVisible)
    {
        ImGui::TableNextRow();

        for (int i = 0; i < 2; ++i)
        {
            ImGui::TableSetColumnIndex(i);
            ImGui::TextUnformatted("Select File");
        }
    }
}

void ImGuiApp::ComparisonManagerFilesLists(ProgramComparisonDescriptor &descriptor)
{
    const ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    ImGui::SetCursorScreenPos(ImVec2(cursorPos.x, cursorPos.y - 5.0f)); // Hack, removes gap.
    const ImVec2 outer_size(0, ImGui::GetTextLineHeightWithSpacing() * 9);
    ImScoped::Child resizeChild("##files_list_resize", outer_size, ImGuiChildFlags_ResizeY);
    if (resizeChild.IsContentVisible)
    {
        ImScoped::Table table("##files_list_table", 2, ComparisonSplitTableFlags);
        if (table.IsContentVisible)
        {
            ImGui::TableNextRow();

            for (int i = 0; i < 2; ++i)
            {
                ImGui::TableSetColumnIndex(i);
                ImScoped::ID id(i);

                ProgramComparisonDescriptor::File &file = descriptor.m_files[i];

                ComparisonManagerFilesList(file);
            }
        }
    }
}

void ImGuiApp::ComparisonManagerFilesList(ProgramComparisonDescriptor::File &file)
{
    ImScoped::Child styleChild(
        "##files_list_style",
        ImVec2(0, 0),
        ImGuiChildFlags_FrameStyle,
        ImGuiWindowFlags_HorizontalScrollbar);

    if (styleChild.IsContentVisible)
    {
        const IndexT count = IndexT(m_programFiles.size());
        for (IndexT n = 0; n < count; ++n)
        {
            ProgramFileDescriptor *program_file = m_programFiles[n].get();
            const std::string name = program_file->create_descriptor_name_with_file_info();
            const bool selected = (file.m_imguiSelectedFileIdx == n);

            if (ImGui::Selectable(name.c_str(), selected))
                file.m_imguiSelectedFileIdx = n;
        }
    }
}

void ImGuiApp::ComparisonManagerFilesActions(ProgramComparisonDescriptor &descriptor)
{
    const IndexT selectedFileIdx0 = descriptor.m_files[0].m_imguiSelectedFileIdx;
    const IndexT selectedFileIdx1 = descriptor.m_files[1].m_imguiSelectedFileIdx;
    ProgramFileDescriptor *fileDescriptor0 = get_program_file_descriptor(selectedFileIdx0);
    ProgramFileDescriptor *fileDescriptor1 = get_program_file_descriptor(selectedFileIdx1);

    if (fileDescriptor0 != nullptr && fileDescriptor1 != nullptr)
    {
        const bool canCompare0 = fileDescriptor0->can_load() || fileDescriptor0->exe_loaded();
        const bool canCompare1 = fileDescriptor1->can_load() || fileDescriptor1->exe_loaded();
        ImScoped::Disabled disabled(!(canCompare0 && canCompare1));
        // Change button color.
        ImScoped::StyleColor color1(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.5f, 0.6f, 0.6f));
        ImScoped::StyleColor color2(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.5f, 0.8f, 0.8f));
        ImScoped::StyleColor color3(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.5f, 1.0f, 1.0f));
        // Change text color too to make it readable with the light ImGui color theme.
        ImScoped::StyleColor text_color1(ImGuiCol_Text, ImVec4(1.00f, 1.00f, 1.00f, 1.00f));
        ImScoped::StyleColor text_color2(ImGuiCol_TextDisabled, ImVec4(0.50f, 0.50f, 0.50f, 1.00f));
        if (Button("Compare"))
        {
            load_and_init_comparison_async({fileDescriptor0, fileDescriptor1}, &descriptor);
        }

        // #TODO: Add button to disassemble and compare all matched functions.
        // #TODO: Add button to disassemble all unmatched functions.
    }
}

void ImGuiApp::ComparisonManagerFilesProgressOverlay(const ProgramComparisonDescriptor &descriptor, const ImRect &rect)
{
    if (descriptor.has_async_work())
    {
        const std::string overlay = fmt::format(
            "Processing commands {:d}:{:d} ..",
            descriptor.m_files[0].get_first_active_command_id(),
            descriptor.m_files[1].get_first_active_command_id());

        OverlayProgressBar(rect, -1.0f * (float)ImGui::GetTime(), overlay.c_str());
    }
}

void ImGuiApp::ComparisonManagerFilesStatus(const ProgramComparisonDescriptor &descriptor)
{
    ImScoped::Table table("##status_table", 2, ComparisonSplitTableFlags);
    if (table.IsContentVisible)
    {
        ImGui::TableNextRow();

        for (size_t i = 0; i < 2; ++i)
        {
            ImGui::TableSetColumnIndex(i);

            const ProgramComparisonDescriptor::File &file = descriptor.m_files[i];
            const ProgramFileRevisionDescriptorPtr &revisionDescriptor = file.m_revisionDescriptor;

            if (revisionDescriptor != nullptr)
            {
                FileManagerDescriptorLoadStatus(*revisionDescriptor);
            }
        }
    }
}

void ImGuiApp::ComparisonManagerBundlesSettings(ProgramComparisonDescriptor &descriptor)
{
    ImScoped::Table table("##bundles_filter_table", 2, ComparisonSplitTableFlags);
    if (table.IsContentVisible)
    {
        ImGui::TableNextRow();

        for (size_t i = 0; i < 2; ++i)
        {
            ImGui::TableSetColumnIndex(i);
            ImScoped::ItemWidth item_width(ImGui::GetFontSize() * -12);
            ImScoped::ID id(i);

            ProgramComparisonDescriptor::File &file = descriptor.m_files[i];

            ComparisonManagerBundlesTypeSelection(file);
            ComparisonManagerBundlesFilter(file);
        }
    }
}

void ImGuiApp::ComparisonManagerBundlesTypeSelection(ProgramComparisonDescriptor::File &file)
{
    std::array<const char *, size_t(MatchBundleType::Count)> options;
    IndexT count = 0;
    if (file.m_compilandBundlesBuilt == TriState::True)
        options[count++] = "Compiland Bundles";
    if (file.m_sourceFileBundlesBuilt == TriState::True)
        options[count++] = "Source File Bundles";
    if (file.m_singleBundleBuilt)
        options[count++] = "Single Bundle";

    assert(count > 0);
    IndexT &index = file.m_imguiSelectedBundleTypeIdx;
    index = std::clamp(index, IndexT(0), count - 1);
    const char *preview = options[index];

    ImScoped::Combo combo("Select Bundle Type", preview);
    if (combo.IsOpen)
    {
        for (IndexT n = 0; n < count; n++)
        {
            const bool selected = (index == n);
            if (ImGui::Selectable(options[n], selected))
            {
                index = n;
                file.on_bundles_changed();
            }
        }
    }
}

void ImGuiApp::ComparisonManagerBundlesFilter(ProgramComparisonDescriptor::File &file)
{
    const MatchBundleType type = file.get_selected_bundle_type();
    const ImGuiSelectionBasicStorage &selection = file.get_bundles_selection(type);
    const span<const NamedFunctionBundle> bundles = file.get_bundles(type);

    const bool selectionChanged = UpdateFilter(
        file.m_bundlesFilter,
        bundles,
        [](const ImGuiTextFilterEx &filter, const NamedFunctionBundle &bundle) -> bool {
            return filter.PassFilter(bundle.name);
        });

    if (selectionChanged)
    {
        file.on_bundles_interaction();
    }

    ImGui::Text(
        "Select Bundle(s) - Count: %d/%d, Selected: %d/%d",
        file.m_bundlesFilter.filtered.size(),
        int(bundles.size()),
        int(file.m_selectedBundles.size()),
        selection.Size);
}

void ImGuiApp::ComparisonManagerBundlesLists(ProgramComparisonDescriptor &descriptor)
{
    const ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    ImGui::SetCursorScreenPos(ImVec2(cursorPos.x, cursorPos.y - 5.0f)); // Hack, removes gap.
    const ImVec2 defaultSize(0, ImGui::GetTextLineHeightWithSpacing() * 9);
    ImScoped::Child resizeChild("##bundles_list_resize", defaultSize, ImGuiChildFlags_ResizeY);
    if (resizeChild.IsContentVisible)
    {
        ImScoped::Table table("##bundles_list_table", 2, ComparisonSplitTableFlags);
        if (table.IsContentVisible)
        {
            ImGui::TableNextRow();

            for (size_t i = 0; i < 2; ++i)
            {
                ImGui::TableSetColumnIndex(i);
                ImScoped::ID id(i);

                ProgramComparisonDescriptor::File &file = descriptor.m_files[i];

                ComparisonManagerBundlesList(file);
            }
        }
    }
}

void ImGuiApp::ComparisonManagerBundlesList(ProgramComparisonDescriptor::File &file)
{
    using File = ProgramComparisonDescriptor::File;

    // Using AlwaysHorizontalScrollbar instead of HorizontalScrollbar because the list is glitching a bit
    // when the clipper makes it alternate between scroll bar on and off every frame in some situations.
    ImScoped::Child styleChild(
        "##bundles_list_style",
        ImVec2(0, 0),
        ImGuiChildFlags_FrameStyle,
        ImGuiWindowFlags_AlwaysHorizontalScrollbar);

    if (styleChild.IsContentVisible)
    {
        const MatchBundleType type = file.get_selected_bundle_type();
        ImGuiSelectionBasicStorage &selection = file.get_bundles_selection(type);
        const int count = file.m_bundlesFilter.filtered.size();
        const int oldSelectionSize = selection.Size;
        bool selectionChanged = false;
        ImGuiMultiSelectIO *ms_io = ImGui::BeginMultiSelect(ImGuiMultiSelectFlags_BoxSelect1d, selection.Size, count);
        selection.UserData = &file;
        selection.AdapterIndexToStorageId = [](ImGuiSelectionBasicStorage *self, int idx) -> ImGuiID {
            const auto file = static_cast<const File *>(self->UserData);
            return ImGuiID(file->get_filtered_bundle(idx).id);
        };
        selection.ApplyRequests(ms_io);

        ImGuiListClipper clipper;
        clipper.Begin(count);

        while (clipper.Step())
        {
            for (int n = clipper.DisplayStart; n < clipper.DisplayEnd; ++n)
            {
                // 1
                ImGui::SetNextItemSelectionUserData(n);
                // 2
                const NamedFunctionBundle &bundle = file.get_filtered_bundle(n);
                const File::ListItemUiInfo &uiInfo = file.get_filtered_bundle_ui_info(n);

                ScopedStyleColor styleColor;

                if (uiInfo.m_similarity.has_value())
                {
                    ComparisonManagerItemListStyleColor(styleColor, uiInfo);
                }

                const bool selected = selection.Contains(ImGuiID(bundle.id));
                selectionChanged |= ImGui::Selectable(uiInfo.m_label.c_str(), selected);
            }
        }

        ms_io = ImGui::EndMultiSelect();
        selection.ApplyRequests(ms_io);

        if (selectionChanged || oldSelectionSize != selection.Size)
        {
            file.on_bundles_interaction();
        }
    }
}

void ImGuiApp::ComparisonManagerFunctionsSettings(ProgramComparisonDescriptor &descriptor)
{
    ImScoped::Table table("##functions_filter_table", 2, ComparisonSplitTableFlags);
    if (table.IsContentVisible)
    {
        ImGui::TableNextRow();

        for (size_t i = 0; i < 2; ++i)
        {
            ImGui::TableSetColumnIndex(i);
            ImScoped::ItemWidth item_width(ImGui::GetFontSize() * -12);
            ImScoped::ID id(i);

            ProgramComparisonDescriptor::File &file = descriptor.m_files[i];

            ComparisonManagerFunctionsFilter(descriptor, file);
        }
    }
}

void ImGuiApp::ComparisonManagerFunctionsFilter(
    ProgramComparisonDescriptor &descriptor,
    ProgramComparisonDescriptor::File &file)
{
    bool selectionChanged = false;

    selectionChanged |= ImGui::Checkbox("Show Matched Functions", &file.m_imguiShowMatchedFunctions);
    ImGui::SameLine();
    selectionChanged |= ImGui::Checkbox("Show Unmatched Functions", &file.m_imguiShowUnmatchedFunctions);

    if (selectionChanged)
    {
        file.m_functionIndicesFilter.reset();
        file.m_functionIndicesFilter.set_external_filter_condition(
            !file.m_imguiShowMatchedFunctions || !file.m_imguiShowUnmatchedFunctions);
    }

    assert(file.m_revisionDescriptor != nullptr);
    const span<const IndexT> functionIndices = file.get_active_named_function_indices();
    const NamedFunctions &namedFunctions = file.m_revisionDescriptor->m_namedFunctions;

    selectionChanged |= UpdateFilter(
        file.m_functionIndicesFilter,
        functionIndices,
        [&](const ImGuiTextFilterEx &filter, IndexT index) -> bool {
            bool isMatched = file.is_matched_function(index);
            if (isMatched && !file.m_imguiShowMatchedFunctions)
                return false;
            if (!isMatched && !file.m_imguiShowUnmatchedFunctions)
                return false;
            return filter.PassFilter(namedFunctions[index].name);
        });

    if (selectionChanged)
    {
        // #TODO: bundles and functions are not updated if hidden and files are compared anew
        on_functions_interaction(descriptor, file);
    }

    ImGui::Text(
        "Select Function(s) - Count: %d/%d, Selected: %d/%d",
        file.m_functionIndicesFilter.filtered.size(),
        int(functionIndices.size()),
        int(file.m_selectedNamedFunctionIndices.size()),
        file.m_imguiFunctionsSelection.Size);
}

void ImGuiApp::ComparisonManagerFunctionsLists(ProgramComparisonDescriptor &descriptor)
{
    const ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    ImGui::SetCursorScreenPos(ImVec2(cursorPos.x, cursorPos.y - 5.0f)); // Hack, removes gap.
    const ImVec2 defaultSize(0, ImGui::GetTextLineHeightWithSpacing() * 9);
    ImScoped::Child resizeChild("##functions_list_resize", defaultSize, ImGuiChildFlags_ResizeY);
    if (resizeChild.IsContentVisible)
    {
        ImScoped::Table table("##functions_list_table", 2, ComparisonSplitTableFlags);
        if (table.IsContentVisible)
        {
            ImGui::TableNextRow();

            for (size_t i = 0; i < 2; ++i)
            {
                ImGui::TableSetColumnIndex(i);
                ImScoped::ID id(i);

                ProgramComparisonDescriptor::File &file = descriptor.m_files[i];

                ComparisonManagerFunctionsList(descriptor, file);
            }
        }
    }
}

void ImGuiApp::ComparisonManagerFunctionsList(
    ProgramComparisonDescriptor &descriptor,
    ProgramComparisonDescriptor::File &file)
{
    using File = ProgramComparisonDescriptor::File;

    // Using AlwaysHorizontalScrollbar instead of HorizontalScrollbar because the list is glitching a bit
    // when the clipper makes it alternate between scroll bar on and off every frame in some situations.
    ImScoped::Child styleChild(
        "##functions_list_style",
        ImVec2(0, 0),
        ImGuiChildFlags_FrameStyle,
        ImGuiWindowFlags_AlwaysHorizontalScrollbar);

    if (styleChild.IsContentVisible)
    {
        assert(file.m_revisionDescriptor != nullptr);
        ImGuiSelectionBasicStorage &selection = file.m_imguiFunctionsSelection;
        const NamedFunctions &namedFunctions = file.m_revisionDescriptor->m_namedFunctions;
        const int count = file.m_functionIndicesFilter.filtered.size();
        const int oldSelectionSize = selection.Size;
        bool selectionChanged = false;
        ImGuiMultiSelectIO *ms_io = ImGui::BeginMultiSelect(ImGuiMultiSelectFlags_BoxSelect1d, selection.Size, count);
        selection.UserData = &file;
        selection.AdapterIndexToStorageId = [](ImGuiSelectionBasicStorage *self, int idx) -> ImGuiID {
            const auto file = static_cast<const File *>(self->UserData);
            return ImGuiID(file->get_filtered_named_function(idx).id);
        };
        selection.ApplyRequests(ms_io);

        ImGuiListClipper clipper;
        clipper.Begin(count);

        while (clipper.Step())
        {
            for (int n = clipper.DisplayStart; n < clipper.DisplayEnd; ++n)
            {
                // 1
                ImGui::SetNextItemSelectionUserData(n);
                // 2
                const NamedFunction &namedFunction = file.get_filtered_named_function(n);
                const File::NamedFunctionUiInfo &uiInfo = file.get_filtered_named_function_ui_info(n);

                ScopedStyleColor styleColor;

                if (uiInfo.m_similarity.has_value())
                {
                    ComparisonManagerItemListStyleColor(styleColor, uiInfo);
                }

                const bool selected = selection.Contains(ImGuiID(namedFunction.id));
                selectionChanged |= ImGui::Selectable(uiInfo.m_label.c_str(), selected);
            }
        }

        ms_io = ImGui::EndMultiSelect();
        selection.ApplyRequests(ms_io);

        if (selectionChanged || oldSelectionSize != selection.Size)
        {
            on_functions_interaction(descriptor, file);
        }
    }
}

void ImGuiApp::ComparisonManagerFunctionEntries(ProgramComparisonDescriptor &descriptor)
{
    ComparisonManagerFunctionEntriesControls(descriptor);

    ImScoped::Child child("##function_entries");
    if (child.IsContentVisible)
    {
        const ProgramComparisonDescriptor::FunctionsPageData pageData = descriptor.get_selected_functions_page_data();

        ComparisonManagerMatchedFunctions(descriptor, pageData.matchedFunctionIndices);

        for (IndexT i = 0; i < 2; ++i)
        {
            ComparisonManagerNamedFunctions(descriptor, i, pageData.namedFunctionIndicesArray[i]);
        }
    }
}

void ImGuiApp::ComparisonManagerFunctionEntriesControls(ProgramComparisonDescriptor &descriptor)
{
    {
        ImScoped::ItemWidth itemWidth(100);

        ImGui::DragInt("Page Size", &descriptor.m_imguiPageSize, 0.5f, 1, 100, "%d", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SameLine();
        TooltipTextMarker(
            "Click and drag to edit value.\n"
            "Hold SHIFT/ALT for faster/slower edit.\n"
            "Double-click or CTRL+click to input value.");
    }

    {
        ImScoped::ItemWidth itemWidth(200);

        const int pageCount = std::max(descriptor.get_functions_page_count(), 1);
        descriptor.m_imguiSelectedPage = std::min(descriptor.m_imguiSelectedPage, pageCount);
        ImGui::SameLine();
        ImGui::SliderInt("Page Select", &descriptor.m_imguiSelectedPage, 1, pageCount, "%d", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SameLine();
        TooltipTextMarker("CTRL+click to input value.");
    }
}

void ImGuiApp::ComparisonManagerMatchedFunctions(
    const ProgramComparisonDescriptor &descriptor,
    span<const IndexT> matchedFunctionIndices)
{
    const float treeOffsetX = ImGui::GetTreeNodeToLabelSpacing();

    for (IndexT matchedFunctionIndex : matchedFunctionIndices)
    {
        const MatchedFunction &matchedFunction = descriptor.m_matchedFunctions[matchedFunctionIndex];
        if (!matchedFunction.is_compared())
            continue;

        const ProgramComparisonDescriptor::File::NamedFunctionUiInfo *uiInfo =
            descriptor.get_first_valid_named_function_ui_info(matchedFunction);

        assert(uiInfo != nullptr);

        ScopedStyleColor styleColor;
        if (uiInfo->m_similarity.has_value())
        {
            ComparisonManagerItemListStyleColor(styleColor, *uiInfo, treeOffsetX);
        }

        // #TODO: Check if node can be excluded from imgui ini save because it makes it big and slow.
        ImScoped::TreeNodeEx tree(
            uiInfo->m_label.c_str(),
            ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth);

        styleColor.PopAll();

        if (tree.IsOpen)
        {
            ComparisonManagerMatchedFunction(descriptor, matchedFunction);
        }
    }
}

void ImGuiApp::ComparisonManagerMatchedFunction(
    const ProgramComparisonDescriptor &descriptor,
    const MatchedFunction &matchedFunction)
{
    assert(matchedFunction.is_compared());

    const AsmComparisonRecords &records = matchedFunction.comparison.records;

    // Constrain the child window to max height of the table inside.
    // + 4 because the child tables add this much somewhere (???).
    const float maxHeight = GetMaxTableHeight(records.size()) + 4.0f;
    const float defaultHeight = GetDefaultTableHeight(records.size(), 10) + 4.0f;
    ImGui::SetNextWindowSizeConstraints(ImVec2(0.0f, 0.0f), ImVec2(FLT_MAX, maxHeight));
    ImScoped::Child resizeChild(
        "##matched_function_resize",
        ImVec2(0.0f, defaultHeight),
        ImGuiChildFlags_ResizeY,
        ImGuiWindowFlags_AlwaysVerticalScrollbar);

    if (resizeChild.IsContentVisible)
    {
        constexpr ImGuiTableFlags comparisonTableFlags = ComparisonSplitTableFlags | ImGuiTableFlags_NoPadInnerX;

        ImScoped::Table table("##matched_function_table", 3, comparisonTableFlags);
        if (table.IsContentVisible)
        {
            // * 4 because column text is intended to be 4 characters wide.
            const float cellPadding = ImGui::GetStyle().CellPadding.x;
            const float column1Width = ImGui::GetFont()->GetCharAdvance(' ') * 4 + cellPadding * 2;

            ImGui::TableSetupColumn("##column0", ImGuiTableColumnFlags_WidthStretch, 50.0f);
            ImGui::TableSetupColumn("##column1", ImGuiTableColumnFlags_WidthFixed, column1Width);
            ImGui::TableSetupColumn("##column2", ImGuiTableColumnFlags_WidthStretch, 50.0f);
            ImGui::TableNextRow();

            {
                ImGui::TableNextColumn();

                const IndexT side = 0;
                ImScoped::ID id(static_cast<int>(side));

                const ProgramFileRevisionDescriptor &revision = *descriptor.m_files[side].m_revisionDescriptor;
                const IndexT namedFunctionIndex = matchedFunction.named_idx_pair[side];
                const NamedFunction &namedFunction = revision.m_namedFunctions[namedFunctionIndex];
                ComparisonManagerMatchedFunctionContentTable(side, records, revision, namedFunction);
            }

            {
                ImGui::TableNextColumn();

                ComparisonManagerMatchedFunctionDiffSymbolTable(records);
            }

            {
                ImGui::TableNextColumn();

                const IndexT side = 1;
                ImScoped::ID id(static_cast<int>(side));

                const ProgramFileRevisionDescriptor &revision = *descriptor.m_files[side].m_revisionDescriptor;
                const IndexT namedFunctionIndex = matchedFunction.named_idx_pair[side];
                const NamedFunction &namedFunction = revision.m_namedFunctions[namedFunctionIndex];
                ComparisonManagerMatchedFunctionContentTable(side, records, revision, namedFunction);
            }
        }
    }
}

void ImGuiApp::ComparisonManagerMatchedFunctionContentTable(
    IndexT sideIdx,
    const AsmComparisonRecords &records,
    const ProgramFileRevisionDescriptor &fileRevision,
    const NamedFunction &namedFunction)
{
    assert(sideIdx < 2);

    const std::string &sourceFile = namedFunction.function.get_source_file_name();
    const TextFileContent *fileContent = fileRevision.m_fileContentStorage.find_content(sourceFile);
    const bool showSourceCodeColumns = fileContent != nullptr;
    int columnCount = 5;
    if (!showSourceCodeColumns)
        columnCount -= 2;

    const ImVec2 tableSize(0.0f, GetMaxTableHeight(records.size()));
    ImScoped::Table table("##function_assembler_table", columnCount, AssemblerTableFlags, tableSize);
    if (table.IsContentVisible)
    {
        std::string buf;
        buf.reserve(1024);
        const bool leftSide = sideIdx == 0;
        const auto PrintAsmInstructionColumns = leftSide ? PrintAsmInstructionColumnsLeft : PrintAsmInstructionColumnsRight;
        const auto PrintAsmLabelColumns = leftSide ? PrintAsmLabelColumnsLeft : PrintAsmLabelColumnsRight;

        // #TODO: Add feature to auto hide columns by default.
        // #TODO: Add feature to change default or current width of columns.
        // #TODO: Simplify this. Put in a struct maybe. Have array of elements that can be ordered.
        if (leftSide)
        {
            if (showSourceCodeColumns)
            {
                ImGui::TableSetupColumn("Line");
                ImGui::TableSetupColumn("Source Code");
            }
            ImGui::TableSetupColumn("Bytes");
            ImGui::TableSetupColumn("Address");
            ImGui::TableSetupColumn("Assembler");
        }
        else
        {
            ImGui::TableSetupColumn("Address");
            ImGui::TableSetupColumn("Assembler");
            ImGui::TableSetupColumn("Bytes");
            if (showSourceCodeColumns)
            {
                ImGui::TableSetupColumn("Line");
                ImGui::TableSetupColumn("Source Code");
            }
        }
        ImGui::TableHeadersRow();

        ImGuiListClipper clipper;
        clipper.Begin(records.size());

        while (clipper.Step())
        {
            for (int n = clipper.DisplayStart; n < clipper.DisplayEnd; ++n)
            {
                ImGui::TableNextRow();

                const AsmComparisonRecord &record = records[n];
                if (const AsmInstructionPair *instructionPair = std::get_if<AsmInstructionPair>(&record))
                {
                    if (const AsmInstruction *instruction = instructionPair->pair[sideIdx])
                    {
                        PrintAsmInstructionColumns(buf, *instruction, fileContent);
                        continue;
                    }
                }
                else if (const AsmLabelPair *labelPair = std::get_if<AsmLabelPair>(&record))
                {
                    if (const AsmLabel *label = labelPair->pair[sideIdx])
                    {
                        PrintAsmLabelColumns(*label, showSourceCodeColumns);
                        continue;
                    }
                }

                // Add empty columns with dummy texts to satisfy the clipper.
                for (int i = 0; i < columnCount; ++i)
                {
                    ImGui::TableNextColumn();
                    TextUnformatted(" ");
                }
            }
        }
    }
}

void ImGuiApp::ComparisonManagerNamedFunctions(
    const ProgramComparisonDescriptor &descriptor,
    IndexT sideIdx,
    span<const IndexT> namedFunctionIndices)
{
    using File = ProgramComparisonDescriptor::File;
    const File &file = descriptor.m_files[sideIdx];

    for (IndexT namedFunctionIndex : namedFunctionIndices)
    {
        const ProgramFileRevisionDescriptor &revision = *file.m_revisionDescriptor;
        const NamedFunction &namedFunction = revision.m_namedFunctions[namedFunctionIndex];
        if (!namedFunction.is_disassembled())
            continue;

        const File::NamedFunctionUiInfo &uiInfo0 = file.m_namedFunctionUiInfos[namedFunctionIndex];

        // #TODO: Check if node can be excluded from imgui ini save because it makes it big and slow.
        ImScoped::TreeNodeEx tree(
            uiInfo0.m_label.c_str(),
            ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth);

        if (tree.IsOpen)
        {
            ComparisonManagerNamedFunction(sideIdx, revision, namedFunction);
        }
    }
}

void ImGuiApp::ComparisonManagerNamedFunction(
    IndexT sideIdx,
    const ProgramFileRevisionDescriptor &fileRevision,
    const NamedFunction &namedFunction)
{
    assert(sideIdx < 2);
    assert(namedFunction.is_disassembled());

    const AsmInstructionVariants &records = namedFunction.function.get_instructions();

    // Constrain the child window to max height of the table inside.
    // + 4 because the child tables add this much somewhere (???).
    const float maxHeight = GetMaxTableHeight(records.size()) + 4.0f;
    const float defaultHeight = GetDefaultTableHeight(records.size(), 10) + 4.0f;
    ImGui::SetNextWindowSizeConstraints(ImVec2(0.0f, 0.0f), ImVec2(FLT_MAX, maxHeight));
    ImScoped::Child resizeChild(
        "##matched_function_resize",
        ImVec2(0.0f, defaultHeight),
        ImGuiChildFlags_ResizeY,
        ImGuiWindowFlags_NoScrollbar);

    if (resizeChild.IsContentVisible)
    {
        constexpr ImGuiTableFlags comparisonTableFlags = ComparisonSplitTableFlags | ImGuiTableFlags_NoPadInnerX;
        // Subtracts scrollbar width from table width to look consistent with the table of the matched function.
        const float availableWidth = ImGui::GetContentRegionAvail().x;
        const float scrollbarWidth = ImGui::GetStyle().ScrollbarSize;
        ImVec2 tableSize(availableWidth - scrollbarWidth, 0.0f);
        ImScoped::Table table("##matched_function_table", 3, comparisonTableFlags, tableSize);
        if (table.IsContentVisible)
        {
            // * 4 because column text is intended to be 4 characters wide.
            const float cellPadding = ImGui::GetStyle().CellPadding.x;
            const float column1Width = ImGui::GetFont()->GetCharAdvance(' ') * 4 + cellPadding * 2;

            // Creates 3 (invisible) columns to look consistent with the table of the matched function.
            ImGui::TableSetupColumn("##column0", ImGuiTableColumnFlags_WidthStretch, 50.0f);
            ImGui::TableSetupColumn("##column1", ImGuiTableColumnFlags_WidthFixed, column1Width);
            ImGui::TableSetupColumn("##column2", ImGuiTableColumnFlags_WidthStretch, 50.0f);
            ImGui::TableNextRow();

            const bool leftSide = sideIdx == 0;

            ImGui::TableSetColumnIndex(leftSide ? 0 : 2);
            ComparisonManagerNamedFunctionContentTable(sideIdx, fileRevision, namedFunction);
        }
    }
}

void ImGuiApp::ComparisonManagerNamedFunctionContentTable(
    IndexT sideIdx,
    const ProgramFileRevisionDescriptor &fileRevision,
    const NamedFunction &namedFunction)
{
    const AsmInstructionVariants &records = namedFunction.function.get_instructions();
    const std::string &sourceFile = namedFunction.function.get_source_file_name();
    const TextFileContent *fileContent = fileRevision.m_fileContentStorage.find_content(sourceFile);
    const bool showSourceCodeColumns = fileContent != nullptr;
    int columnCount = 5;
    if (!showSourceCodeColumns)
        columnCount -= 2;

    ImScoped::Table table("##function_assembler_table", columnCount, AssemblerTableFlags | ImGuiTableFlags_ScrollY);
    if (table.IsContentVisible)
    {
        std::string buf;
        buf.reserve(1024);
        const bool leftSide = sideIdx == 0;
        const auto PrintAsmInstructionColumns = leftSide ? PrintAsmInstructionColumnsLeft : PrintAsmInstructionColumnsRight;
        const auto PrintAsmLabelColumns = leftSide ? PrintAsmLabelColumnsLeft : PrintAsmLabelColumnsRight;

        ImGui::TableSetupScrollFreeze(0, 1); // Makes top row always visible.

        // #TODO: Simplify this. Put in a struct maybe. Have array of elements that can be ordered.
        if (leftSide)
        {
            if (showSourceCodeColumns)
            {
                ImGui::TableSetupColumn("Line");
                ImGui::TableSetupColumn("Source Code");
            }
            ImGui::TableSetupColumn("Bytes");
            ImGui::TableSetupColumn("Address");
            ImGui::TableSetupColumn("Assembler");
        }
        else
        {
            ImGui::TableSetupColumn("Address");
            ImGui::TableSetupColumn("Assembler");
            ImGui::TableSetupColumn("Bytes");
            if (showSourceCodeColumns)
            {
                ImGui::TableSetupColumn("Line");
                ImGui::TableSetupColumn("Source Code");
            }
        }
        ImGui::TableHeadersRow();

        ImGuiListClipper clipper;
        clipper.Begin(records.size());

        while (clipper.Step())
        {
            for (int n = clipper.DisplayStart; n < clipper.DisplayEnd; ++n)
            {
                ImGui::TableNextRow();

                const AsmInstructionVariant &record = records[n];
                if (const AsmInstruction *instruction = std::get_if<AsmInstruction>(&record))
                {
                    PrintAsmInstructionColumns(buf, *instruction, fileContent);
                }
                else if (const AsmLabel *label = std::get_if<AsmLabel>(&record))
                {
                    PrintAsmLabelColumns(*label, showSourceCodeColumns);
                }
                else
                {
                    // Add empty columns with dummy texts to satisfy the clipper.
                    for (int i = 0; i < columnCount; ++i)
                    {
                        ImGui::TableNextColumn();
                        TextUnformatted(" ");
                    }
                }
            }
        }
    }
}

void ImGuiApp::PrintAsmInstructionColumnsLeft(
    std::string &buf,
    const AsmInstruction &instruction,
    const TextFileContent *fileContent)
{
    // Always print a character to satisfy the clipper.

    const bool showSourceCodeColumns = fileContent != nullptr;

    if (showSourceCodeColumns)
    {
        ImGui::TableNextColumn(); // Source Line
        if (!PrintAsmInstructionSourceLine(instruction, *fileContent))
            TextUnformatted(" ");

        ImGui::TableNextColumn(); // Source Code
        if (!PrintAsmInstructionSourceCode(instruction, *fileContent))
            TextUnformatted(" ");
    }

    ImGui::TableNextColumn(); // Bytes
    if (!PrintAsmInstructionBytes(buf, instruction))
        TextUnformatted(" ");

    ImGui::TableNextColumn(); // Address
    PrintAsmInstructionAddress(instruction);

    ImGui::TableNextColumn(); // Assembler
    if (!PrintAsmInstructionAssembler(buf, instruction))
        TextUnformatted(" ");
}

void ImGuiApp::PrintAsmInstructionColumnsRight(
    std::string &buf,
    const AsmInstruction &instruction,
    const TextFileContent *fileContent)
{
    // Always print a character to satisfy the clipper.

    const bool showSourceCodeColumns = fileContent != nullptr;

    ImGui::TableNextColumn(); // Address
    PrintAsmInstructionAddress(instruction);

    ImGui::TableNextColumn(); // Assembler
    if (!PrintAsmInstructionAssembler(buf, instruction))
        TextUnformatted(" ");

    ImGui::TableNextColumn(); // Bytes
    if (!PrintAsmInstructionBytes(buf, instruction))
        TextUnformatted(" ");

    if (showSourceCodeColumns)
    {
        ImGui::TableNextColumn(); // Source Line
        if (!PrintAsmInstructionSourceLine(instruction, *fileContent))
            TextUnformatted(" ");

        ImGui::TableNextColumn(); // Source Code
        if (!PrintAsmInstructionSourceCode(instruction, *fileContent))
            TextUnformatted(" ");
    }
}

void ImGuiApp::PrintAsmLabelColumnsLeft(const AsmLabel &label, bool showSourceCodeColumns)
{
    // Always print a character to satisfy the clipper.

    if (showSourceCodeColumns)
    {
        ImGui::TableNextColumn(); // Source Line
        TextUnformatted(" ");
        ImGui::TableNextColumn(); // Source Code
        TextUnformatted(" ");
    }
    ImGui::TableNextColumn(); // Bytes
    TextUnformatted(" ");
    ImGui::TableNextColumn(); // Address
    TextUnformatted(" ");
    ImGui::TableNextColumn(); // Assembler
    PrintAsmLabel(label);
}

void ImGuiApp::PrintAsmLabelColumnsRight(const AsmLabel &label, bool showSourceCodeColumns)
{
    // Always print a character to satisfy the clipper.

    ImGui::TableNextColumn(); // Address
    TextUnformatted(" ");
    ImGui::TableNextColumn(); // Assembler
    PrintAsmLabel(label);
    ImGui::TableNextColumn(); // Bytes
    TextUnformatted(" ");

    if (showSourceCodeColumns)
    {
        ImGui::TableNextColumn(); // Source Line
        TextUnformatted(" ");
        ImGui::TableNextColumn(); // Source Code
        TextUnformatted(" ");
    }
}

bool ImGuiApp::PrintAsmInstructionSourceLine(const AsmInstruction &instruction, const TextFileContent &fileContent)
{
    const uint16_t lineIdx = instruction.get_line_index();
    const bool lineOk = lineIdx < fileContent.lines.size();
    if (lineOk)
    {
        if (instruction.isFirstLine)
        {
            ImGui::Text("%u", uint32_t(instruction.lineNumber));
            return true;
        }
        else
        {
            ImScoped::StyleColor greyText(ImGuiCol_Text, LightGrayColor);
            ImGui::Text("%u", uint32_t(instruction.lineNumber));
            return true;
        }
    }
    return false;
}

bool ImGuiApp::PrintAsmInstructionSourceCode(const AsmInstruction &instruction, const TextFileContent &fileContent)
{
    const uint16_t lineIdx = instruction.get_line_index();
    const bool lineOk = lineIdx < fileContent.lines.size();
    if (lineOk && instruction.isFirstLine)
    {
        TextUnformatted(fileContent.lines[lineIdx]);
        return true;
    }
    return false;
}

bool ImGuiApp::PrintAsmInstructionBytes(std::string &buf, const AsmInstruction &instruction)
{
    buf.clear();
    uint8_t b = 0;
    for (; b < instruction.bytes.size - 1; ++b)
    {
        buf += fmt::format("{:02x} ", instruction.bytes.elements[b]);
    }
    if (b < instruction.bytes.size)
    {
        buf += fmt::format("{:02x}", instruction.bytes.elements[b]);
    }
    TextUnformatted(buf);
    return !buf.empty();
}

void ImGuiApp::PrintAsmInstructionAddress(const AsmInstruction &instruction)
{
    ImGui::Text("%08x", down_cast<uint32_t>(instruction.address));
}

bool ImGuiApp::PrintAsmInstructionAssembler(std::string &buf, const AsmInstruction &instruction)
{
    constexpr std::string_view quote = "\"";

    if (instruction.isInvalid)
    {
        TextUnformatted("Unrecognized opcode");
        return true;
    }
    else
    {
        buf = instruction.text;
        util::strip_inplace(buf, quote);
        TextUnformatted(buf);

        if (instruction.isJump)
        {
            ImGui::SameLine();

            buf = fmt::format("{:+d} bytes", instruction.jumpLen);

            ImScoped::StyleColor greyText(ImGuiCol_Text, LightGrayColor);
            TextUnformatted(buf);
        }
        return !buf.empty();
    }
}

void ImGuiApp::PrintAsmLabel(const AsmLabel &label)
{
    ImScoped::StyleColor greyText(ImGuiCol_Text, LightGrayColor);
    TextUnformatted(label.label.c_str());
    ImGui::SameLine();
    TextUnformatted(":");
}

void ImGuiApp::ComparisonManagerMatchedFunctionDiffSymbolTable(const AsmComparisonRecords &records)
{
    constexpr ImGuiTableFlags tableFlags =
        ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_SizingFixedFit;

    const ImVec2 tableSize(0.0f, GetMaxTableHeight(records.size()));
    ImScoped::Table table("##function_match_table", 1, tableFlags, tableSize);
    if (table.IsContentVisible)
    {
        ImGui::TableSetupColumn("    ");
        ImGui::TableHeadersRow();

        ImGuiListClipper clipper;
        clipper.Begin(records.size());

        while (clipper.Step())
        {
            for (int n = clipper.DisplayStart; n < clipper.DisplayEnd; ++n)
            {
                ImGui::TableNextRow();

                const AsmComparisonRecord &record = records[n];
                if (const AsmInstructionPair *instructionPair = std::get_if<AsmInstructionPair>(&record))
                {
                    ImGui::TableNextColumn();
                    PrintDiffSymbol(*instructionPair);
                }
                else
                {
                    // Add empty column and dummy text to satisfy the clipper.
                    ImGui::TableNextColumn();
                    TextUnformatted(" ");
                }
            }
        }
    }
}

void ImGuiApp::PrintDiffSymbol(const AsmInstructionPair &instructionPair)
{
    // #TODO: Create function to create equal status and use it here and in AsmPrinter.

    constexpr std::string_view equal = "==";
    constexpr std::string_view unequal = "xx";
    constexpr std::string_view maybeEqual = "??";
    constexpr std::string_view leftMissing = ">>";
    constexpr std::string_view rightMissing = "<<";

    // #TODO: Make strictness configurable.
    const AsmMatchValue matchValue = instructionPair.mismatch_info.get_match_value(AsmMatchStrictness::Lenient);

    switch (matchValue)
    {
        case AsmMatchValue::IsMatch:
            TextUnformattedCenteredX(equal);
            break;

        case AsmMatchValue::IsMaybeMatch:
            TextUnformattedCenteredX(maybeEqual);
            break;

        case AsmMatchValue::IsMismatch: {
            const AsmInstruction *instruction0 = instructionPair.pair[0];
            const AsmInstruction *instruction1 = instructionPair.pair[1];
            if (instruction0 != nullptr && instruction1 != nullptr)
            {
                TextUnformattedCenteredX(unequal);
            }
            else if (instruction0 == nullptr && instruction1 != nullptr)
            {
                TextUnformattedCenteredX(leftMissing);
            }
            else if (instruction0 != nullptr && instruction1 == nullptr)
            {
                TextUnformattedCenteredX(rightMissing);
            }
            else
            {
                assert(false);
            }
            break;
        }
        default:
            assert(false);
            break;
    }
}

void ImGuiApp::ComparisonManagerItemListStyleColor(
    ScopedStyleColor &styleColor,
    const ProgramComparisonDescriptor::File::ListItemUiInfo &uiInfo,
    float offsetX)
{
    ImU32 mainColor;

    assert(uiInfo.m_similarity.has_value());
    if (uiInfo.m_similarity.value() == 100)
    {
        // Set green color when similarity is 100%.
        mainColor = GreenColor;
    }
    else
    {
        // Blend from red to somewhat green when similarity is 0-99%.
        const float similarity = float(uiInfo.m_similarity.value()) / 100;
        const ImU32 CloseToGreenColor = IM_COL32(0, 255, 0, ImU32(similarity * 160));
        mainColor = ImAlphaBlendColors(RedColor, CloseToGreenColor);
    }

    // Set main color for text background.
    const ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    const ImVec2 labelSize = ImGui::CalcTextSize(uiInfo.m_label.c_str(), nullptr, true);
    const ImRect rect(
        ImVec2(cursorPos.x + offsetX, cursorPos.y),
        ImVec2(cursorPos.x + labelSize.x + offsetX, cursorPos.y + labelSize.y));

    ImDrawList *drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(rect.Min, rect.Max, CreateColor(mainColor, 128));

    // Set blended colors for selectable region.
    const ImU32 headerColor = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_Header));
    const ImU32 headerColorHovered = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_HeaderHovered));
    const ImU32 headerColorActive = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_HeaderActive));
    const ImU32 blendedHeaderColor = ImAlphaBlendColors(mainColor, CreateColor(headerColor, 128));
    const ImU32 blendedHeaderColorHovered = ImAlphaBlendColors(mainColor, CreateColor(headerColorHovered, 64));
    const ImU32 blendedHeaderColorActive = ImAlphaBlendColors(mainColor, CreateColor(headerColorActive, 64));

    styleColor.Push(ImGuiCol_Header, CreateColor(blendedHeaderColor, 79));
    styleColor.Push(ImGuiCol_HeaderHovered, CreateColor(blendedHeaderColorHovered, 204));
    styleColor.Push(ImGuiCol_HeaderActive, CreateColor(blendedHeaderColorActive, 255));
}

bool ImGuiApp::Button(const char *label, ImGuiButtonFlags flags)
{
    ImGuiWindow *window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiStyle &style = GImGui->Style;
    const ImVec2 labelSize = ImGui::CalcTextSize(label, NULL, true);
    ImVec2 size;
    if (labelSize.x + style.FramePadding.x * 2.0f > StandardMinButtonSize.x)
    {
        size = ImVec2(0, 0); // This will make ImGui auto scale it.
    }
    else
    {
        size = StandardMinButtonSize;
    }
    return ImGui::ButtonEx(label, size, flags);
}

bool ImGuiApp::FileDialogButton(
    const char *button_label,
    std::string *file_path_name,
    const std::string &key,
    const std::string &title,
    const char *filters)
{
    const std::string button_label_key = fmt::format("{:s}##{:s}", button_label, key);
    const bool open = ImGui::Button(button_label_key.c_str());
    UpdateFileDialog(open, file_path_name, key, title, filters);
    return open;
}

bool ImGuiApp::TreeNodeHeader(const char *label, ImGuiTreeNodeFlags flags)
{
    ScopedStyleColor styleColor;
    TreeNodeHeaderStyleColor(styleColor);
    return ImGui::TreeNodeEx(label, flags | TreeNodeHeaderFlags);
}

bool ImGuiApp::TreeNodeHeader(const char *str_id, ImGuiTreeNodeFlags flags, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    ScopedStyleColor styleColor;
    TreeNodeHeaderStyleColor(styleColor);
    bool isOpen = ImGui::TreeNodeExV(str_id, flags | TreeNodeHeaderFlags, fmt, args);
    va_end(args);
    return isOpen;
}

void ImGuiApp::TreeNodeHeaderStyleColor(ScopedStyleColor &styleColor)
{
    styleColor.Push(ImGuiCol_Header, IM_COL32(0xDB, 0x61, 0x40, 150));
    styleColor.Push(ImGuiCol_HeaderHovered, IM_COL32(0xDB, 0x61, 0x40, 204));
    styleColor.Push(ImGuiCol_HeaderActive, IM_COL32(0xDB, 0x61, 0x40, 255));
}

} // namespace unassemblize::gui
