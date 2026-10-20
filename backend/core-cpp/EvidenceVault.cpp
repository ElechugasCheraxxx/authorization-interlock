#include "EvidenceVault.hpp"
#include "PgConnectionPool.hpp"
#include "ElasticsearchManager.hpp"
#include "KafkaManager.hpp"
#include <pqxx/pqxx>
#include <iostream>
#include <chrono>

namespace interlock {

bool EvidenceVault::store(const Evidence& ev) {
    // 1. 🧠 POSTGRESQL: Permanent Storage (Source of Truth)
    try {
        auto conn = PgConnectionPool::getInstance().tryAcquire();
        if (conn) {
            pqxx::work W(*conn);
            W.exec0("INSERT INTO evidence_vault (id, session_id, agent, type, content_path, hash, timestamp) VALUES (" +
                   W.quote(ev.id) + ", " +
                   W.quote(ev.session_id) + ", " +
                   W.quote(ev.agent_alias) + ", " +
                   W.quote(ev.type) + ", " +
                   W.quote(ev.local_path) + ", " +
                   W.quote(ev.sha256_hash) + ", " +
                   W.quote(ev.timestamp) + ")");
            W.commit();
        }
    } catch (const std::exception& e) {
        std::cerr << "[EvidenceVault] PostgreSQL Error: " << e.what() << std::endl;
        return false;
    }

    // 2. 🔎 ELASTICSEARCH: Operational Analysis
    nlohmann::json doc = {
        {"id", ev.id},
        {"session_id", ev.session_id},
        {"agent", ev.agent_alias},
        {"type", ev.type},
        {"hash", ev.sha256_hash},
        {"timestamp", ev.timestamp}
    };
    ElasticsearchManager::getInstance().indexFinding("evidence_index", ev.id, doc);

    // 3. 🔄 KAFKA: Notification for Real-time scaled UI
    KafkaManager::getInstance().produce("evidence_updates", ev.session_id, doc.dump());

    return true;
}

void EvidenceVault::indexForSearch(const Evidence& evidence) {
    // Implementation for targeted indexing if needed
}

std::vector<Evidence> EvidenceVault::getBySession(const std::string& session_id) {
    std::vector<Evidence> results;
    try {
        auto conn = PgConnectionPool::getInstance().tryAcquire();
        if (conn) {
            pqxx::nontransaction N(*conn);
            pqxx::result res = N.exec("SELECT * FROM evidence_vault WHERE session_id = " + N.quote(session_id));
            for (auto row : res) {
                results.push_back({
                    row["id"].as<std::string>(),
                    row["session_id"].as<std::string>(),
                    row["agent"].as<std::string>(),
                    row["type"].as<std::string>(),
                    row["content_path"].as<std::string>(),
                    row["hash"].as<std::string>(),
                    row["timestamp"].as<long long>()
                });
            }
        }
    } catch (...) {}
    return results;
}

} // namespace interlock
