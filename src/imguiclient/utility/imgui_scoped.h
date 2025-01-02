#pragma once

#include "util/nocopy.h"
#include <imgui.h>

// clang-format off

namespace ImScoped
{
    struct Window : NoCopyNoMove
    {
        bool IsContentVisible;

        Window(const char* name, bool* p_open = NULL, ImGuiWindowFlags flags = 0) { IsContentVisible = ImGui::Begin(name, p_open, flags); }
        ~Window() { ImGui::End(); }

        explicit operator bool() const { return IsContentVisible; }
    };

    struct Child : NoCopyNoMove
    {
        bool IsContentVisible;

        Child(const char* str_id, const ImVec2& size = ImVec2(0,0), ImGuiChildFlags child_flags = 0, ImGuiWindowFlags window_flags = 0) { IsContentVisible = ImGui::BeginChild(str_id, size, child_flags, window_flags); }
        Child(ImGuiID id, const ImVec2& size = ImVec2(0,0), ImGuiChildFlags child_flags = 0, ImGuiWindowFlags window_flags = 0) { IsContentVisible = ImGui::BeginChild(id, size, child_flags, window_flags); }

        ~Child() { ImGui::EndChild(); }

        explicit operator bool() const { return IsContentVisible; }
    };

    struct Font : NoCopyNoMove
    {
        Font(ImFont* font) { ImGui::PushFont(font); }
        ~Font() { ImGui::PopFont(); }
    };

    struct StyleColor : NoCopyNoMove
    {
        StyleColor(ImGuiCol idx, ImU32 col) { ImGui::PushStyleColor(idx, col); }
        StyleColor(ImGuiCol idx, const ImVec4& col) { ImGui::PushStyleColor(idx, col); }
        ~StyleColor() { ImGui::PopStyleColor(); }
    };

    struct StyleVar : NoCopyNoMove
    {
        StyleVar(ImGuiStyleVar idx, float val) { ImGui::PushStyleVar(idx, val); }
        StyleVar(ImGuiStyleVar idx, const ImVec2& val) { ImGui::PushStyleVar(idx, val); }
        ~StyleVar() { ImGui::PopStyleVar(); }
    };

    struct ItemWidth : NoCopyNoMove
    {
        ItemWidth(float item_width) { ImGui::PushItemWidth(item_width); }
        ~ItemWidth() { ImGui::PopItemWidth(); }
    };

    struct TextWrapPos : NoCopyNoMove
    {
        TextWrapPos(float wrap_pos_x = 0.0f) { ImGui::PushTextWrapPos(wrap_pos_x); }
        ~TextWrapPos() { ImGui::PopTextWrapPos(); }
    };

    struct AllowKeyboardFocus : NoCopyNoMove
    {
        AllowKeyboardFocus(bool tab_stop) { ImGui::PushAllowKeyboardFocus(tab_stop); }
        ~AllowKeyboardFocus() { ImGui::PopAllowKeyboardFocus(); }
    };

    struct ButtonRepeat : NoCopyNoMove
    {
        ButtonRepeat(bool repeat) { ImGui::PushButtonRepeat(repeat); }
        ~ButtonRepeat() { ImGui::PopButtonRepeat(); }
    };

    struct Group : NoCopyNoMove
    {
        Group() { ImGui::BeginGroup(); }
        ~Group() { ImGui::EndGroup(); }
    };

    struct ID : NoCopyNoMove
    {
        ID(const char* str_id) { ImGui::PushID(str_id); }
        ID(const char* str_id_begin, const char* str_id_end) { ImGui::PushID(str_id_begin, str_id_end); }
        ID(const void* ptr_id) { ImGui::PushID(ptr_id); }
        ID(int int_id) { ImGui::PushID(int_id); }
        ~ID() { ImGui::PopID(); }
    };

    struct Combo : NoCopyNoMove
    {
        bool IsOpen;

        Combo(const char* label, const char* preview_value, ImGuiComboFlags flags = 0) { IsOpen = ImGui::BeginCombo(label, preview_value, flags); }
        ~Combo() { if (IsOpen) ImGui::EndCombo(); }

        explicit operator bool() const { return IsOpen; }
    };

    struct TreeNode : NoCopyNoMove
    {
        bool IsOpen;

