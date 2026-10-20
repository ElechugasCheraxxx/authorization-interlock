#pragma once
#include "NativeAgent.hpp"
#include "GrammarLibrary.hpp"

namespace interlock {

class AtlasAgent : public NativeAgent {
public:
    AtlasAgent(const std::string& model_path);
protected:
    void onResponseGenerated(const std::string& raw_output) override;
    std::string getGrammar() const override { return GrammarLibrary::ATLAS_GBNF; }
};

} // namespace interlock
