#pragma once
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <nlohmann/json.hpp>
#include <iomanip>
#include <sstream>

using json = nlohmann::json;

namespace interlock {

/**
 * @brief Helper for XML escaping to ensure precision in Assessment operations.
 */
inline std::string xml_escape(const std::string& data) {
    std::string buffer;
    buffer.reserve(data.size());
    for(size_t pos = 0; pos != data.size(); ++pos) {
        switch(data[pos]) {
            case '&':  buffer.append("&amp;");       break;
            case '\"': buffer.append("&quot;");      break;
            case '\'': buffer.append("&apos;");      break;
            case '<':  buffer.append("&lt;");        break;
            case '>':  buffer.append("&gt;");        break;
            default:   buffer.append(1, data[pos]); break;
        }
    }
    return buffer;
}

inline std::string to_iso8601(long long micros) {
    std::time_t seconds = micros / 1000000;
    int remaining_micros = micros % 1000000;
    std::tm* tm = std::gmtime(&seconds);
    std::stringstream ss;
    ss << std::put_time(tm, "%Y-%m-%dT%H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(6) << remaining_micros << "Z";
    return ss.str();
}

enum class MessagePriority { LOW, NORMAL, HIGH, CRITICAL };

struct Message {
    std::string msg_id;
    std::string from_agent;
    std::string to;
    std::string channel;
    std::string type;
    MessagePriority priority = MessagePriority::NORMAL;
    std::string content;
    json metadata;
    long long timestamp;
    std::string ts_precise;

    std::string to_xml() const {
        std::stringstream ss;
        ss << "<Message id=\"" << xml_escape(msg_id) << "\" priority=\"" << static_cast<int>(priority) << "\">\n";
        ss << "  <From>" << xml_escape(from_agent) << "</From>\n";
        ss << "  <To>" << xml_escape(to) << "</To>\n";
        ss << "  <Channel>" << xml_escape(channel) << "</Channel>\n";
        ss << "  <Type>" << xml_escape(type) << "</Type>\n";
        ss << "  <Content><![CDATA[" << content << "]]></Content>\n";
        ss << "  <Metadata>" << xml_escape(metadata.dump()) << "</Metadata>\n";
        ss << "  <Timestamp precise=\"" << ts_precise << "\">" << timestamp << "</Timestamp>\n";
        ss << "</Message>";
        return ss.str();
    }

    json to_json() const {
        return {
            {"msg_id", msg_id}, {"from", from_agent}, {"to", to},
            {"channel", channel}, {"type", type}, {"priority", static_cast<int>(priority)},
            {"content", content}, {"metadata", metadata}, {"timestamp", timestamp},
            {"ts_precise", ts_precise}
        };
    }
};

struct Ticket {
    std::string id;
    std::string creator;
    std::string assigned_to;
    std::string instruction;
    int priority;
    std::string status; // PENDING, IN_PROGRESS, etc.
    long long created_at;

    std::string to_xml() const {
        std::stringstream ss;
        ss << "<Ticket id=\"" << xml_escape(id) << "\" priority=\"" << priority << "\">\n";
        ss << "  <Creator>" << xml_escape(creator) << "</Creator>\n";
        ss << "  <AssignedTo>" << xml_escape(assigned_to) << "</AssignedTo>\n";
        ss << "  <Instruction><![CDATA[" << instruction << "]]></Instruction>\n";
        ss << "  <Status>" << xml_escape(status) << "</Status>\n";
        ss << "  <CreatedAt>" << created_at << "</CreatedAt>\n";
        ss << "</Ticket>";
        return ss.str();
    }

    json to_json() const {
        return {
            {"id", id}, {"creator", creator}, {"assigned_to", assigned_to},
            {"instruction", instruction}, {"priority", priority},
            {"status", status}, {"created_at", created_at}
        };
    }
};

struct Primitive {
    std::string id;
    std::string type;
    std::string title;
    float impact_weight;
    std::string raw_evidence;

    std::string to_xml() const {
        std::stringstream ss;
        ss << "<Primitive id=\"" << xml_escape(id) << "\" type=\"" << xml_escape(type) << "\">\n";
        ss << "  <Title>" << xml_escape(title) << "</Title>\n";
        ss << "  <ImpactWeight>" << impact_weight << "</ImpactWeight>\n";
        ss << "  <Evidence><![CDATA[" << raw_evidence << "]]></Evidence>\n";
        ss << "</Primitive>";
        return ss.str();
    }
};

struct DependencyEdge {
    std::string source_id;
    std::string target_id;
    std::string type;

    std::string to_xml() const {
        return "<Dependency source=\"" + xml_escape(source_id) + "\" target=\"" + xml_escape(target_id) + "\" type=\"" + xml_escape(type) + "\"/>";
    }
};

struct Finding {
    std::string id;
    std::string agent;
    std::string title;
    std::string severity;
    std::string type;
    std::string description;
    float confidence;
    std::string source_agent;
    std::string raw_stream;
    json metadata;
    long long timestamp;
    std::string ts_precise;

    std::string to_xml() const {
        std::stringstream ss;
        ss << "<Finding id=\"" << xml_escape(id) << "\" severity=\"" << xml_escape(severity) << "\">\n";
        ss << "  <Agent>" << xml_escape(agent) << "</Agent>\n";
        ss << "  <Title>" << xml_escape(title) << "</Title>\n";
        ss << "  <Type>" << xml_escape(type) << "</Type>\n";
        ss << "  <Description><![CDATA[" << description << "]]></Description>\n";
        ss << "  <Confidence>" << confidence << "</Confidence>\n";
        ss << "  <SourceAgent>" << xml_escape(source_agent) << "</SourceAgent>\n";
        ss << "  <RawStream><![CDATA[" << raw_stream << "]]></RawStream>\n";
        ss << "  <Metadata>" << xml_escape(metadata.dump()) << "</Metadata>\n";
        ss << "  <Timestamp precise=\"" << ts_precise << "\">" << timestamp << "</Timestamp>\n";
        ss << "</Finding>";
        return ss.str();
    }

    json to_json() const {
        return {
            {"id", id}, {"agent", agent}, {"title", title},
            {"severity", severity}, {"type", type}, {"description", description},
            {"confidence", confidence}, {"source_agent", source_agent},
            {"raw_stream", raw_stream}, {"metadata", metadata},
            {"timestamp", timestamp}, {"ts_precise", ts_precise}
        };
    }
};

struct AgentState {
    std::string alias;
    std::string status;
    float vram_used_gb;
    int total_tokens;
    std::string current_model;

    std::string to_xml() const {
        std::stringstream ss;
        ss << "<AgentState alias=\"" << xml_escape(alias) << "\">\n";
        ss << "  <Status>" << xml_escape(status) << "</Status>\n";
        ss << "  <VramUsedGB>" << vram_used_gb << "</VramUsedGB>\n";
        ss << "  <TotalTokens>" << total_tokens << "</TotalTokens>\n";
        ss << "  <CurrentModel>" << xml_escape(current_model) << "</CurrentModel>\n";
        ss << "</AgentState>";
        return ss.str();
    }

    json to_json() const {
        return {
            {"alias", alias}, {"status", status},
            {"vram_used_gb", vram_used_gb}, {"total_tokens", total_tokens},
            {"current_model", current_model}
        };
    }
};

struct MapNode {
    std::string id;
    std::string type;
    json metadata;

    std::string to_xml() const {
        std::stringstream ss;
        ss << "<Node id=\"" << xml_escape(id) << "\" type=\"" << xml_escape(type) << "\">\n";
        ss << "  <Metadata>" << xml_escape(metadata.dump()) << "</Metadata>\n";
        ss << "</Node>";
        return ss.str();
    }
};

struct MapEdge {
    std::string id;
    std::string source;
    std::string target;
    std::string type;

    std::string to_xml() const {
        return "<Edge id=\"" + xml_escape(id) + "\" source=\"" + xml_escape(source) + "\" target=\"" + xml_escape(target) + "\" type=\"" + xml_escape(type) + "\"/>";
    }
};

struct DepartmentState {
    std::string name;
    std::string status;
    int active_agents;
    json metadata;

    std::string to_xml() const {
        std::stringstream ss;
        ss << "<Department name=\"" << xml_escape(name) << "\">\n";
        ss << "  <Status>" << xml_escape(status) << "</Status>\n";
        ss << "  <ActiveAgents>" << active_agents << "</ActiveAgents>\n";
        ss << "  <Metadata>" << xml_escape(metadata.dump()) << "</Metadata>\n";
        ss << "</Department>";
        return ss.str();
    }

    json to_json() const {
        return {
            {"name", name}, {"status", status},
            {"active_agents", active_agents}, {"metadata", metadata}
        };
    }
};

struct HypothesisNode {
    std::string id;
    std::string content;
    float confidence;
    std::string status;
    std::vector<std::string> children;

    std::string to_xml() const {
        std::stringstream ss;
        ss << "<HypothesisNode id=\"" << xml_escape(id) << "\" confidence=\"" << confidence << "\">\n";
        ss << "  <Content><![CDATA[" << content << "]]></Content>\n";
        ss << "  <Status>" << xml_escape(status) << "</Status>\n";
        ss << "  <Children>\n";
        for (const auto& child : children) {
            ss << "    <ChildID>" << xml_escape(child) << "</ChildID>\n";
        }
        ss << "  </Children>\n";
        ss << "</HypothesisNode>";
        return ss.str();
    }
};

struct HypothesisTree {
    std::string vector_id;
    std::string root_node_id;
    std::map<std::string, HypothesisNode> nodes;

    std::string to_xml() const {
        std::stringstream ss;
        ss << "<HypothesisTree vector_id=\"" << xml_escape(vector_id) << "\" root=\"" << xml_escape(root_node_id) << "\">\n";
        ss << "  <Nodes>\n";
        for (const auto& [id, node] : nodes) {
            ss << node.to_xml() << "\n";
        }
        ss << "  </Nodes>\n";
        ss << "</HypothesisTree>";
        return ss.str();
    }
};

} // namespace interlock
