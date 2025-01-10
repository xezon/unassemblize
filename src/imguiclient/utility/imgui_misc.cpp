/**
 * @file
 *
 * @brief ImGui utility
 *
 * @copyright Unassemblize is free software: you can redistribute it and/or
 *            modify it under the terms of the GNU General Public License
 *            as published by the Free Software Foundation, either version
 *            3 of the License, or (at your option) any later version.
 *            A full copy of the GNU General Public License can be found in
 *            LICENSE
 */
#include "imgui_misc.h"
#include "imgui_scoped.h"
#include <ImGuiFileDialog.h>
#include <fmt/core.h>
#include <misc/cpp/imgui_stdlib.h>

namespace unassemblize::gui
{
static WindowPlacement g_lastFileDialogPlacement;
static WindowPlacement g_lastConfirmationDialogPlacement;

ScopedStyleColor::~ScopedStyleColor()
{
    PopAll();
}

void ScopedStyleColor::Push(ImGuiCol idx, ImU32 col)
{
    ImGui::PushStyleColor(idx, col);
    ++m_popStyleCount;
}

void ScopedStyleColor::Push(ImGuiCol idx, const ImVec4 &col)
{
    ImGui::PushStyleColor(idx, col);
    ++m_popStyleCount;
}

void ScopedStyleColor::PopAll()
{
    if (m_popStyleCount > 0)
    {
        ImGui::PopStyleColor(m_popStyleCount);
        m_popStyleCount = 0;
    }
}

ImVec2 CalcTextSize(std::string_view view, bool hide_text_after_double_hash, float wrap_width)
{
    return ImGui::CalcTextSize(view.data(), view.data() + view.size(), hide_text_after_double_hash, wrap_width);
}

void TextUnformatted(std::string_view view)
{
    ImGui::TextUnformatted(view.data(), view.data() + view.size());
}

void TextUnformattedCenteredX(std::string_view view, float width_x)
{
    if (width_x == 0.f)
    {
        width_x = ImGui::GetContentRegionAvail().x;
    }
    const ImVec2 text_size = CalcTextSize(view);
    const float text_x = (width_x - text_size.x) / 2.0f;

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + text_x);

    TextUnformatted(view);
}

void TooltipText(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    TooltipTextV(fmt, args);
    va_end(args);
}

