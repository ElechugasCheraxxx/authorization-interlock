#pragma once

#include "Schema.hpp"
#include <vector>
#include <string>
#include <future>

namespace interlock {

struct VectorEntry {
    std::string id;
    void* heap_data_ptr = nullptr; // Pointer managed by SovereignHeap
    json metadata;

    // Helper to access embedding as float array
    float* data() const { return static_cast<float*>(heap_data_ptr); }
};

class RAGEngine {
public:
    static RAGEngine& getInstance() {
        static RAGEngine instance;
        return instance;
    }

    // Indexación de hallazgos en tiempo real (Zero-copy memory)
    void indexFinding(const Finding& finding);

    // Búsqueda de contexto para agentes (Latencia sub-1ms)
    std::vector<VectorEntry> search(const std::string& query_text, int limit = 5);

    // Consolidación de memoria episódica
    void distillMemories();

    // Persistencia binaria de alto rendimiento (Zero-Disk I/O via mmap)
    void persistToBinary(const std::string& path);
    void loadFromBinary(const std::string& path);

private:
    RAGEngine() = default;

    // High-performance vector database in memory
    std::vector<VectorEntry> vector_db;
    std::mutex db_mtx;
};

} // namespace interlock
