#include "interop/TrustedCoreFFI.h"
#include <caml/mlvalues.h>
#include <caml/callback.h>
#include <caml/alloc.h>
#include <caml/memory.h>
#include <iostream>

namespace interlock {

void init_ocaml_runtime(char** argv) {
    static bool initialized = false;
    if (!initialized) {
        caml_main(argv);
        initialized = true;
    }
}

bool ocaml_is_process_safe(uint32_t pid, bool is_hidden, bool is_protected, bool is_opaque) {
    static const value* f = nullptr;
    if (f == nullptr) {
        f = caml_named_value("ffi_is_process_safe");
    }

    if (f == nullptr) {
        std::cerr << "OCaml function 'ffi_is_process_safe' not found!" << std::endl;
        return true; // Safe default or panic?
    }

    value res = caml_callbackN(*f, 4, (value[]){
        Val_int(pid),
        Val_bool(is_hidden),
        Val_bool(is_protected),
        Val_bool(is_opaque)
    });

    return Bool_val(res);
}

bool ocaml_is_safe_to_inject(uint32_t target_pid, uint32_t core_pid) {
    static const value* f = nullptr;
    if (f == nullptr) {
        f = caml_named_value("ffi_is_safe_to_inject");
    }

    if (f == nullptr) {
        std::cerr << "OCaml function 'ffi_is_safe_to_inject' not found!" << std::endl;
        return false;
    }

    value res = caml_callback2(*f, Val_int(target_pid), Val_int(core_pid));
    return Bool_val(res);
}

bool ocaml_pdg_is_acyclic(const std::vector<std::string>& ids) {
    static const value* f = nullptr;
    if (f == nullptr) {
        f = caml_named_value("ffi_pdg_is_acyclic");
    }

    if (f == nullptr) return true;

    CAMLparam0();
    CAMLlocal2(list, cons);
    list = Val_int(0); // []
    for (auto it = ids.rbegin(); it != ids.rend(); ++it) {
        cons = caml_alloc(2, 0);
        Store_field(cons, 0, caml_copy_string(it->c_str()));
        Store_field(cons, 1, list);
        list = cons;
    }

    value res = caml_callback(*f, list);
    bool result = Bool_val(res);
    CAMLreturnT(bool, result);
}

bool ocaml_is_legal_transition(int from, int to) {
    static const value* f = nullptr;
    if (f == nullptr) {
        f = caml_named_value("ffi_sm_is_legal_transition");
    }

    if (f == nullptr) return false;

    value res = caml_callback2(*f, Val_int(from), Val_int(to));
    return Bool_val(res);
}

bool ocaml_verify_extraction(const std::string& art_id, const std::string& source_node, const std::string& integrity_hash, bool is_corrupted) {
    static const value* f = nullptr;
    if (f == nullptr) {
        f = caml_named_value("ffi_verify_extraction");
    }

    if (f == nullptr) return false;

    CAMLparam0();
    CAMLlocal3(ml_art_id, ml_source, ml_hash);
    ml_art_id = caml_copy_string(art_id.c_str());
    ml_source = caml_copy_string(source_node.c_str());
    ml_hash = caml_copy_string(integrity_hash.c_str());

    value res = caml_callbackN(*f, 4, (value[]){
        ml_art_id,
        ml_source,
        ml_hash,
        Val_bool(is_corrupted)
    });

    CAMLreturnT(bool, Bool_val(res));
}

bool ocaml_is_valid_route(const std::string& from_agent, const std::string& to_agent, const std::string& channel) {
    static const value* f = nullptr;
    if (f == nullptr) {
        f = caml_named_value("ffi_is_valid_route");
    }

    if (f == nullptr) return false;

    CAMLparam0();
    CAMLlocal3(ml_from, ml_to, ml_chan);
    ml_from = caml_copy_string(from_agent.c_str());
    ml_to = caml_copy_string(to_agent.c_str());
    ml_chan = caml_copy_string(channel.c_str());

    value res = caml_callback3(*f, ml_from, ml_to, ml_chan);
    CAMLreturnT(bool, Bool_val(res));
}

bool ocaml_pqc_is_vulnerable(const std::string& algo, int vuln_type_idx) {
    static const value* f = nullptr;
    if (f == nullptr) {
        f = caml_named_value("ffi_pqc_is_vulnerable");
    }

    if (f == nullptr) return false;

    CAMLparam0();
    CAMLlocal1(ml_algo);
    ml_algo = caml_copy_string(algo.c_str());

    value res = caml_callback2(*f, ml_algo, Val_int(vuln_type_idx));
    CAMLreturnT(bool, Bool_val(res));
}

int ocaml_certified_add(int a, int b) {
    static const value* f = nullptr;
    if (f == nullptr) f = caml_named_value("ffi_certified_add");
    if (f == nullptr) return a + b;
    return Int_val(caml_callback2(*f, Val_int(a), Val_int(b)));
}

bool ocaml_verify_handle(uint32_t handle, uint32_t capacity) {
    static const value* f = nullptr;
    if (f == nullptr) f = caml_named_value("ffi_verify_handle");
    if (f == nullptr) return (handle >> 8) < capacity;
    return Bool_val(caml_callback2(*f, Val_int(handle), Val_int(capacity)));
}

bool ocaml_verify_choreo_step(const std::string& choreo_name, const std::string& from_phase, const std::string& to_phase) {
    static const value* f = nullptr;
    if (f == nullptr) f = caml_named_value("ffi_verify_choreo_step");
    if (f == nullptr) return true;

    CAMLparam0();
    CAMLlocal3(ml_name, ml_from, ml_to);
    ml_name = caml_copy_string(choreo_name.c_str());
    ml_from = caml_copy_string(from_phase.c_str());
    ml_to = caml_copy_string(to_phase.c_str());

    value res = caml_callback3(*f, ml_name, ml_from, ml_to);
    CAMLreturnT(bool, Bool_val(res));
}

bool ocaml_verify_buffer_purge(const std::string& buffer_id) {
    static const value* f = nullptr;
    if (f == nullptr) f = caml_named_value("ffi_verify_buffer_purge");
    if (f == nullptr) return true;

    CAMLparam0();
    CAMLlocal1(ml_id);
    ml_id = caml_copy_string(buffer_id.c_str());

    value res = caml_callback(*f, ml_id);
    CAMLreturnT(bool, Bool_val(res));
}

} // namespace interlock
