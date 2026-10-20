#pragma once

#include "Schema.hpp"
#include "Blackboard.hpp"
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <queue>
#include <memory>
#include <functional>
#include <set>
#include <condition_variable>
#include <thread>

namespace interlock {

using MessageHandler = std::function<void(const Message&)>;

class MessageRouter {
public:
    static MessageRouter& getInstance() {
        static MessageRouter instance;
        return instance;
    }

    void start();
    void stop();

    void route(Message& msg);
    void registerHandler(const std::string& recipient, MessageHandler handler);
    void unregisterHandler(const std::string& recipient, MessageHandler handler);

    // Integración de Streaming (Kafka/Redis)
    void publishToBus(const Message& msg);
    void subscribeToExternalBus();

    // Orchestration Interceptor
    using MessageInterceptor = std::function<void(const Message&)>;
    void setInterceptor(MessageInterceptor interceptor);

    static std::string resolveChannel(const std::string& from_agent, const std::string& to);

private:
    MessageRouter();
    ~MessageRouter();

    void dispatchLoop();
    void deliver(const Message& msg);

    MessageInterceptor interceptor = nullptr;
    std::map<std::string, std::vector<MessageHandler>> handlers;
    std::mutex handlers_mtx;

    // Colas de prioridad
    std::priority_queue<Message, std::vector<Message>, std::function<bool(const Message&, const Message&)>> queue;
    std::mutex queue_mtx;
    std::condition_variable queue_cv;

    bool running = false;
    std::thread dispatch_thread;

    std::set<std::string> seen_messages;
    std::mutex seen_mtx;

    static const std::map<std::string, std::string> CHANNEL_MAP;
};

} // namespace interlock
