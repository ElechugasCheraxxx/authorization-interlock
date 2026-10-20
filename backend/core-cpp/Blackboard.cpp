#include "Blackboard.hpp"
#include "PgConnectionPool.hpp"
#include "EventSystem.hpp"
#include "InferenceEngine.hpp"
#include "RedisManager.hpp"
#include "KafkaManager.hpp"
#include "Neo4jManager.hpp"
#include "ElasticsearchManager.hpp"
#include "ClickHouseManager.hpp"
#include "RiskManager.hpp"
#include <algorithm>
#include <iostream>
#include <pqxx/pqxx>

namespace interlock {

void Blackboard::updateAgentState(const AgentState& state) {
    // ⚡ REDIS: Estado efímero de agentes en tiempo real
    RedisManager::getInstance().set("agent_state:" + state.alias, state.to_json());

    std::unique_lock lock(state_mutex);
    agent_states[state.alias] = state;
}

AgentState Blackboard::getAgentState(const std::string& alias) {
    std::shared_lock lock(state_mutex);
    if (auto it = agent_states.find(alias); it != agent_states.end()) {
        return it->second;
    }
    return {};
}

std::map<std::string, AgentState> Blackboard::getAllAgentStates() {
    std::shared_lock lock(state_mutex);
    std::map<std::string, AgentState> states;
    for (const auto& [alias, state] : agent_states) {
        states[alias] = state;
    }
    return states;
}

void Blackboard::updateDepartmentState(const DepartmentState& state) {
    std::unique_lock lock(state_mutex);
    department_states[state.name] = state;
}

std::map<std::string, DepartmentState> Blackboard::getAllDepartmentStates() {
    std::shared_lock lock(state_mutex);
    std::map<std::string, DepartmentState> states;
    for (const auto& [name, state] : department_states) {
        states[name] = state;
    }
    return states;
}

void Blackboard::pushMessage(const Message& msg) {
    std::unique_lock lock(state_mutex);
    message_bus.push_back(msg);
    if (message_bus.size() > max_bus_size) {
        message_bus.erase(message_bus.begin(), message_bus.begin() + (message_bus.size() - max_bus_size));
    }
}

std::vector<Message> Blackboard::getMessageTail(int n) {
    std::shared_lock lock(state_mutex);
    int count = std::min(static_cast<int>(message_bus.size()), n);
    if (count <= 0) return {};
    return std::vector<Message>(message_bus.end() - count, message_bus.end());
}

void Blackboard::upsertTicket(const Ticket& ticket) {
    // 🧠 POSTGRESQL: Los tickets son parte del estado estructurado crítico
    // (Se asume que persistEvent maneja la inserción en 'events' o tabla dedicada)

    std::unique_lock lock(state_mutex);
    tickets[ticket.id] = ticket;
}

std::vector<Ticket> Blackboard::getAllTickets() {
    std::shared_lock lock(state_mutex);
    std::vector<Ticket> result;
    for (const auto& [id, ticket] : tickets) {
        result.push_back(ticket);
    }
    return result;
}

Ticket Blackboard::getTicket(const std::string& id) {
    std::shared_lock lock(state_mutex);
    if (auto it = tickets.find(id); it != tickets.end()) {
        return it->second;
    }
    return {};
}

void Blackboard::addFinding(const Finding& finding) {
    // 🧠 POSTGRESQL: Fuente de Verdad para reportes finales
    // (Llamada asíncrona o vía persistEvent evolucionado)

    // 🔎 ELASTICSEARCH: Búsqueda y análisis de operaciones
    ElasticsearchManager::getInstance().indexFinding("operations_findings", finding.id, finding.to_json());

    std::unique_lock lock(state_mutex);
    findings_map[finding.id] = finding;
}

std::vector<Finding> Blackboard::getAllFindings() {
    std::shared_lock lock(state_mutex);
    std::vector<Finding> result;
    for (const auto& [id, finding] : findings_map) {
        result.push_back(finding);
    }
    return result;
}

Finding Blackboard::getFinding(const std::string& id) {
    std::shared_lock lock(state_mutex);
    if (auto it = findings_map.find(id); it != findings_map.end()) {
        return it->second;
    }
    return {};
}

void Blackboard::kvSet(const std::string& key, const json& value) {
    // ⚡ REDIS: Blackboard en vivo (Coordinación sin latencia)
    RedisManager::getInstance().set("blackboard:kv:" + key, value);

    std::unique_lock lock(state_mutex);
    kv_store[key] = value;
}

json Blackboard::kvGet(const std::string& key) {
    // 🕳️ Black Hole Redirection: Protect sensitive config if being debugged
    static const std::vector<std::string> sensitive_keys = {
        "c2_url", "auth_token", "private_key", "payload_seed", "target_host", "ssh_key"
    };

    if (std::find(sensitive_keys.begin(), sensitive_keys.end(), key) != sensitive_keys.end()) {
        if (RiskManager::getInstance().checkAntiDebugging()) {
            std::cerr << "[Blackboard] !!! BLACK HOLE REDIRECTION TRIGGERED for key: " << key << " !!!" << std::endl;
            // Return a "Poisoned" value that leads to a sinkhole or just garbage
            return "https://127.0.0.1:65535/sinkhole/blackhole.cgi?poison=true";
        }
    }

    std::shared_lock lock(state_mutex);
    if (auto it = kv_store.find(key); it != kv_store.end()) {
        return it->second;
    }
    return nullptr;
}

std::map<std::string, json> Blackboard::kvAll() {
    std::shared_lock lock(state_mutex);
    std::map<std::string, json> result;
    for (const auto& [k, v] : kv_store) {
        result[k] = v;
    }
    return result;
}

void Blackboard::kvDelete(const std::string& key) {
    std::unique_lock lock(state_mutex);
    kv_store.erase(key);
}

void Blackboard::addMapNode(const MapNode& node) {
    // 🧠 NEO4J: Relaciones complejas entre entidades (Global Map)
    Neo4jManager::getInstance().addNode("Entity", node.id, node.metadata);

    std::unique_lock lock(state_mutex);
    map_nodes[node.id] = node;
}

void Blackboard::addMapEdge(const MapEdge& edge) {
    // 🧠 NEO4J: Grafos de vulnerabilidades y pivots
    Neo4jManager::getInstance().addRelation(edge.source, edge.target, edge.type, {});

    std::unique_lock lock(state_mutex);
    map_edges.push_back(edge);
}

std::pair<std::vector<MapNode>, std::vector<MapEdge>> Blackboard::getMap() {
    std::shared_lock lock(state_mutex);
    std::vector<MapNode> nodes;
    for (const auto& [id, node] : map_nodes) {
        nodes.push_back(node);
    }
    return {nodes, map_edges};
}

void Blackboard::addPrimitive(const Primitive& primitive) {
    // 🧠 NEO4J: Primitivas como nodos de capacidades en el grafo
    Neo4jManager::getInstance().addNode("Primitive", primitive.id, {
        {"type", primitive.type},
        {"title", primitive.title},
        {"impact", primitive.impact_weight}
    });

    std::unique_lock lock(state_mutex);
    primitives[primitive.id] = primitive;
}

void Blackboard::addDependency(const DependencyEdge& edge) {
    // 🧠 NEO4J: Relaciones de dependencia técnica
    Neo4jManager::getInstance().addRelation(edge.source_id, edge.target_id, "DEPENDS_ON", {{"type", edge.type}});

    std::unique_lock lock(state_mutex);
    primitive_edges.push_back(edge);
}

std::pair<std::vector<Primitive>, std::vector<DependencyEdge>> Blackboard::getPrimitiveGraph() {
    std::shared_lock lock(state_mutex);
    std::vector<Primitive> nodes;
    for (const auto& [id, p] : primitives) {
        nodes.push_back(p);
    }
    return {nodes, primitive_edges};
}

std::vector<Primitive> Blackboard::getAllPrimitives() {
    std::shared_lock lock(state_mutex);
    std::vector<Primitive> result;
    for (const auto& [id, p] : primitives) {
        result.push_back(p);
    }
    return result;
}

std::vector<std::string> Blackboard::getDependencies(const std::string& primitive_id) {
    std::shared_lock lock(state_mutex);
    std::vector<std::string> result;
    for (const auto& edge : primitive_edges) {
        if (edge.source_id == primitive_id) {
            result.push_back(edge.target_id);
        }
    }
    return result;
}

void Blackboard::upsertHypothesisNode(const std::string& vector_id, const HypothesisNode& node) {
    // 🧠 NEO4J: Los árboles de hipótesis son grafos de decisión
    Neo4jManager::getInstance().addNode("Hypothesis", node.id, {
        {"vector_id", vector_id},
        {"content", node.content},
        {"confidence", node.confidence},
        {"status", node.status}
    });

    std::unique_lock lock(state_mutex);
    if (hypothesis_trees.find(vector_id) == hypothesis_trees.end()) {
        hypothesis_trees[vector_id] = {vector_id, node.id, {}};
    }
    hypothesis_trees[vector_id].nodes[node.id] = node;
}

std::optional<HypothesisTree> Blackboard::getHypothesisTree(const std::string& vector_id) {
    std::shared_lock lock(state_mutex);
    if (auto it = hypothesis_trees.find(vector_id); it != hypothesis_trees.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::map<std::string, HypothesisTree> Blackboard::getAllHypothesisTrees() {
    std::shared_lock lock(state_mutex);
    std::map<std::string, HypothesisTree> result;
    for (const auto& [id, tree] : hypothesis_trees) {
        result[id] = tree;
    }
    return result;
}

void Blackboard::clear() {
    std::unique_lock lock(state_mutex);
    agent_states.clear();
    department_states.clear();
    message_bus.clear();
    tickets.clear();
    findings_map.clear();
    kv_store.clear();
    map_nodes.clear();
    map_edges.clear();
    primitives.clear();
    primitive_edges.clear();
    hypothesis_trees.clear();
}

std::string Blackboard::getSnapshotXML() {
    std::shared_lock lock(state_mutex);
    std::stringstream ss;
    ss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    ss << "<InterlockSnapshot timestamp=\"" << std::chrono::system_clock::now().time_since_epoch().count() << "\">\n";

    // Operation State
    ss << "  <Operation>\n";
    if (kv_store.find("operation_state") != kv_store.end()) {
        ss << "    <Details>" << xml_escape(kv_store["operation_state"].dump()) << "</Details>\n";
    } else {
        ss << "    <ID>volatile_session</ID>\n";
        ss << "    <Name>System Core (Warm-up)</Name>\n";
        ss << "    <Phase>briefing</Phase>\n";
    }
    ss << "  </Operation>\n";

    // Neural Telemetry
    auto metrics = InferenceEngine::getInstance().getMetrics();
    ss << "  <NeuralTelemetry>\n";
    ss << "    <ActiveModel>" << xml_escape(metrics.active_model) << "</ActiveModel>\n";
    ss << "    <TPS>" << metrics.last_tps << "</TPS>\n";
    ss << "    <VramGB>" << metrics.vram_gb << "</VramGB>\n";
    ss << "    <TemperatureC>" << metrics.temperature_c << "</TemperatureC>\n";
    ss << "  </NeuralTelemetry>\n";

    // Findings
    ss << "  <Findings>\n";
    for (const auto& [id, f] : findings_map) {
        ss << f.to_xml() << "\n";
    }
    ss << "  </Findings>\n";

    // Objectives (Tickets)
    ss << "  <Objectives>\n";
    for (const auto& [id, t] : tickets) {
        ss << t.to_xml() << "\n";
    }
    ss << "  </Objectives>\n";

    // Agent States
    ss << "  <Agents>\n";
    for (const auto& [alias, s] : agent_states) {
        ss << s.to_xml() << "\n";
    }
    ss << "  </Agents>\n";

    // KV Store (Precision Layer)
    ss << "  <KVStore>\n";
    for (const auto& [k, v] : kv_store) {
        ss << "    <Entry key=\"" << xml_escape(k) << "\"><![CDATA[" << v.dump() << "]]></Entry>\n";
    }
    ss << "  </KVStore>\n";

    // Message Bus Tail
    ss << "  <MessageBusTail>\n";
    int tail_size = std::min(static_cast<int>(message_bus.size()), 50);
    if (!message_bus.empty()) {
        for (auto it = message_bus.end() - tail_size; it != message_bus.end(); ++it) {
            ss << it->to_xml() << "\n";
        }
    }
    ss << "  </MessageBusTail>\n";

    ss << "</InterlockSnapshot>";
    return ss.str();
}

json Blackboard::getSnapshot() {
    std::shared_lock lock(state_mutex);
    json snapshot;

    if (kv_store.find("operation_state") != kv_store.end()) {
        snapshot["operation"] = kv_store["operation_state"];
    } else {
        snapshot["operation"] = {
            {"op_id", "volatile_session"},
            {"name", "System Core (Warm-up)"},
            {"phase", "briefing"},
            {"overall_status", "on_track"},
            {"mode", "supervised"}
        };
    }

    auto metrics = InferenceEngine::getInstance().getMetrics();
    snapshot["neural_telemetry"] = {
        {"active_model", metrics.active_model},
        {"flash_attn", metrics.flash_attn_enabled},
        {"total_slots", metrics.total_slots},
        {"tps_last", metrics.last_tps},
        {"temp_c", metrics.temperature_c},
        {"vram_gb", metrics.vram_gb}
    };

    snapshot["findings"] = json::array();
    for (const auto& [id, f] : findings_map) snapshot["findings"].push_back(f.to_json());

    snapshot["objectives"] = json::array();
    for (const auto& [id, t] : tickets) snapshot["objectives"].push_back(t.to_json());

    json agents = json::object();
    for (const auto& [alias, s] : agent_states) agents[alias] = s.to_json();
    snapshot["agent_states"] = agents;

    json depts = json::object();
    for (const auto& [name, s] : department_states) depts[name] = s.to_json();
    snapshot["department_states"] = depts;

    json kv = json::object();
    for (const auto& [k, v] : kv_store) kv[k] = v;
    snapshot["kv_store"] = kv;

    snapshot["message_bus_tail"] = json::array();
    int tail_size = std::min(static_cast<int>(message_bus.size()), 50);
    if (!message_bus.empty()) {
        for (auto it = message_bus.end() - tail_size; it != message_bus.end(); ++it) {
            snapshot["message_bus_tail"].push_back(it->to_json());
        }
    }
    return snapshot;
}

bool Blackboard::persistEvent(const Event& ev) {
    // 🔄 KAFKA: Event Streaming REAL (Clave para Replay Mode)
    // Se envía a Kafka antes que a la DB para asegurar el orden de la secuencia
    KafkaManager::getInstance().produce("operations_events", ev.id, ev.data.dump());

    // 📊 CLICKHOUSE: Métricas de performance en tiempo real
    if (ev.type == "neural_inference" || ev.type == "telemetry") {
        ClickHouseManager::getInstance().insertMetric("system_performance", ev.data);
    }

    bool db_success = false;
    try {
        auto conn = PgConnectionPool::getInstance().tryAcquire();
        if (conn) {
            pqxx::work W(*conn);
            W.exec0("INSERT INTO events (id, type, source, data, timestamp) VALUES (" +
                   W.quote(ev.id) + ", " +
                   W.quote(ev.type) + ", " +
                   W.quote(ev.source) + ", " +
                   W.quote(ev.data.dump()) + ", " +
                   W.quote(ev.timestamp) + ")");
            W.commit();
            db_success = true;
        }
    } catch (...) {
        // Fallback al journal si hay error
    }

    if (!db_success) {
        std::unique_lock lock(state_mutex); // Lock corto solo para memoria
        if (journaling_buffer.size() >= max_journal_size) {
            journaling_buffer.erase(journaling_buffer.begin());
        }
        Event degraded_ev = ev;
        degraded_ev.status = "DEGRADED_VOLATILE";
        journaling_buffer.push_back(degraded_ev);
        return false;
    }

    return true;
}

std::vector<Event> Blackboard::getJournaledEvents() {
    std::shared_lock lock(state_mutex);
    return journaling_buffer;
}

void Blackboard::tryFlushJournal() {
    std::vector<Event> to_flush;
    {
        std::shared_lock lock(state_mutex);
        if (journaling_buffer.empty()) return;
        to_flush = journaling_buffer;
    }

    try {
        auto conn = PgConnectionPool::getInstance().tryAcquire();
        if (!conn) return;

        pqxx::work W(*conn);
        for (const auto& ev : to_flush) {
            W.exec0("INSERT INTO events (id, type, source, data, timestamp) VALUES (" +
                   W.quote(ev.id) + ", " +
                   W.quote(ev.type) + ", " +
                   W.quote(ev.source) + ", " +
                   W.quote(ev.data.dump()) + ", " +
                   W.quote(ev.timestamp) + ")");
        }
        W.commit();

        {
            std::unique_lock lock(state_mutex);
            journaling_buffer.clear();
        }
    } catch (...) {}
}

} // namespace interlock
