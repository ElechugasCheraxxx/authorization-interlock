/**
 * @file IPCBridge.cpp
 * @brief Implementación del puente IPC
 */

#include "IPCBridge.hpp"
#include "mach/MachProtector.hpp"
#include "mach/MachServer.hpp"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <cstring>
#include <sys/select.h>

namespace interlock {

// ============================================================================
// IPCServer
// ============================================================================
IPCServer::IPCServer(IPCComponentType component, const std::string& socket_path)
    : component_(component), socket_path_(socket_path), server_fd_(-1), running_(false) {}

IPCServer::~IPCServer() {
    stop();
    if (server_fd_ != -1) {
        close(server_fd_);
    }
}

bool IPCServer::start() {
    // Crear Unix Domain Socket
    server_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd_ == -1) {
        std::cerr << "[IPC] Error creando socket" << std::endl;
        return false;
    }

    // Configurar socket no bloqueante
    int flags = fcntl(server_fd_, F_GETFL, 0);
    fcntl(server_fd_, F_SETFL, flags | O_NONBLOCK);

    // Preparar dirección
    struct sockaddr_un addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, socket_path_.c_str(), sizeof(addr.sun_path) - 1);

    // Eliminar socket previo si existe
    unlink(socket_path_.c_str());

    // Bind
    if (bind(server_fd_, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        std::cerr << "[IPC] Error en bind: " << socket_path_ << std::endl;
        close(server_fd_);
        server_fd_ = -1;
        return false;
    }

    // Listen
    if (listen(server_fd_, 5) == -1) {
        std::cerr << "[IPC] Error en listen" << std::endl;
        close(server_fd_);
        server_fd_ = -1;
        return false;
    }

    running_ = true;
    server_thread_ = std::thread(&IPCServer::acceptLoop, this);

    std::cout << "[IPC] Servidor iniciado: " << socket_path_ << std::endl;
    return true;
}

void IPCServer::stop() {
    running_ = false;
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
}

void IPCServer::registerHandler(uint32_t message_type, MessageHandler handler) {
    handlers_[message_type] = handler;
}

bool IPCServer::sendResponse(const IPCMessage& response) {
    // Esta función sería más compleja en una implementación real
    return true;
}

void IPCServer::acceptLoop() {
    while (running_) {
        struct sockaddr_un client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(server_fd_, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd == -1) {
            // Non-blocking socket, esperar un poco
            usleep(100000);  // 100ms
            continue;
        }

        // Leer mensaje
        IPCMessage msg;
        if (recv(client_fd, &msg, sizeof(msg), 0) > 0) {
            // Buscar handler
            auto it = handlers_.find(msg.message_id);
            if (it != handlers_.end()) {
                IPCMessage response = it->second(msg);
                send(client_fd, &response, sizeof(response), 0);
            }
        }

        close(client_fd);
    }
}

// ============================================================================
// IPCClient
// ============================================================================
IPCClient::IPCClient(IPCComponentType component)
    : component_(component), socket_fd_(-1), message_counter_(0) {}

IPCClient::~IPCClient() {
    disconnect();
}

bool IPCClient::connect(const std::string& socket_path) {
    socket_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socket_fd_ == -1) {
        return false;
    }

    struct sockaddr_un addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);

    if (::connect(socket_fd_, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }

    return true;
}

bool IPCClient::sendRequest(const IPCMessage& request, IPCMessage& response, uint32_t timeout_ms) {
    if (socket_fd_ == -1) {
        return false;
    }

    // Enviar request
    if (send(socket_fd_, &request, sizeof(request), 0) <= 0) {
        return false;
    }

    // Esperar respuesta con timeout
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(socket_fd_, &readfds);

    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    int ret = select(socket_fd_ + 1, &readfds, nullptr, nullptr, &tv);
    if (ret <= 0) {
        return false;  // Timeout or error
    }

    return receiveMessage(response);
}

bool IPCClient::sendAsync(const IPCMessage& message) {
    if (socket_fd_ == -1) {
        return false;
    }

    return send(socket_fd_, &message, sizeof(message), 0) > 0;
}

void IPCClient::disconnect() {
    if (socket_fd_ != -1) {
        close(socket_fd_);
        socket_fd_ = -1;
    }
}

bool IPCClient::receiveMessage(IPCMessage& msg) {
    return recv(socket_fd_, &msg, sizeof(msg), 0) == sizeof(msg);
}

// ============================================================================
// IPCBridge SINGLETON
// ============================================================================
static IPCBridge* g_ipc_bridge = nullptr;

IPCBridge& IPCBridge::getInstance() {
    if (!g_ipc_bridge) {
        g_ipc_bridge = new IPCBridge();
    }
    return *g_ipc_bridge;
}

IPCBridge::IPCBridge() {}
IPCBridge::~IPCBridge() {}

bool IPCBridge::initializeServers() {
    // 0. Inicializar Servidores Mach Nativos (Prioridad 0 en M4 Pro)
    mach::StartMachServer();

    // Inicializar servidores para cada componente de la orquesta políglota
    std::vector<IPCComponentType> components = {
        IPCComponentType::CPP_CORE,
        IPCComponentType::SWIFT_MIDDLEWARE,
        IPCComponentType::RUST_CIPHER,
        IPCComponentType::GO_ORCHESTRATOR,
        IPCComponentType::ZIG_PAYLOADS,
        IPCComponentType::C_SHIELD,
        IPCComponentType::ASM_STUBS,
        IPCComponentType::TS_INTEL,
        IPCComponentType::PERL_SURGERY,
        IPCComponentType::CS_AGENTS,
        IPCComponentType::UNITY_HOLO,
        IPCComponentType::RUBY_DISPATCHER,
        IPCComponentType::FORMAL_PROOFS
    };

    for (auto component : components) {
        auto server = std::make_shared<IPCServer>(component, getSocketPath(component));
        if (!server->start()) {
            std::cerr << "[IPC] Fallo al iniciar servidor para componente: " << static_cast<int>(component) << std::endl;
            // No retornamos false inmediatamente para permitir que otros componentes suban
        }
        servers_[static_cast<int>(component)] = server;
    }

    std::cout << "[IPC] Orquesta políglota inicializada con " << components.size() << " canales activos." << std::endl;
    return true;
}

std::shared_ptr<IPCClient> IPCBridge::getClient(IPCComponentType target) {
    auto it = clients_.find(static_cast<int>(target));
    if (it != clients_.end()) {
        return it->second;
    }

    auto client = std::make_shared<IPCClient>(target);
    if (client->connect(getSocketPath(target))) {
        clients_[static_cast<int>(target)] = client;
        return client;
    }

    return nullptr;
}

void IPCBridge::registerGlobalHandler(IPCComponentType component, uint32_t message_type,
                                     IPCServer::MessageHandler handler) {
    auto it = servers_.find(static_cast<int>(component));
    if (it != servers_.end()) {
        it->second->registerHandler(message_type, handler);
    }
}

bool IPCBridge::sendMessage(IPCComponentType sender, IPCComponentType receiver,
                           const IPCMessage& message) {
    auto client = getClient(receiver);
    if (!client) {
        return false;
    }

    IPCMessage response;
    return client->sendRequest(message, response);
}

std::string IPCBridge::getSocketPath(IPCComponentType component) {
    return std::string(SOCKET_BASE) + std::to_string(static_cast<int>(component));
}

} // namespace interlock
