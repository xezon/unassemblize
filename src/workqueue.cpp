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
#include "workqueue.h"

#define BS_THREAD_POOL_DISABLE_EXCEPTION_HANDLING
#include <BS_thread_pool.hpp>

namespace unassemblize
{
WorkQueueDelayedCommand *get_last_delayed_command(WorkQueueDelayedCommand *delayedCommand)
{
    WorkQueueDelayedCommand *nextCommand = delayedCommand;
    while (nextCommand->has_delayed_command())
        nextCommand = nextCommand->next_delayed_command.get();
    return nextCommand;
}

WorkQueueCommandId WorkQueueCommand::s_id = InvalidWorkQueueCommandId + 1; // 1

WorkQueue::~WorkQueue()
{
    stop(true);
}

void WorkQueue::start()
{
    assert(!is_busy());
    m_busy = true;
    m_thread = std::thread(ThreadFunction, this);
}

void WorkQueue::stop(bool wait)
{
    if (m_thread.joinable())
    {
        assert(is_busy());
        // Signal quit and wake up the command queue.
        m_quit = true;
        {
            auto command = std::make_unique<WorkQueueCommand>();
            command->work = []() -> WorkQueueResultPtr { return nullptr; };
            m_commandQueue.enqueue(std::move(command));
        }
        m_thread.detach();
        if (wait)
        {
            // Update callbacks until all work is completed and the thread has quit.
            while (is_busy())
            {
                update_callbacks();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    }
}

bool WorkQueue::is_busy() const
{
    return m_busy;
}

bool WorkQueue::enqueue(WorkQueueCommandPtr &&command)
{
    return m_commandQueue.enqueue(std::move(command));
}

bool WorkQueue::enqueue(WorkQueueDelayedCommand &delayed_command)
{
    if (delayed_command.next_delayed_command == nullptr)
        return false;

    return enqueue(std::move(delayed_command.next_delayed_command));
}

bool WorkQueue::enqueue(WorkQueueDelayedCommandPtr &&delayed_command)
{
    while (delayed_command != nullptr)
    {
        WorkQueueCommandPtr chained_command = delayed_command->create();

        if (chained_command == nullptr)
        {
            // Delayed work can decide to not create a chained command.
            // In this case, the next delayed command is worked on.
            delayed_command = std::move(delayed_command->next_delayed_command);
            continue;
        }

        // Moves next delayed command to the very end of the new chained command.
        WorkQueueDelayedCommand *last_command = get_last_delayed_command(chained_command.get());
        last_command->next_delayed_command = std::move(delayed_command->next_delayed_command);

        return enqueue(std::move(chained_command));
    }
    return false;
}

void WorkQueue::update_callbacks()
{
    WorkQueueResultPtr result;

    while (m_callbackQueue.try_dequeue(result))
    {
        assert(result != nullptr);
        assert(result->command != nullptr);
        assert(result->command->has_callback() || result->command->has_delayed_command());

        // Invokes callback if applicable.
        {
            WorkQueueCommandPtr &command = result->command;

            if (command->has_callback())
            {
                // Moves the callback to decouple it from the result.
                WorkQueueCommandCallbackFunction callback = std::move(command->callback);
                callback(result);
            }
        }

        // Evaluates and enqueues next command if applicable.
        if (result != nullptr)
        {
            WorkQueueCommandPtr &command = result->command;

            if (command != nullptr && command->has_delayed_command())
            {
                enqueue(std::move(command->next_delayed_command));
            }
        }

        --m_callbackQueueSize;
    }
}

void WorkQueue::ThreadFunction(WorkQueue *self)
{
    self->ThreadRun();
}

namespace
{
struct CommandWrapper
{
    CommandWrapper(WorkQueueCommandPtr &&p) : command(std::move(p)) {}
    WorkQueueCommandPtr command;
};
} // namespace

void WorkQueue::ThreadRun()
{
    while (true)
    {
        WorkQueueCommandPtr command;

        if (m_quit)
        {
            if (!m_commandQueue.try_dequeue(command))
            {
                if (!HasPendingTasks() && m_callbackQueueSize == 0)
                {
                    // Check the command queue once more in case another command
                    // was enqueued after the previous checks completed.
                    if (!m_commandQueue.try_dequeue(command))
                        // Command queue is empty and no tasks are pending. Quit.
                        break;
                }
                else
                {
                    // Command queue is empty and tasks are pending. Continue.
                    continue;
                }
            }
        }
        else
        {
            m_commandQueue.wait_dequeue(command);
        }

        assert(command != nullptr);
        assert(command->has_work());

        if (m_threadPool != nullptr)
        {
            // Needs to allocate wrapper object because the thread pool is incompatible with mutable lambda.
            m_threadPool->detach_task([this, wrapper = new CommandWrapper(std::move(command))]() {
                DoWork(std::move(wrapper->command), true);
                delete wrapper;
            });
        }
        else
        {
            DoWork(std::move(command), false);
        }
    }

    m_busy = false;
}

void WorkQueue::DoWork(WorkQueueCommandPtr &&command, bool pooled)
{
    WorkQueueResultPtr result = command->work();
    const bool hasCallback = command->has_callback();
    const bool hasDelayedCommand = command->has_delayed_command();
    // Command work functions do not need to return a result,
    // but when using a callback or delayed command then a result is required.
    if (result == nullptr && (hasCallback || hasDelayedCommand))
    {
        result = std::make_unique<WorkQueueResult>();
    }
    if (result != nullptr)
    {
        result->command = std::move(command);
        if (hasCallback || hasDelayedCommand)
        {
            ++m_callbackQueueSize;
            // Take lock if thread pool will enqueue here.
            std::unique_lock lock(m_callbackMutex, std::defer_lock);
            if (pooled)
                lock.lock();
            m_callbackQueue.enqueue(std::move(result));
        }
    }
}

bool WorkQueue::HasPendingTasks() const
{
    return m_threadPool != nullptr && m_threadPool->get_tasks_total() != 0;
}

} // namespace unassemblize
