#pragma once

#include "Schema.hpp"
#include <vector>
#include <string>
#include <map>

namespace interlock {

struct AttackChain {
    std::vector<std::string> primitive_ids;
    float total_probability;
    float cumulative_impact;
};

class PDGEngine {
public:
    static PDGEngine& getInstance() {
        static PDGEngine instance;
        return instance;
    }

    // Analiza el grafo actual en el Blackboard para encontrar rutas de compromiso
    std::vector<AttackChain> findZeroErrorChains();

    // Evalúa si una nueva primitiva desbloquea rutas críticas
    void evaluatePrimitiveImpact(const Primitive& p);

    // Generates a real-time PDG from a compiled .chrxxx mission
    std::string generatePDGFromMission(const std::string& chrxSource);

private:
    PDGEngine() = default;

    // Algoritmos de grafos para encontrar caminos (Dijkstra/DFS optimizado)
    void traverseGraph(const std::string& start_node, AttackChain current_chain, std::vector<AttackChain>& results);
};

} // namespace interlock
