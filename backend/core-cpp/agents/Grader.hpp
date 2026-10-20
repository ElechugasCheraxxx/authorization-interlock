#pragma once
#include "NativeAgent.hpp"
#include "GrammarLibrary.hpp"

namespace interlock {

class GraderAgent : public NativeAgent {
public:
    GraderAgent(const std::string& model_path);
protected:
    void onResponseGenerated(const std::string& raw_output) override;
    std::string getGrammar() const override { return GrammarLibrary::GRADER_GBNF; }
};

} // namespace interlock
