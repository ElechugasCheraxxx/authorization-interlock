#pragma once
#include "cipher_pqc.h"
#include <vector>
#include <string>
#include <memory>
#include <stdexcept>

namespace interlock::crypto {

class PqcKey {
public:
    PqcKey(PqcKeyPair kp) : m_kp(kp) {}
    ~PqcKey() { rust_mlkem768_free_keypair(m_kp); }

    const uint8_t* pk() const { return m_kp.pk; }
    size_t pk_len() const { return m_kp.pk_len; }
    const uint8_t* sk() const { return m_kp.sk; }
    size_t sk_len() const { return m_kp.sk_len; }

private:
    PqcKeyPair m_kp;
};

class CipherPQC {
public:
    static std::unique_ptr<PqcKey> generate_kem_keypair() {
        return std::make_unique<PqcKey>(rust_mlkem768_keygen());
    }

    static std::vector<uint8_t> encapsulate(const uint8_t* pk, size_t pk_len, std::vector<uint8_t>& out_ss) {
        auto res = rust_mlkem768_encapsulate(pk, pk_len);
        if (!res.ct || !res.ss) {
            rust_mlkem768_free_encapsulation(res);
            throw std::runtime_error("PQC Encapsulation failed");
        }

        std::vector<uint8_t> ct(res.ct, res.ct + res.ct_len);
        out_ss.assign(res.ss, res.ss + res.ss_len);

        rust_mlkem768_free_encapsulation(res);
        return ct;
    }

    static std::vector<uint8_t> decapsulate(const uint8_t* ct, size_t ct_len, const uint8_t* sk, size_t sk_len) {
        uint8_t* ss_ptr = rust_mlkem768_decapsulate(ct, ct_len, sk, sk_len);
        if (!ss_ptr) throw std::runtime_error("PQC Decapsulation failed");

        // ML-KEM-768 shared secret is 32 bytes
        std::vector<uint8_t> ss(ss_ptr, ss_ptr + 32);
        rust_mlkem768_free_buffer(ss_ptr, 32);
        return ss;
    }
};

} // namespace interlock::crypto
