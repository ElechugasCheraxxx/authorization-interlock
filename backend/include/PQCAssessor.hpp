#pragma once

#include <string>
#include <vector>
#include <filesystem>

namespace interlock {

/**
 * @brief Identifies flaws in Kyber/Dilithium implementations in third-party software.
 * Implementation of Milestone 5.2 of the Glasswing Masterplan.
 */
class PQCAssessor {
public:
    struct VulnerabilityReport {
        std::string file_path;
        std::string algorithm; // ML-KEM (Kyber), ML-DSA (Dilithium)
        std::string vulnerability_type;
        int confidence_score; // 1-100
        std::string snippet;
        std::string mitigation;
    };

    static PQCAssessor& getInstance() {
        static PQCAssessor instance;
        return instance;
    }

    /**
     * @brief Scans a codebase for PQC implementation flaws.
     */
    std::vector<VulnerabilityReport> explore(const std::string& target_path);

private:
    PQCAssessor() = default;

    VulnerabilityReport analyzeFileForPQC(const std::filesystem::path& path);
};

} // namespace interlock
