#include "SessionManager.hpp"
#include "StateMachine.hpp"
#include "MessageRouter.hpp"
#include "PgConnectionPool.hpp"
#include "KafkaManager.hpp"
#include <iostream>
#include <chrono>
#include <pqxx/pqxx>

namespace interlock {

void OperationSession::start() {
    if (is_active) return;
    is_active = true;

    // 🧠 POSTGRESQL: Registrar sesión en la Fuente de Verdad
    try {
        auto conn = PgConnectionPool::getInstance().tryAcquire();
        if (conn) {
            pqxx::work W(*conn);
            W.exec0("INSERT INTO sessions (id, name, phase, start_time, metadata) VALUES (" +
                   W.quote(op_id) + ", " +
                   W.quote(config["name"].get<std::string>()) + ", 'INIT', " +
                   W.quote(std::chrono::system_clock::now().time_since_epoch().count()) + ", " +
                   W.quote(config.dump()) + ")");
            W.commit();
        }
    } catch (const std::exception& e) {
        std::cerr << "[OperationSession] DB Sync Error: " << e.what() << std::endl;
    }

    // 🔄 KAFKA: Marcar el inicio del stream para Replay Mode
    KafkaManager::getInstance().produce("session_lifecycle", op_id, "{\"event\": \"STARTED\"}");

    std::cout << "[OperationSession] Started session: " << op_id << std::endl;

    auto& router = MessageRouter::getInstance();

    // Notify all agents about the new session
    Message msg;
    msg.from_agent = "session_manager";
    msg.to = "all";
    msg.type = "session_start";
    msg.content = "New operation session started: " + op_id;
    msg.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    router.route(msg);
}

void OperationSession::stop() {
    if (!is_active) return;
    is_active = false;
    std::cout << "[OperationSession] Stopped session: " << op_id << std::endl;
}

void OperationSession::setConfig(const json& config) {
    this->config = config;
}

// --- SessionManager Implementation ---

std::shared_ptr<OperationSession> SessionManager::createSession(const std::string& op_id) {
    std::lock_guard<std::mutex> lock(mtx);
    auto session = std::make_shared<OperationSession>(op_id);
    active_sessions[op_id] = session;
    return session;
}

std::shared_ptr<OperationSession> SessionManager::getSession(const std::string& op_id) {
    std::lock_guard<std::mutex> lock(mtx);
    if (auto it = active_sessions.find(op_id); it != active_sessions.end()) {
        return it->second;
    }
    return nullptr;
}

void SessionManager::removeSession(const std::string& op_id) {
    std::lock_guard<std::mutex> lock(mtx);
    active_sessions.erase(op_id);
}

} // namespace interlock
