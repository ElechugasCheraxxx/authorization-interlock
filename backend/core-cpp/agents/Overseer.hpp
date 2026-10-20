#pragma once
#include "NativeAgent.hpp"
#include "GrammarLibrary.hpp"

namespace interlock {

class OverseerAgent : public NativeAgent {
public:
    OverseerAgent(const std::string& model_path);
protected:
    void onResponseGenerated(const std::string& raw_output) override;
    std::string getGrammar() const override { return GrammarLibrary::OVERSEER_GBNF; }
};

} // namespace interlock
