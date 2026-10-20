#pragma once
#include <string>
#include <pqxx/pqxx>

namespace interlock {

class DatabaseManager {
public:
    static DatabaseManager& getInstance() {
        static DatabaseManager instance;
        return instance;
    }

    void initialize(const std::string& dsn);

private:
    DatabaseManager() = default;
    void applyPending(pqxx::work& txn, int current);
};

} // namespace interlock
