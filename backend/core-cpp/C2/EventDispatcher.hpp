#pragma once

#include <functional>
#include <vector>
#include <map>
#include <mutex>
#include <memory>
#include <any>

namespace interlock::c2 {

/**
 * @brief High-performance Event Dispatcher for C2 Operations.
 * Inspired by GTA's CAnimBlendAssociation and CEvent systems.
 */
enum class EventType {
    Finish = 1,
    Delete = 2,
    CommandComplete = 3,
    PayloadDeployed = 4,
    AgentHeartbeat = 5
};

class EventDispatcher {
public:
    using Callback = std::function<void(EventType, std::any)>;

    static EventDispatcher& getInstance() {
        static EventDispatcher instance;
        return instance;
    }

    void subscribe(EventType type, Callback cb) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_subscribers[type].push_back(cb);
    }

    void dispatch(EventType type, std::any data = {}) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (auto it = m_subscribers.find(type); it != m_subscribers.end()) {
            for (const auto& cb : it->second) {
                cb(type, data);
            }
        }
    }

private:
    EventDispatcher() = default;
    std::mutex m_mutex;
    std::map<EventType, std::vector<Callback>> m_subscribers;
};

} // namespace interlock::c2
