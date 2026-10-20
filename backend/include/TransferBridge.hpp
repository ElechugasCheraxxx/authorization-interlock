#pragma once
#include <vector>
#include <string>
#include <cstdint>

namespace interlock::zig {

enum class TicketType : uint32_t {
    Golden = 0x1,
    Silver = 0x2,
    Service = 0x3,
};

/**
 * @brief Bridge para el motor Shadow-Mover de Zig.
 * Permite la distribución sigilosa de tickets Kerberos forjados entre procesos.
 */
class TransferBridge {
public:
    static TransferBridge& getInstance() {
        static TransferBridge instance;
        return instance;
    }

    /**
     * @brief Distribuye un ticket a múltiples procesos objetivos.
     * Esta operación es de alta prioridad y se ejecuta en P-Cores.
     */
    bool broadcastTicket(TicketType type, const std::vector<uint8_t>& ticket_data, const std::vector<int32_t>& target_pids);

    /**
     * @brief Induce un crash inmediato utilizando las primitivas de Shadow-Mover.
     */
    void induceCrash(bool gpu = false);

private:
    TransferBridge() = default;
};

} // namespace interlock::zig