        TreeNode(const char* label) { IsOpen = ImGui::TreeNode(label); }
        TreeNode(const char* str_id, const char* fmt, ...) IM_FMTARGS(3) { va_list ap; va_start(ap, fmt); IsOpen = ImGui::TreeNodeV(str_id, fmt, ap); va_end(ap); }
        TreeNode(const void* ptr_id, const char* fmt, ...) IM_FMTARGS(3) { va_list ap; va_start(ap, fmt); IsOpen = ImGui::TreeNodeV(ptr_id, fmt, ap); va_end(ap); }
        ~TreeNode() { if (IsOpen) ImGui::TreePop(); }

        explicit operator bool() const { return IsOpen; }
    };

    struct TreeNodeV : NoCopyNoMove
    {
        bool IsOpen;

        TreeNodeV(const char* str_id, const char* fmt, va_list args) IM_FMTLIST(3) { IsOpen = ImGui::TreeNodeV(str_id, fmt, args); }
        TreeNodeV(const void* ptr_id, const char* fmt, va_list args) IM_FMTLIST(3) { IsOpen = ImGui::TreeNodeV(ptr_id, fmt, args); }
        ~TreeNodeV() { if (IsOpen) ImGui::TreePop(); }

        explicit operator bool() const { return IsOpen; }
    };

    struct TreeNodeEx : NoCopyNoMove
    {
        bool IsOpen;

        TreeNodeEx(const char* label, ImGuiTreeNodeFlags flags = 0) { IM_ASSERT(!(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen)); IsOpen = ImGui::TreeNodeEx(label, flags); }
        TreeNodeEx(const char* str_id, ImGuiTreeNodeFlags flags, const char* fmt, ...) IM_FMTARGS(4) { va_list ap; va_start(ap, fmt); IM_ASSERT(!(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen)); IsOpen = ImGui::TreeNodeExV(str_id, flags, fmt, ap); va_end(ap); }
        TreeNodeEx(const void* ptr_id, ImGuiTreeNodeFlags flags, const char* fmt, ...) IM_FMTARGS(4) { va_list ap; va_start(ap, fmt); IM_ASSERT(!(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen)); IsOpen = ImGui::TreeNodeExV(ptr_id, flags, fmt, ap); va_end(ap); }
        ~TreeNodeEx() { if (IsOpen) ImGui::TreePop(); }

        explicit operator bool() const { return IsOpen; }
    };

    struct TreeNodeExV : NoCopyNoMove
    {
        bool IsOpen;

        TreeNodeExV(const char* str_id, ImGuiTreeNodeFlags flags, const char* fmt, va_list args) IM_FMTLIST(4) { IM_ASSERT(!(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen)); IsOpen = ImGui::TreeNodeExV(str_id, flags, fmt, args); }
        TreeNodeExV(const void* ptr_id, ImGuiTreeNodeFlags flags, const char* fmt, va_list args) IM_FMTLIST(4) { IM_ASSERT(!(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen)); IsOpen = ImGui::TreeNodeExV(ptr_id, flags, fmt, args); }
        ~TreeNodeExV() { if (IsOpen) ImGui::TreePop(); }

        explicit operator bool() const { return IsOpen; }
    };

    struct MainMenuBar : NoCopyNoMove
    {
        bool IsOpen;

        MainMenuBar() { IsOpen = ImGui::BeginMainMenuBar(); }
        ~MainMenuBar() { if (IsOpen) ImGui::EndMainMenuBar(); }

        explicit operator bool() const { return IsOpen; }
    };

    struct MenuBar : NoCopyNoMove
    {
        bool IsOpen;

        MenuBar() { IsOpen = ImGui::BeginMenuBar(); }
        ~MenuBar() { if (IsOpen) ImGui::EndMenuBar(); }

        explicit operator bool() const { return IsOpen; }
    };

    struct Menu : NoCopyNoMove
    {
        bool IsOpen;

        Menu(const char* label, bool enabled = true) { IsOpen = ImGui::BeginMenu(label, enabled); }
        ~Menu() { if (IsOpen) ImGui::EndMenu(); }

        explicit operator bool() const { return IsOpen; }
    };

    struct Tooltip : NoCopyNoMove
    {
        Tooltip() { ImGui::BeginTooltip(); }
        ~Tooltip() { ImGui::EndTooltip(); }
    };

    struct Popup : NoCopyNoMove
    {
        bool IsOpen;

        Popup(const char* str_id, ImGuiWindowFlags flags = 0) { IsOpen = ImGui::BeginPopup(str_id, flags); }
        ~Popup() { if (IsOpen) ImGui::EndPopup(); }

