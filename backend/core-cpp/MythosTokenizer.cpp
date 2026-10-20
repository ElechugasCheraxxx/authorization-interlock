#include "MythosTokenizer.hpp"
#include "common.h"
#include <stdexcept>

namespace interlock {

MythosTokenizer::MythosTokenizer(const struct llama_model* model) : model(model) {
    if (!model) {
        throw std::runtime_error("MythosTokenizer: model cannot be null");
    }
}

std::vector<llama_token> MythosTokenizer::encode(const std::string& text, bool add_bos, bool parse_special) {
    const struct llama_vocab* vocab = llama_model_get_vocab(model);
    return common_tokenize(vocab, text, add_bos, parse_special);
}

std::string MythosTokenizer::decode(const std::vector<llama_token>& tokens, bool skip_special) {
    const struct llama_vocab* vocab = llama_model_get_vocab(model);
    std::string result;
    for (auto token : tokens) {
        char buf[128];
        int n = llama_token_to_piece(vocab, token, buf, sizeof(buf), 0, !skip_special);
        if (n > 0) {
            result.append(buf, n);
        }
    }
    return result;
}

int MythosTokenizer::vocab_size() const {
    return llama_vocab_n_tokens(llama_model_get_vocab(model));
}

} // namespace interlock
