#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/optional.h>
#include <nanobind/ndarray.h>
#include "Blackboard.hpp"
#include "MessageRouter.hpp"
#include "InferenceEngine.hpp"
#include "SessionManager.hpp"
#include "GPUAllocator.hpp"
#include "RiskManager.hpp"
#include "ScopeGuard.hpp"
#include "AutomationModule.hpp"
#include "C2Manager.hpp"
#include "PivotingManager.hpp"
#include "ArtifactManager.hpp"
#include "RAGEngine.hpp"
#include "PDGEngine.hpp"
#include "EvolutionEngine.hpp"
#include "EventSystem.hpp"
#include "DetectionModule.hpp"
#include "CognitiveEngines.hpp"
#include "TokenDispatcher.hpp"
#include "HealthMonitor.hpp"
#include "Schema.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace interlock;

NB_MODULE(interlock_core_bind, m) {
    nb::enum_<MessagePriority>(m, "MessagePriority")
        .value("LOW", MessagePriority::LOW)
        .value("NORMAL", MessagePriority::NORMAL)
        .value("HIGH", MessagePriority::HIGH)
        .value("CRITICAL", MessagePriority::CRITICAL)
        .export_values();

    nb::class_<Message>(m, "Message")
        .def(nb::init<>())
        .def_rw("msg_id", &Message::msg_id)
        .def_rw("from_agent", &Message::from_agent)
        .def_rw("to", &Message::to)
        .def_rw("channel", &Message::channel)
        .def_rw("type", &Message::type)
        .def_rw("priority", &Message::priority)
        .def_rw("content", &Message::content)
        .def_rw("timestamp", &Message::timestamp)
        .def_rw("ts_precise", &Message::ts_precise)
        .def("to_json", [](const Message& m) {
            return m.to_json().dump();
        });

    nb::class_<AgentState>(m, "AgentState")
        .def(nb::init<>())
        .def_rw("alias", &AgentState::alias)
        .def_rw("status", &AgentState::status)
        .def_rw("vram_used_gb", &AgentState::vram_used_gb)
        .def_rw("total_tokens", &AgentState::total_tokens)
        .def_rw("current_model", &AgentState::current_model);

    nb::class_<DepartmentState>(m, "DepartmentState")
        .def(nb::init<>())
        .def_rw("name", &DepartmentState::name)
        .def_rw("status", &DepartmentState::status)
        .def_rw("active_agents", &DepartmentState::active_agents);

    nb::class_<Ticket>(m, "Ticket")
        .def(nb::init<>())
        .def_rw("id", &Ticket::id)
        .def_rw("creator", &Ticket::creator)
        .def_rw("assigned_to", &Ticket::assigned_to)
        .def_rw("instruction", &Ticket::instruction)
        .def_rw("priority", &Ticket::priority)
        .def_rw("status", &Ticket::status)
        .def_rw("created_at", &Ticket::created_at);

    nb::class_<Finding>(m, "Finding")
        .def(nb::init<>())
        .def_rw("id", &Finding::id)
        .def_rw("agent", &Finding::agent)
        .def_rw("title", &Finding::title)
        .def_rw("severity", &Finding::severity)
        .def_rw("raw_stream", &Finding::raw_stream)
        .def_rw("timestamp", &Finding::timestamp)
        .def_rw("ts_precise", &Finding::ts_precise);

    nb::class_<MapNode>(m, "MapNode")
        .def(nb::init<>())
        .def_rw("id", &MapNode::id)
        .def_rw("type", &MapNode::type);

    nb::class_<MapEdge>(m, "MapEdge")
        .def(nb::init<>())
        .def_rw("id", &MapEdge::id)
        .def_rw("source", &MapEdge::source)
        .def_rw("target", &MapEdge::target)
        .def_rw("type", &MapEdge::type);

    nb::class_<Primitive>(m, "Primitive")
        .def(nb::init<>())
        .def_rw("id", &Primitive::id)
        .def_rw("type", &Primitive::type)
        .def_rw("title", &Primitive::title)
        .def_rw("impact_weight", &Primitive::impact_weight)
        .def_rw("raw_evidence", &Primitive::raw_evidence);

    nb::class_<DependencyEdge>(m, "DependencyEdge")
        .def(nb::init<>())
        .def_rw("source_id", &DependencyEdge::source_id)
        .def_rw("target_id", &DependencyEdge::target_id)
        .def_rw("type", &DependencyEdge::type);

    nb::class_<HypothesisNode>(m, "HypothesisNode")
        .def(nb::init<>())
        .def_rw("id", &HypothesisNode::id)
        .def_rw("content", &HypothesisNode::content)
        .def_rw("confidence", &HypothesisNode::confidence)
        .def_rw("status", &HypothesisNode::status)
        .def_rw("children", &HypothesisNode::children);

    nb::class_<HypothesisTree>(m, "HypothesisTree")
        .def(nb::init<>())
        .def_rw("vector_id", &HypothesisTree::vector_id)
        .def_rw("root_node_id", &HypothesisTree::root_node_id)
        .def_rw("nodes", &HypothesisTree::nodes);

    nb::class_<InferenceRequest>(m, "InferenceRequest")
        .def(nb::init<>())
        .def_rw("model_id", &InferenceRequest::model_id)
        .def_rw("prompt", &InferenceRequest::prompt)
        .def_rw("temperature", &InferenceRequest::temperature)
        .def_rw("max_tokens", &InferenceRequest::max_tokens);

    nb::class_<InferenceResponse>(m, "InferenceResponse")
        .def_ro("text", &InferenceResponse::text)
        .def_ro("tokens_generated", &InferenceResponse::tokens_generated)
        .def_ro("latency_ms", &InferenceResponse::latency_ms)
        .def_ro("tokens_per_second", &InferenceResponse::tokens_per_second)
        .def_ro("finish_reason", &InferenceResponse::finish_reason);

    nb::class_<InferenceEngine>(m, "InferenceEngine")
        .def_static("get_instance", &InferenceEngine::getInstance, nb::rv_policy::reference)
        .def("initialize", &InferenceEngine::initialize)
        .def("generate", [](InferenceEngine& e, const InferenceRequest& req) {
            return e.generate(req).get();
        })
        .def("load_model", &InferenceEngine::loadModel)
        .def("select_model", &InferenceEngine::selectModel)
        .def("get_loaded_models", &InferenceEngine::getLoadedModels)
        .def("allocate_pool", &InferenceEngine::allocatePool)
        .def("release_pool", &InferenceEngine::releasePool);

    nb::class_<OperationSession>(m, "OperationSession")
        .def("start", &OperationSession::start)
        .def("stop", &OperationSession::stop)
        .def_prop_ro("op_id", &OperationSession::getOpId);

    nb::class_<SessionManager>(m, "SessionManager")
        .def_static("get_instance", &SessionManager::getInstance, nb::rv_policy::reference)
        .def("create_session", &SessionManager::createSession)
        .def("get_session", &SessionManager::getSession)
        .def("remove_session", &SessionManager::removeSession);

    nb::class_<Blackboard>(m, "Blackboard")
        .def_static("get_instance", &Blackboard::getInstance, nb::rv_policy::reference)
        .def("update_agent_state", &Blackboard::updateAgentState)
        .def("get_agent_state", &Blackboard::getAgentState)
        .def("get_all_agent_states", &Blackboard::getAllAgentStates)
        .def("update_department_state", &Blackboard::updateDepartmentState)
        .def("get_all_department_states", &Blackboard::getAllDepartmentStates)
        .def("push_message", &Blackboard::pushMessage)
        .def("get_message_tail", &Blackboard::getMessageTail)
        .def("upsert_ticket", &Blackboard::upsertTicket)
        .def("get_all_tickets", &Blackboard::getAllTickets)
        .def("get_ticket", &Blackboard::getTicket)
        .def("add_finding", &Blackboard::addFinding)
        .def("get_all_findings", &Blackboard::getAllFindings)
        .def("get_finding", &Blackboard::getFinding)
        .def("kv_set", [](Blackboard& b, const std::string& k, const std::string& v_json) {
            b.kvSet(k, json::parse(v_json));
        })
        .def("kv_get", [](Blackboard& b, const std::string& k) {
            return b.kvGet(k).dump();
        })
        .def("kv_all", [](Blackboard& b) {
            std::map<std::string, std::string> res;
            for (auto const& [k, v] : b.kvAll()) {
                res[k] = v.dump();
            }
            return res;
        })
        .def("kv_delete", &Blackboard::kvDelete)
        .def("add_map_node", &Blackboard::addMapNode)
        .def("add_map_edge", &Blackboard::addMapEdge)
        .def("get_map", &Blackboard::getMap)
        .def("add_primitive", &Blackboard::addPrimitive)
        .def("add_dependency", &Blackboard::addDependency)
        .def("get_primitive_graph", &Blackboard::getPrimitiveGraph)
        .def("upsert_hypothesis_node", &Blackboard::upsertHypothesisNode)
        .def("get_hypothesis_tree", &Blackboard::getHypothesisTree)
        .def("get_all_hypothesis_trees", &Blackboard::getAllHypothesisTrees)
        .def("clear", &Blackboard::clear);

    nb::class_<GPUMetrics>(m, "GPUMetrics")
        .def_ro("vram_total_gb", &GPUMetrics::vram_total_gb)
        .def_ro("vram_reserved_gb", &GPUMetrics::vram_reserved_gb)
        .def_ro("vram_actual_used_gb", &GPUMetrics::vram_actual_used_gb)
        .def_ro("vram_free_gb", &GPUMetrics::vram_free_gb)
        .def_ro("temperature_c", &GPUMetrics::temperature_c)
        .def_ro("gpu_utilization_pct", &GPUMetrics::gpu_utilization_pct);

    nb::class_<GPUAllocator>(m, "GPUAllocator")
        .def_static("get_instance", &GPUAllocator::getInstance, nb::rv_policy::reference)
        .def("get_metrics", &GPUAllocator::getMetrics)
        .def("reserve_vram", &GPUAllocator::reserveVRAM)
        .def("release_vram", &GPUAllocator::releaseVRAM)
        .def("get_pool_saturation", &GPUAllocator::getPoolSaturation)
        .def("calculate_optimal_rdt_factor", &GPUAllocator::calculateOptimalRDTFactor)
        .def("update_thermals", &GPUAllocator::updateThermals)
        .def("update_hardware_stats", &GPUAllocator::updateHardwareStats)
        .def("set_compute_priority", &GPUAllocator::setComputePriority);

    nb::class_<VectorEntry>(m, "VectorEntry")
        .def_ro("id", &VectorEntry::id)
        .def_ro("embedding", &VectorEntry::embedding);

    nb::class_<RAGEngine>(m, "RAGEngine")
        .def_static("get_instance", &RAGEngine::getInstance, nb::rv_policy::reference)
        .def("index_finding", &RAGEngine::indexFinding)
        .def("search", &RAGEngine::search, "query_text"_a, "limit"_a = 5)
        .def("distill_memories", &RAGEngine::distillMemories)
        .def("persist_to_binary", &RAGEngine::persistToBinary)
        .def("load_from_binary", &RAGEngine::loadFromBinary);

    nb::class_<AttackChain>(m, "AttackChain")
        .def_ro("primitive_ids", &AttackChain::primitive_ids)
        .def_ro("total_probability", &AttackChain::total_probability)
        .def_ro("cumulative_impact", &AttackChain::cumulative_impact);

    nb::class_<PDGEngine>(m, "PDGEngine")
        .def_static("get_instance", &PDGEngine::getInstance, nb::rv_policy::reference)
        .def("find_zero_error_chains", &PDGEngine::findZeroErrorChains)
        .def("evaluate_primitive_impact", &PDGEngine::evaluatePrimitiveImpact);

    nb::class_<EvolutionTask>(m, "EvolutionTask")
        .def(nb::init<>())
        .def_rw("agent_alias", &EvolutionTask::agent_alias)
        .def_rw("failure_reason", &EvolutionTask::failure_reason);

    nb::class_<EvolutionEngine>(m, "EvolutionEngine")
        .def_static("get_instance", &EvolutionEngine::getInstance, nb::rv_policy::reference)
        .def("start_autonomous_self_improvement", &EvolutionEngine::startAutonomousSelfImprovement)
        .def("trigger_jit_evolution", &EvolutionEngine::triggerJITEvolution)
        .def("start_dreaming_loop", &EvolutionEngine::startDreamingLoop)
        .def("run_shadow_training", &EvolutionEngine::runShadowTraining)
        .def("run_mlx_finetune", &EvolutionEngine::runMLXFinetune);

    nb::class_<MessageRouter>(m, "MessageRouter")
        .def_static("get_instance", &MessageRouter::getInstance, nb::rv_policy::reference)
        .def("start", &MessageRouter::start)
        .def("stop", &MessageRouter::stop)
        .def("route", &MessageRouter::route);

    nb::enum_<RiskLevel>(m, "RiskLevel")
        .value("MINIMAL", RiskLevel::MINIMAL)
        .value("LOW", RiskLevel::LOW)
        .value("MEDIUM", RiskLevel::MEDIUM)
        .value("HIGH", RiskLevel::HIGH)
        .value("CRITICAL", RiskLevel::CRITICAL)
        .export_values();

    nb::class_<OpsecStatus>(m, "OpsecStatus")
        .def_ro("risk_score", &OpsecStatus::risk_score)
        .def_ro("level", &OpsecStatus::level)
        .def_ro("is_vm", &OpsecStatus::is_vm)
        .def_ro("is_debugged", &OpsecStatus::is_debugged)
        .def_ro("detected_edr", &OpsecStatus::detected_edr);

    nb::class_<RiskManager>(m, "RiskManager")
        .def_static("get_instance", &RiskManager::getInstance, nb::rv_policy::reference)
        .def("secure_wipe", &RiskManager::secureWipe, "path"_a, "passes"_a = 3)
        .def("timestomp", &RiskManager::timestomp, "target"_a, "reference"_a = "")
        .def("scrub_logs", &RiskManager::scrubLogs)
        .def("wipe_shell_history", &RiskManager::wipeShellHistory)
        .def("detect_analysis_env", &RiskManager::detectAnalysisEnv)
        .def("check_anti_debugging", &RiskManager::checkAntiDebugging)
        .def("detect_edr", &RiskManager::detectEDR)
        .def("get_status", &RiskManager::getStatus)
        .def("report_deviation", &RiskManager::reportDeviation)
        .def("trigger_self_destruct", &RiskManager::triggerSelfDestruct);

    // --- C2 Manager Bindings ---
    nb::class_<C2Session>(m, "C2Session")
        .def_ro("id", &C2Session::id)
        .def_ro("remote_addr", &C2Session::remote_addr)
        .def_ro("type", &C2Session::type)
        .def_ro("last_seen", &C2Session::last_seen)
        .def_ro("active", &C2Session::active);

    nb::class_<C2Manager>(m, "C2Manager")
        .def_static("get_instance", &C2Manager::getInstance, nb::rv_policy::reference)
        .def("start_listener", &C2Manager::startListener)
        .def("stop_listener", &C2Manager::stopListener)
        .def("generate_implant", [](C2Manager& c, const std::string& type, const std::string& url, const std::string& config_json) {
            return c.generateImplant(type, url, json::parse(config_json));
        })
        .def("get_sessions", &C2Manager::getSessions)
        .def("send_command", &C2Manager::sendCommand)
        .def("get_command_response", &C2Manager::getCommandResponse)
        .def("close_session", &C2Manager::closeSession);

    // --- Pivoting Manager Bindings ---
    nb::class_<Route>(m, "Route")
        .def_ro("path", &Route::path)
        .def_ro("technique", &Route::technique)
        .def_ro("cost", &Route::cost);

    nb::class_<PivotingManager>(m, "PivotingManager")
        .def_static("get_instance", &PivotingManager::getInstance, nb::rv_policy::reference)
        .def("find_routes", &PivotingManager::findRoutes)
        .def("get_reachable_subnets", &PivotingManager::getReachableSubnets)
        .def("create_tunnel", &PivotingManager::createTunnel)
        .def("close_tunnel", &PivotingManager::closeTunnel);

    // --- Artifact Manager Bindings ---
    nb::class_<ArtifactManager>(m, "ArtifactManager")
        .def_static("get_instance", &ArtifactManager::getInstance, nb::rv_policy::reference)
        .def("obfuscate_strings", &ArtifactManager::obfuscateStrings)
        .def("add_junk_code", &ArtifactManager::addJunkCode)
        .def("b64_encode_layer", &ArtifactManager::b64EncodeLayer)
        .def("xor_encode_layer", &ArtifactManager::xorEncodeLayer, "code"_a, "key"_a = 42)
        .def("generate_variants", &ArtifactManager::generateVariants, "original"_a, "count"_a = 3)
        .def("wrap_for_os", &ArtifactManager::wrapForOS);

    // --- Event System Bindings ---
    nb::class_<Event>(m, "Event")
        .def(nb::init<>())
        .def_rw("id", &Event::id)
        .def_rw("type", &Event::type)
        .def_rw("source", &Event::source)
        .def("to_json", [](const Event& e) { return e.to_json().dump(); });

    nb::class_<EventBus>(m, "EventBus")
        .def_static("get_instance", &EventBus::getInstance, nb::rv_policy::reference)
        .def("publish", &EventBus::publish)
        .def("subscribe", &EventBus::subscribe);

    nb::class_<EventStore>(m, "EventStore")
        .def_static("get_instance", &EventStore::getInstance, nb::rv_policy::reference)
        .def("get_history", &EventStore::getHistory, "type"_a = "")
        .def("clear", &EventStore::clear);

    nb::class_<ReplayEngine>(m, "ReplayEngine")
        .def_static("get_instance", &ReplayEngine::getInstance, nb::rv_policy::reference)
        .def("replay_from", &ReplayEngine::replayFrom)
        .def("create_branch", &ReplayEngine::createBranch);

    // --- Detection Module Bindings ---
    nb::class_<TrafficSignal>(m, "TrafficSignal")
        .def(nb::init<>())
        .def_rw("source_ip", &TrafficSignal::source_ip)
        .def_rw("path", &TrafficSignal::path)
        .def_rw("method", &TrafficSignal::method)
        .def_rw("body", &TrafficSignal::body);

    nb::class_<WAFDetector>(m, "WAFDetector")
        .def_static("get_instance", &WAFDetector::getInstance, nb::rv_policy::reference)
        .def("analyze", &WAFDetector::analyze);

    nb::class_<RateLimiter>(m, "RateLimiter")
        .def_static("get_instance", &RateLimiter::getInstance, nb::rv_policy::reference)
        .def("is_rate_limited", &RateLimiter::isRateLimited);

    // --- Cognitive Engines Bindings ---
    nb::class_<HypothesisEngine>(m, "HypothesisEngine")
        .def_static("get_instance", &HypothesisEngine::getInstance, nb::rv_policy::reference)
        .def("generate_hypotheses", &HypothesisEngine::generateHypotheses)
        .def("update_confidence", &HypothesisEngine::updateConfidence)
        .def("get_tree", &HypothesisEngine::getTree);

    nb::class_<DebateEngine>(m, "DebateEngine")
        .def_static("get_instance", &DebateEngine::getInstance, nb::rv_policy::reference)
        .def("resolve_debate", &DebateEngine::resolveDebate);

    // --- Token & Health Bindings ---
    nb::class_<TokenDispatcher>(m, "TokenDispatcher")
        .def_static("get_instance", &TokenDispatcher::getInstance, nb::rv_policy::reference)
        .def("request_tokens", &TokenDispatcher::requestTokens)
        .def("report_usage", &TokenDispatcher::reportUsage)
        .def("rebalance_quotas", &TokenDispatcher::rebalanceQuotas);

    nb::class_<ComponentStatus>(m, "ComponentStatus")
        .def_ro("name", &ComponentStatus::name)
        .def_ro("status", &ComponentStatus::status)
        .def_ro("cpu_usage", &ComponentStatus::cpu_usage)
        .def_ro("mem_usage_mb", &ComponentStatus::mem_usage_mb);

    nb::class_<HealthMonitor>(m, "HealthMonitor")
        .def_static("get_instance", &HealthMonitor::getInstance, nb::rv_policy::reference)
        .def("report_heartbeat", [](HealthMonitor& h, const std::string& name, const std::string& metrics_json) {
            h.reportHeartbeat(name, json::parse(metrics_json));
        })
        .def("get_full_status", &HealthMonitor::getFullStatus);

    // --- Scope Guard Bindings ---
    nb::class_<ROERule>(m, "ROERule")
        .def(nb::init<>())
        .def_rw("pattern", &ROERule::pattern)
        .def_rw("allow", &ROERule::allow)
        .def_rw("description", &ROERule::description);

    nb::class_<ScopeGuard>(m, "ScopeGuard")
        .def_static("get_instance", &ScopeGuard::getInstance, nb::rv_policy::reference)
        .def("is_authorized", &ScopeGuard::isAuthorized)
        .def("load_rules", &ScopeGuard::loadRules)
        .def("log_violation", &ScopeGuard::logViolation);

    // --- Automation Module Bindings ---
    nb::enum_<HeadlessState>(m, "HeadlessState")
        .value("CIPHER_ACTIVE", HeadlessState::CIPHER_ACTIVE)
        .value("RESTARTING", HeadlessState::RESTARTING)
        .value("HEADLESS", HeadlessState::HEADLESS)
        .value("AWAITING_OPERATOR", HeadlessState::AWAITING_OPERATOR)
        .value("PAUSED", HeadlessState::PAUSED)
        .export_values();

    nb::class_<AutomationModule>(m, "AutomationModule")
        .def_static("get_instance", &AutomationModule::getInstance, nb::rv_policy::reference)
        .def("on_orchestrator_failure", &AutomationModule::onOrchestratorFailure)
        .def("on_orchestrator_recovery", &AutomationModule::onOrchestratorRecovery)
        .def("get_state", &AutomationModule::getState);
}
