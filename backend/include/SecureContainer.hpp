#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace interlock {

/**
 * @brief Independent Cryptographic Motor for the Interlock Assessment IDE.
 *
 * Orchestrates Hybrid Post-Quantum Cryptography (PQC) and Classical Encryption.
 * Implements: RSA-4096 + ML-KEM-768 (Kyber) + ChaCha20-Poly1305.
 */
class SecureContainer {
public:
    struct CryptoConfig {
        std::string rsa_public_key;
        std::string rsa_private_key;
        std::vector<uint8_t> ml_kem_public_key;
        std::vector<uint8_t> ml_kem_private_key;
        std::vector<uint8_t> ml_dsa_public_key;
        std::vector<uint8_t> ml_dsa_private_key;
    };

    struct EncryptedPackage {
        std::vector<uint8_t> ciphertext;
        std::vector<uint8_t> mac;
        std::vector<uint8_t> nonce;
        std::vector<uint8_t> encapsulated_key; // Hybrid (RSA + ML-KEM)
        std::vector<uint8_t> signature;        // ML-DSA-65
    };

    static SecureContainer& getInstance() {
        static SecureContainer instance;
        return instance;
    }

    // --- Core Operations ---

    /**
     * @brief Encapsulates a artifact using the Hybrid PQC + RSA-4096 lock.
     * Includes ML-DSA-65 signing.
     */
    std::optional<EncryptedPackage> protect(const std::string& artifact, const CryptoConfig& config);

    /**
     * @brief Decapsulates and decrypts a artifact.
     * Verifies ML-DSA-65 signature.
     */
    std::optional<std::string> recover(const EncryptedPackage& package, const CryptoConfig& config);

    /**
     * @brief Signs a message using ML-DSA-65.
     */
    std::vector<uint8_t> sign(const std::string& message, const CryptoConfig& config);

    /**
     * @brief Verifies an ML-DSA-65 signature.
     */
    bool verify(const std::string& message, const std::vector<uint8_t>& signature, const CryptoConfig& config);

    // --- Key Management ---
    CryptoConfig generateKeyPair();

    // --- Utilities ---
    std::string deriveKey(const std::vector<uint8_t>& shared_secret, size_t length = 32);

private:
    SecureContainer() = default;

    // Internal helper methods for specific algorithms
    std::vector<uint8_t> encryptChaCha20(const std::string& plaintext, const std::vector<uint8_t>& key, std::vector<uint8_t>& nonce, std::vector<uint8_t>& mac);
    std::string decryptChaCha20(const std::vector<uint8_t>& ciphertext, const std::vector<uint8_t>& key, const std::vector<uint8_t>& nonce, const std::vector<uint8_t>& mac);

    std::vector<uint8_t> hybridKeyExchange(const CryptoConfig& config);
};

} // namespace interlock
