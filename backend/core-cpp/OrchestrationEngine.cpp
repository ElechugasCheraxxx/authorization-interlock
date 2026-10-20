#include "OrchestrationEngine.hpp"
#include "InferenceEngine.hpp"
#include "Blackboard.hpp"
#include "EventSystem.hpp"
#include "MessageRouter.hpp"
#include "GPUAllocator.hpp"
#include <iostream>

namespace interlock {

OrchestrationEngine::OrchestrationEngine() {
    // Inicializar MoERouter para 11 agentes totales, activando los 2 mejores (N_r)
    // y manteniendo 2 expertos compartidos (K_s) fijos para lógica general.
    swarm_router = std::make_unique<MoERouter>(11, 2, 2);
}

OrchestrationEngine::~OrchestrationEngine() {
    stop();
}

void OrchestrationEngine::start() {
    if (running) return;
    running = true;

    // Register this engine as an interceptor in the MessageRouter
    MessageRouter::getInstance().setInterceptor([this](const Message& msg) {
        this->onMessageRouted(msg);
    });

    monitor_thread = std::thread(&OrchestrationEngine::monitorLoop, this);
    std::cout << "[OrchestrationEngine] Advanced Agent Lifecycle Manager Started." << std::endl;
}

void OrchestrationEngine::stop() {
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (!running) return;
        running = false;
    }
    cv.notify_all();

    // Unregister interceptor
    MessageRouter::getInstance().setInterceptor(nullptr);
    if (monitor_thread.joinable()) {
        monitor_thread.join();
    }

    // Hibernate all agents to release VRAM and llama resources
    std::lock_guard<std::mutex> lock(mtx);
    for (auto& [alias, ctx] : agents) {
        performHibernate(ctx);
    }
}

void OrchestrationEngine::setModelsPath(const std::string& path) {
    std::lock_guard<std::mutex> lock(mtx);
    models_base_path = path;
}

void OrchestrationEngine::registerAgent(const std::string& alias, const std::string& model_id, const std::string& model_path) {
    std::lock_guard<std::mutex> lock(mtx);
    AgentRuntimeContext ctx;
    ctx.alias = alias;
    ctx.model_id = model_id;
    ctx.model_path = models_base_path + model_path;
    ctx.state = AgentLifecycleState::HIBERNATED;
    agents[alias] = ctx;

    // Register in Rust Maestro for lock-free state management
    rust_orch_register_agent(alias.c_str());

    // Update Blackboard
    AgentState state;
    state.alias = alias;
    state.status = "HIBERNATED";
    state.current_model = model_id;
    Blackboard::getInstance().updateAgentState(state);

    std::cout << "[OrchestrationEngine] Registered Agent: " << alias << " (Managed by Rust Maestro)" << std::endl;
}

void OrchestrationEngine::registerNativeAgent(std::shared_ptr<NativeAgent> agent, const std::string& model_id, const std::string& model_path) {
    {
        std::lock_guard<std::mutex> lock(mtx);
        native_agents[model_id] = agent; // Key by model_id or alias
    }
    registerAgent(model_id, model_id, model_path);

    // Register handler in MessageRouter to trigger the native logic
    MessageRouter::getInstance().registerHandler(model_id, [this, model_id](const Message& msg) {
        std::shared_ptr<NativeAgent> target_agent;
        {
            std::lock_guard<std::mutex> lock(mtx);
            target_agent = native_agents[model_id];
        }
        if (target_agent) {
            target_agent->processTask(msg);
        }
    });
}

