#pragma once
#include "NativeAgent.hpp"
#include "GrammarLibrary.hpp"

namespace interlock {

class RexAgent : public NativeAgent {
public:
    RexAgent(const std::string& model_path);
protected:
    void onResponseGenerated(const std::string& raw_output) override;
    std::string getGrammar() const override { return GrammarLibrary::REX_GBNF; }
};

} // namespace interlock
