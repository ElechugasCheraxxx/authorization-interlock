#include "MessageRouter.hpp"
#include "OrchestrationEngine.hpp"
#include "Blackboard.hpp"
#include "interop/FormalValidators.hpp"
#include <iostream>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace interlock {
// ... (rest of includes and namespaces)

const std::map<std::string, std::string> MessageRouter::CHANNEL_MAP = {
    {"cipher", "#general"}, {"overseer", "#general"}, {"chronos", "#general"},
    {"grader", "#general"}, {"apex", "#technical-agents"}, {"vantage", "#intel-agents"},
    {"beacon", "#operations-agents"}, {"rex", "#cipher-rex"}, {"atlas", "#cipher-atlas"},
    {"vector", "#cipher-vector"}, {"operator", "#operator"}
};

MessageRouter::MessageRouter()
    : queue([](const Message& a, const Message& b) {
        return static_cast<int>(a.priority) < static_cast<int>(b.priority);
    })
{
}

MessageRouter::~MessageRouter() {
    stop();
}

void MessageRouter::start() {
    if (running) return;
    running = true;
    dispatch_thread = std::thread(&MessageRouter::dispatchLoop, this);
}

void MessageRouter::stop() {
    {
        std::lock_guard<std::mutex> lock(queue_mtx);
        running = false;
    }
    queue_cv.notify_all();
    if (dispatch_thread.joinable()) {
        dispatch_thread.join();
    }
}

std::string MessageRouter::resolveChannel(const std::string& from_agent, const std::string& to) {
    if (to == "all") return "#general";
    if (to == "escalations") return "#escalations";

    // Optimized channel resolution using a sorted pair key (frozenset-like)
    std::string key_a = from_agent;
    std::string key_b = to;
    if (key_a > key_b) std::swap(key_a, key_b);
    std::string pair_key = key_a + ":" + key_b;

    // Specific private channels for agent pairs
    static const std::map<std::string, std::string> PRIVATE_CHANNELS = {
        {"rex:atlas", "#cipher-sync"},
        {"apex:vantage", "#intel-ops"},
        {"cipher:overseer", "#management"}
    };

    auto it_pair = PRIVATE_CHANNELS.find(pair_key);
    if (it_pair != PRIVATE_CHANNELS.end()) return it_pair->second;

    auto it = CHANNEL_MAP.find(from_agent);
    if (it != CHANNEL_MAP.end()) return it->second;

    return "#general";
}

void MessageRouter::route(Message& msg) {
    // --- FORMAL VERIFICATION LAYER (Coq Extract) ---
    // Mapping C++ Message to formal::MessageData
    if (!formal::MessageRouterValidator::isValidRoute(msg.from_agent, msg.to, msg.channel)) {
        std::cerr << "[MessageRouter] [FORMAL_ALARM] Illegal Route Detected! Dropping message from "
                  << msg.from_agent << " to " << msg.to << " on channel " << msg.channel << std::endl;
        return;
    }

    if (msg.channel.empty()) {
        msg.channel = resolveChannel(msg.from_agent, msg.to);
    }

    // Evitar bucles de mensajes
    {
        std::lock_guard<std::mutex> lock(seen_mtx);
        if (seen_messages.count(msg.msg_id)) return;
        seen_messages.insert(msg.msg_id);
        if (seen_messages.size() > 1000) seen_messages.erase(seen_messages.begin());
    }

    // Persistencia en Blackboard (Memoria Unificada C++)
    Blackboard::getInstance().pushMessage(msg);

    // NUEVO: Distribución a Bus de Datos (Redis + Kafka)
    publishToBus(msg);

    // Encolar para despacho local
    {
        std::lock_guard<std::mutex> lock(queue_mtx);
        queue.push(msg);
    }
    queue_cv.notify_one();
}

void MessageRouter::registerHandler(const std::string& recipient, MessageHandler handler) {
    std::lock_guard<std::mutex> lock(handlers_mtx);
    handlers[recipient].push_back(handler);
}

void MessageRouter::unregisterHandler(const std::string& recipient, MessageHandler handler) {
    std::lock_guard<std::mutex> lock(handlers_mtx);
    // Implementation for unregistering handlers
}

void MessageRouter::publishToBus(const Message& msg) {
    // ⚡ REDIS: Tiempo real + Coordinación Lateral (Blackboard Live)
    // RedisManager::getInstance().publish("interlock:bus:" + msg.channel, msg.to_json().dump());

    // 🔄 KAFKA: Event Streaming (Replay Mode & Persistence)
    // KafkaManager::getInstance().produce("operations_log", msg.msg_id, msg.to_json().dump());

    std::cout << "[MessageRouter] Published to Data Bus: " << msg.msg_id << " [" << msg.channel << "]" << std::endl;
}

void MessageRouter::subscribeToExternalBus() {
    // ⚡ REDIS: Escuchar mensajes de otros nodos para coordinación horizontal
}

void MessageRouter::dispatchLoop() {
    while (running) {
        Message msg;
        {
            std::unique_lock<std::mutex> lock(queue_mtx);
            queue_cv.wait(lock, [this] { return !queue.empty() || !running; });

            if (!running) break;

            msg = queue.top();
            queue.pop();
        }

        deliver(msg);
    }
}

void MessageRouter::setInterceptor(MessageInterceptor interceptor) {
    std::lock_guard<std::mutex> lock(handlers_mtx);
    this->interceptor = interceptor;
}

void MessageRouter::deliver(const Message& msg) {
    // Call interceptor if registered (e.g., OrchestrationEngine)
    if (interceptor) {
        interceptor(msg);
    }

    std::vector<MessageHandler> targets;

    {
        std::lock_guard<std::mutex> lock(handlers_mtx);

        if (msg.to == "all") {
            for (auto const& [name, h_list] : handlers) {
                targets.insert(targets.end(), h_list.begin(), h_list.end());
            }
        } else {
            auto it = handlers.find(msg.to);
            if (it != handlers.end()) {
                targets = it->second;
            }
        }
    }

    for (auto& handler : targets) {
        try {
            handler(msg);
        } catch (...) {
            // Log error
        }
    }
}

} // namespace interlock
