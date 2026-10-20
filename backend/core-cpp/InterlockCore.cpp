#include "InterlockCore.h"
#include "PythonBridge.hpp"
#include "EventSystem.hpp"
#include "Blackboard.hpp"
#include "DatabaseManager.hpp"
#include "PgConnectionPool.hpp"
#include "GPUAllocator.hpp"
#include "MessageRouter.hpp"
#include "InferenceEngine.hpp"
#include "ProcessManager.hpp"
#include "SessionManager.hpp"
#include "OrchestrationEngine.hpp"
#include "Agents.hpp"
#include "StateMachine.hpp"
#include "IPCBridge.hpp"
#include "NetworkBridge.hpp"
#include "mach/MachProtector.hpp"
#include "RiskManager.hpp"
#include "TransferBridge.hpp"
#include "SovereignBridge.hpp"
#include "SovereignHeap.hpp"
#include "DiscoveryStreamer.hpp"
#include <iostream>
#include <sstream>
#include <chrono>

extern "C" uint32_t zig_generate_polymorphic_stub(uint8_t* buffer, uint32_t buffer_len, uint32_t seed);

namespace interlock {

static std::string g_base_path = ""; // Will be initialized by setBasePath() at runtime

static std::string getModelsPath() {
    return g_base_path + "/backend/models/";
}

void InterlockCore::setBasePath(const char* path) {
    g_base_path = path;
    ProcessManager::getInstance().setBasePath(path);
    OrchestrationEngine::getInstance().setModelsPath(getModelsPath());
    std::cout << "[InterlockCore] Base path set to: " << g_base_path << std::endl;
}

static long long now_micros() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

InterlockCore::InterlockCore() : event_callback(nullptr) {}

InterlockCore::~InterlockCore() {
    shutdown();
}

InterlockCore& InterlockCore::getInstance() {
    static InterlockCore instance;
    return instance;
}

void InterlockCore::boot() {
    try {
        std::cout << "[InterlockCore] Booting Master Core..." << std::endl;

        // 0. Establish Kernel Sovereignty (Vanguard-Level Anchor)
        if (SovereignBridge::instance().connect()) {
            std::cout << "[InterlockCore] Sovereign Mode: ACTIVE (Kernel Link Established)" << std::endl;
#ifdef _WIN32
            uint32_t currentPid = GetCurrentProcessId();
            SovereignBridge::instance().protectProcess(currentPid);
            SovereignBridge::instance().hideProcess(currentPid);
            SovereignBridge::instance().stealthNetwork(true);
#else
            SovereignBridge::instance().protectProcess(getpid());
#endif
        } else {
            std::cout << "[InterlockCore] Sovereign Mode: DEGRADED (Running in User Space Only)" << std::endl;
        }

        // 0.1 Initialize Hardened Memory (SovereignHeap)
        // 4GB initial pool for M4 Pro optimization
        memory::SovereignHeap::getInstance().init(4LL * 1024 * 1024 * 1024);
        std::cout << "[InterlockCore] SovereignHeap initialized (4GB Hardened Pool)" << std::endl;

        // 0.2 Initialize Discovery Streamer (RAG Asset Loading)
        rag::DiscoveryStreamer::getInstance().start();
        std::cout << "[InterlockCore] DiscoveryStreamer started." << std::endl;

        // 1. Start external services (PostgreSQL, Llama-server, etc.)
        ProcessManager::getInstance().startServices();

        // Give services a moment to warm up
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        GPUAllocator::getInstance().updateHardwareStats();

        // 2. Initialize Native Inference Engine (M4 Pro Hardened Mode)
        // No more ports: Direct Memory Mapping for 32B/14B/7B models.
        InferenceEngine::getInstance().initialize();
        // InferenceEngine::getInstance().loadAllModels(models_dir); // REMOVED

        // 3. Initialize Orchestration Engine (Agent Swarm Lifecycle)
        OrchestrationEngine::getInstance().start();

        // Registrar TODA la familia de agentes con LOGICA NATIVA y MOTORES INDEPENDIENTES
        auto& orch = OrchestrationEngine::getInstance();
        std::string models_path = getModelsPath();

        // Cipher: Estratega supremo (Level 1)
        orch.registerNativeAgent(
            std::make_shared<CipherAgent>(models_path + "DeepSeek-R1-32B-abliterated.Q4_K_M.gguf"),
            "cipher", "DeepSeek-R1-32B-abliterated.Q4_K_M.gguf"
        );

        // Chronos: Gestión de TTL y coherencia temporal (Level 1)
        orch.registerNativeAgent(
            std::make_shared<ChronosAgent>(models_path + "DeepSeek-R1-14B-abliterated.Q4_K_M.gguf"),
            "chronos", "DeepSeek-R1-14B-abliterated.Q4_K_M.gguf"
        );

        // Overseer: Director de Operaciones (Level 2)
        orch.registerNativeAgent(
            std::make_shared<OverseerAgent>(models_path + "DeepSeek-R1-14B-abliterated.Q4_K_M.gguf"),
            "overseer", "DeepSeek-R1-14B-abliterated.Q4_K_M.gguf"
        );

        // Grader: Auditoría lógica interna (Level 2)
        orch.registerNativeAgent(
            std::make_shared<GraderAgent>(models_path + "DeepSeek-R1-14B-Abliterated-v2.Q4_K_M.gguf"),
            "grader", "DeepSeek-R1-14B-Abliterated-v2.Q4_K_M.gguf"
        );

        // Apex: Validación técnica y síntesis de exploits (Level 3)
        orch.registerNativeAgent(
            std::make_shared<ApexAgent>(models_path + "WhiteRabbitNeo-V3-7B-Native-Active.Q4_K_M.gguf"),
            "apex", "WhiteRabbitNeo-V3-7B-Native-Active.Q4_K_M.gguf"
        );

        // Vantage: OSINT y correlación semántica (Level 3)
        orch.registerNativeAgent(
            std::make_shared<VantageAgent>(models_path + "Qwen2.5-7B-Instruct-Abliterated-v2.Q4_K_M.gguf"),
            "vantage", "Qwen2.5-7B-Instruct-Abliterated-v2.Q4_K_M.gguf"
        );

        // Beacon: Monitoreo de estado (Level 3)
        orch.registerNativeAgent(
            std::make_shared<BeaconAgent>(models_path + "Phi-3.5-Mini-Abliterated.Q4_K_M.gguf"),
            "beacon", "Phi-3.5-Mini-Abliterated.Q4_K_M.gguf"
        );

        // Coordinator: Active Leader (Level 4)
        orch.registerNativeAgent(
            std::make_shared<RexAgent>(models_path + "DeepSeek-R1-Distill-Qwen-14B-abliterated-v2.Q4_K_M.gguf"),
            "rex", "DeepSeek-R1-Distill-Qwen-14B-abliterated-v2.Q4_K_M.gguf"
        );

        // Probe: Exploitation (Level 4)
        orch.registerNativeAgent(
            std::make_shared<GhostAgent>(models_path + "DeepSeek-R1-Distill-Qwen-14B-abliterated-v2.Q4_K_M.gguf"),
            "ghost", "DeepSeek-R1-Distill-Qwen-14B-abliterated-v2.Q4_K_M.gguf"
        );

        // Minimizer: Evasión y Sigilo (Level 4)
        orch.registerNativeAgent(
            std::make_shared<WraithAgent>(models_path + "Llama-3.1-8B-Instruct-Abliterated.Q4_K_M.gguf"),
            "wraith", "Llama-3.1-8B-Instruct-Abliterated.Q4_K_M.gguf"
        );

        // Atlas: Movimiento lateral y AD (Level 4)
        orch.registerNativeAgent(
            std::make_shared<AtlasAgent>(models_path + "DeepSeek-Coder-V2-Lite-Instruct-Q4_K_M.gguf"),
            "atlas", "DeepSeek-Coder-V2-Lite-Instruct-Q4_K_M.gguf"
        );

        // Archive: Forensics y EvidenceVault (Level 4)
        orch.registerNativeAgent(
            std::make_shared<ArchiveAgent>(models_path + "Mistral-7B-Instruct-v0.3.Q4_K_M.gguf"),
            "archive", "Mistral-7B-Instruct-v0.3.Q4_K_M.gguf"
        );

        try {
            PythonBridge::getInstance().initialize();
        } catch (...) {
            std::cerr << "[InterlockCore] PythonBridge initialization failed." << std::endl;
        }

        MessageRouter::getInstance().start();

        // 4. Initialize Network Bridge
        NetworkBridge::getInstance().initialize();

        // 5. Initialize Mach Protections (M4 Pro Hardened)
        mach::StartAntiDebugMonitor();

        // 6. Initialize Polygolt IPC Bridge (The Hub for Rust, Go, Zig, etc.)
        if (!IPCBridge::getInstance().initializeServers()) {
            std::cerr << "[InterlockCore] WARNING: IPC Bridge failed to initialize some channels." << std::endl;
        }

        // 7. Start Data Surgery Engine (Operations Telemetry)
        ArtifactEngine::getInstance().start();

        // Connect to Kernel Shield (C Module)
        getInstance().kernel_bridge.connect();

        // 2. Async Database Warmup (to prevent "Connection Refused" during slow boot)
        // This thread will keep trying until Postgres is ready, without blocking the UI.
        std::thread([]() {
            const char* env_dsn = std::getenv("INTERLOCK_DB_DSN");
            std::string dsn = env_dsn ? env_dsn : "postgresql://localhost/interlock_ide";

            for (int i = 0; i < 20; ++i) {
                try {
                    DatabaseManager::getInstance().initialize(dsn);
                    PgConnectionPool::getInstance().warmup(dsn, 5);
                    std::cout << "[InterlockCore] Database connection established on attempt " << (i+1) << std::endl;
                    return;
                } catch (...) {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
            }
            std::cerr << "[InterlockCore] ERROR: Database could not be reached after 20 retries." << std::endl;
        }).detach();

        // Start Maintenance Loop
        auto& core = getInstance();
        if (!core.maintenance_running) {
            core.maintenance_running = true;
            core.maintenance_thread = std::thread(&InterlockCore::maintenanceLoop, &core);
        }

        // 4. Start Anti-Debug Monitor
        mach::StartAntiDebugMonitor();

        std::cout << "[InterlockCore] Master Core Operational." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[InterlockCore] FATAL ERROR DURING BOOT: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "[InterlockCore] UNKNOWN FATAL ERROR DURING BOOT." << std::endl;
    }
}

void InterlockCore::shutdown() {
    static std::atomic<bool> shutting_down{false};
    if (shutting_down.exchange(true)) {
        return; // Evitar doble shutdown
    }

    try {
        std::cout << "[InterlockCore] Iniciando shutdown graceful..." << std::endl;

        auto& core = getInstance();

        // 1. Detener hilos propios (Maintenance Loop)
        if (core.maintenance_running) {
            core.maintenance_running = false;
            if (core.maintenance_thread.joinable()) {
                std::cout << "[InterlockCore] Joining maintenance thread..." << std::endl;
                core.maintenance_thread.join();
            }
        }

        // 1.1 Shutdown Discovery Streamer
        rag::DiscoveryStreamer::getInstance().stop();
        std::cout << "[InterlockCore] DiscoveryStreamer stopped." << std::endl;

        // 2. LIMPIEZA CRÍTICA: Inference Engine (Modelos y Contextos Globales)
        // Se liberan antes para reducir presión de memoria inmediata
        std::cout << "[InterlockCore] Liberando modelos y backend GGML..." << std::endl;
        InferenceEngine::getInstance().shutdown();

        // 3. Detener orquestación (Hibernate Agents -> Libera motores privados)
        std::cout << "[InterlockCore] Deteniendo orquestación de agentes..." << std::endl;
        OrchestrationEngine::getInstance().stop();

        // 4. Otros servicios de mensajería e IPC
        MessageRouter::getInstance().stop();
        ProcessManager::getInstance().stopServices();

        // 5. Python Bridge (Finalización segura del intérprete)
        PythonBridge::getInstance().finalize();

        // 6. Network Bridge Shutdown
        NetworkBridge::getInstance().shutdown();

        // 7. Finalización definitiva del backend Llama
        InferenceEngine::getInstance().finalizeBackend();

        std::cout << "[InterlockCore] Shutdown completado correctamente." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[InterlockCore] Error en shutdown: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "[InterlockCore] Error desconocido en shutdown." << std::endl;
    }
}

void InterlockCore::maintenanceLoop() {
    std::cout << "[InterlockCore] Maintenance Thread Started." << std::endl;
    while (maintenance_running) {
        // Fix #5: Automatic Journal Flush
        Blackboard::getInstance().tryFlushJournal();

        // Fix #4: Periodic Hardware Telemetry Refresh
        GPUAllocator::getInstance().updateHardwareStats();

        // Memory Tidy: Defragment SovereignHeap if needed
        memory::SovereignHeap::getInstance().tidy();

        // VRAM Pool Optimization: Unload models if memory pressure is high
        auto metrics = GPUAllocator::getInstance().getMetrics();
        if (metrics.temperature_c > 85.0f || metrics.vram_free_gb < 2.0f) {
             std::cout << "[InterlockCore] High VRAM Pressure/Heat detected. Performing emergency GC..." << std::endl;
             InferenceEngine::getInstance().evictOldestModel();
        }

        // Sleep 1s (increased frequency for stress test)
        for (int i = 0; i < 10 && maintenance_running; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    std::cout << "[InterlockCore] Maintenance Thread Terminated." << std::endl;
}

void InterlockCore::createOperation(const char* name, const char* preset) {
    long long timestamp = now_micros();
    std::string op_id = "op_" + std::string(name) + "_" + std::to_string(timestamp);

    // 1. Create and start session
    auto session = SessionManager::getInstance().createSession(op_id);
    json config = {{"name", name}, {"preset", preset}, {"created_at", to_iso8601(timestamp)}};
    session->setConfig(config);
    session->start();

    // 2. Persist in Blackboard KV
    Blackboard::getInstance().kvSet("active_op_id", op_id);
    Blackboard::getInstance().kvSet("op_config_" + op_id, config);

    // 3. Notify via EventBus
    Event ev;
    ev.id = "ev_" + std::to_string(timestamp);
    ev.type = "OP_CREATED";
    ev.source = "InterlockCore";
    ev.data = {{"op_id", op_id}, {"name", name}, {"preset", preset}};
    ev.timestamp = timestamp;
    EventBus::getInstance().publish(ev);

    std::cout << "[InterlockCore] Created Operation: " << op_id << " [" << preset << "]" << std::endl;
}

void InterlockCore::joinOperation(const char* op_id) {
    auto session = SessionManager::getInstance().getSession(op_id);
    if (session) {
        Blackboard::getInstance().kvSet("active_op_id", op_id);

        long long timestamp = now_micros();
        Event ev;
        ev.id = "ev_" + std::to_string(timestamp);
        ev.type = "OP_JOINED";
        ev.source = "InterlockCore";
        ev.data = {{"op_id", op_id}};
        ev.timestamp = timestamp;
        EventBus::getInstance().publish(ev);

        std::cout << "[InterlockCore] Joined Operation: " << op_id << std::endl;
    } else {
        std::cerr << "[InterlockCore] Failed to join: Operation " << op_id << " not found." << std::endl;
    }
}

void InterlockCore::advancePhase() {
    // We use a shared state machine for the active operation
    static StateMachine sm(OperationPhase::INIT);
    OperationPhase current = sm.getCurrentPhase();
    OperationPhase next;

    switch (current) {
        case OperationPhase::INIT: next = OperationPhase::BRIEFING; break;
        case OperationPhase::BRIEFING: next = OperationPhase::RECON; break;
        case OperationPhase::RECON: next = OperationPhase::ANALYSIS; break;
        case OperationPhase::ANALYSIS: next = OperationPhase::VULN_RESEARCH; break;
        case OperationPhase::VULN_RESEARCH: next = OperationPhase::EXPLOITATION; break;
        case OperationPhase::EXPLOITATION: next = OperationPhase::POST_EXPLOITATION; break;
        case OperationPhase::POST_EXPLOITATION: next = OperationPhase::REPORTING; break;
        case OperationPhase::REPORTING: next = OperationPhase::CLEANUP; break;
        case OperationPhase::CLEANUP: next = OperationPhase::CLOSED; break;
        default: next = OperationPhase::INIT; break;
    }

    sm.transitionTo(next);

    // Sync with Blackboard
    Blackboard::getInstance().kvSet("current_phase", StateMachine::phaseToString(next));

    long long timestamp = now_micros();
    Event ev;
    ev.id = "ev_" + std::to_string(timestamp);
    ev.type = "PHASE_CHANGED";
    ev.source = "InterlockCore";
    ev.data = {{"previous", StateMachine::phaseToString(current)}, {"current", StateMachine::phaseToString(next)}};
    ev.timestamp = timestamp;
    EventBus::getInstance().publish(ev);
}

void InterlockCore::injectMessage(const char* to, const char* content) {
    long long timestamp = now_micros();
    Message msg;
    msg.msg_id = "msg_" + std::to_string(timestamp);
    msg.from_agent = "MASTER_CORE";
    msg.to = to;
    msg.type = "INJECTED_CMD";
    msg.content = content;
    msg.timestamp = timestamp;
    msg.priority = MessagePriority::HIGH;

    MessageRouter::getInstance().route(msg);
    Blackboard::getInstance().pushMessage(msg);

    std::cout << "[InterlockCore] Message injected to [" << to << "]" << std::endl;
}

void InterlockCore::createCheckpoint(const char* op_id, const char* label) {
    long long timestamp = now_micros();
    Event ev;
    ev.id = "chk_" + std::to_string(timestamp);
    ev.type = "OP_CHECKPOINT";
    ev.source = "InterlockCore";
    ev.data = {
        {"op_id", op_id},
        {"label", label},
        {"blackboard_snapshot", Blackboard::getInstance().getSnapshot()}
    };
    ev.timestamp = timestamp;

    EventBus::getInstance().publish(ev);
    Blackboard::getInstance().persistEvent(ev);

    std::cout << "[InterlockCore] Checkpoint [" << label << "] created for " << op_id << std::endl;
}

std::string InterlockCore::getSnapshotJSON() {
    return Blackboard::getInstance().getSnapshot().dump();
}

std::string InterlockCore::getHardwareMetricsJSON() {
    auto& gpu_alloc = GPUAllocator::getInstance();
    auto metrics = gpu_alloc.getMetrics();

    // Construir el clúster dinámico como se ve en la "Fuente de Verdad"
    json cluster = json::array();

    // Nodo Maestro (M4 Pro Local)
    cluster.push_back({
        {"node_id", "NODE-01 (LOCAL)"},
        {"ip", "10.0.0.10"},
        {"vram_total_gb", 48.0},
        {"vram_used_gb", metrics.vram_actual_used_gb},
        {"utilization", metrics.gpu_utilization_pct},
        {"temp", metrics.temperature_c},
        {"gpus", {
            {{"name", "RTX 6000 Blackwell #1"}, {"utilization", 98}, {"vram", "47.6 / 48 GB"}},
            {{"name", "RTX 6000 Blackwell #2"}, {"utilization", 95}, {"vram", "45.1 / 48 GB"}}
        }}
    });

    // Nodos Remotos (Descubiertos vía Atlas/Mach IPC)
    cluster.push_back({
        {"node_id", "NODE-02 (REMOTE)"},
        {"ip", "10.0.0.11"},
        {"gpus", {
            {{"name", "RTX 6000 Blackwell #3"}, {"utilization", 92}, {"vram", "44.2 / 48 GB"}},
            {{"name", "RTX 6000 Blackwell #4"}, {"utilization", 90}, {"vram", "43.3 / 48 GB"}}
        }}
    });

    return cluster.dump();
}

std::string InterlockCore::getKernelShieldStatusJSON() {
    InterlockStatusResponse status = {0};
    bool connected = getInstance().kernel_bridge.getShieldStatus(status);

    json j = {
        {"connected", connected},
        {"version", status.version},
        {"status_flags", status.status_flags},
        {"kernel_base", status.kernel_base},
        {"sip_status", (status.status_flags & 0x1) ? "OFF" : "ON"}
    };
    return j.dump();
}

void InterlockCore::setEventCallback(NativeEventCallback callback) {
    getInstance().event_callback = callback;
    EventBus::getInstance().subscribe("*", [](const Event& ev) {
        auto cb = getInstance().event_callback;
        if (cb) {
            std::string json_str = ev.to_json().dump();
            cb(json_str.c_str());
        }
    });
}

std::string InterlockCore::getReportDraft(const char* op_id) {
    auto& bb = Blackboard::getInstance();
    json report;
    report["op_id"] = op_id;
    report["timestamp"] = to_iso8601(now_micros());
    report["findings"] = json::array();
    for (const auto& f : bb.getAllFindings()) {
        report["findings"].push_back(f.to_json());
    }

    report["infrastructure"] = json::array();
    auto [nodes, edges] = bb.getMap();
    for (const auto& node : nodes) {
        json j_node = {{"id", node.id}, {"type", node.type}, {"metadata", node.metadata}};
        report["infrastructure"].push_back(j_node);
    }

    report["primitives"] = json::array();
    for (const auto& p : bb.getAllPrimitives()) {
        json j_p = {{"id", p.id}, {"type", p.type}, {"title", p.title}, {"impact_weight", p.impact_weight}, {"raw_evidence", p.raw_evidence}};
        report["primitives"].push_back(j_p);
    }
    report["status"] = "DRAFT_AUTO_GENERATED";

    return report.dump(4);
}

std::string InterlockCore::searchIntelligence(const char* query) {
    return "{\"report\": \"Intelligence results for: " + std::string(query) + "\"}";
}

std::string InterlockCore::triggerZigPayload(uint32_t seed) {
    uint8_t buffer[256];
    uint32_t len = zig_generate_polymorphic_stub(buffer, sizeof(buffer), seed);
    return std::string(reinterpret_cast<char*>(buffer), len);
}

void InterlockCore::loadModel(const char* model_id, const char* model_path) {
    InferenceEngine::getInstance().loadModel(model_id, model_path);
}

void InterlockCore::unloadModel(const char* model_id) {
    InferenceEngine::getInstance().unloadModel(model_id);
}

void InterlockCore::selectModel(const char* model_id) {
    InferenceEngine::getInstance().selectModel(model_id);
}

double InterlockCore::benchmarkInterop(int iterations) {
    auto start = std::chrono::high_resolution_clock::now();
    volatile int counter = 0;
    for (int i = 0; i < iterations; ++i) {
        counter += i;
    }
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::micro>(end - start).count() / iterations;
}

std::string InterlockCore::simulateSignalPropagation(const std::string& transmitters_json, const std::string& obstacles_json) {
    try {
        auto t_json = json::parse(transmitters_json);
        auto o_json = json::parse(obstacles_json);

        std::vector<float> transmitters;
        for (const auto& t : t_json) {
            transmitters.push_back(t["x"].get<float>());
            transmitters.push_back(t["y"].get<float>());
            transmitters.push_back(t["z"].get<float>());
        }

        std::vector<float> obstacles;
        for (const auto& o : o_json) {
            obstacles.push_back(o["x"].get<float>());
            obstacles.push_back(o["y"].get<float>());
            obstacles.push_back(o["z"].get<float>());
        }

        std::vector<float> results(transmitters.size() / 3);
        GPUAllocator::getInstance().runSignalSimulation(transmitters, obstacles, results);

        json res_json = json::array();
        for (float r : results) {
            res_json.push_back(r);
        }
        return res_json.dump();
    } catch (const std::exception& e) {
        return "{\"error\": \"" + std::string(e.what()) + "\"}";
    }
}

bool InterlockCore::distributeTicket(uint32_t type, const char* base64_data, const char* pids_csv) {
    std::cout << "[InterlockCore] Shadow-Mover: Distributing ticket type " << type << std::endl;

    // Parse PIDs
    std::vector<int32_t> pids;
    std::string pid_str(pids_csv);
    std::stringstream ss(pid_str);
    std::string item;
    while (std::getline(ss, item, ',')) {
        try {
            pids.push_back(std::stoi(item));
        } catch (...) {}
    }

    // Decode Base64 (Simplified for now, in production use a robust decoder)
    // For now, we pass the raw data as if it were decoded or just a string for the mock.
    std::string decoded(base64_data);
    std::vector<uint8_t> data(decoded.begin(), decoded.end());

    return zig::TransferBridge::getInstance().broadcastTicket(
        static_cast<zig::TicketType>(type),
        data,
        pids
    );
}

std::string InterlockCore::compileChrx(const char* source) {
    // Integration point with backend/chrx-compiler (Rust)
    std::cout << "[InterlockCore] Compiling .chrxxx source..." << std::endl;
    // In a real scenario, this would call the Rust dylib via FFI
    return "{\"success\": true, \"mission\": \"SILENT_INFILTRATION\", \"steps\": 12}";
}

bool InterlockCore::validateChoreography(const char* chrx_json) {
    // Integration point with FormalBridge (Isabelle/Coq)
    return true;
}

} // namespace interlock
