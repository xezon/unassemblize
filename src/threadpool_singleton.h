#pragma once

#include <BS_thread_pool.hpp>
#include <thread>

namespace unassemblize
{

class ThreadPoolSingleton
{
public:
    static BS::thread_pool &get_workqueue_pool()
    {
        static BS::thread_pool instance(calculate_workqueue_threads());
        return instance;
    }

    static BS::thread_pool &get_runner_pool()
    {
        static BS::thread_pool instance(calculate_runner_threads());
        return instance;
    }

private:
    ThreadPoolSingleton() = default;

    static size_t calculate_workqueue_threads()
    {
        const size_t total_cores = std::thread::hardware_concurrency();

        // Allocation strategy for WorkQueue pool:
        // - Minimum: 2 threads for quick tasks
        // - Maximum: 25% of available cores
        const size_t suggested = total_cores / 4; // 25% of cores
        return std::clamp(suggested, size_t(2), size_t(4)); // Between 2-4 threads
    }

    static size_t calculate_runner_threads()
    {
        const size_t total_cores = std::thread::hardware_concurrency();
        const size_t reserved = 1 + // UI thread
            1 + // WorkQueue thread
            calculate_workqueue_threads(); // WorkQueue pool

        // Allocation strategy for Runner pool:
        // - Use remaining cores after reservations
        // - Ensure at least 1 thread
        return std::max(total_cores > reserved ? total_cores - reserved : 1, size_t(1));
    }
};

} // namespace unassemblize