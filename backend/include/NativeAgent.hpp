#pragma once

#include "Schema.hpp"
#include "AgentEngine.hpp"
#include <string>
#include <vector>
#include <future>
#include <memory>

namespace interlock {

/**
 * NativeAgent (M4 Pro Hardened)
 * Base class for agents whose logic lives in C++.
 * Each agent owns a dedicated AgentEngine (llama_context).
 */
class NativeAgent {
public:
    NativeAgent(const std::string& alias, const std::string& model_path);
    virtual ~NativeAgent() = default;

    // Ciclo de vida del motor independiente
    virtual bool wake();
    virtual void hibernate();
    bool isReady() const;

    // Procesa una tarea de forma nativa usando su motor dedicado
    virtual void processTask(const Message& msg);

protected:
    std::string alias;
    std::unique_ptr<AgentEngine> engine;

    // Obtiene la gramática GBNF específica del agente
    virtual std::string getGrammar() const { return ""; }

    /**
     * Ejecuta código (Python/Ruby/etc) directamente vía STDIN (Heredoc-style)
     * SIN guardar en disco y SIN usar /bin/sh.
     */
    std::string executeInMemory(const std::string& interpreter, const std::string& script_content);

    // Lógica específica de cada agente (Reemplaza a los antiguos .py)
    virtual void onResponseGenerated(const std::string& raw_output) = 0;

    // Streaming support
    long long last_used_timestamp = 0;
};

} // namespace interlock
