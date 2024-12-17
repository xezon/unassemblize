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
WindowPlacement g_lastFileDialogPlacement;

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
    const ImVec2 text_size = ImGui::CalcTextSize(view.data(), view.data() + view.size());
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

void TooltipTextUnformatted(const char *text, const char *text_end)
{
    if (ImGui::BeginItemTooltip())
    {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(text, text_end);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void TooltipTextUnformatted(std::string_view view)
{
    TooltipTextUnformatted(view.data(), view.data() + view.size());
}

void TooltipTextMarker(const char *fmt, ...)
{
    ImGui::TextDisabled("(?)");
    va_list args;
    va_start(args, fmt);
    TooltipTextV(fmt, args);
    va_end(args);
}

void TooltipTextUnformattedMarker(const char *text, const char *text_end)
{
    ImGui::TextDisabled("(?)");
    TooltipTextUnformatted(text, text_end);
}

void OverlayProgressBar(const ImRect &rect, float fraction, const char *overlay)
{
    // Set position and size for the child window
    ImGui::SetCursorScreenPos(rect.Min);
    ImVec2 childSize = rect.GetSize();

    // Create a child window
    constexpr ImGuiWindowFlags childFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
        | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground;

    ImScoped::Child child("##progress_child", childSize, false, childFlags);

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

ImU32 CreateColor(ImU32 color, uint8_t alpha)
{
    ImU32 rgb = color & 0x00FFFFFF;
    ImU32 newAlpha = static_cast<ImU32>(alpha) << 24;
    return rgb | newAlpha;
}

void DrawInTextCircle(ImU32 color)
{
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const ImVec2 font_size = ImGui::CalcTextSize("a");
    const float x_radius = font_size.x * 0.5f;
    const float y_radius = font_size.y * 0.5f;
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    draw_list->AddCircleFilled(ImVec2(pos.x + x_radius, pos.y + y_radius), x_radius, color, 0);
    ImGui::SetCursorScreenPos(ImVec2(pos.x + font_size.x, pos.y));
}

void ApplyPlacementToNextWindow(WindowPlacement &placement)
{
    if (placement.pos.x != -FLT_MAX)
    {
        ImGui::SetNextWindowPos(placement.pos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(placement.size, ImGuiCond_Always);
    }
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

void AddFileDialogButton(
    std::string *file_path_name,
    std::string_view button_label,
    const std::string &key,
    const std::string &title,
    const char *filters)
{
    IGFD::FileDialog *instance = ImGuiFileDialog::Instance();

    const std::string button_label_key = fmt::format("{:s}##{:s}", button_label, key);
    if (ImGui::Button(button_label_key.c_str()))
    {
        // Restore position and size of any last file dialog.
        ApplyPlacementToNextWindow(g_lastFileDialogPlacement);

        IGFD::FileDialogConfig config;
        config.path = ".";
        config.flags = ImGuiFileDialogFlags_Modal;
        instance->OpenDialog(key, title, filters, config);
    }

    if (instance->Display(key, ImGuiWindowFlags_NoCollapse, ImVec2(600, 300)))
    {
        // Note: Is using internals of ImGuiFileDialog
        const std::string window_name = title + "##" + key;
        FetchPlacementFromWindowByName(g_lastFileDialogPlacement, window_name.c_str());

        if (instance->IsOk())
        {
            *file_path_name = instance->GetFilePathName();
        }
        instance->Close();
    }
}
} // namespace unassemblize::gui
