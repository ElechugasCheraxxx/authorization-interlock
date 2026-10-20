#pragma once

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include "Schema.hpp"

namespace interlock {

struct GhidraAnalysis {
    std::string function_name;
    std::string decompiled_code;
    std::vector<std::string> symbols;
    std::map<uint64_t, std::string> cross_references;
};

/**
 * GhidraBridge: Native bridge for on-demand binary analysis.
 * Integrates with Ghidra Headless Analyzer or a running Ghidra Server.
 */
class GhidraBridge {
public:
    static GhidraBridge& getInstance() {
        static GhidraBridge instance;
        return instance;
    }

    // Inicializa el entorno de Ghidra (rutas a scripts, headless analyzer, etc.)
    bool initialize(const std::string& ghidra_path, const std::string& project_path);

    // Analiza un binario completo y extrae metadatos iniciales
    bool analyzeBinary(const std::string& binary_path);

    // Descompila una función específica bajo demanda del agente
    std::string decompileFunction(const std::string& binary_path, const std::string& function_name);

    // Busca vulnerabilidades conocidas en el binario usando scripts de Ghidra (Python/Java)
    GhidraAnalysis runScript(const std::string& script_name, const std::string& binary_path);

    // Exporta el mapa de memoria para Context Injection (Fase 3.1)
    std::map<uint64_t, std::string> getMemoryMap(const std::string& binary_path);

private:
    GhidraBridge() = default;

    std::string ghidra_home;
    std::string current_project;
    std::mutex mtx;

    bool is_initialized = false;
};

} // namespace interlock
