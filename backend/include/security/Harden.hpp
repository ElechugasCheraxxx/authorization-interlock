#pragma once

#include <string>
#include <vector>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <cstring>
#include <stdexcept>
#include <sstream>
#include <iomanip>

namespace interlock::security {

/**
 * @brief Logic adapted from referencia previa TunablesVerifier.
 * Uses SHA-256 to verify the integrity of sensitive buffers (C2 configs, gRPC payloads).
 */
class IntegrityVerifier {
public:
    static std::string calculateHash(const void* data, size_t size) {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256_CTX sha256;
        SHA256_Init(&sha256);
        SHA256_Update(&sha256, data, size);
        SHA256_Final(hash, &sha256);

        char result[SHA256_DIGEST_LENGTH * 2 + 1];
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            sprintf(result + (i * 2), "%02x", hash[i]);
        }
        return std::string(result);
    }

    static bool verify(const void* data, size_t size, const std::string& expected_hash) {
        return calculateHash(data, size) == expected_hash;
    }

    /**
     * @brief Custom Jenkins Hash implementation (0x04c11db7).
     * Ported from referencia previa ragesechashutility.h for high-speed memory integrity.
     */
    static uint32_t calculateJenkinsHash(const uint8_t* data, size_t len) {
        uint32_t hash = 0x04c11db7; // referencia previa Initial Seed
        for (size_t i = 0; i < len; ++i) {
            hash += data[i];
            hash += (hash << 10);
            hash ^= (hash >> 6);
        }
        hash += (hash << 3);
        hash ^= (hash >> 11);
        hash += (hash << 15);
        return hash;
    }

    /**
     * @brief Ported from referencia previa TunablesVerifierPlugin_Work logic.
     * Iterates through a structured buffer and hashes its key components.
     */
    template<typename T>
    static std::string hashSequence(const std::vector<T>& items, int version) {
        SHA256_CTX sha256;
        SHA256_Init(&sha256);

        SHA256_Update(&sha256, &version, sizeof(version));

        for (const auto& item : items) {
            // Analogous to hashing MemoryCheck fields
            SHA256_Update(&sha256, &item, sizeof(T));
        }

        int count = static_cast<int>(items.size());
        SHA256_Update(&sha256, &count, sizeof(count));

        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256_Final(hash, &sha256);

        char result[SHA256_DIGEST_LENGTH * 2 + 1];
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            sprintf(result + (i * 2), "%02x", hash[i]);
        }
        return std::string(result);
    }

    /**
     * @brief Generates a cryptographically secure 64-bit random ID.
     * Inspired by referencia previa uuid64.c using BCryptGenRandom/OpenSSL RAND_bytes.
     */
    static std::string generateSecureID() {
        uint64_t random_val;
        if (RAND_bytes(reinterpret_cast<unsigned char*>(&random_val), sizeof(random_val)) != 1) {
            // Fallback for M4 Pro hardware entropy if OpenSSL fails
            random_val = static_cast<uint64_t>(std::chrono::system_clock::now().time_since_epoch().count());
        }

        std::stringstream ss;
        ss << std::hex << std::setw(16) << std::setfill('0') << random_val;
        return ss.str();
    }
};

/**
 * @brief Self-Preservation triggers ported from RageSec.
 */
class TamperResponse {
public:
    static void induceCrash() {
        // Port of RageSecInduceStreamerCrash concept
        // In Shadow-Mover (Zig), this will trigger a memory violation or GBuffer corruption
        #ifdef __APPLE__
            // Trigger a Mach exception or simply abort
            __builtin_trap();
        #else
            *((volatile int*)0) = 0xDEADBEEF;
        #endif
    }

    static void induceGpuCrash() {
        // Port of RageSecInduceGpuCrash logic
        // This usually involves feeding illegal state to the GPU driver
        // Placeholder for Interlock Assessment's GBuffer manipulation
        induceCrash();
    }
};

} // namespace interlock::security
