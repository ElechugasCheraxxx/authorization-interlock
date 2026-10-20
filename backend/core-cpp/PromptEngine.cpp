#include "PromptEngine.hpp"
#include "Blackboard.hpp"
#include <sstream>
#include <chrono>
#include <iostream>
#include <algorithm>

namespace interlock {

// Helper to wrap content in CDATA to protect special characters (scripts, payloads, etc)
static std::string wrapCDATA(const std::string& data) {
    if (data.empty()) return "";
    return "<![CDATA[" + data + "]]>";
}

static const char* CHRXXX_LANGUAGE_SPEC =
    "CHERAX_DSL (.chrxxx) Specification:\n"
    "- Directives: @compiler, @memory_pool, @alignment, @temporal_drift\n"
    "- Blocks: mission, verify, resource, buffer, swarm, agent, flow, choreography, swarm_sync\n"
    "- Commands: bind, await, lock, assert, purge, halt, commit, resume\n"
    "- Hardware: M4 Pro (48GB Unified Memory)\n"
    "- Integrity: ML-DSA-65 signatures, Jenkins-32 checks\n";

AdaptivePromptEngine::AdaptivePromptEngine() {
    // Inicialización de metadatos básicos por defecto
    operation_metadata = {
        {"op_id", "OPERACION_ALPHA"},
        {"security_level", "ASSESSMENT_STRICT"},
        {"scope", "INTERNAL_NETWORK"}
    };
}

void AdaptivePromptEngine::updateOperationMetadata(const json& metadata) {
    std::lock_guard<std::mutex> lock(mtx);
    for (auto& [key, value] : metadata.items()) {
        operation_metadata[key] = value;
    }
}

void AdaptivePromptEngine::injectContext(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mtx);
    runtime_context[key] = value;
}

std::string AdaptivePromptEngine::getGlobalOperationXML() {
    std::stringstream ss;
    ss << "  <operation_context>\n";
    ss << "    <id>" << operation_metadata.value("op_id", "UNKNOWN") << "</id>\n";
    ss << "    <objective>" << operation_metadata.value("objective", "ASSESS_AND_EXFIL") << "</objective>\n";
    ss << "    <rules_of_engagement>\n";
    ss << "      <stealth_mode>ENABLED</stealth_mode>\n";
    ss << "      <avoid_critical_systems>TRUE</avoid_critical_systems>\n";
    ss << "    </rules_of_engagement>\n";
    ss << "  </operation_context>\n";
    return ss.str();
}

std::string AdaptivePromptEngine::getAgentSystemBase(const std::string& alias) {
    if (alias == "rex") {
        return "    <role>EXPLOITATION_EXPERT</role>\n"
               "    <capabilities>PAYLOAD_CRAFTING, MEMORY_INJECTION, PIVOTING</capabilities>";
    } else if (alias == "vantage") {
        return "    <role>INTELLIGENCE_ANALYST</role>\n"
               "    <capabilities>RECONNAISSANCE, OSINT, NETWORK_MAPPING</capabilities>";
    } else if (alias == "apex") {
        return "    <role>TECHNICAL_LEAD</role>\n"
               "    <capabilities>ORCHESTRATION, STRATEGY, RISK_ASSESSMENT</capabilities>";
    }
    return "    <role>GENERAL_AGENT</role>";
}

std::string AdaptivePromptEngine::buildAgentPrompt(const std::string& agent_alias, const std::string& user_instruction) {
    std::lock_guard<std::mutex> lock(mtx);
    std::stringstream ss;

    ss << XML_HEADER << "\n";

    // 1. Contexto Global de la Operación
    ss << getGlobalOperationXML();

    // 2. Definición del Agente (Persona)
    ss << "  <agent_identity>\n";
    ss << "    <alias>" << agent_alias << "</alias>\n";
    ss << getAgentSystemBase(agent_alias) << "\n";
    ss << "  </agent_identity>\n";

    // 3. Estado Dinámico (Lo que ha pasado recientemente)
    ss << "  <runtime_memory>\n";
    for (const auto& [key, value] : runtime_context) {
        ss << "    <evidence key=\"" << key << "\">" << wrapCDATA(value) << "</evidence>\n";
    }
    ss << "  </runtime_memory>\n";

    // 4. Instrucción Actual
    ss << "  <current_task>\n";
    ss << "    <instruction>" << wrapCDATA(user_instruction) << "</instruction>\n";
    ss << "    <format_requirement>CHERAX_DSL_BLOCKS (.chrxxx)</format_requirement>\n";
    ss << "    <guidelines>\n";
    ss << "      <rule>All cognitive output must be encapsulated in a '.chrxxx' block.</rule>\n";
    ss << "      <rule>Use hardware-aware directives (@alignment, @memory_pool) for M4 Pro optimization.</rule>\n";
    ss << "      <rule>Include a 'verify {}' block with formal invariants for every mission.</rule>\n";
    ss << "      <rule>Use 'choreography' for cross-language (C++/Rust/Go/Zig) atomic state transitions.</rule>\n";
    ss << "    </guidelines>\n";
    ss << "  </current_task>\n";

    // 5. Language Specification
    ss << "  <language_spec>\n";
    ss << "    " << wrapCDATA(CHRXXX_LANGUAGE_SPEC) << "\n";
    ss << "  </language_spec>\n";

    ss << XML_FOOTER;

    return ss.str();
}

} // namespace interlock
