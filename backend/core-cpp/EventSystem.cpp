#include "EventSystem.hpp"
#include <iostream>

namespace interlock {

void EventBus::publish(const Event& event) {
    std::lock_guard<std::mutex> lock(mtx);

    // Almacenar en el Store automáticamente
    EventStore::getInstance().store(event);

    if (subscribers.find(event.type) != subscribers.end()) {
        for (auto& callback : subscribers[event.type]) {
            callback(event);
        }
    }

    // Suscriptores globales (*)
    if (subscribers.find("*") != subscribers.end()) {
        for (auto& callback : subscribers["*"]) {
            callback(event);
        }
    }
}

void EventBus::subscribe(const std::string& type, EventCallback callback) {
    std::lock_guard<std::mutex> lock(mtx);
    subscribers[type].push_back(callback);
}

void EventStore::store(const Event& event) {
    std::lock_guard<std::mutex> lock(mtx);
    history.push_back(event);
}

std::vector<Event> EventStore::getHistory(const std::string& type) const {
    std::lock_guard<std::mutex> lock(mtx);
    if (type.empty()) return history;

    std::vector<Event> filtered;
    for (const auto& e : history) {
        if (e.type == type) filtered.push_back(e);
    }
    return filtered;
}

void EventStore::clear() {
    std::lock_guard<std::mutex> lock(mtx);
    history.clear();
}

void ReplayEngine::replayFrom(long long timestamp) {
    auto history = EventStore::getInstance().getHistory();
    for (const auto& e : history) {
        if (e.timestamp >= timestamp) {
            EventBus::getInstance().publish(e);
        }
    }
}

void ReplayEngine::createBranch(const std::string& branch_name, long long start_timestamp) {
    std::lock_guard<std::mutex> lock(mtx);
    auto history = EventStore::getInstance().getHistory();
    std::vector<Event> branch_events;
    for (const auto& e : history) {
        if (e.timestamp >= start_timestamp) {
            branch_events.push_back(e);
        }
    }
    branches[branch_name] = branch_events;
}

} // namespace interlock
