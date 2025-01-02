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
#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS
#include "util/nocopy.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <string>
#include <string_view>

namespace unassemblize::gui
{
struct WindowPlacement
{
    ImVec2 pos = ImVec2(-FLT_MAX, -FLT_MAX);
    ImVec2 size = ImVec2(-FLT_MAX, -FLT_MAX);
};

struct ScopedStyleColor : NoCopyNoMove
{
    ScopedStyleColor() = default;
    ~ScopedStyleColor();

    void Push(ImGuiCol idx, ImU32 col);
    void Push(ImGuiCol idx, const ImVec4 &col);
    void PopAll();

private:
    int m_popStyleCount = 0;
};

ImVec2 CalcTextSize(std::string_view view, bool hide_text_after_double_hash = false, float wrap_width = -1.0f);

void TextUnformatted(std::string_view view);
void TextUnformattedCenteredX(std::string_view view, float width_x = 0.f);

void TooltipText(const char *fmt, ...);
void TooltipTextV(const char *fmt, va_list args);
void TooltipTextUnformatted(std::string_view view);
void TooltipTextMarker(const char *fmt, ...);
void TooltipTextUnformattedMarker(std::string_view view);

void MoveCursorScreenPos(float x, float y);

void OverlayProgressBar(const ImRect &rect, float fraction, const char *overlay = nullptr);

ImU32 ImAlphaBlendColors(ImU32 col_a, ImU32 col_b);

constexpr ImU32 CreateColor(ImU32 color, uint8_t alpha)
{
    ImU32 rgb = color & 0x00FFFFFF;
    ImU32 newAlpha = static_cast<ImU32>(alpha) << 24;
    return rgb | newAlpha;
}

void DrawInTextCircle(ImU32 color);
void DrawInTextTriangle(ImU32 color, ImGuiDir dir);

void DrawTriangle(ImDrawList *draw_list, ImVec2 center, float r, ImU32 color, ImGuiDir dir);

void DrawTextBackgroundColor(std::string_view view, ImU32 color, const ImVec2 &pos);

bool ApplyPlacementToNextWindow(WindowPlacement &placement);
void FetchPlacementFromWindowByName(WindowPlacement &placement, const char *window_name);
void FetchPlacementFromCurrentWindow(WindowPlacement &placement);

void UpdateFileDialog(
    bool open,
    std::string *file_path_name,
    const std::string &key,
    const std::string &title,
    const char *filters);

bool UpdateConfirmationPopup(bool open, const char *name, const char *message);

// Calculate a default table height.
template<bool TableUsesHeader = true>
float GetDefaultTableHeight(size_t max_rows, size_t default_rows)
{
    size_t extra = 0;
    if constexpr (TableUsesHeader)
        ++extra;
    return ImGui::GetTextLineHeightWithSpacing() * (std::min<size_t>(max_rows, default_rows) + extra);
}

// Calculate a max table height.
template<bool TableUsesHeader = true, bool TableUsesHorizontalScroll = true>
float GetMaxTableHeight(size_t max_rows)
{
    size_t extra = 0;
    if constexpr (TableUsesHeader)
        ++extra;
    if constexpr (TableUsesHorizontalScroll)
        ++extra;
    return ImGui::GetTextLineHeightWithSpacing() * (max_rows + extra);
}

} // namespace unassemblize::gui
