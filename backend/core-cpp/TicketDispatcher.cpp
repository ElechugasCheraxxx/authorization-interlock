#include "TicketDispatcher.hpp"
#include <iostream>
#include <chrono>

namespace interlock {

void TicketDispatcher::dispatch(const Ticket& ticket) {
    std::cout << "[TicketDispatcher] Processing Ticket: " << ticket.id << " -> " << ticket.instruction << std::endl;

    // 1. Persistir en Blackboard
    Blackboard::getInstance().upsertTicket(ticket);

    // 2. Resolver el mejor agente para la tarea
    std::string agent = ticket.assigned_to;
    if (agent == "unassigned" || agent.empty()) {
        agent = selectBestAgent(ticket.instruction, ticket.priority);
    }

    // 3. Crear mensaje de tarea para el MessageRouter
    Message msg;
    msg.msg_id = "MSG-TCK-" + ticket.id;
    msg.from_agent = ticket.creator;
    msg.to = agent;
    msg.type = "task";
    msg.priority = static_cast<MessagePriority>(ticket.priority);
    msg.content = ticket.instruction;
    msg.timestamp = std::chrono::system_clock::now().time_since_epoch().count();

    json meta;
    meta["ticket_id"] = ticket.id;
    msg.metadata = meta;

    MessageRouter::getInstance().route(msg);

    std::cout << "[TicketDispatcher] Ticket " << ticket.id << " dispatched to " << agent << std::endl;
}

std::string TicketDispatcher::selectBestAgent(const std::string& instruction, int priority) {
    // Lógica simplificada de selección:
    // En una implementación real, esto usaría un clasificador de texto o reglas de negocio

    // Clasificación basada en palabras clave con priorización
    if (instruction.find("Assess") != std::string::npos || instruction.find("Buffer") != std::string::npos) {
        return "apex"; // Especialista en explotación
    }
    if (instruction.find("Scan") != std::string::npos || instruction.find("Recon") != std::string::npos || instruction.find("Network") != std::string::npos) {
        return "vantage"; // Especialista en reconocimiento y escaneo
    }
    if (instruction.find("Report") != std::string::npos || instruction.find("Analyze") != std::string::npos) {
        return "atlas"; // Especialista en análisis y reportes
    }
    if (instruction.find("Cloud") != std::string::npos || instruction.find("AWS") != std::string::npos) {
        return "skyline"; // Especialista en nube
    }

    return priority > 2 ? "apex" : "rex"; // Coordinator es el trabajador general, Apex toma tareas críticas
}

void TicketDispatcher::auditTicket(const std::string& ticket_id) {
    std::cout << "[TicketDispatcher] Auditing results for Ticket: " << ticket_id << std::endl;

    // Scan Blackboard for findings or messages related to this ticket
    auto& bb = Blackboard::getInstance();
    auto findings = bb.getAllFindings();

    bool completed = false;
    for (const auto& f : findings) {
        if (f.metadata.count("ticket_id") && f.metadata["ticket_id"] == ticket_id) {
            std::cout << "[TicketDispatcher] Found linked finding: " << f.title << " (Severity: " << f.severity << ")" << std::endl;
            completed = true;
        }
    }

    if (completed) {
        auto ticket = bb.getTicket(ticket_id);
        ticket.status = "COMPLETED";
        bb.upsertTicket(ticket);
        std::cout << "[TicketDispatcher] Ticket " << ticket_id << " marked as COMPLETED." << std::endl;
    }
}

} // namespace interlock
