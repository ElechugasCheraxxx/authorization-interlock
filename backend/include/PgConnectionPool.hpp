#pragma once
#include <pqxx/pqxx>
#include <vector>
#include <mutex>
#include <memory>
#include <condition_variable>
#include <functional>

namespace interlock {

class PgConnectionPool {
public:
    static PgConnectionPool& getInstance() {
        static PgConnectionPool instance;
        return instance;
    }

    void warmup(const std::string& dsn, size_t pool_size) {
        std::unique_lock<std::mutex> lock(mtx);
        for (size_t i = 0; i < pool_size; ++i) {
            try {
                connections.push_back(std::make_unique<pqxx::connection>(dsn));
            } catch (const std::exception& e) {
                // In production, we might want to log this but keep going if some connections succeeded
                continue;
            }
        }
    }

    using ConnectionPtr = std::unique_ptr<pqxx::connection, std::function<void(pqxx::connection*)>>;

    ConnectionPtr acquire() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this] { return !connections.empty(); });

        auto conn = std::move(connections.back());
        connections.pop_back();

        return ConnectionPtr(
            conn.release(),
            [this](pqxx::connection* p) {
                std::unique_lock<std::mutex> lock(mtx);
                connections.push_back(std::unique_ptr<pqxx::connection>(p));
                cv.notify_one();
            }
        );
    }

    ConnectionPtr tryAcquire() {
        std::unique_lock<std::mutex> lock(mtx);
        if (connections.empty()) return nullptr;

        auto conn = std::move(connections.back());
        connections.pop_back();

        return ConnectionPtr(
            conn.release(),
            [this](pqxx::connection* p) {
                std::unique_lock<std::mutex> lock(mtx);
                connections.push_back(std::unique_ptr<pqxx::connection>(p));
                cv.notify_one();
            }
        );
    }

private:
    PgConnectionPool() = default;
    std::vector<std::unique_ptr<pqxx::connection>> connections;
    std::mutex mtx;
    std::condition_variable cv;
};

} // namespace interlock