        explicit operator bool() const { return IsOpen; }
    };

    struct PopupContextItem : NoCopyNoMove
    {
        bool IsOpen;

        PopupContextItem(const char* str_id = NULL, int mouse_button = 1) { IsOpen = ImGui::BeginPopupContextItem(str_id, mouse_button); }
        ~PopupContextItem() { if (IsOpen) ImGui::EndPopup(); }

        explicit operator bool() const { return IsOpen; }
    };

    struct PopupContextWindow : NoCopyNoMove
    {
        bool IsOpen;

        PopupContextWindow(const char* str_id = NULL, ImGuiPopupFlags popup_flags = 1) { IsOpen = ImGui::BeginPopupContextWindow(str_id, popup_flags); }
        ~PopupContextWindow() { if (IsOpen) ImGui::EndPopup(); }

        explicit operator bool() const { return IsOpen; }
    };

    struct PopupContextVoid : NoCopyNoMove
    {
        bool IsOpen;

        PopupContextVoid(const char* str_id = NULL, int mouse_button = 1) { IsOpen = ImGui::BeginPopupContextVoid(str_id, mouse_button); }
        ~PopupContextVoid() { if (IsOpen) ImGui::EndPopup(); }

        explicit operator bool() const { return IsOpen; }
    };

    struct PopupModal : NoCopyNoMove
    {
        bool IsOpen;

        PopupModal(const char* name, bool* p_open = NULL, ImGuiWindowFlags flags = 0) { IsOpen = ImGui::BeginPopupModal(name, p_open, flags); }
        ~PopupModal() { if (IsOpen) ImGui::EndPopup(); }

        explicit operator bool() const { return IsOpen; }
    };

    struct DragDropSource : NoCopyNoMove
    {
        bool IsOpen;

        DragDropSource(ImGuiDragDropFlags flags = 0) { IsOpen = ImGui::BeginDragDropSource(flags); }
        ~DragDropSource() { if (IsOpen) ImGui::EndDragDropSource(); }

        explicit operator bool() const { return IsOpen; }
    };

    struct DragDropTarget : NoCopyNoMove
    {
        bool IsOpen;

        DragDropTarget() { IsOpen = ImGui::BeginDragDropTarget(); }
        ~DragDropTarget() { if (IsOpen) ImGui::EndDragDropTarget(); }

        explicit operator bool() const { return IsOpen; }
    };

    struct ClipRect : NoCopyNoMove
    {
        ClipRect(const ImVec2& clip_rect_min, const ImVec2& clip_rect_max, bool intersect_with_current_clip_rect) { ImGui::PushClipRect(clip_rect_min, clip_rect_max, intersect_with_current_clip_rect); }
        ~ClipRect() { ImGui::PopClipRect(); }
    };

    
#if 0 // DEPRECATED in 1.90.0 (from September 2023)
    struct ChildFrame : NoCopyNoMove
    {
        bool IsOpen;

        ChildFrame(ImGuiID id, const ImVec2& size, ImGuiWindowFlags flags = 0) { IsOpen = ImGui::BeginChildFrame(id, size, flags); }
        ~ChildFrame() { ImGui::EndChildFrame(); }

        explicit operator bool() const { return IsOpen; }
    };
#endif

    struct Disabled : NoCopyNoMove
    {
        Disabled(bool disabled) { ImGui::BeginDisabled(disabled); }
        ~Disabled() { ImGui::EndDisabled(); }
    };

    struct ListBox : NoCopyNoMove
    {
        bool IsContentVisible;

        ListBox(const char* label, const ImVec2& size = ImVec2(0, 0)) { IsContentVisible = ImGui::BeginListBox(label, size); }
        ~ListBox() { if (IsContentVisible) ImGui::EndListBox(); }

        explicit operator bool() const { return IsContentVisible; }
    };

    struct Table : NoCopyNoMove
    {
        bool IsContentVisible;

        Table(const char* str_id, int columns, ImGuiTableFlags flags = 0, const ImVec2& outer_size = ImVec2(0.0f, 0.0f), float inner_width = 0.0f)
        {
            IsContentVisible = ImGui::BeginTable(str_id, columns, flags, outer_size, inner_width);
        }
        ~Table() { if (IsContentVisible) ImGui::EndTable(); }

        explicit operator bool() const { return IsContentVisible; }
    };
} // namespace ImScoped

// clang-format on
