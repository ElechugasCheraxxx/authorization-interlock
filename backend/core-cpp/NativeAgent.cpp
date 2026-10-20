#include "NativeAgent.hpp"
#include "PromptEngine.hpp"
#include "ToolDispatcher.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <vector>
#include <sstream>

namespace interlock {

NativeAgent::NativeAgent(const std::string& alias, const std::string& model_path)
    : alias(alias) {
    // Inicializar el motor dedicado para este agente
    engine = std::make_unique<AgentEngine>(alias, model_path);
}

bool NativeAgent::wake() {
    return engine->load();
}

void NativeAgent::hibernate() {
    engine->unload();
}

bool NativeAgent::isReady() const {
    return engine->isLoaded();
}

void NativeAgent::processTask(const Message& msg) {
    // 1. Asegurar que el motor esté despierto (Lazy Loading / Re-wake)
    if (!isReady()) {
        if (!wake()) {
            std::cerr << "[NativeAgent:" << alias << "] FAILED TO WAKE ENGINE" << std::endl;
            return;
        }
    }

    // 2. Construir Prompt XML Dinámico (Nativo)
    std::string prompt = AdaptivePromptEngine::getInstance().buildAgentPrompt(alias, msg.content);

    // 3. Decidir si usar "Deep Think" basándose en la complejidad de la tarea
    bool needs_reasoning = (static_cast<int>(msg.priority) > 3 || msg.content.find("analyze") != std::string::npos);

    InferenceResponse res;
    if (needs_reasoning) {
        std::cout << "[NativeAgent:" << alias << "] Entering Deep Think mode (LTI Injection)..." << std::endl;
        res = engine->deepThink(prompt, getGrammar()).get();
    } else {
        res = engine->think(prompt, getGrammar()).get();
    }

    // 4. Lógica de post-procesamiento específica del agente
    onResponseGenerated(res.text);
}

std::string NativeAgent::executeInMemory(const std::string& interpreter, const std::string& script_content) {
    int pipe_stdin[2];
    int pipe_stdout[2];

    if (pipe(pipe_stdin) == -1 || pipe(pipe_stdout) == -1) {
        return "ERROR: Failed to create pipes";
    }

    pid_t pid = fork();
    if (pid == 0) {
        // Proceso Hijo
        close(pipe_stdin[1]);
        close(pipe_stdout[0]);

        dup2(pipe_stdin[0], STDIN_FILENO);
        dup2(pipe_stdout[1], STDOUT_FILENO);
        dup2(pipe_stdout[1], STDERR_FILENO);

        char* const args[] = {const_cast<char*>(interpreter.c_str()), nullptr};
        execvp(args[0], args);
        exit(1);
    } else if (pid > 0) {
        // Proceso Padre
        close(pipe_stdin[0]);
        close(pipe_stdout[1]);

        write(pipe_stdin[1], script_content.c_str(), script_content.size());
        close(pipe_stdin[1]);

        std::stringstream output;
        char buffer[4096];
        ssize_t bytes_read;
        while ((bytes_read = read(pipe_stdout[0], buffer, sizeof(buffer))) > 0) {
            output.write(buffer, bytes_read);
        }
        close(pipe_stdout[0]);

        waitpid(pid, nullptr, 0);
        return output.str();
    } else {
        return "ERROR: Fork failed";
    }
}

} // namespace interlock
