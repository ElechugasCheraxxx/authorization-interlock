#pragma once

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <memory>
#include <functional>
#include "Schema.hpp"

namespace interlock {

struct Event {
    std::string id;
    std::string type;
    std::string source;
    json data;
    long long timestamp;
    std::string status = "OK"; // "OK" or "DEGRADED_VOLATILE"

    std::string to_xml() const {
        std::stringstream ss;
        ss << "<Event id=\"" << xml_escape(id) << "\" type=\"" << xml_escape(type) << "\" status=\"" << xml_escape(status) << "\">\n";
        ss << "  <Source>" << xml_escape(source) << "</Source>\n";
        ss << "  <Data><![CDATA[" << data.dump() << "]]></Data>\n";
        ss << "  <Timestamp>" << timestamp << "</Timestamp>\n";
        ss << "  <TimeISO>" << to_iso8601(timestamp) << "</TimeISO>\n";
        ss << "</Event>";
        return ss.str();
    }

    json to_json() const {
        return {
            {"id", id},
            {"type", type},
            {"source", source},
            {"data", data},
            {"timestamp", to_iso8601(timestamp)},
            {"status", status}
        };
    }
};

using EventCallback = std::function<void(const Event&)>;

class EventBus {
public:
    static EventBus& getInstance() {
        static EventBus instance;
        return instance;
    }

    void publish(const Event& event);
    void subscribe(const std::string& type, EventCallback callback);

private:
    EventBus() = default;
    std::map<std::string, std::vector<EventCallback>> subscribers;
    std::mutex mtx;
};

class EventStore {
public:
    static EventStore& getInstance() {
        static EventStore instance;
        return instance;
    }

    void store(const Event& event);
    std::vector<Event> getHistory(const std::string& type = "") const;
    void clear();

private:
    EventStore() = default;
    std::vector<Event> history;
    mutable std::mutex mtx;
};

// Replay Engine & Branch Manager
class ReplayEngine {
public:
    static ReplayEngine& getInstance() {
        static ReplayEngine instance;
        return instance;
    }

    void replayFrom(long long timestamp);
    void createBranch(const std::string& branch_name, long long start_timestamp);

private:
    ReplayEngine() = default;
    std::map<std::string, std::vector<Event>> branches;
    std::mutex mtx;
};

} // namespace interlock
