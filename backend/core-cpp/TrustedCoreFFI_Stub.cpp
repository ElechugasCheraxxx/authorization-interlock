#include "interop/TrustedCoreFFI.h"
#include <iostream>

namespace interlock {

void init_ocaml_runtime(char** argv) {
    // Stub implementation
}

bool ocaml_is_process_safe(uint32_t pid, bool is_hidden, bool is_protected, bool is_opaque) {
    return true; // Default safe
}

bool ocaml_is_safe_to_inject(uint32_t target_pid, uint32_t core_pid) {
    return true;
}

bool ocaml_pdg_is_acyclic(const std::vector<std::string>& ids) {
    return true;
}

bool ocaml_is_legal_transition(int from, int to) {
    return true;
}

bool ocaml_verify_extraction(const std::string& art_id, const std::string& source_node, const std::string& integrity_hash, bool is_corrupted) {
    return !is_corrupted;
}

bool ocaml_is_valid_route(const std::string& from_agent, const std::string& to_agent, const std::string& channel) {
    return true;
}

bool ocaml_pqc_is_vulnerable(const std::string& algo, int vuln_type_idx) {
    return false;
}

int ocaml_certified_add(int a, int b) {
    return a + b;
}

bool ocaml_verify_handle(uint32_t handle, uint32_t capacity) {
    uint32_t index = handle >> 8;
    return index < capacity;
}

} // namespace interlock
