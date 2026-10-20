#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace interlock {

struct Evidence {
    std::string id;
    std::string session_id;
    std::string agent_alias;
    std::string type; // "hash", "loot", "screen", "log"
    std::string local_path;
    std::string sha256_hash;
    long long timestamp;
};

class EvidenceVault {
public:
    static EvidenceVault& getInstance() {
        static EvidenceVault instance;
        return instance;
    }

    // 🧠 Fuente de Verdad: Persistencia en PostgreSQL
    bool store(const Evidence& evidence);
    std::vector<Evidence> getBySession(const std::string& session_id);

    // 🔎 Indexación en Elasticsearch para búsqueda rápida
    void indexForSearch(const Evidence& evidence);

private:
    EvidenceVault() = default;
};

} // namespace interlock
