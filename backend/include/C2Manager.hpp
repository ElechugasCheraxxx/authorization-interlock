#pragma once

#include "ResourcePool.hpp"
#include <string>
#include <vector>
#include <mutex>
#include <system_error>
#include <thread>
#include <map>
#include <memory>
#include "Schema.hpp"

namespace interlock {

struct C2Session {
    char id[64];
    char remote_addr[48];
    char type[32]; // "reverse_shell", "meterpreter", etc.
    long last_seen;
    bool active;

    C2Session() : last_seen(0), active(false) {
        id[0] = '\0';
        remote_addr[0] = '\0';
        type[0] = '\0';
    }
};

class C2Manager {
public:
    static C2Manager& getInstance() {
        static C2Manager instance;
        return instance;
    }

    // --- Listener Management ---
    bool startListener(const std::string& type, const std::string& host, int port);
    void stopListener(const std::string& type, int port);

    // --- Implant Generation ---
    std::string generateImplant(const std::string& type, const std::string& listener_url, const json& config);

    // --- Session Management ---
    std::vector<C2Session> getSessions();
    bool sendCommand(const std::string& session_id, const std::string& cmd);
    std::string getCommandResponse(const std::string& session_id);
    void closeSession(const std::string& session_id);

private:
    C2Manager() = default;
    std::mutex mtx;

    // GTA-style Operations Pool for high-performance session management
    ResourcePool<C2Session, 4096> session_pool;
    std::map<std::string, C2Session*> session_map;

    // Socket handlers
    int server_fd = -1;
    bool running = false;
    std::thread accept_thread;
    void acceptLoop();
};

} // namespace interlock
