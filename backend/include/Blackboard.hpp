#pragma once
#include "Schema.hpp"
#include "EventSystem.hpp"
#include <shared_mutex>
#include <unordered_map>
#include <vector>
#include <map>

namespace interlock {

class Blackboard {
public:
    static Blackboard& getInstance() {
        static Blackboard instance;
        return instance;
    }

    // Agent States
    void updateAgentState(const AgentState& state);
    AgentState getAgentState(const std::string& alias);
    std::map<std::string, AgentState> getAllAgentStates();

    // Department States
    void updateDepartmentState(const DepartmentState& state);
    std::map<std::string, DepartmentState> getAllDepartmentStates();

    // Messages
    void pushMessage(const Message& msg);
    std::vector<Message> getMessageTail(int n);

    // Tickets (Objectives)
    void upsertTicket(const Ticket& ticket);
    std::vector<Ticket> getAllTickets();
    Ticket getTicket(const std::string& id);

    // Findings
    void addFinding(const Finding& finding);
    std::vector<Finding> getAllFindings();
    Finding getFinding(const std::string& id);

    // KV Store
    void kvSet(const std::string& key, const json& value);
    json kvGet(const std::string& key);
    std::map<std::string, json> kvAll();
    void kvDelete(const std::string& key);

    // Infrastructure Map
    void addMapNode(const MapNode& node);
    void addMapEdge(const MapEdge& edge);
    std::pair<std::vector<MapNode>, std::vector<MapEdge>> getMap();

    // PDG (Primitive Dependency Graph)
    void addPrimitive(const Primitive& primitive);
    void addDependency(const DependencyEdge& edge);
    std::pair<std::vector<Primitive>, std::vector<DependencyEdge>> getPrimitiveGraph();
    std::vector<Primitive> getAllPrimitives();
    std::vector<std::string> getDependencies(const std::string& primitive_id);

    // Hypothesis Trees
    void upsertHypothesisNode(const std::string& vector_id, const HypothesisNode& node);
    std::optional<HypothesisTree> getHypothesisTree(const std::string& vector_id);
    std::map<std::string, HypothesisTree> getAllHypothesisTrees();

    // Maintenance
    void clear();

    // Snapshot
    json getSnapshot();
    std::string getSnapshotXML();

    // Event Persistence & Journaling (Fix #1)
    bool persistEvent(const Event& ev);
    std::vector<Event> getJournaledEvents();
    void tryFlushJournal();

private:
    Blackboard() = default;

    // Journaling Buffer (Lock-free MPMC ring buffer simplified for v7.5)
    std::vector<Event> journaling_buffer;
    size_t max_journal_size = 5000;

    std::unordered_map<std::string, AgentState> agent_states;
    std::unordered_map<std::string, DepartmentState> department_states;
    std::vector<Message> message_bus;
    std::unordered_map<std::string, Ticket> tickets;
    std::unordered_map<std::string, Finding> findings_map;
    std::unordered_map<std::string, json> kv_store;

    std::unordered_map<std::string, MapNode> map_nodes;
    std::vector<MapEdge> map_edges;

    std::unordered_map<std::string, Primitive> primitives;
    std::vector<DependencyEdge> primitive_edges;

    std::unordered_map<std::string, HypothesisTree> hypothesis_trees;

    mutable std::shared_mutex state_mutex;
    size_t max_bus_size = 1000;
};

} // namespace interlock
