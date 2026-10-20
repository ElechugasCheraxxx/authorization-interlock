#pragma once
#include "NativeAgent.hpp"
#include "GrammarLibrary.hpp"

namespace interlock {

class CipherAgent : public NativeAgent {
public:
    CipherAgent(const std::string& model_path);
protected:
    void onResponseGenerated(const std::string& raw_output) override;
    std::string getGrammar() const override { return GrammarLibrary::CIPHER_GBNF; }
};

} // namespace interlock
