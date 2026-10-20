#pragma once

#include <cstdint>
#include <vector>
#include <string>

namespace interlock {

#ifdef __cplusplus
extern "C" {
#endif

// Initialization function for OCaml runtime
void init_ocaml_runtime(char** argv);

// Sovereign Bridge Wrappers
bool ocaml_is_process_safe(uint32_t pid, bool is_hidden, bool is_protected, bool is_opaque);
bool ocaml_is_safe_to_inject(uint32_t target_pid, uint32_t core_pid);

// State Machine Wrappers
bool ocaml_is_legal_transition(int from, int to);

#ifdef __cplusplus
}
#endif

// PDG Wrappers
bool ocaml_pdg_is_acyclic(const std::vector<std::string>& ids);

// Data Surgery Wrappers
bool ocaml_verify_extraction(const std::string& art_id, const std::string& source_node, const std::string& integrity_hash, bool is_corrupted);

// Message Router Wrappers
bool ocaml_is_valid_route(const std::string& from_agent, const std::string& to_agent, const std::string& channel);

// PQC Explorer Wrappers
bool ocaml_pqc_is_vulnerable(const std::string& algo, int vuln_type_idx);

// Math Wrappers
int ocaml_certified_add(int a, int b);

// Pool Memory Wrappers (Isabelle-backed)
bool ocaml_verify_handle(uint32_t handle, uint32_t capacity);

// Choreography Wrappers
bool ocaml_verify_choreo_step(const std::string& choreo_name, const std::string& from_phase, const std::string& to_phase);
bool ocaml_verify_buffer_purge(const std::string& buffer_id);

} // namespace interlock
