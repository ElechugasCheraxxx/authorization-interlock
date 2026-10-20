#include "TransferBridge.hpp"
#include "mach/MachPriority.hpp"
#include <iostream>

// Exported from Zig
extern "C" bool shadow_mover_broadcast(const uint8_t* ptr, size_t len, const int32_t* pids, size_t pid_count);
extern "C" void shadow_mover_induce_crash();
extern "C" void shadow_mover_induce_gpu_crash();

namespace interlock::zig {

bool TransferBridge::broadcastTicket(TicketType type, const std::vector<uint8_t>& ticket_data, const std::vector<int32_t>& target_pids) {
    std::cout << "[TransferBroker] Broadcasting " << static_cast<uint32_t>(type)
              << " ticket to " << target_pids.size() << " targets." << std::endl;

    // Ejecutar en P-Core para asegurar mínima latencia en la inyección de memoria
    // Aunque esta llamada es síncrona desde el thread llamador, el trabajo de Zig
    // se beneficiará si el thread actual tiene prioridad alta.

    if (ticket_data.empty() || target_pids.empty()) {
        return false;
    }

    // Llamada al motor nativo de Zig
    return shadow_mover_broadcast(ticket_data.data(), ticket_data.size(), target_pids.data(), target_pids.size());
}

void TransferBridge::induceCrash(bool gpu) {
    if (gpu) {
        shadow_mover_induce_gpu_crash();
    } else {
        shadow_mover_induce_crash();
    }
}

} // namespace interlock::zig
