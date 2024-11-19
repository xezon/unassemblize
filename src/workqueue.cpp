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

namespace unassemblize
{
WorkQueueCommandId WorkQueueCommand::s_id = InvalidWorkQueueCommandId + 1; // 1

WorkQueue::~WorkQueue()
{
    stop(true);
}

void WorkQueue::start()
{
    assert(!m_thread.joinable());
    m_thread = std::thread(ThreadFunction, this);
}

void WorkQueue::stop(bool wait)
{
    if (m_thread.joinable())
    {
        m_commandQueue.enqueue(std::make_unique<WorkQueueCommandQuit>(*this));
        if (wait)
            m_thread.join();
        else
            m_thread.detach();
    }
}

bool WorkQueue::is_busy() const
{
    return m_thread.joinable();
}

bool WorkQueue::enqueue(WorkQueueCommandPtr &&command)
{
    return m_commandQueue.enqueue(std::move(command));
}

bool WorkQueue::enqueue(WorkQueueDelayedCommand &delayed_command)
{
    if (delayed_command.next_delayed_command == nullptr)
        return false;

    WorkQueueResultPtr result;
    return enqueue(std::move(delayed_command.next_delayed_command), result);
}

bool WorkQueue::enqueue(WorkQueueDelayedCommandPtr &&delayed_command, WorkQueueResultPtr &result)
{
    WorkQueueCommandPtr chained_command = delayed_command->create(result);

    // Delayed work can decide to not create a chained command. In this case, the command chain is done.
    if (chained_command == nullptr)
        return false;

    // Loses original command chain if next_delayed_command is set.
    if (chained_command->next_delayed_command == nullptr)
        chained_command->next_delayed_command = std::move(delayed_command->next_delayed_command);

    return enqueue(std::move(chained_command));
}

bool WorkQueue::try_dequeue(WorkQueueResultPtr &result)
{
    return m_pollingQueue.try_dequeue(result);
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
                enqueue(std::move(command->next_delayed_command), result);
            }
        }
    }
}

void WorkQueue::ThreadFunction(WorkQueue *self)
{
    self->ThreadRun();
}

void WorkQueue::ThreadRun()
{
    while (true)
    {
        WorkQueueCommandPtr command;

        if (m_quit)
        {
            if (!m_commandQueue.try_dequeue(command))
            {
                // Queue is finally empty. Quit.
                // Wait for jobs here when applicable.
                break;
            }
        }
        else
        {
            m_commandQueue.wait_dequeue(command);
        }

        assert(command != nullptr);
        assert(command->has_work());

        // Submit work to thread pool
        WorkQueueResultPtr result = m_threadPool.submit_task([cmd = std::move(command)]() { return cmd->work(); }).get();

        m_lastFinishedCommandId = command->command_id;

        // Command work functions do not need to return a result,
        // but when using a callback or delayed command then a result is required.
        if (result == nullptr)
        {
            const bool has_callback = command->has_callback();
            const bool has_delayed_command = command->has_delayed_command();

            if (has_callback || has_delayed_command)
            {
                result = std::make_unique<WorkQueueResult>();
            }
        }

        if (result != nullptr)
        {
            result->command = std::move(command);

            const bool has_callback = result->command->has_callback();
            const bool has_delayed_command = result->command->has_delayed_command();

            if (has_callback || has_delayed_command)
            {
                m_callbackQueue.enqueue(result);
            }
            else
            {
                m_pollingQueue.enqueue(result);
            }
        }
    }
}

} // namespace unassemblize
