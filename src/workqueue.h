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
#include <mutex>
#include <thread>

namespace BS
{
class thread_pool;
}

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

using WorkQueueCommandCreateFunction = std::function<WorkQueueCommandPtr()>;
using WorkQueueCommandWorkFunction = std::function<WorkQueueResultPtr(void)>;
using WorkQueueCommandCallbackFunction = std::function<void(WorkQueueResultPtr &result)>;

WorkQueueDelayedCommand *get_last_delayed_command(WorkQueueDelayedCommand *delayedCommand);

// The delayed command is a substitute for a real command, used to chain commands on demand.
struct WorkQueueDelayedCommand
{
    friend class WorkQueue;

    bool has_delayed_command() const { return next_delayed_command != nullptr; }

    // Optional function to create and chain 1 new command.
    // The function is invoked after the previous command has completed its work and has returned its result.
    WorkQueueDelayedCommand *chain(WorkQueueCommandCreateFunction &&create_function)
    {
        // It would be possible to chain multiple, but currently we just chain one maximum.
        assert(next_delayed_command == nullptr);

        next_delayed_command = std::make_unique<WorkQueueDelayedCommand>();
        next_delayed_command->create = create_function;
        return next_delayed_command.get();
    }

    WorkQueueDelayedCommand *chain_to_last(WorkQueueCommandCreateFunction &&create_function)
    {
        WorkQueueDelayedCommand *next_command = get_last_delayed_command(this);
        return next_command->chain(std::move(create_function));
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

    const WorkQueueCommandId command_id;

private:
    static WorkQueueCommandId s_id; // Not atomic, because intended to be created on single thread.
};

struct WorkQueueResult
{
    virtual ~WorkQueueResult() = default;

    WorkQueueCommandPtr command;
};

// All public functions are meant to be called from a single thread only.
class WorkQueue
{
    friend class WorkQueueCommandQuit;

    template<typename T, size_t MAX_BLOCK_SIZE = 512>
    using ReaderWriterQueue = moodycamel::ReaderWriterQueue<T, MAX_BLOCK_SIZE>;

    template<typename T, size_t MAX_BLOCK_SIZE = 512>
    using BlockingReaderWriterQueue = moodycamel::BlockingReaderWriterQueue<T, MAX_BLOCK_SIZE>;

public:
    explicit WorkQueue(BS::thread_pool *threadPool = nullptr) :
        m_threadPool(threadPool), m_commandQueue(31), m_callbackQueue(31){};
    ~WorkQueue();

    void start();

    // Must keep calling update_callbacks if wait=false until is_busy returns false.
    void stop(bool wait);

    // Returns true as long as the work queue thread is running and waiting or working on commands.
    bool is_busy() const;

    bool enqueue(WorkQueueCommandPtr &&command);
    bool enqueue(WorkQueueDelayedCommand &delayed_command);

    // Updates callbacks and delayed commands.
    void update_callbacks();

private:
    bool enqueue(WorkQueueDelayedCommandPtr &&delayed_command);

    static void ThreadFunction(WorkQueue *self);
    void ThreadRun();
    void DoWork(WorkQueueCommandPtr &&command, bool pooled);
    bool HasPendingTasks() const;

private:
    BS::thread_pool *m_threadPool = nullptr; // Used to work on commands.
    std::thread m_thread; // Used to dequeue commands.

    BlockingReaderWriterQueue<WorkQueueCommandPtr, 32> m_commandQueue;
    ReaderWriterQueue<WorkQueueResultPtr, 32> m_callbackQueue;
    std::atomic<int> m_callbackQueueSize = 0;
    std::mutex m_callbackMutex;

    std::atomic<bool> m_busy = false;
    std::atomic<bool> m_quit = false;
};

} // namespace unassemblize
