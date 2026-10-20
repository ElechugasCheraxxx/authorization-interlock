#pragma once
#include "NativeAgent.hpp"
#include "GrammarLibrary.hpp"

namespace interlock {

class ArchiveAgent : public NativeAgent {
public:
    ArchiveAgent(const std::string& model_path);
protected:
    void onResponseGenerated(const std::string& raw_output) override;
    std::string getGrammar() const override { return GrammarLibrary::ARCHIVE_GBNF; }
};

} // namespace interlock
