/**
 * @file
 *
 * @brief Helper struct for ImGuiApp to keep track of items
 *        that are scheduled to be processed just once.
 *
 * @copyright Unassemblize is free software: you can redistribute it and/or
 *            modify it under the terms of the GNU General Public License
 *            as published by the Free Software Foundation, either version
 *            3 of the License, or (at your option) any later version.
 *            A full copy of the GNU General Public License can be found in
 *            LICENSE
 */
#include "processedstate.h"

namespace unassemblize::gui
{
void ProcessedState::init(size_t maxItemsCount)
{
    m_processedItems.reserve(maxItemsCount);
    m_processedItemStates = BitArray(maxItemsCount, false);
}

bool ProcessedState::set_item_processed(IndexT index)
{
    assert(index < m_processedItemStates.size());

    const BitArray::BitIndexer bitIndexer = m_processedItemStates.get_indexer(index);

    if (m_processedItemStates.is_set(bitIndexer))
        return false;

    m_processedItems.push_back(index);
    m_processedItemStates.set(bitIndexer);
    return true;
}

size_t ProcessedState::get_processed_item_count() const
{
    return m_processedItems.size();
}

span<const IndexT> ProcessedState::get_processed_items(size_t begin, size_t end) const
{
    assert(begin <= end);
    assert(end <= m_processedItems.size());

    return span<const IndexT>{m_processedItems.data() + begin, m_processedItems.data() + end};
}

span<const IndexT> ProcessedState::get_items_for_processing(span<const IndexT> items)
{
    const size_t begin = get_processed_item_count();

    for (IndexT index : items)
    {
        set_item_processed(index);
    }

    const size_t end = get_processed_item_count();
    return get_processed_items(begin, end);
}

} // namespace unassemblize::gui
