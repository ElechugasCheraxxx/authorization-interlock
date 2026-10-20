#include "PQCAssessor.hpp"
#include "interop/FormalValidators.hpp"
#include <fstream>
#include <regex>
#include <iostream>
#include <algorithm>

namespace interlock {

std::vector<PQCAssessor::VulnerabilityReport> PQCAssessor::explore(const std::string& target_path) {
    std::vector<VulnerabilityReport> reports;
    try {
        if (!std::filesystem::exists(target_path)) return reports;

        for (const auto& entry : std::filesystem::recursive_directory_iterator(target_path)) {
            if (entry.is_regular_file()) {
                auto extension = entry.path().extension();
                if (extension == ".c" || extension == ".cpp" || extension == ".h" || extension == ".hpp" || extension == ".go" || extension == ".rs") {
                    VulnerabilityReport report = analyzeFileForPQC(entry.path());
                    if (!report.vulnerability_type.empty()) {
                        reports.push_back(report);
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[PQCAssessor] Error exploring path: " << e.what() << std::endl;
    }
    return reports;
}

PQCAssessor::VulnerabilityReport PQCAssessor::analyzeFileForPQC(const std::filesystem::path& path) {
    VulnerabilityReport report;
    report.file_path = path.string();

    std::ifstream file(path);
    if (!file.is_open()) return report;

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    // Detection Patterns for PQC Implementations and Flaws
    struct PQCScanner {
        std::string regex_pattern;
        std::string algo;
        std::string type;
        int confidence;
        std::string mitigation;
    };

    std::vector<PQCScanner> scanners = {
        {
            "OQS_KEM_kyber_512", "ML-KEM (Kyber)",
            "Low Security Level (Kyber-512)", 60,
            "Upgrade to Kyber-768 or Kyber-1024 for post-quantum security margins."
        },
        {
            "static const uint8_t seed\\[\\] = \\{", "PQC General",
            "Hardcoded Seed Detection", 85,
            "Seeds for PQC key generation must be sourced from a high-entropy hardware RNG (getrandom/arc4random)."
        },
        {
            "dilithium_sign.*nonce", "ML-DSA (Dilithium)",
            "Potential Nonce Reuse Risk", 40,
            "Ensure Dilithium nonces are never repeated. Use deterministic signing (RFC 6979 style) if possible."
        },
        {
            "rand\\(\\) %", "Entropy",
            "Weak PRNG for PQC", 90,
            "Standard libc rand() is cryptographically insecure. Use OQS_randombytes or OS-native secure RNG."
        },
        {
            "kyber_unpacked_sk", "ML-KEM (Kyber)",
            "Unprotected Secret Key in Memory", 50,
            "Secret keys should be zeroized after use and protected with memory protections (e.g., mlock)."
        }
    };

    for (const auto& scanner : scanners) {
        std::regex re(scanner.regex_pattern);
        std::smatch match;
        if (std::regex_search(content, match, re)) {
            // --- FORMAL VERIFICATION LAYER (Coq Extract) ---
            formal::PQCExplorerValidator::ScanResult res = {
                scanner.algo,
                scanner.confidence,
                (scanner.type == "Weak PRNG for PQC")
            };

            if (!formal::PQCExplorerValidator::isReliableReport(res)) {
                std::cerr << "[PQCAssessor] [FORMAL_DROPPED] Low reliability report for: " << scanner.type << std::endl;
                continue;
            }

            report.algorithm = scanner.algo;
            report.vulnerability_type = scanner.type;
            report.confidence_score = scanner.confidence;
            report.snippet = match.str();
            report.mitigation = scanner.mitigation;
            break; // Return the first high-confidence hit for now
        }
    }

    return report;
}

} // namespace interlock
