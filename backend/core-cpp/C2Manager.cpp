#include "C2Manager.hpp"
#include <iostream>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <vector>
#include <thread>

namespace interlock {

bool C2Manager::startListener(const std::string& type, const std::string& host, int port) {
    std::lock_guard<std::mutex> lock(mtx);
    if (running) return false;

    std::cout << "[C2Manager] Starting " << type << " listener on " << host << ":" << port << std::endl;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("[C2Manager] Socket creation failed");
        return false;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("[C2Manager] setsockopt failed");
        close(server_fd);
        return false;
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(host.c_str());
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("[C2Manager] Bind failed");
        close(server_fd);
        return false;
    }

    if (listen(server_fd, 10) < 0) {
        perror("[C2Manager] Listen failed");
        close(server_fd);
        return false;
    }

    running = true;
    accept_thread = std::thread(&C2Manager::acceptLoop, this);

    std::cout << "[C2Manager] Listener active on fd " << server_fd << std::endl;
    return true;
}

void C2Manager::acceptLoop() {
    while (running) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);

        if (client_fd < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                perror("[C2Manager] Accept failed");
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        std::string ip = inet_ntoa(client_addr.sin_addr);
        std::cout << "[C2Manager] New connection from " << ip << " on fd " << client_fd << std::endl;

        std::lock_guard<std::mutex> lock(mtx);

        // Use ResourcePool for ultra-fast session allocation
        C2Session* session = session_pool.construct();
        if (!session) {
            std::cerr << "[C2Manager] Pool exhausted! Dropping connection." << std::endl;
            close(client_fd);
            continue;
        }

        std::string sid = "sess_" + std::to_string(client_fd);
        strncpy(session->id, sid.c_str(), sizeof(session->id) - 1);
        strncpy(session->remote_addr, ip.c_str(), sizeof(session->remote_addr) - 1);
        session->active = true;
        session->last_seen = std::chrono::system_clock::now().time_since_epoch().count();

        session_map[sid] = session;
    }
}

void C2Manager::stopListener(const std::string& type, int port) {
    std::lock_guard<std::mutex> lock(mtx);
    std::cout << "[C2Manager] Stopping listener on port " << port << std::endl;
}

std::string C2Manager::generateImplant(const std::string& type, const std::string& listener_url, const json& config) {
    // Aquí se delega al ArtifactManager para generar el binario/script
    return "implant_payload_here";
}

std::vector<C2Session> C2Manager::getSessions() {
    std::lock_guard<std::mutex> lock(mtx);
    std::vector<C2Session> sessions;
    for (const auto& [id, session_ptr] : session_map) {
        sessions.push_back(*session_ptr);
    }
    return sessions;
}

bool C2Manager::sendCommand(const std::string& session_id, const std::string& cmd) {
    std::lock_guard<std::mutex> lock(mtx);
    if (session_map.find(session_id) == session_map.end()) return false;

    std::cout << "[C2Manager] Sending command to " << session_id << ": " << cmd << std::endl;
    // Escribir al socket de la sesión
    return true;
}

std::string C2Manager::getCommandResponse(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(mtx);
    // Leer del buffer de la sesión
    return "Command executed successfully (Mock)";
}

void C2Manager::closeSession(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = session_map.find(session_id);
    if (it != session_map.end()) {
        session_pool.destroy(it->second);
        session_map.erase(it);
        std::cout << "[C2Manager] Session " << session_id << " closed and returned to pool." << std::endl;
    }
}

} // namespace interlock
