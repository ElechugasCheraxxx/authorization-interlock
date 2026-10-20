#include "RAGEngine.hpp"
#include "InferenceEngine.hpp"
#include "SovereignHeap.hpp"
#include "DiscoveryStreamer.hpp"
#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>

#include <cmath>
#include <algorithm>
#include <numeric>

namespace interlock {

float cosine_similarity(const std::vector<float>& a, const std::vector<float>& b) {
    if (a.empty() || a.size() != b.size()) return 0.0f;
    float dot = 0.0f, ma = 0.0f, mb = 0.0f;
    for(size_t i = 0; i < a.size(); ++i) {
        dot += a[i] * b[i];
        ma += a[i] * a[i];
        mb += b[i] * b[i];
    }
    return dot / (std::sqrt(ma) * std::sqrt(mb) + 1e-9f);
}

// Estructura de cabecera para el mapeo binario de la base vectorial
struct VectorDBHeader {
    uint32_t magic; // 0x56454354 (VECT)
    uint32_t version;
    uint64_t entry_count;
    uint32_t embedding_dim;
};

void RAGEngine::indexFinding(const Finding& finding) {
    std::lock_guard<std::mutex> lock(db_mtx);
    std::cout << "[RAGEngine] Indexing finding: " << finding.id << std::endl;

    // In production, we call InferenceEngine to get an embedding for the finding
    InferenceRequest req;
    req.prompt = "Generate embedding for assessment finding: " + finding.title + " - " + finding.raw_stream;
    req.max_tokens = 0; // Requesting only embedding

    // Use SovereignHeap for the embedding data (Registered Pointer)
    const size_t dim = 4096;
    const size_t size_bytes = dim * sizeof(float);

    VectorEntry entry;
    entry.id = finding.id;

    // Allocate in SovereignHeap
    void* heap_ptr = memory::SovereignHeap::getInstance().malloc_registered(
        size_bytes,
        &entry.heap_data_ptr,
        memory::MemoryID::RAG_ENGINE
    );

    // Mock embedding data
    std::vector<float> mock_embedding(dim, 0.1f);
    std::memcpy(heap_ptr, mock_embedding.data(), size_bytes);

    entry.metadata = finding.metadata;
    vector_db.push_back(entry);

    // Request async discovery loading for related context
    rag::DiscoveryStreamer::getInstance().requestLoad(std::hash<std::string>{}(finding.id), 10);
}

std::vector<VectorEntry> RAGEngine::search(const std::string& query_text, int limit) {
    std::lock_guard<std::mutex> lock(db_mtx);

    // In production, obtain query embedding from InferenceEngine
    std::vector<float> query_vec(4096, 0.2f); // Mock query embedding

    if (vector_db.empty()) return {};

    std::vector<std::pair<float, VectorEntry>> scored_results;

    for (const auto& entry : vector_db) {
        // Convert registered pointer back to vector for similarity calculation
        std::vector<float> entry_vec(4096);
        std::memcpy(entry_vec.data(), entry.heap_data_ptr, 4096 * sizeof(float));

        float score = cosine_similarity(query_vec, entry_vec);
        scored_results.push_back({score, entry});
    }

    std::sort(scored_results.begin(), scored_results.end(),
              [](const auto& a, const auto& b) { return a.first > b.first; });

    std::vector<VectorEntry> final_results;
    for (size_t i = 0; i < std::min(scored_results.size(), static_cast<size_t>(limit)); ++i) {
        final_results.push_back(scored_results[i].second);
    }

    return final_results;
}

void RAGEngine::distillMemories() {
    std::cout << "[RAGEngine] Distilling episodic memories into semantic store..." << std::endl;
}

// Nueva implementación de persistencia binaria con Zero-Disk I/O (mmap)
void RAGEngine::persistToBinary(const std::string& path) {
    std::lock_guard<std::mutex> lock(db_mtx);

    int fd = open(path.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) return;

    // Calcular tamaño necesario: Header + (IDs + Embeddings)
    size_t dim = 4096; // Dimensión por defecto
    size_t file_size = sizeof(VectorDBHeader) + (vector_db.size() * (64 + dim * sizeof(float)));

    if (ftruncate(fd, file_size) == -1) {
        close(fd);
        return;
    }

    void* addr = mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        close(fd);
        return;
    }

    VectorDBHeader* header = static_cast<VectorDBHeader*>(addr);
    header->magic = 0x56454354;
    header->version = 1;
    header->entry_count = vector_db.size();
    header->embedding_dim = static_cast<uint32_t>(dim);

    // Copiar datos reales al mapa de memoria
    uint8_t* data_ptr = static_cast<uint8_t*>(addr) + sizeof(VectorDBHeader);
    for (const auto& entry : vector_db) {
        // Copiar ID (max 64 bytes)
        char id_buf[64] = {0};
        std::strncpy(id_buf, entry.id.c_str(), 63);
        std::memcpy(data_ptr, id_buf, 64);
        data_ptr += 64;

        // Copiar Embedding from SovereignHeap
        std::memcpy(data_ptr, entry.heap_data_ptr, dim * sizeof(float));
        data_ptr += dim * sizeof(float);
    }

    msync(addr, file_size, MS_SYNC);
    munmap(addr, file_size);
    close(fd);

    std::cout << "[RAGEngine] Vector DB persisted to " << path << " via mmap." << std::endl;
}

void RAGEngine::loadFromBinary(const std::string& path) {
    std::lock_guard<std::mutex> lock(db_mtx);

    int fd = open(path.c_str(), O_RDONLY);
    if (fd == -1) return;

    struct stat st;
    fstat(fd, &st);

    void* addr = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED) {
        close(fd);
        return;
    }

    VectorDBHeader* header = static_cast<VectorDBHeader*>(addr);
    if (header->magic != 0x56454354) {
        munmap(addr, st.st_size);
        close(fd);
        return;
    }

    // Reconstruir vector_db desde el mapeo binario
    vector_db.clear();
    uint8_t* data_ptr = static_cast<uint8_t*>(addr) + sizeof(VectorDBHeader);
    for (uint64_t i = 0; i < header->entry_count; ++i) {
        VectorEntry entry;
        char id_buf[64];
        std::memcpy(id_buf, data_ptr, 64);
        entry.id = std::string(id_buf);
        data_ptr += 64;

        // Allocate in SovereignHeap for loaded data
        size_t size_bytes = header->embedding_dim * sizeof(float);
        void* heap_ptr = memory::SovereignHeap::getInstance().malloc_registered(
            size_bytes,
            &entry.heap_data_ptr,
            memory::MemoryID::RAG_ENGINE
        );
        std::memcpy(heap_ptr, data_ptr, size_bytes);
        data_ptr += size_bytes;

        vector_db.push_back(entry);
    }

    std::cout << "[RAGEngine] Loaded " << vector_db.size() << " entries from " << path << " via mmap." << std::endl;

    munmap(addr, st.st_size);
    close(fd);
}

} // namespace interlock
