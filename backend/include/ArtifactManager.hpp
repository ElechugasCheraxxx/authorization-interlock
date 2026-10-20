#pragma once

#include <string>
#include <vector>
#include <map>
#include <mutex>

namespace interlock {

struct PayloadVariant {
    std::string id;
    std::string content;
    std::vector<std::string> techniques;
};

class ArtifactManager {
public:
    static ArtifactManager& getInstance() {
        static ArtifactManager instance;
        return instance;
    }

    // --- Polymorphic Generation (Native C++) ---
    std::string obfuscateStrings(const std::string& code);
    std::string addJunkCode(const std::string& code);
    std::string b64EncodeLayer(const std::string& code);
    std::string xorEncodeLayer(const std::string& code, int key = 42);
    std::string pqcArmorLayer(const std::string& code); // Phase 4.2/4.3

    std::vector<std::string> generateVariants(const std::string& original, int count = 3);

    // --- OS Packaging ---
    std::string wrapForOS(const std::string& artifact, const std::string& os_type);
    std::string generateZigStub(uint32_t seed);

private:
    ArtifactManager() = default;
    std::mutex mtx;
};

} // namespace interlock