void TooltipTextV(const char *fmt, va_list args)
{
    if (ImGui::BeginItemTooltip())
    {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextV(fmt, args);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void TooltipTextUnformatted(std::string_view view)
{
    if (ImGui::BeginItemTooltip())
    {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        TextUnformatted(view);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void TooltipTextMarker(const char *fmt, ...)
{
    ImGui::TextDisabled("(?)");
    va_list args;
    va_start(args, fmt);
    TooltipTextV(fmt, args);
    va_end(args);
}

void TooltipTextUnformattedMarker(std::string_view view)
{
    ImGui::TextDisabled("(?)");
    TooltipTextUnformatted(view);
}

void MoveCursorScreenPos(float x, float y)
{
    const ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    ImGui::SetCursorScreenPos(ImVec2(cursorPos.x + x, cursorPos.y + y));
}

void OverlayProgressBar(const ImRect &rect, float fraction, const char *overlay)
{
    // Set position and size for the child window
    ImGui::SetCursorScreenPos(rect.Min);
    ImVec2 childSize = rect.GetSize();

    // Create a child window
    constexpr ImGuiWindowFlags childFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
        | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground;

    ImScoped::Child child("progress_child", childSize, false, childFlags);

    if (child.IsContentVisible)
    {
        ImDrawList *drawList = ImGui::GetWindowDrawList();

        // Define a translucent overlay color
        const ImVec4 &dimBgVec4 = ImGui::GetStyleColorVec4(ImGuiCol_ModalWindowDimBg);
        const ImU32 dimBgU32 = ImGui::ColorConvertFloat4ToU32(dimBgVec4);

        // Draw a filled rectangle over the group area
        drawList->AddRectFilled(rect.Min, rect.Max, dimBgU32);

        // Calculate the center position for the ImGui ProgressBar
        ImVec2 center = ImVec2((rect.Min.x + rect.Max.x) * 0.5f, (rect.Min.y + rect.Max.y) * 0.5f);
        ImVec2 progressBarSize = ImVec2(rect.Max.x - rect.Min.x, 20.0f);

        // Set cursor position to the center of the overlay rectangle
        ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();
        ImGui::SetCursorScreenPos(ImVec2(center.x - progressBarSize.x * 0.5f, center.y - progressBarSize.y * 0.5f));

        {
            // Set a custom background color with transparency
            ImScoped::StyleColor frameBg(ImGuiCol_FrameBg, ImGui::GetStyleColorVec4(ImGuiCol_TitleBg));

            // Render the ImGui progress bar
            ImGui::ProgressBar(fraction, progressBarSize, overlay);
        }

        // Set cursor position back
        ImGui::SetCursorScreenPos(cursorScreenPos);
    }
}

// Copied from imgui_internal.h ver 1.91.4
ImU32 ImAlphaBlendColors(ImU32 col_a, ImU32 col_b)
{
    float t = ((col_b >> IM_COL32_A_SHIFT) & 0xFF) / 255.f;
    int r = ImLerp((int)(col_a >> IM_COL32_R_SHIFT) & 0xFF, (int)(col_b >> IM_COL32_R_SHIFT) & 0xFF, t);
    int g = ImLerp((int)(col_a >> IM_COL32_G_SHIFT) & 0xFF, (int)(col_b >> IM_COL32_G_SHIFT) & 0xFF, t);
    int b = ImLerp((int)(col_a >> IM_COL32_B_SHIFT) & 0xFF, (int)(col_b >> IM_COL32_B_SHIFT) & 0xFF, t);
    return IM_COL32(r, g, b, 0xFF);
}

void DrawInTextCircle(ImU32 color)
{
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const ImVec2 font_size = CalcTextSize("a");
    const float r = font_size.x * 0.5f;
    const ImVec2 center = pos + ImVec2(r, font_size.y * 0.5f);
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    draw_list->AddCircleFilled(center, r, color, 0);
    ImGui::SetCursorScreenPos(ImVec2(pos.x + font_size.x, pos.y));
}

void DrawInTextTriangle(ImU32 color, ImGuiDir dir)
{
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const ImVec2 font_size = ImGui::CalcTextSize("a");
    const float r = font_size.x * 0.5f;
    const ImVec2 center = pos + ImVec2(r, font_size.y * 0.5f);
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    DrawTriangle(draw_list, center, r, color, dir);
    ImGui::SetCursorScreenPos(ImVec2(pos.x + font_size.x, pos.y));
}

void DrawTriangle(ImDrawList *draw_list, ImVec2 center, float r, ImU32 color, ImGuiDir dir)
{
    ImVec2 a, b, c;
    switch (dir)
    {
        case ImGuiDir_Up:
        case ImGuiDir_Down:
            if (dir == ImGuiDir_Up)
                r = -r;
            a = ImVec2(+0.000f, +0.750f) * r;
            b = ImVec2(-0.866f, -0.750f) * r;
            c = ImVec2(+0.866f, -0.750f) * r;
            break;
        case ImGuiDir_Left:
        case ImGuiDir_Right:
            if (dir == ImGuiDir_Left)
                r = -r;
            a = ImVec2(+0.750f, +0.000f) * r;
            b = ImVec2(-0.750f, +0.866f) * r;
            c = ImVec2(-0.750f, -0.866f) * r;
            break;
        case ImGuiDir_None:
        case ImGuiDir_COUNT:
            IM_ASSERT(0);
            break;
    }
    draw_list->AddTriangleFilled(center + a, center + b, center + c, color);
}

void DrawTextBackgroundColor(std::string_view view, ImU32 color, const ImVec2 &pos)
{
    if (!view.empty())
    {
        const ImVec2 size = CalcTextSize(view, true);
        const ImRect rect(pos, ImVec2(pos.x + size.x, pos.y + size.y));
        ImDrawList *drawList = ImGui::GetWindowDrawList();
        drawList->AddRectFilled(rect.Min, rect.Max, color);
    }
}

bool ApplyPlacementToNextWindow(WindowPlacement &placement)
{
    if (placement.pos.x != -FLT_MAX)
    {
        ImGui::SetNextWindowPos(placement.pos, ImGuiCond_Appearing);
        ImGui::SetNextWindowSize(placement.size, ImGuiCond_Appearing);
        return true;
    }
    return false;
}

void FetchPlacementFromWindowByName(WindowPlacement &placement, const char *window_name)
{
    // Note: Is using internals of ImGui.
    if (const ImGuiWindow *window = ImGui::FindWindowByName(window_name))
    {
        placement.pos = window->Pos;
        placement.size = window->Size;
    }
}

void FetchPlacementFromCurrentWindow(WindowPlacement &placement)
{
    placement.pos = ImGui::GetWindowPos();
    placement.size = ImGui::GetWindowSize();
}

void UpdateFileDialog(
    bool open,
    std::string *file_path_name,
    const std::string &key,
    const std::string &title,
    const char *filters)
{
    constexpr ImVec2 minSize(600.f, 300.f);
    IGFD::FileDialog *instance = ImGuiFileDialog::Instance();

    if (open)
    {
        // Restore position and size of any last file dialog.
        if (!ApplyPlacementToNextWindow(g_lastFileDialogPlacement))
        {
            // If it is the first opened dialog, then center and resize it.
            const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(minSize, ImGuiCond_Appearing);
        }

        IGFD::FileDialogConfig config;
        config.path = ".";
        config.flags = ImGuiFileDialogFlags_Modal;
        instance->OpenDialog(key, title, filters, config);
    }

    if (instance->Display(key, ImGuiWindowFlags_NoCollapse, minSize))
    {
        // Note: Is using internals of ImGuiFileDialog
        const std::string window_name = fmt::format("{:s}##{:s}", title, key);
        FetchPlacementFromWindowByName(g_lastFileDialogPlacement, window_name.c_str());

        if (instance->IsOk())
        {
            *file_path_name = instance->GetFilePathName();
        }
        instance->Close();
    }
}

bool UpdateConfirmationPopup(bool open, const char *name, const char *message)
{
    bool confirmed = false;

    if (open)
    {
        ImGui::OpenPopup(name);
        ImGui::SetNextWindowSizeConstraints(ImVec2(300.0f, 0.0f), ImVec2(FLT_MAX, FLT_MAX));

        // Restore position and size of any last confirmation dialog.
        if (!ApplyPlacementToNextWindow(g_lastConfirmationDialogPlacement))
        {
            // If it is the first opened dialog, then center it.
            const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        }
    }

    if (ImGui::BeginPopupModal(name, NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        FetchPlacementFromCurrentWindow(g_lastConfirmationDialogPlacement);

        ImGui::TextWrapped(message);
        ImGui::Spacing();

        const float availWidth = ImGui::GetContentRegionAvail().x;
        const float buttonWidth = ImMin(120.0f, (availWidth - ImGui::GetStyle().ItemSpacing.x) / 2);
        const ImVec2 buttonSize(buttonWidth, 0.0f);

        // Center the 2 buttons in the dialog.
        const float buttonsWidth = buttonWidth * 2 + ImGui::GetStyle().ItemSpacing.x;
        const float indent = (availWidth - buttonsWidth) * 0.5f;
        if (indent > 0.0f)
        {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + indent);
        }

        if (ImGui::Button("OK", buttonSize))
        {
            confirmed = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", buttonSize))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
    return confirmed;
}

} // namespace unassemblize::gui
