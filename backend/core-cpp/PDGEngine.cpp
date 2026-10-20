#include "PDGEngine.hpp"
#include "MessageRouter.hpp"
#include "interop/FormalValidators.hpp"
#include <iostream>
#include <queue>
#include <set>
#include <dlfcn.h>

namespace interlock {

typedef char* (*CompileMissionFn)(const char*);
typedef char* (*GeneratePDGFn)(const char*);
typedef void (*FreeStringFn)(char*);

std::string PDGEngine::generatePDGFromMission(const std::string& chrxSource) {
    void* handle = dlopen("/Users/Cheraxxx/Interlock IDE/backend/chrx-compiler/target/release/libchrx_compiler.dylib", RTLD_LAZY);
    if (!handle) {
        std::cerr << "[PDGEngine] Error: Could not load libchrx_compiler.dylib: " << dlerror() << std::endl;
        return "{}";
    }

    auto generate_pdg = (GeneratePDGFn)dlsym(handle, "chrx_generate_pdg_json");
    auto free_string = (FreeStringFn)dlsym(handle, "chrx_free_string");

    if (!generate_pdg || !free_string) {
        std::cerr << "[PDGEngine] Error: Could not find FFI symbols in dylib" << std::endl;
        dlclose(handle);
        return "{}";
    }

    char* result = generate_pdg(chrxSource.c_str());
    std::string jsonResult = result ? result : "{}";

    if (result) {
        free_string(result);
    }

    dlclose(handle);
    std::cout << "[PDGEngine] Real-time PDG generated from .chrxxx mission." << std::endl;
    return jsonResult;
}

std::vector<AttackChain> PDGEngine::findZeroErrorChains() {
    std::cout << "[PDGEngine] Analyzing Graph for Zero-Error Assess Chains..." << std::endl;
    std::vector<AttackChain> chains;

    auto& bb = Blackboard::getInstance();
    auto primitives = bb.getAllPrimitives();

    for (const auto& p : primitives) {
        AttackChain chain;
        chain.total_probability = 1.0f;
        chain.cumulative_impact = 0.0f;
        traverseGraph(p.id, chain, chains);
    }

    // --- FORMAL VERIFICATION LAYER (Isabelle/Coq Extract) ---
    std::vector<AttackChain> verified_chains;
    for (const auto& chain : chains) {
        std::vector<formal::PrimitiveData> formal_chain;
        for (const auto& id : chain.primitive_ids) {
            // Mapping to formal data structure
            formal_chain.push_back({id, 0, false});
        }

        if (formal::PDGValidator::isAcyclic(formal_chain)) {
            verified_chains.push_back(chain);
        } else {
            std::cerr << "[PDGEngine] [FORMAL_ALARM] Dropping Invalid Cyclic Chain!" << std::endl;
        }
    }

    return verified_chains;
}

void PDGEngine::evaluatePrimitiveImpact(const Primitive& p) {
    std::cout << "[PDGEngine] Evaluating Impact of Primitive: " << p.id << " [" << p.type << "]" << std::endl;

    // Si la primitiva es de tipo 'Logic Flaw' con impacto > 0.8,
    // podría disparar una re-evaluación táctica en el Cipher (vía MessageRouter).
    if (p.impact_weight > 0.8f) {
        std::cout << "[PDGEngine] CRITICAL PRIMITIVE DETECTED: Triggering Operations Re-evaluation." << std::endl;

        Message alert;
        alert.from_agent = "pdg_engine";
        alert.to = "cipher";
        alert.type = "correlation";
        alert.priority = MessagePriority::HIGH;
        alert.content = "New critical primitive unlocked: " + p.title;

        MessageRouter::getInstance().route(alert);
    }
}

void PDGEngine::traverseGraph(const std::string& start_node, AttackChain current_chain, std::vector<AttackChain>& results) {
    // Algoritmo de búsqueda recursiva para encadenamiento de exploits
    if (std::find(current_chain.primitive_ids.begin(), current_chain.primitive_ids.end(), start_node) != current_chain.primitive_ids.end()) {
        return; // Evitar ciclos
    }

    current_chain.primitive_ids.push_back(start_node);

    // Obtener sucesores del grafo desde el Blackboard
    auto successors = Blackboard::getInstance().getDependencies(start_node);

    if (successors.empty()) {
        results.push_back(current_chain);
        return;
    }

    for (const auto& next : successors) {
        traverseGraph(next, current_chain, results);
    }
}

} // namespace interlock
