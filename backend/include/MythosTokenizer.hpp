#pragma once
#include <string>
#include <vector>
#include "llama.h"

namespace interlock {

/**
 * MythosTokenizer (Native BPE)
 * Reemplaza la dependencia de HuggingFace/Python.
 * Utiliza el vocabulario del modelo cargado en el AgentEngine.
 */
class MythosTokenizer {
public:
    MythosTokenizer(const struct llama_model* model);

    std::vector<llama_token> encode(const std::string& text, bool add_bos = true, bool parse_special = true);
    std::string decode(const std::vector<llama_token>& tokens, bool skip_special = true);

    int vocab_size() const;

private:
    const struct llama_model* model;
};

} // namespace interlock
