#pragma once

#include "Schema.hpp"
#include <string>
#include <map>
#include <functional>
#include <mutex>

namespace interlock {

/**
 * ToolDispatcher (M4 Pro Native)
 * Decodes <action> XML tags from Agents and executes native C++ / OS tools.
 */
class ToolDispatcher {
public:
    static ToolDispatcher& getInstance() {
        static ToolDispatcher instance;
        return instance;
    }

    // Procesa el XML de salida del modelo y dispara las acciones encontradas
    void dispatchActions(const std::string& agent_alias, const std::string& xml_output);

    // Parsea y extrae datos estructurados de GBNF (MoDA/MoE)
    std::string parseGBNFResponse(const std::string& response);

    // Registra una función nativa como herramienta
    using ToolFunction = std::function<void(const std::string& agent, const std::string& args)>;
    void registerTool(const std::string& name, ToolFunction func);

private:
    ToolDispatcher();

    void parseAndExecute(const std::string& agent, const std::string& action_xml);
    std::string extractAttribute(const std::string& xml, const std::string& attr);
    std::string extractTagContent(const std::string& xml, const std::string& tag);

    std::map<std::string, ToolFunction> tool_registry;
    std::mutex mtx;
};

} // namespace interlock
