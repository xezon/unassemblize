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
#pragma once

#include "commontypes.h"
#include "util/bitarray.h"

namespace unassemblize::gui
{
class ProcessedState
{
public:
    void init(size_t maxItemsCount);
    [[nodiscard]] span<const IndexT> get_items_for_processing(span<const IndexT> indices);

private:
    bool set_item_processed(IndexT index);
    size_t get_processed_item_count() const;
    span<const IndexT> get_processed_items(size_t begin, size_t end) const;

    // Items that have been processed.
    std::vector<IndexT> m_processedItems;
    // Array of bits for all items to keep track of which ones have been processed.
    BitArray m_processedItemStates;
};

} // namespace unassemblize::gui
