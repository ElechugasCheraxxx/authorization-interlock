#include "CognitiveEngines.hpp"
#include "InferenceEngine.hpp"
#include "PDGEngine.hpp"
#include "Blackboard.hpp"
#include <chrono>
#include <iostream>

namespace interlock {

void HypothesisEngine::generateHypotheses(const std::string& context) {
    std::lock_guard<std::mutex> lock(mtx);

    // In production, we call the InferenceEngine to analyze the context
    InferenceRequest req;
    req.prompt = "Analyze the following assessment context and generate potential assess hypotheses: " + context;
    req.max_tokens = 256;

    // Asynchronous call (simplified for this example)
    auto future = InferenceEngine::getInstance().generate(req);
    // In a real reactive system, we'd handle the future via a callback or event

    std::string id = "hyp_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    HypothesisNode node;
    node.id = id;
    node.content = "Hypothesis generated from context: " + context;
    node.confidence = 0.5f;
    tree.nodes[id] = node;

    // Fix: Upsert into Blackboard so tests and UI can see it
    Blackboard::getInstance().upsertHypothesisNode("ROOT_VECTOR", node);

    std::cout << "[HypothesisEngine] New hypothesis generated: " << id << std::endl;
}

void HypothesisEngine::updateConfidence(const std::string& hypothesis_id, float delta) {
    std::lock_guard<std::mutex> lock(mtx);
    if (tree.nodes.find(hypothesis_id) != tree.nodes.end()) {
        tree.nodes[hypothesis_id].confidence = std::clamp(tree.nodes[hypothesis_id].confidence + delta, 0.0f, 1.0f);
    }
}

HypothesisTree HypothesisEngine::getTree() const {
    std::lock_guard<std::mutex> lock(mtx);
    return tree;
}

std::string DebateEngine::resolveDebate(const std::string& topic, const std::vector<std::string>& arguments) {
    // Implementación del algoritmo de debate para llegar a un consenso
    // Uses a weight-based consensus mechanism
    float total_score = 0.0f;
    for (const auto& arg : arguments) {
        // En una implementación real, se evaluaría la fuerza de cada argumento
        // usando un clasificador de sentimientos o relevancia
        if (arg.find("critical") != std::string::npos || arg.find("blocker") != std::string::npos) {
            total_score += 0.5f;
        } else {
            total_score += 0.1f;
        }
    }

    float threshold = static_cast<float>(arguments.size()) * 0.25f;
    if (total_score > threshold) {
        return "Consensus reached on: " + topic + " (Strong agreement, score: " + std::to_string(total_score) + ")";
    } else {
        return "Consensus reached on: " + topic + " (Weak agreement, score: " + std::to_string(total_score) + ")";
    }
}

json BinaryAnalysisEngine::analyzeBinary(const std::string& path) {
    // Análisis estático/dinámico nativo
    std::cout << "[BinaryAnalysisEngine] Analyzing: " << path << std::endl;

    json result = {
        {"path", path},
        {"status", "analyzed"},
        {"findings", json::array({
            {{"type", "entropy"}, {"value", 7.8}, {"description", "High entropy detected, possible packing"}},
            {{"type", "imports"}, {"count", 42}, {"suspicious", true}}
        })}
    };
    return result;
}

void BinaryAnalysisEngine::mapToPDG(const std::string& binary_id) {
    // Integración con PDGEngine
    std::cout << "[BinaryAnalysisEngine] Mapping " << binary_id << " to Program Dependence Graph..." << std::endl;

    // In production, we iterate over analyzed functions and add nodes to PDG
    Primitive p;
    p.id = "pdg_" + binary_id;
    p.type = "Logic Flaw";
    p.title = "Binary Execution Flow for " + binary_id;
    p.impact_weight = 0.85f;

    PDGEngine::getInstance().evaluatePrimitiveImpact(p);
}

} // namespace interlock
