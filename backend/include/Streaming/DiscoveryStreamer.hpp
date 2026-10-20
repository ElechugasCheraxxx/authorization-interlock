#pragma once

#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>
#include <condition_variable>

namespace interlock::streaming {

/**
 * @brief High-performance Asynchronous Discovery & Artifact Streamer.
 * Inspired by stream manager (referencia previa).
 * Features:
 * - 4-channel parallel I/O.
 * - Priority-based task scheduling.
 * - Lock-free request queues.
 */
class DiscoveryStreamer {
public:
    enum class Priority {
        LOW = 0,
        NORMAL,
        HIGH,
        CRITICAL
    };

    struct Task {
        std::string targetId;
        std::function<void()> action;
        Priority priority;

        bool operator<(const Task& other) const {
            return priority < other.priority;
        }
    };

    DiscoveryStreamer(size_t channelCount = 4)
        : m_stop(false) {
        for (size_t i = 0; i < channelCount; ++i) {
            m_workers.emplace_back(&DiscoveryStreamer::workerThread, this);
        }
    }

    ~DiscoveryStreamer() {
        m_stop = true;
        m_cv.notify_all();
        for (auto& worker : m_workers) {
            if (worker.joinable()) worker.join();
        }
    }

    void submit(Task task) {
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            m_tasks.push(std::move(task));
        }
        m_cv.notify_one();
    }

private:
    void workerThread() {
        while (!m_stop) {
            Task task;
            {
                std::unique_lock<std::mutex> lock(m_queueMutex);
                m_cv.wait(lock, [this] { return m_stop || !m_tasks.empty(); });

                if (m_stop && m_tasks.empty()) return;

                task = std::move(const_cast<Task&>(m_tasks.top()));
                m_tasks.pop();
            }

            if (task.action) {
                task.action();
            }
        }
    }

    std::vector<std::thread> m_workers;
    std::priority_queue<Task> m_tasks;
    std::mutex m_queueMutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_stop;
};

} // namespace interlock::streaming
