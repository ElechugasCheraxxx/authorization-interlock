#include "ToolDispatcher.hpp"
#include "ExecutionManager.hpp"
#include "Blackboard.hpp"
#include "security/SymbolExtractor.hpp"
#include <iostream>
#include <regex>
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace interlock {

ToolDispatcher::ToolDispatcher() {
    // Registro de herramientas base nativas
    registerTool("command", [](const std::string& agent, const std::string& args) {
        std::cout << "[ToolDispatcher] Agent " << agent << " executing command: " << args << std::endl;
        ExecutionManager::getInstance().spawnProcess(args, {}, true);
    });

    registerTool("report", [](const std::string& agent, const std::string& args) {
        std::cout << "[ToolDispatcher] Agent " << agent << " reporting: " << args << std::endl;
        Finding f;
        f.id = "find_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        f.agent = agent;
        f.title = "Agent Intelligence Report";
        f.raw_stream = args;
        f.severity = "INFO";
        f.timestamp = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        Blackboard::getInstance().addFinding(f);
    });

    registerTool("network_packet", [](const std::string& agent, const std::string& args) {
        std::cout << "[ToolDispatcher] Agent " << agent << " sending raw network packet." << std::endl;
    });

    registerTool("ghidra_decompile", [](const std::string& agent, const std::string& args) {
        std::cout << "[ToolDispatcher] Agent " << agent << " requesting decompilation of: " << args << std::endl;
    });

    registerTool("extract_symbols", [](const std::string& agent, const std::string& args) {
        // args expected to be a path to a binary or raw nm output
        std::cout << "[ToolDispatcher] Agent " << agent << " extracting symbols..." << std::endl;

        // If it's a file path, run nm first (simplified for this port)
        std::string cmd = "nm -S " + args;
        std::string output = ExecutionManager::getInstance().executeAndCapture(cmd);

        auto symbols = security::SymbolExtractor::parseBsdOutput(output);

        json j_symbols = json::array();
        for (const auto& s : symbols) {
            j_symbols.push_back({
                {"name", s.name},
                {"rva", s.rva},
                {"size", s.size},
                {"section", s.section}
            });
        }

        // Post to Blackboard or return via some mechanism
        std::cout << "[ToolDispatcher] Extracted " << symbols.size() << " symbols." << std::endl;
    });
}

void ToolDispatcher::registerTool(const std::string& name, ToolFunction func) {
    std::lock_guard<std::mutex> lock(mtx);
    tool_registry[name] = func;
}

void ToolDispatcher::dispatchActions(const std::string& agent_alias, const std::string& xml_output) {
    if (xml_output.empty()) return;

    try {
        // Buscamos todas las etiquetas <action type="...">...</action>
        std::string rs = "<action\\s+type=\"([^\"]+)\">([\\s\\S]*?)<\\/action>";
        std::regex action_regex(rs);
        auto words_begin = std::sregex_iterator(xml_output.begin(), xml_output.end(), action_regex);
        auto words_end = std::sregex_iterator();

        for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
            std::smatch match = *i;
            if (match.size() < 3) continue;

            std::string type = match[1].str();
            std::string body = match[2].str();

            // Limpieza básica de CDATA si existe dentro del cuerpo
            size_t cdata_start = body.find("<![CDATA[");
            if (cdata_start != std::string::npos) {
                body = body.substr(cdata_start + 9);
                size_t cdata_end = body.find("]]>");
                if (cdata_end != std::string::npos) {
                    body = body.substr(0, cdata_end);
                }
            }

            ToolFunction target_tool;
            {
                std::lock_guard<std::mutex> lock(mtx);
                auto it = tool_registry.find(type);
                if (it != tool_registry.end()) {
                    target_tool = it->second;
                }
            }

            if (target_tool) {
                try {
                    target_tool(agent_alias, body);
                } catch (const std::exception& e) {
                    std::cerr << "[ToolDispatcher] Tool '" << type << "' failed: " << e.what() << std::endl;
                } catch (...) {
                    std::cerr << "[ToolDispatcher] Tool '" << type << "' failed with unknown error." << std::endl;
                }
            } else {
                std::cerr << "[ToolDispatcher] Unknown tool type: " << type << " requested by " << agent_alias << std::endl;
            }
        }
    } catch (const std::regex_error& e) {
        std::cerr << "[ToolDispatcher] Regex error while parsing actions: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[ToolDispatcher] Error in dispatchActions: " << e.what() << std::endl;
    }
}

std::string ToolDispatcher::parseGBNFResponse(const std::string& response) {
    if (response.empty()) return "<Response status=\"empty\"/>";

    std::stringstream ss;
    ss << "<Response>\n";

    try {
        // Extracción de etiquetas XML comunes forzadas por GBNF
        std::vector<std::string> tags = {"code", "filepath", "explanation", "reasoning", "vulnerability", "artifact", "action", "status"};

        bool found_any = false;
        for (const auto& tag : tags) {
            std::regex tag_regex("<" + tag + "[^>]*>([\\s\\S]*?)</" + tag + ">");
            std::smatch match;
            auto words_begin = std::sregex_iterator(response.begin(), response.end(), tag_regex);
            auto words_end = std::sregex_iterator();

            for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
                std::smatch match = *i;
                ss << "  <" << tag << "><![CDATA[" << match[1].str() << "]]></" << tag << ">\n";
                found_any = true;
            }
        }

        if (!found_any) {
            ss << "  <Raw><![CDATA[" << response << "]]></Raw>\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "[ToolDispatcher] Error parsing GBNF response to XML: " << e.what() << std::endl;
        ss << "  <Error>" << xml_escape(e.what()) << "</Error>\n";
    }

    ss << "</Response>";
    return ss.str();
}

} // namespace interlock
