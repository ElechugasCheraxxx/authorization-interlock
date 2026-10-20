#pragma once

#include "Schema.hpp"
#include <vector>
#include <string>
#include <map>
#include <thread>
#include <atomic>

namespace interlock {

struct EvolutionTask {
    std::string agent_alias;
    std::string failure_reason;
    std::string raw_telemetry;
    int rdt_factor_increase;
};

class EvolutionEngine {
public:
    static EvolutionEngine& getInstance() {
        static EvolutionEngine instance;
        return instance;
    }

    // Bucles principales de evolución autónoma
    void startAutonomousSelfImprovement();

    // JIT Evolution: Ajuste de pesos en caliente ante bloqueos de EDR
    void triggerJITEvolution(const EvolutionTask& task);

    // Neural Dreaming: Generación de escenarios sintéticos en background
    void startDreamingLoop();

    // Shadow Training: Simulación de variaciones de cadenas de éxito
    void runShadowTraining(const std::string& success_chain_id);

    // Interfaz con los kernels de Metal para optimización de gradientes
    void runMLXFinetune(const std::string& dataset_path, const std::string& model_path, const std::string& adapter_path);

private:
    EvolutionEngine();
    ~EvolutionEngine();

    void backgroundSyntheticGenerator();
    void optimizeSteeringMasks();
    void applyWeightAdaptation(const std::string& agent, const std::vector<float>& gradients);

    std::atomic<bool> is_generating_synthetic;
    std::atomic<bool> running;
    std::thread improvement_thread;
    std::thread dreaming_thread;
};

} // namespace interlock
