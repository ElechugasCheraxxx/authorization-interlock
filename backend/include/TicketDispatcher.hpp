#pragma once

#include "Schema.hpp"
#include "Blackboard.hpp"
#include "MessageRouter.hpp"
#include <string>
#include <vector>
#include <mutex>

namespace interlock {

class TicketDispatcher {
public:
    static TicketDispatcher& getInstance() {
        static TicketDispatcher instance;
        return instance;
    }

    // Procesa un ticket entrante y genera los mensajes de despacho
    void dispatch(const Ticket& ticket);

    // Auditoría de resultados: Verifica si un ticket se completó con éxito
    void auditTicket(const std::string& ticket_id);

private:
    TicketDispatcher() = default;

    // Lógica interna para decidir qué agente es óptimo para una tarea
    std::string selectBestAgent(const std::string& instruction, int priority);
};

} // namespace interlock
