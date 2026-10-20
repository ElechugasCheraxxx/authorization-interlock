#pragma once

#include "Schema.hpp"
#include "Blackboard.hpp"
#include "MessageRouter.hpp"
#include <string>
#include <memory>
#include <map>

namespace interlock {

class OperationSession {
public:
    OperationSession(const std::string& op_id);
    ~OperationSession();

    void start();
    void stop();

    std::string getOpId() const { return op_id; }

    // Configuración de la misión
    void setConfig(const json& config);
    json getConfig() const { return config; }

private:
    std::string op_id;
    json config;
    bool is_active = false;
};

class SessionManager {
public:
    static SessionManager& getInstance() {
        static SessionManager instance;
        return instance;
    }

    std::shared_ptr<OperationSession> createSession(const std::string& op_id);
    std::shared_ptr<OperationSession> getSession(const std::string& op_id);
    void removeSession(const std::string& op_id);

private:
    SessionManager() = default;
    std::map<std::string, std::shared_ptr<OperationSession>> active_sessions;
    std::mutex mtx;
};

} // namespace interlock
