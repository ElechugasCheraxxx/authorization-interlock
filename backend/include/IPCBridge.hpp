/**
 * @file IPCBridge.hpp
 * @brief Infraestructura de IPC unificada para comunicación inter-lenguajes
 * 
 * Soporta:
 * - Unix Domain Sockets (UDS)
 * - FlatBuffers para serialización
 * - Mecanismo de solicitud/respuesta
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <queue>
#include <thread>

namespace interlock {

/**
 * @enum IPCComponentType
 * @brief Tipos de componentes en el sistema IPC
 */
enum class IPCComponentType : uint8_t {
    CPP_CORE          = 1,
    SWIFT_MIDDLEWARE  = 2,
    RUST_CIPHER       = 3,
    GO_ORCHESTRATOR   = 4,
    ZIG_PAYLOADS      = 5,
    C_SHIELD          = 6,
    ASM_STUBS         = 7,
    TS_INTEL          = 8,
    PERL_SURGERY      = 9,
    CS_AGENTS         = 10,
    UNITY_HOLO        = 11,
    RUBY_DISPATCHER   = 12,
    FORMAL_PROOFS     = 13
};

/**
 * @struct IPCMessage
 * @brief Mensaje IPC estándar
 */
struct IPCMessage {
    static constexpr size_t MAX_PAYLOAD = 65536;  // 64 KB

    uint32_t message_id;       // ID único del mensaje
    uint32_t sequence_num;     // Número de secuencia
    IPCComponentType sender;   // Remitente
    IPCComponentType receiver; // Destinatario
    uint32_t payload_size;     // Tamaño del artifact
    uint32_t flags;            // Flags (request/response, urgency)
    uint64_t timestamp;        // Timestamp UNIX
    uint8_t artifact[MAX_PAYLOAD];  // Artifact (FlatBuffers)

    // Getters/setters
    void setRequest() { flags |= 0x01; }
    void setResponse() { flags &= ~0x01; }
    bool isRequest() const { return (flags & 0x01) != 0; }
    bool isUrgent() const { return (flags & 0x02) != 0; }
};

/**
 * @class IPCServer
 * @brief Servidor IPC que escucha en Unix Domain Socket
 */
class IPCServer {
public:
    using MessageHandler = std::function<IPCMessage(const IPCMessage&)>;

    IPCServer(IPCComponentType component, const std::string& socket_path);
    ~IPCServer();

    /**
     * @brief Inicia el servidor en un thread separado
     */
    bool start();

    /**
     * @brief Detiene el servidor
     */
    void stop();

    /**
     * @brief Registra un handler para un tipo de mensaje
     */
    void registerHandler(uint32_t message_type, MessageHandler handler);

    /**
     * @brief Envía una respuesta
     */
    bool sendResponse(const IPCMessage& response);

private:
    IPCComponentType component_;
    std::string socket_path_;
    int server_fd_;
    std::thread server_thread_;
    bool running_;
    std::queue<IPCMessage> message_queue_;
    std::unordered_map<uint32_t, MessageHandler> handlers_;

    void acceptLoop();
};

/**
 * @class IPCClient
 * @brief Cliente IPC para comunicarse con otros componentes
 */
class IPCClient {
public:
    IPCClient(IPCComponentType component);
    ~IPCClient();

    /**
     * @brief Conecta a un servidor IPC
     */
    bool connect(const std::string& socket_path);

    /**
     * @brief Envía un mensaje y espera respuesta (bloqueante)
     */
    bool sendRequest(const IPCMessage& request, IPCMessage& response, uint32_t timeout_ms = 5000);

    /**
     * @brief Envía un mensaje sin esperar respuesta
     */
    bool sendAsync(const IPCMessage& message);

    /**
     * @brief Desconecta del servidor
     */
    void disconnect();

private:
    IPCComponentType component_;
    int socket_fd_;
    uint32_t message_counter_;

    bool receiveMessage(IPCMessage& msg);
};

/**
 * @class IPCBridge
 * @brief Puente de comunicación global para toda la arquitectura
 */
class IPCBridge {
public:
    static IPCBridge& getInstance();

    /**
     * @brief Inicializa los servidores IPC para todos los componentes
     */
    bool initializeServers();

    /**
     * @brief Obtiene un cliente para comunicarse con un componente específico
     */
    std::shared_ptr<IPCClient> getClient(IPCComponentType target);

    /**
     * @brief Registra un handler global
     */
    void registerGlobalHandler(IPCComponentType component, uint32_t message_type,
                              IPCServer::MessageHandler handler);

    /**
     * @brief Envía mensaje punto-a-punto
     */
    bool sendMessage(IPCComponentType sender, IPCComponentType receiver,
                    const IPCMessage& message);

private:
    IPCBridge();
    ~IPCBridge();

    std::unordered_map<int, std::shared_ptr<IPCServer>> servers_;
    std::unordered_map<int, std::shared_ptr<IPCClient>> clients_;

    static constexpr const char* SOCKET_BASE = "/tmp/interlock_ipc_";

    std::string getSocketPath(IPCComponentType component);
};

} // namespace interlock
