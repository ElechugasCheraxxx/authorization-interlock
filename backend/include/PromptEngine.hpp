#pragma once

#include "Schema.hpp"
#include <string>
#include <map>
#include <vector>
#include <mutex>
#include <mutex>

namespace interlock {

/**
 * AdaptivePromptEngine (M4 Pro Optimized)
 * Generates structured XML prompts dynamically based on mission context.
 */
class AdaptivePromptEngine {
public:
    static AdaptivePromptEngine& getInstance() {
        static AdaptivePromptEngine instance;
        return instance;
    }

    // Genera un prompt XML completo para un agente específico
    std::string buildAgentPrompt(const std::string& agent_alias, const std::string& user_instruction);

    // Actualiza el contexto de la operación en "caliente"
    void updateOperationMetadata(const json& metadata);

    // Inyecta evidencias o hallazgos recientes del Blackboard
    void injectContext(const std::string& key, const std::string& value);

private:
    AdaptivePromptEngine();

    std::string getAgentSystemBase(const std::string& alias);
    std::string getGlobalOperationXML();

    std::map<std::string, std::string> runtime_context;
    json operation_metadata;
    std::mutex mtx;

    // XML Templates (Hardcoded as base, but filled dynamically)
    const std::string XML_HEADER = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<assessment_session>";
    const std::string XML_FOOTER = "\n</assessment_session>";
};

} // namespace interlock
