#pragma once

#include "Schema.hpp"
#include <string>
#include <vector>
#include <map>
#include <mutex>

namespace interlock {

class HypothesisEngine {
public:
    static HypothesisEngine& getInstance() {
        static HypothesisEngine instance;
        return instance;
    }

    void generateHypotheses(const std::string& context);
    void updateConfidence(const std::string& hypothesis_id, float delta);
    HypothesisTree getTree() const;

private:
    HypothesisEngine() = default;
    HypothesisTree tree;
    mutable std::mutex mtx;
};

class DebateEngine {
public:
    static DebateEngine& getInstance() {
        static DebateEngine instance;
        return instance;
    }

    std::string resolveDebate(const std::string& topic, const std::vector<std::string>& arguments);

private:
    DebateEngine() = default;
};

class BinaryAnalysisEngine {
public:
    static BinaryAnalysisEngine& getInstance() {
        static BinaryAnalysisEngine instance;
        return instance;
    }

    json analyzeBinary(const std::string& path);
    void mapToPDG(const std::string& binary_id);

private:
    BinaryAnalysisEngine() = default;
};

} // namespace interlock
