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

#include "commontypes.h"
#include "imguiclient/imguicore.h"

#include <functional>
#include <string_view>

struct ImGuiTextFilterEx : public ImGuiTextFilter
{
    bool Draw(const char *key, const char *label = "Filter (inc,-exc)", float width = 0.0f);
    bool PassFilter(std::string_view view) const;

    bool hasExternalFilterCondition = false;
};

namespace unassemblize::gui
{
template<typename Type>
struct TextFilterDescriptor
{
    template<typename SourceContainerValueType>
    using FilterCallback = std::function<bool(const ImGuiTextFilterEx &filter, const SourceContainerValueType &value)>;
    using FilterType = Type;

    TextFilterDescriptor(const char *key) : m_key(key) {}

    bool DrawFilter();

    template<typename Container>
    void UpdateFilter(const Container &source, FilterCallback<typename Container::value_type> &&filterCallback);

    template<typename Container>
    bool DrawAndUpdateFilter(const Container &source, FilterCallback<typename Container::value_type> &&filterCallback);

    const ImVector<FilterType> &Filtered() const;

    // Clears the filtered state but does not reset the user specified filter words.
    void Reset();

    // Set true if the filter callback has more than just the text to filter with.
    void SetExternalFilterCondition(bool value);

private:
    bool NeedsUpdate() const;

    template<typename Container>
    static void UpdateFilter(
        ImVector<FilterType> &filtered,
        const ImGuiTextFilterEx &filter,
        const Container &source,
        FilterCallback<typename Container::value_type> &&filterCallback);

private:
    const char *const m_key;
    ImGuiTextFilterEx m_filter; // The filter state for the input field.
    ImVector<FilterType> m_filtered; // Array to the filtered elements after an update.
    bool m_filteredOnce = false; // Set false to force the next draw to update the filtered state.
};

template<typename Type>
bool TextFilterDescriptor<Type>::DrawFilter()
{
    return m_filter.Draw(m_key) || NeedsUpdate();
}

template<typename Type>
template<typename Container>
void TextFilterDescriptor<Type>::UpdateFilter(
    const Container &source,
    FilterCallback<typename Container::value_type> &&filterCallback)
{
    UpdateFilter(m_filtered, m_filter, source, std::move(filterCallback));
    m_filteredOnce = true;
}

template<typename Type>
template<typename Container>
bool TextFilterDescriptor<Type>::DrawAndUpdateFilter(
    const Container &source,
    FilterCallback<typename Container::value_type> &&filterCallback)
{
    if (DrawFilter())
    {
        UpdateFilter(source, std::move(filterCallback));
        return true;
    }
    return false;
}

template<typename Type>
const ImVector<Type> &TextFilterDescriptor<Type>::Filtered() const
{
    return m_filtered;
}

template<typename Type>
void TextFilterDescriptor<Type>::Reset()
{
    m_filtered.clear();
    m_filteredOnce = false;
}

template<typename Type>
void TextFilterDescriptor<Type>::SetExternalFilterCondition(bool value)
{
    m_filter.hasExternalFilterCondition = value;
}

template<typename Type>
bool TextFilterDescriptor<Type>::NeedsUpdate() const
{
    return !m_filteredOnce;
}

template<typename Type>
template<typename Container>
void TextFilterDescriptor<Type>::UpdateFilter(
    ImVector<FilterType> &filtered,
    const ImGuiTextFilterEx &filter,
    const Container &source,
    FilterCallback<typename Container::value_type> &&filterCallback)
{
    if (filter.IsActive() || filter.hasExternalFilterCondition)
    {
        const size_t size = source.size();
        filtered.clear();
        filtered.reserve(size);
        for (size_t i = 0; i < size; ++i)
        {
            if (filterCallback(filter, source[i]))
            {
                if constexpr (std::is_pointer_v<FilterType>)
                    filtered.push_back(&source[i]);
                else
                    filtered.push_back(source[i]);
            }
        }
    }
    else
    {
        const size_t size = source.size();
        filtered.resize(size);
        for (size_t i = 0; i < size; ++i)
        {
            if constexpr (std::is_pointer_v<FilterType>)
                filtered[i] = &source[i];
            else
                filtered[i] = source[i];
        }
    }
}

} // namespace unassemblize::gui