void OrchestrationEngine::onMessageRouted(const Message& msg) {
    // Si el mensaje es para "swarm" o "auto", usamos el MoERouter
    if (msg.to == "swarm" || msg.to == "auto") {
        std::cout << "[OrchestrationEngine] Routing swarm message via MoE..." << std::endl;

        // Simular un embedding de la tarea (en el futuro vendría del MythosTokenizer/Embedder)
        std::vector<float> task_embedding(128, 0.5f);
        std::vector<std::string> agent_pool;
        {
            std::lock_guard<std::mutex> lock(mtx);
            for (const auto& [alias, _] : agents) agent_pool.push_back(alias);
        }

        auto decision = swarm_router->route(task_embedding, agent_pool);

        // Wake Shared Experts (K_s)
        for (const auto& agent : decision.shared_experts) {
            std::cout << "[OrchestrationEngine] MoE Shared Expert (K_s) Active: " << agent << std::endl;
            wakeAgent(agent);
        }

        // Wake Routed Experts (N_r)
        for (const auto& [agent, weight] : decision.selected_experts) {
            std::cout << "[OrchestrationEngine] MoE Routed Expert (N_r) Selected: " << agent << " (Weight: " << weight << ")" << std::endl;
            wakeAgent(agent);
        }
    } else if (msg.to != "all" && msg.to != "broadcast") {
        wakeAgent(msg.to);
    }
}

bool OrchestrationEngine::wakeAgent(const std::string& alias) {
    AgentRuntimeContext* ctx = nullptr;
    {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = agents.find(alias);
        if (it == agents.end()) return false;
        ctx = &it->second;

        if (ctx->state == AgentLifecycleState::ACTIVE || ctx->state == AgentLifecycleState::WAKING_UP) {
            ctx->last_activity_micros = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();

            rust_orch_update_activity(alias.c_str());
            return true;
        }
        ctx->state = AgentLifecycleState::WAKING_UP;
        rust_orch_set_state(alias.c_str(), AgentLifecycleState::WAKING_UP);
    }

    std::cout << "[OrchestrationEngine] Waking up agent: " << alias << "..." << std::endl;

    bool success = false;

    // Si es un agente nativo, usamos su motor independiente
    std::shared_ptr<NativeAgent> native_target;
    {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = native_agents.find(alias);
        if (it != native_agents.end()) native_target = it->second;
    }

    if (native_target) {
        success = native_target->wake();
    } else {
        // Fallback al motor global para agentes legacy/estándar
        success = InferenceEngine::getInstance().loadModel(ctx->model_id, ctx->model_path);
    }

    {
        std::lock_guard<std::mutex> lock(mtx);
        if (success) {
            ctx->state = AgentLifecycleState::ACTIVE;
            ctx->last_activity_micros = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();

            rust_orch_set_state(alias.c_str(), AgentLifecycleState::ACTIVE);
            rust_orch_update_activity(alias.c_str());

            // Update Blackboard status
            auto state = Blackboard::getInstance().getAgentState(alias);
            state.status = "ACTIVE";
            Blackboard::getInstance().updateAgentState(state);

            std::cout << "[OrchestrationEngine] Agent " << alias << " is now ACTIVE." << std::endl;
        } else {
            ctx->state = AgentLifecycleState::HIBERNATED;
            rust_orch_set_state(alias.c_str(), AgentLifecycleState::HIBERNATED);
            std::cerr << "[OrchestrationEngine] FAILED to wake agent: " << alias << std::endl;
        }
    }

    return success;
}

void OrchestrationEngine::hibernateAgent(const std::string& alias) {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = agents.find(alias);
    if (it != agents.end()) {
        performHibernate(it->second);
    }
}

void OrchestrationEngine::performHibernate(AgentRuntimeContext& ctx) {
    if (ctx.state == AgentLifecycleState::HIBERNATED) return;

    std::cout << "[OrchestrationEngine] Hibernating agent: " << ctx.alias << " (Releasing VRAM)" << std::endl;

    // Si es nativo, liberamos su motor privado
    auto it_native = native_agents.find(ctx.alias);
    if (it_native != native_agents.end()) {
        it_native->second->hibernate();
    } else {
        InferenceEngine::getInstance().unloadModel(ctx.model_id);
    }

    ctx.state = AgentLifecycleState::HIBERNATED;
    rust_orch_set_state(ctx.alias.c_str(), AgentLifecycleState::HIBERNATED);

    // Update Blackboard
    auto state = Blackboard::getInstance().getAgentState(ctx.alias);
    state.status = "HIBERNATED";
    Blackboard::getInstance().updateAgentState(state);
}

