#pragma once

#include <string>
#include <vector>
#include <optional>

namespace interlock {
namespace formal {

// This header defines the interface for the OCaml code extracted from Coq.
// In a full build, these are implemented in a shared library generated from OCaml.

struct PrimitiveData {
    std::string id;
    int impact_weight;
    bool is_logic_flaw;
};

class PDGValidator {
public:
    static bool isValidChain(const std::vector<PrimitiveData>& chain);
    static bool isAcyclic(const std::vector<PrimitiveData>& chain);
};

class StateMachineValidator {
public:
    enum class Phase { INIT, BRIEFING, RECON, ANALYSIS, VULN_RESEARCH, EXPLOITATION, POST_EXPLOITATION, POST_OP, REPORTING, CLEANUP, PAUSED, CLOSED };
    static bool isLegalTransition(Phase from, Phase to);
    static bool isValidPath(Phase start, const std::vector<Phase>& path);
};

class SovereignBridgeValidator {
public:
    struct ProcessData {
        uint32_t pid;
        bool is_hidden;
        bool is_protected;
        bool is_opaque;
    };
    static bool isProcessSafe(const ProcessData& p);
    static bool isSafeToInject(uint32_t target_pid, uint32_t core_pid);
};

class DataSurgeryValidator {
public:
    struct ArtifactData {
        std::string id;
        std::string source_node;
        std::string integrity_hash;
        bool is_corrupted;
    };
    static bool verifyExtraction(const std::string& raw, const ArtifactData& art);
};

class MessageRouterValidator {
public:
    struct MessageData {
        std::string from_agent;
        std::string to_agent;
        std::string channel;
    };
    static bool isValidRoute(const std::string& from, const std::string& to, const std::string& channel);
};

class PQCExplorerValidator {
public:
    struct ScanResult {
        std::string algo;
        int confidence;
        bool is_weak_entropy;
    };
    static bool isReliableReport(const ScanResult& res);
};

class MathValidator {
public:
    static bool verifyFixedPointOp(int a, int b, int result, const std::string& op);
};

class PoolValidator {
public:
    static bool isHandleValid(uint32_t handle, size_t capacity);
    static bool verifyAlignment(uintptr_t address, size_t alignment);
};

class ChoreographyValidator {
public:
    static bool verifyTransition(const std::string& choreo_name, const std::string& from_phase, const std::string& to_phase);
    static bool verifyPurge(const std::string& buffer_id);
};

} // namespace formal
} // namespace interlock
