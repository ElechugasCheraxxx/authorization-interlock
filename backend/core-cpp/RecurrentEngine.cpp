#include "RecurrentEngine.hpp"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <filesystem>

namespace interlock {

RecurrentEngine::RecurrentEngine() : running(true) {
    slots.resize(MAX_BATCH_SIZE);
    batch_worker = std::thread(&RecurrentEngine::processBatch, this);
}

RecurrentEngine::RecurrentEngine(int n_embd) : running(true) {
    slots.resize(MAX_BATCH_SIZE);
    // Note: In a dedicated agent engine, we might not need the batch worker
    // if we are doing synchronous deep thinking, but we keep it for consistency.
    batch_worker = std::thread(&RecurrentEngine::processBatch, this);
}

void RecurrentEngine::step(std::vector<float>& hidden, const std::vector<float>& input, const std::vector<float>& delta) {
    std::lock_guard<std::mutex> lock(batch_mtx);
    for (size_t i = 0; i < hidden.size() && i < input.size() && i < delta.size(); ++i) {
        // Milestone 2.10: Latent Operations Injection
        // simplified RNN-like update: h = h + (input * delta)
        hidden[i] += input[i] * delta[i];
    }
}

bool RecurrentEngine::shouldHalt(const std::vector<float>& hidden, float threshold) {
    // Adaptive Computation Time (ACT) simulation
    float sum = 0.0f;
    for (float v : hidden) sum += std::abs(v);
    float avg = sum / (hidden.empty() ? 1.0f : hidden.size());
    return avg > threshold;
}

std::future<InferenceResponse> RecurrentEngine::enqueueReasoning(const InferenceRequest& request, const RDTParams& params) {
    auto promise = std::make_shared<std::promise<InferenceResponse>>();
    auto future = promise->get_future();

    {
        std::lock_guard<std::mutex> lock(batch_mtx);
        bool found_slot = false;
        for (auto& slot : slots) {
            if (!slot.active) {
                slot.request = request;
                slot.params = params;
                slot.current_loop = 0;
                slot.current_convergence = 0.0f;
                slot.active = true;
                slot.result_promise = promise;
                found_slot = true;
                break;
            }
        }

        if (!found_slot) {
            std::cerr << "[RecurrentEngine] No available slots in RDT Batch. Task rejected." << std::endl;
            InferenceResponse err;
            err.text = "Error: RDT Batcher Full";
            promise->set_value(err);
        }
    }

    return future;
}

void RecurrentEngine::processBatch() {
    std::cout << "[RecurrentEngine] Continuous Depth-wise Batching worker started." << std::endl;

    while (running) {
        bool work_done = false;

        // Note: In a real M4 Pro implementation, we would batch the latent hops
        // into a single Metal command buffer to maximize throughput.

        for (int i = 0; i < MAX_BATCH_SIZE; ++i) {
            BatchSlot* slot = nullptr;
            {
                std::lock_guard<std::mutex> lock(batch_mtx);
                if (slots[i].active) {
                    slot = &slots[i];
                }
            }

            if (slot) {
                work_done = true;

                // 1. Milestone 2.12: Depth-wise LoRA Adaptation
                std::string lora_path = "adapters/rdt_depth_" + std::to_string(slot->current_loop) + ".gguf";
                if (std::filesystem::exists(lora_path)) {
                    InferenceEngine::getInstance().applyLora(slot->request.model_id, lora_path, 1.0f);
                }

                // 2. Perform Latent Hop (RDT)
                performLatentHop(slot->request.model_id, slot->current_loop, slot->current_convergence);
                slot->current_loop++;

                // 3. Check ACT Halting or Max Loops
                if (slot->current_convergence >= slot->params.halting_threshold || slot->current_loop >= slot->params.max_loops) {
                    std::cout << "[RecurrentEngine] Slot [" << i << "] complete at loop " << slot->current_loop << std::endl;

                    InferenceRequest rdt_req = slot->request;
                    rdt_req.sampling_params["rdt_loops"] = slot->current_loop;

                    // Final generation (Offloaded to InferenceEngine)
                    auto response = InferenceEngine::getInstance().generate(rdt_req).get();
                    slot->result_promise->set_value(response);

                    std::lock_guard<std::mutex> lock(batch_mtx);
                    slot->active = false;
                }
            }
        }

        if (!work_done) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        } else {
            // Yield to allow other threads and Metal to breathe
            std::this_thread::yield();
        }
    }
}

std::future<InferenceResponse> RecurrentEngine::think(const InferenceRequest& request, const RDTParams& params) {
    return enqueueReasoning(request, params);
}

void RecurrentEngine::performLatentHop(const std::string& model_id, int hop_index, float& convergence_score) {
    // Milestone 2.11: Loop-Index Positional Embedding simulation
    // convergence_score reflects the "logic density" accumulated.
    convergence_score = 0.4f + (0.2f * hop_index);
    if (convergence_score > 1.0f) convergence_score = 0.99f;
}

} // namespace interlock