void OrchestrationEngine::monitorLoop() {
    while (running) {
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait_for(lock, std::chrono::seconds(5), [this] { return !running; });
        }
        if (!running) break;

        // 1. Verificar presión de GPU para desalojo de emergencia
        if (GPUAllocator::getInstance().isUnderHighPressure()) {
            std::cout << "[OrchestrationEngine] HIGH GPU PRESSURE: Aggressive Hibernation mode." << std::endl;

            // Consultar agentes activos a Rust
            auto active_res = rust_orch_get_active_agents();
            if (active_res.data && active_res.length > 0) {
                std::string active_str(active_res.data, active_res.length);
                rust_free_surgery_result(active_res);

                std::lock_guard<std::mutex> lock(mtx);
                size_t pos = 0;
                while (pos < active_str.size()) {
                    size_t next_pipe = active_str.find('|', pos);
                    std::string alias = active_str.substr(pos, next_pipe - pos);
                    if (agents.count(alias)) {
                        performHibernate(agents[alias]);
                    }
                    if (next_pipe == std::string::npos) break;
                    pos = next_pipe + 1;
                }
            }
            continue; // Evitar el monitor idle si acabamos de limpiar por presión
        }

        // 2. Monitor de Idle estándar vía Rust Maestro
        auto idle_res = rust_orch_get_idle_agents(IDLE_TIMEOUT_SECONDS);
        if (idle_res.data && idle_res.length > 0) {
            std::string idle_str(idle_res.data, idle_res.length);
            rust_free_surgery_result(idle_res);

            std::lock_guard<std::mutex> lock(mtx);
            size_t pos = 0;
            while (pos < idle_str.size()) {
                size_t next_pipe = idle_str.find('|', pos);
                std::string alias = idle_str.substr(pos, next_pipe - pos);
                if (agents.count(alias)) {
                    performHibernate(agents[alias]);
                }
                if (next_pipe == std::string::npos) break;
                pos = next_pipe + 1;
            }
        }

        // 3. GTA-style "Least Used Agent" eviction under memory pressure
        auto metrics = GPUAllocator::getInstance().getMetrics();
        if (metrics.vram_free_gb < 5.0f) {
            std::lock_guard<std::mutex> lock(mtx);
            std::string oldest_alias = "";
            long long oldest_time = 0;

            for (auto& [alias, ctx] : agents) {
                if (ctx.state == AgentLifecycleState::ACTIVE) {
                    if (oldest_alias == "" || ctx.last_activity_micros < oldest_time) {
                        oldest_time = ctx.last_activity_micros;
                        oldest_alias = alias;
                    }
                }
            }

            if (oldest_alias != "") {
                std::cout << "[OrchestrationEngine] Memory pressure detected. Evicting least used agent (GTA-style): " << oldest_alias << std::endl;
                performHibernate(agents[oldest_alias]);
            }
        }
    }
}

AgentLifecycleState OrchestrationEngine::getAgentState(const std::string& alias) {
    return rust_orch_get_state(alias.c_str());
}

std::vector<std::string> OrchestrationEngine::getActiveAgents() {
    std::vector<std::string> active;
    auto active_res = rust_orch_get_active_agents();
    if (active_res.data && active_res.length > 0) {
        std::string active_str(active_res.data, active_res.length);
        rust_free_surgery_result(active_res);

        size_t pos = 0;
        while (pos < active_str.size()) {
            size_t next_pipe = active_str.find('|', pos);
            active.push_back(active_str.substr(pos, next_pipe - pos));
            if (next_pipe == std::string::npos) break;
            pos = next_pipe + 1;
        }
    }
    return active;
}

} // namespace interlock
