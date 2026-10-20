#pragma once

#include "Schema.hpp"
#include "NativeAgent.hpp"
#include "MoERouter.hpp"
#include "ArtifactEngine.hpp" // Para SurgeryResult
#include <string>
#include <mutex>
#include <map>
#include <memory>
#include <thread>
#include <atomic>

namespace interlock {

// Sincronizado con RustAgentState en Rust
enum class AgentLifecycleState : int {
    HIBERNATED = 0,
    WAKING_UP = 1,
    ACTIVE = 2,
    IDLE = 3,
    HIBERNATING = 4
};

// Declaraciones FFI de Rust Maestro
extern "C" {
    struct SurgeryResult {
        char* data;
        size_t length;
    };

    void rust_orch_register_agent(const char* alias);
    void rust_orch_update_activity(const char* alias);
    void rust_orch_set_state(const char* alias, AgentLifecycleState state);
    AgentLifecycleState rust_orch_get_state(const char* alias);
    SurgeryResult rust_orch_get_idle_agents(int64_t timeout_secs);
    SurgeryResult rust_orch_get_active_agents();
    void rust_free_surgery_result(SurgeryResult res);
}

struct AgentRuntimeContext {
    std::string alias;
    std::string model_id;
    std::string model_path;
    AgentLifecycleState state = AgentLifecycleState::HIBERNATED;
    long long last_activity_micros = 0;
    float reserved_vram_gb = 0.0f;
};

/**
 * OrchestrationEngine (M4 Pro Advanced Agent Manager)
 * Manages the "Hibernate-on-Idle" lifecycle for the Assessment agent swarm.
 */
class OrchestrationEngine {
public:
    static OrchestrationEngine& getInstance() {
        static OrchestrationEngine* instance = new OrchestrationEngine();
        return *instance;
    }

    // Lifecycle
    void start();
    void stop();

    // Agent Management
    void registerAgent(const std::string& alias, const std::string& model_id, const std::string& model_path);

    // Register a hard-coded native agent logic
    void registerNativeAgent(std::shared_ptr<NativeAgent> agent, const std::string& model_id, const std::string& model_path);

    /**
     * Interceptor method called by MessageRouter.
     * Decisions on waking agents are made here.
     */
    void onMessageRouted(const Message& msg);

    /**
     * Ensures an agent is ready to process a message.
     * If hibernated, triggers the "Wake Up" sequence (Hot Load into VRAM).
     */
    bool wakeAgent(const std::string& alias);

    /**
     * Manually puts an agent to sleep (Evict from VRAM).
     */
    void hibernateAgent(const std::string& alias);

    // Getters
    AgentLifecycleState getAgentState(const std::string& alias);
    std::vector<std::string> getActiveAgents();

    void setModelsPath(const std::string& path);

private:
    OrchestrationEngine();
    ~OrchestrationEngine();

    void monitorLoop();
    void performHibernate(AgentRuntimeContext& ctx);

    std::map<std::string, AgentRuntimeContext> agents;
    std::map<std::string, std::shared_ptr<NativeAgent>> native_agents;
    std::unique_ptr<MoERouter> swarm_router;
    std::mutex mtx;
    std::condition_variable cv;

    std::thread monitor_thread;
    std::atomic<bool> running{false};

    // Configuration
    const int IDLE_TIMEOUT_SECONDS = 45;
    std::string models_base_path = "/Users/Cheraxxx/Interlock IDE/data/models/";
};

} // namespace interlock
