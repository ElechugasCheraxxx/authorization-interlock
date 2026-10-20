#pragma once

#include <string>
#include <vector>
#include <memory>
#include <future>
#include <mutex>
#include <thread>
#include <chrono>
#include "InferenceEngine.hpp"

namespace interlock {

/**
 * @brief Recurrent-Depth Transformer (RDT) Core.
 * Orchestrates multi-hop latent reasoning by re-injecting activations
 * into recurrent transformer blocks.
 */
class RecurrentEngine {
public:
    struct RDTParams {
        int max_loops;
        float halting_threshold;
        bool inject_lti;
        float spectral_radius;
        int current_loop;

        RDTParams()
            : max_loops(4),
              halting_threshold(0.95f),
              inject_lti(true),
              spectral_radius(0.99f),
              current_loop(0) {}
    };

    static RecurrentEngine& getInstance() {
        static RecurrentEngine instance;
        return instance;
    }

    RecurrentEngine(int n_embd);

    /**
     * @brief Performs a single step of the recurrence.
     */
    void step(std::vector<float>& hidden, const std::vector<float>& input, const std::vector<float>& delta);

    /**
     * @brief Checks if the reasoning should halt.
     */
    bool shouldHalt(const std::vector<float>& hidden, float threshold);

    /**
     * @brief High-level reasoning call that uses the RDT loop.
     */
    std::future<InferenceResponse> think(const InferenceRequest& request, const RDTParams& params = RDTParams());

    /**
     * @brief Continuous Depth-wise Batching (Milestone 2.13).
     * Manages multiple concurrent reasoning tasks, advancing each through its RDT loops.
     */
    void processBatch();

    /**
     * @brief Adds a reasoning task to the batch processing queue.
     */
    std::future<InferenceResponse> enqueueReasoning(const InferenceRequest& request, const RDTParams& params);

private:
    RecurrentEngine();

    struct BatchSlot {
        InferenceRequest request;
        RDTParams params;
        int current_loop = 0;
        float current_convergence = 0.0f;
        bool active = false;
        std::shared_ptr<std::promise<InferenceResponse>> result_promise;
    };

    static const int MAX_BATCH_SIZE = 8;
    std::vector<BatchSlot> slots;
    std::mutex batch_mtx;
    std::thread batch_worker;
    bool running = true;

    /**
     * @brief Performs one "hop" of latent reasoning.
     */
    void performLatentHop(const std::string& model_id, int hop_index, float& convergence_score);
};

} // namespace interlock
