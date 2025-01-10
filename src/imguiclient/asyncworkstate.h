/**
 * @file
 *
 * @brief Helper struct for ImGuiApp
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
#include "util.h"
#include "workqueue.h"

namespace unassemblize::gui
{
template<typename AsyncWorkReason>
class AsyncWorkState
{
public:
    using WorkReason = AsyncWorkReason;

    template<uint32_t Size>
    using WorkQueueCommandIdArray = SizedArray<WorkQueueCommandId, uint32_t, Size>;

    struct WorkItem
    {
        WorkQueueCommandId commandId;
        WorkReason reason;
    };

    void add(WorkItem &&item) { m_workItems.emplace_back(std::move(item)); }

    void remove(WorkQueueCommandId commandId)
    {
        util::find_and_erase_if(m_workItems, [=](const WorkItem &item) { return item.commandId == commandId; });
    }

    [[nodiscard]] bool empty() const { return m_workItems.empty(); }

    [[nodiscard]] span<const WorkItem> get() const { return span<const WorkItem>{m_workItems}; }

    template<uint32_t Size>
    [[nodiscard]] WorkQueueCommandIdArray<Size> get_command_id_array(uint64_t reasonMask) const
    {
        const uint32_t count = std::min<uint32_t>(Size, m_workItems.size());
        WorkQueueCommandIdArray<Size> commandIdArray;
        uint32_t arrayIndex = commandIdArray.size();
        for (uint32_t itemIdx = 0; itemIdx < count; ++itemIdx)
        {
            const WorkItem &item = m_workItems[itemIdx];
            if ((uint64_t(1) << uint64_t(item.reason)) & reasonMask)
            {
                commandIdArray[arrayIndex++] = item.commandId;
            }
        }
        commandIdArray.set_size(arrayIndex);
        return commandIdArray;
    }

    [[nodiscard]] static constexpr uint64_t get_reason_mask(std::initializer_list<WorkReason> reasons)
    {
        uint64_t reasonMask = 0;
        for (const WorkReason reason : reasons)
        {
            reasonMask |= uint64_t(1) << uint64_t(reason);
        }
        return reasonMask;
    }

private:
    std::vector<WorkItem> m_workItems;
};

} // namespace unassemblize::gui
