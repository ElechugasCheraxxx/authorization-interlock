#pragma once
#include <string_view>
#include <map>
#include "BlackboardScripts.hpp"
#include "EngineScripts.hpp"
#include "ArsenalScripts.hpp"
#include "AgentScripts.hpp"
#include "RiskScripts.hpp"
#include "ArtifactScripts.hpp"

namespace interlock::embedded {

// Este script es el "Cargador Mágico". Inyecta un buscador en sys.meta_path
inline constexpr std::string_view IMPORT_HOOK = R"python(
import sys
import types
import importlib.machinery

class InterlockEmbeddedLoader:
    def __init__(self, modules_map):
        self.modules = modules_map

    def find_spec(self, fullname, path, target=None):
        if fullname in self.modules:
            return importlib.machinery.ModuleSpec(fullname, self)
        # Soporte para sub-módulos si el padre está en el mapa
        return None

    def create_module(self, spec):
        return None

    def exec_module(self, module):
        source = self.modules[module.__name__]
        exec(source, module.__dict__)

def install_hook(modules_map):
    sys.meta_path.insert(0, InterlockEmbeddedLoader(modules_map))
)python";

inline constexpr std::string_view CORE_MAIN = R"python(
import interlock_core_bind as core

# FastAPI/Uvicorn removed in favor of direct C++ -> Swift Interop.
# Logic is now driven by the C++ Master Core directly.

def run_core():
    # Placeholder for any background Python tasks needed on boot
    pass
)python";

/**
 * Agregador Global de Recursos Embebidos.
 * Combina todos los mapas de scripts para el Import Hook.
 */
inline const std::map<std::string_view, std::string_view> get_all_embedded_modules() {
    std::map<std::string_view, std::string_view> all;

    // Core Main Entry
    all["interlock.core_main"] = CORE_MAIN;

    // Core & Blackboard
    all.insert(BLACKBOARD_MODULES.begin(), BLACKBOARD_MODULES.end());

    // Engine
    all.insert(ENGINE_MODULES.begin(), ENGINE_MODULES.end());

    // Arsenal
    all.insert(ARSENAL_MODULES.begin(), ARSENAL_MODULES.end());

    // Agents
    all.insert(AGENT_MODULES.begin(), AGENT_MODULES.end());

    // Risk
    all.insert(OPSEC_MODULES.begin(), OPSEC_MODULES.end());

    // Payloads
    all.insert(PAYLOAD_MODULES.begin(), PAYLOAD_MODULES.end());

    return all;
}

} // namespace interlock::embedded
