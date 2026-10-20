#pragma once

#include <vector>
#include <memory>
#include <algorithm>
#include <type_traits>

namespace interlock::c2 {

/**
 * @brief Base class for Agent Tasks.
 * Inspired by GTA's CTask system.
 */
class Task {
public:
    virtual ~Task() = default;
    virtual void makeExecutable() = 0;
    virtual void stop() = 0;
    virtual bool isFinished() const = 0;
    virtual int getPriority() const = 0;
};

/**
 * @brief Template-based Task Manager for Agent orchestration.
 * Manages primary and secondary task stacks with priority-based preemption.
 */
class TaskManager {
public:
    enum TaskSlot {
        PRIMARY = 0,
        SECONDARY = 1,
        MAX_SLOTS = 2
    };

    void setTask(std::unique_ptr<Task> task, TaskSlot slot) {
        if (m_tasks[slot] && m_tasks[slot]->getPriority() > task->getPriority()) {
            return; // Lower priority tasks cannot preempt
        }

        if (m_tasks[slot]) {
            m_tasks[slot]->stop();
        }

        m_tasks[slot] = std::move(task);
        m_tasks[slot]->makeExecutable();
    }

    void update() {
        for (auto& task : m_tasks) {
            if (task && task->isFinished()) {
                task.reset();
            }
        }
    }

    bool hasActiveTask(TaskSlot slot) const {
        return m_tasks[slot] != nullptr;
    }

private:
    std::unique_ptr<Task> m_tasks[MAX_SLOTS];
};

} // namespace interlock::c2
