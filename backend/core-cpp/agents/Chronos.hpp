#pragma once
#include "NativeAgent.hpp"
#include "GrammarLibrary.hpp"

namespace interlock {

class ChronosAgent : public NativeAgent {
public:
    ChronosAgent(const std::string& model_path);
protected:
    void onResponseGenerated(const std::string& raw_output) override;
    std::string getGrammar() const override { return GrammarLibrary::CHRONOS_GBNF; }
};

} // namespace interlock
