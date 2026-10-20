#pragma once

#include <string>
#include <mutex>
#include <nlohmann/json.hpp>

// Usaremos nanobind/pybind11 conceptualmente, pero definimos la interfaz
namespace interlock {

class PythonBridge {
public:
    static PythonBridge& getInstance() {
        static PythonBridge instance;
        return instance;
    }

    // Inicializa el intérprete y añade 'backend/' al sys.path
    void initialize(const std::string& python_path = ".");
    void finalize();

    // Ejecuta una función en un módulo Python y devuelve el resultado como JSON
    // Ideal para: "Llamar a un agente de Python desde el Router de C++"
    nlohmann::json call(const std::string& module, const std::string& function, const nlohmann::json& args);

    // Ejecuta un script arbitrario (Hot-reloading de exploits)
    void executeString(const std::string& code);

private:
    PythonBridge() = default;
    ~PythonBridge() { finalize(); }

    bool initialized = false;
    std::mutex interpreter_mtx;
};

} // namespace interlock
