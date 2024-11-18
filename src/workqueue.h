/**
 * @file
 *
 * @brief Class to instigate all high level functionality for unassemblize
 *        from another thread
 *
 * @copyright Unassemblize is free software: you can redistribute it and/or
 *            modify it under the terms of the GNU General Public License
 *            as published by the Free Software Foundation, either version
 *            3 of the License, or (at your option) any later version.
 *            A full copy of the GNU General Public License can be found in
 *            LICENSE
 */
#pragma once

#define MOODYCAMEL_EXCEPTIONS_ENABLED 0
#include "readerwriterqueue.h"
#include "runner.h"
#include <functional>
#include <memory>
#include <thread>

namespace unassemblize
{
class WorkQueue;
struct WorkQueueCommand;
struct WorkQueueDelayedCommand;
struct WorkQueueResult;
struct WorkQueueCommandQuit;

using WorkQueueCommandPtr = std::unique_ptr<WorkQueueCommand>;
using WorkQueueDelayedCommandPtr = std::unique_ptr<WorkQueueDelayedCommand>;
using WorkQueueResultPtr = std::unique_ptr<WorkQueueResult>;
using WorkQueueCommandId = uint32_t;
inline constexpr WorkQueueCommandId InvalidWorkQueueCommandId = 0;

using WorkQueueCommandCreateFunction = std::function<WorkQueueCommandPtr(WorkQueueResultPtr &result)>;
using WorkQueueCommandWorkFunction = std::function<WorkQueueResultPtr(void)>;
using WorkQueueCommandCallbackFunction = std::function<void(WorkQueueResultPtr &result)>;

// The delayed command is a substitute for a real command, used to chain commands on demand.
struct WorkQueueDelayedCommand
{
    friend class WorkQueue;

    bool has_delayed_command() const { return next_delayed_command != nullptr; }

    // Optional function to create and chain 1 new command.
    // The function is invoked after the previous command has completed its work and has returned its result.
    // Note 1: The delayed command is result->command->next_delayed_command at the time of invocation.
    // Note 2: The result is null if the delayed command is the head of the command chain.
    WorkQueueDelayedCommand *chain(WorkQueueCommandCreateFunction &&create_function)
    {
        // It would be possible to chain multiple, but currently we just chain one maximum.
        assert(next_delayed_command == nullptr);

        next_delayed_command = std::make_unique<WorkQueueDelayedCommand>();
        next_delayed_command->create = create_function;
        return next_delayed_command.get();
    }

    WorkQueueDelayedCommandPtr next_delayed_command;

private:
    WorkQueueCommandCreateFunction create;
};

struct WorkQueueCommand : public WorkQueueDelayedCommand
{
    WorkQueueCommand() : command_id(s_id++) {}
    virtual ~WorkQueueCommand() = default;

    bool has_work() const { return bool(work); }
    bool has_callback() const { return bool(callback); }

    // Mandatory function to perform any work for this command.
    // Returns the result that is used for callback or polling.
    // Can return null result.
    WorkQueueCommandWorkFunction work;

    // Optional callback when the issued command has been completed.
    // Prefer chaining commands with the chain function instead of doing it in the callback.
    // The result will not be pushed to the polling queue.
    WorkQueueCommandCallbackFunction callback;

    // Optional context. Can point to address or store an id.
    // Necessary to identify when polling.
    void *context = nullptr;

    const WorkQueueCommandId command_id;

private:
    static WorkQueueCommandId s_id; // Not atomic, because intended to be created on single thread.
};

struct WorkQueueResult
{
    virtual ~WorkQueueResult() = default;

    WorkQueueCommandPtr command;
};

class WorkQueue
{
    friend class WorkQueueCommandQuit;

    template<typename T, size_t MAX_BLOCK_SIZE = 512>
    using ReaderWriterQueue = moodycamel::ReaderWriterQueue<T, MAX_BLOCK_SIZE>;

    template<typename T, size_t MAX_BLOCK_SIZE = 512>
    using BlockingReaderWriterQueue = moodycamel::BlockingReaderWriterQueue<T, MAX_BLOCK_SIZE>;

public:
    WorkQueue() : m_commandQueue(31), m_pollingQueue(31), m_callbackQueue(31) {};
    ~WorkQueue();

    void start();
    void stop(bool wait);

    bool is_busy() const;
    bool is_quitting() const { return m_quit; }

    WorkQueueCommandId get_last_finished_command_id() const { return m_lastFinishedCommandId.load(); }

    bool enqueue(WorkQueueCommandPtr &&command);
    bool enqueue(WorkQueueDelayedCommand &delayed_command);

    bool try_dequeue(WorkQueueResultPtr &result);

    void update_callbacks();

private:
    bool enqueue(WorkQueueDelayedCommandPtr &&delayed_command, WorkQueueResultPtr &result);

    static void ThreadFunction(WorkQueue *self);
    void ThreadRun();

    BlockingReaderWriterQueue<WorkQueueCommandPtr, 32> m_commandQueue;
    ReaderWriterQueue<WorkQueueResultPtr, 32> m_pollingQueue;
    ReaderWriterQueue<WorkQueueResultPtr, 32> m_callbackQueue;

    std::thread m_thread;

    std::atomic<WorkQueueCommandId> m_lastFinishedCommandId = InvalidWorkQueueCommandId;

    volatile bool m_quit = false;
};

struct WorkQueueCommandQuit : public WorkQueueCommand
{
    WorkQueueCommandQuit(WorkQueue &owner_queue) : m_ownerQueue(owner_queue)
    {
        WorkQueueCommand::work = [this]() {
            m_ownerQueue.m_quit = true;
            return WorkQueueResultPtr();
        };
    }

    virtual ~WorkQueueCommandQuit() override = default;

    WorkQueue &m_ownerQueue;
};

} // namespace unassemblize
