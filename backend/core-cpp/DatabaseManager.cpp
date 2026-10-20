#include "DatabaseManager.hpp"
#include <pqxx/pqxx>
#include <iostream>
#include <vector>

namespace interlock {

void DatabaseManager::initialize(const std::string& dsn) {
    try {
        pqxx::connection conn(dsn);
        pqxx::work txn(conn);

        // 1. Crear tabla de versiones si no existe
        txn.exec("CREATE TABLE IF NOT EXISTS schema_migrations (version INTEGER PRIMARY KEY, applied_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP);").no_rows();

        // 2. Obtener versión actual
        pqxx::result res = txn.exec("SELECT MAX(version) FROM schema_migrations;");
        int current_version = res[0][0].is_null() ? 0 : res[0][0].as<int>();

        // 3. Aplicar migraciones pendientes
        applyPending(txn, current_version);

        // Actualizar current_version para el log si es necesario,
        // o mejor aún, volver a consultar si queremos ser 100% precisos post-commit.
        // Pero dado que applyPending es síncrono dentro de la txn:

        txn.commit();

        // Obtener versión final después del commit para confirmación real
        pqxx::connection conn2(dsn);
        pqxx::work txn2(conn2);
        pqxx::result res_final = txn2.exec("SELECT MAX(version) FROM schema_migrations;");
        int final_version = res_final[0][0].is_null() ? 0 : res_final[0][0].as<int>();

        std::cout << "[DatabaseManager] Schema synchronized at version " << final_version << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[DatabaseManager] CRITICAL: Migration failed: " << e.what() << std::endl;
        throw; // Panic shutdown
    }
}

void DatabaseManager::applyPending(pqxx::work& txn, int current) {
    // Diccionario de migraciones embebidas (Simulado para v8.0 - Fuente de Verdad)
    std::vector<std::pair<int, std::string>> migrations = {
        {1, "CREATE TABLE events (id TEXT PRIMARY KEY, timestamp BIGINT, type TEXT, source TEXT, data JSONB, status TEXT);"},
        {2, "CREATE INDEX idx_events_timestamp ON events(timestamp);"},
        {3, "ALTER TABLE events ADD COLUMN rdt_depth INTEGER DEFAULT 1;"},
        {4, "CREATE TABLE sessions (id TEXT PRIMARY KEY, name TEXT, phase TEXT, start_time BIGINT, metadata JSONB);"},
        {5, "CREATE TABLE evidence_vault (id TEXT PRIMARY KEY, session_id TEXT, agent TEXT, type TEXT, content_path TEXT, hash TEXT, timestamp BIGINT);"},
        {6, "CREATE TABLE findings (id TEXT PRIMARY KEY, session_id TEXT, agent TEXT, title TEXT, severity TEXT, type TEXT, description TEXT, metadata JSONB, timestamp BIGINT);"}
    };

    for (const auto& [version, sql] : migrations) {
        if (version > current) {
            std::cout << "[DatabaseManager] Applying migration v" << version << "..." << std::endl;
            txn.exec(sql).no_rows();
            txn.exec("INSERT INTO schema_migrations (version) VALUES (" + txn.quote(version) + ")").no_rows();
        }
    }
}

} // namespace interlock
