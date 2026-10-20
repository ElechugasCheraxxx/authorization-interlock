#include "SecureContainer.hpp"
#include "interop/CipherPQC.hpp"
#include "RiskManager.hpp"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <iostream>
#include <stdexcept>

namespace interlock {

using namespace interlock::crypto;

std::vector<uint8_t> SecureContainer::sign(const std::string& message, const CryptoConfig& config) {
    if (config.ml_dsa_private_key.empty()) {
        throw std::runtime_error("ML-DSA private key missing");
    }

    auto sig_res = rust_mldsa65_sign(
        reinterpret_cast<const uint8_t*>(message.data()), message.size(),
        config.ml_dsa_private_key.data(), config.ml_dsa_private_key.size()
    );

    if (!sig_res.sig) throw std::runtime_error("ML-DSA signing failed");

    std::vector<uint8_t> signature(sig_res.sig, sig_res.sig + sig_res.sig_len);
    rust_mldsa65_free_signature(sig_res);
    return signature;
}

bool SecureContainer::verify(const std::string& message, const std::vector<uint8_t>& signature, const CryptoConfig& config) {
    return rust_mldsa65_verify(
        reinterpret_cast<const uint8_t*>(message.data()), message.size(),
        signature.data(), signature.size(),
        config.ml_dsa_public_key.data(), config.ml_dsa_public_key.size()
    );
}

std::optional<SecureContainer::EncryptedPackage> SecureContainer::protect(const std::string& artifact, const CryptoConfig& config) {
    try {
        EncryptedPackage package;
        std::vector<uint8_t> shared_secret;

        std::cout << "[DEBUG] Protecting artifact, public key size: " << config.ml_kem_public_key.size() << std::endl;

        // 1. Hybrid KEM (ML-KEM-768)
        package.encapsulated_key = CipherPQC::encapsulate(
            config.ml_kem_public_key.data(), config.ml_kem_public_key.size(), shared_secret
        );

        std::cout << "[DEBUG] Encapsulation done, encapsulated key size: " << package.encapsulated_key.size() << ", shared secret size: " << shared_secret.size() << std::endl;

        // 2. Derive ChaCha20 key from Shared Secret using HKDF (simplified here for brevity)
        std::vector<uint8_t> symmetric_key = shared_secret; // In prod: use HKDF-SHA3-256

        // 3. Encrypt Artifact with ChaCha20-Poly1305
        package.nonce.resize(12);
        RAND_bytes(package.nonce.data(), package.nonce.size());
        package.mac.resize(16);

        package.ciphertext = encryptChaCha20(artifact, symmetric_key, package.nonce, package.mac);

        // 4. ML-DSA-65 Signing
        package.signature = sign(artifact, config);

        return package;
    } catch (const std::exception& e) {
        std::cerr << "[SecureContainer] Production Error: " << e.what() << std::endl;
        return std::nullopt;
    }
}

std::optional<std::string> SecureContainer::recover(const EncryptedPackage& package, const CryptoConfig& config) {
    try {
        std::cout << "[DEBUG] Starting recovery..." << std::endl;
        std::cout << "[DEBUG] ML-KEM Private Key size: " << config.ml_kem_private_key.size() << std::endl;
        std::cout << "[DEBUG] Encapsulated Key size: " << package.encapsulated_key.size() << std::endl;

        // 1. Decapsulate symmetric key
        std::vector<uint8_t> shared_secret = CipherPQC::decapsulate(
            package.encapsulated_key.data(), package.encapsulated_key.size(),
            config.ml_kem_private_key.data(), config.ml_kem_private_key.size()
        );

        if (shared_secret.empty()) {
            std::cerr << "[SecureContainer] Decapsulation failed. CT size: " << package.encapsulated_key.size() << ", SK size: " << config.ml_kem_private_key.size() << std::endl;
            return std::nullopt;
        }
        std::cout << "[DEBUG] Shared secret recovered, size: " << shared_secret.size() << std::endl;

        // 2. Decrypt with ChaCha20-Poly1305
        std::string plaintext = decryptChaCha20(package.ciphertext, shared_secret, package.nonce, package.mac);

        // Security Hook: Zero-Return Poisoning
        if (RiskManager::getInstance().checkAntiDebugging()) {
            std::cerr << "[SecureContainer] !!! POISONING TRIGGERED !!! Debugger detected during recovery." << std::endl;
            // Return NOP-filled buffer of the same size to mislead analysis
            return std::string(plaintext.size(), '\x90');
        }

        std::cout << "[DEBUG] Decryption successful, plaintext size: " << plaintext.size() << std::endl;

        // 3. Verify ML-DSA-65 Signature
        if (!verify(plaintext, package.signature, config)) {
            std::cerr << "[SecureContainer] ML-DSA Signature verification failed" << std::endl;
            return std::nullopt;
        }

        return plaintext;
    } catch (const std::exception& e) {
        std::cerr << "[SecureContainer] Recovery error: " << e.what() << std::endl;
        return std::nullopt;
    }
}

std::vector<uint8_t> SecureContainer::encryptChaCha20(const std::string& plaintext, const std::vector<uint8_t>& key, std::vector<uint8_t>& nonce, std::vector<uint8_t>& mac) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    std::vector<uint8_t> ciphertext(plaintext.size());
    int len;

    EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, NULL, NULL);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_IVLEN, nonce.size(), NULL);
    EVP_EncryptInit_ex(ctx, NULL, NULL, key.data(), nonce.data());

    EVP_EncryptUpdate(ctx, ciphertext.data(), &len, reinterpret_cast<const uint8_t*>(plaintext.data()), plaintext.size());
    EVP_EncryptFinal_ex(ctx, mac.data(), &len);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, mac.data());

    EVP_CIPHER_CTX_free(ctx);
    return ciphertext;
}

std::string SecureContainer::decryptChaCha20(const std::vector<uint8_t>& ciphertext, const std::vector<uint8_t>& key, const std::vector<uint8_t>& nonce, const std::vector<uint8_t>& mac) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    std::vector<uint8_t> plaintext(ciphertext.size());
    int len;

    EVP_DecryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, NULL, NULL);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_IVLEN, nonce.size(), NULL);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_TAG, 16, const_cast<uint8_t*>(mac.data()));
    EVP_DecryptInit_ex(ctx, NULL, NULL, key.data(), nonce.data());

    EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext.data(), ciphertext.size());

    if (EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len) <= 0) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Decryption integrity check FAILED");
    }

    EVP_CIPHER_CTX_free(ctx);
    return std::string(plaintext.begin(), plaintext.end());
}

SecureContainer::CryptoConfig SecureContainer::generateKeyPair() {
    CryptoConfig config;
    auto kem_kp = rust_mlkem768_keygen();
    config.ml_kem_public_key.assign(kem_kp.pk, kem_kp.pk + kem_kp.pk_len);
    config.ml_kem_private_key.assign(kem_kp.sk, kem_kp.sk + kem_kp.sk_len);

    rust_mlkem768_free_keypair(kem_kp);

    // ML-DSA Keygen
    auto dsa_kp = rust_mldsa65_keygen();
    config.ml_dsa_public_key.assign(dsa_kp.pk, dsa_kp.pk + dsa_kp.pk_len);
    config.ml_dsa_private_key.assign(dsa_kp.sk, dsa_kp.sk + dsa_kp.sk_len);
    rust_mlkem768_free_keypair(dsa_kp); // rust_mlkem768_free_keypair works for both since same struct layout

    return config;
}

} // namespace interlock
