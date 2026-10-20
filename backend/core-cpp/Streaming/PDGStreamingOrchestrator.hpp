#pragma once

#include <vector>
#include <unordered_map>
#include "StreamingPriority.hpp"
#include "../C2/TaskManager.hpp"

namespace interlock::core::streaming {

/**
 * @brief Primitive Dependency Graph (PDG) Orchestrator for Streaming.
 *
 * Integrates TaskManager with a dependency-aware streaming system.
 * Uses patterns from ComputeDependencyCost in streamingdebug.cpp.
 */
class PDGStreamingOrchestrator {
public:
    struct ResourceNode {
        StrIndex index;
        uint32_t virtSize;
        uint32_t physSize;
        std::vector<StrIndex> dependencies;
    };

    void RegisterResource(const ResourceNode& node) {
        m_nodes[node.index] = node;
    }

    /**
     * @brief Computes total cost (memory) for a resource and its dependencies.
     *
     * Ported logic from ComputeDependencyCost in streamingdebug.cpp.
     */
    void ComputeResourceCost(StrIndex index, uint32_t& outVirtSize, uint32_t& outPhysSize, bool recursive) {
        auto it = m_nodes.find(index);
        if (it == m_nodes.end()) return;

        const auto& node = it->second;
        for (StrIndex depIndex : node.dependencies) {
            auto depIt = m_nodes.find(depIndex);
            if (depIt != m_nodes.end()) {
                outVirtSize += depIt->second.virtSize;
                outPhysSize += depIt->second.physSize;

                if (recursive) {
                    ComputeResourceCost(depIndex, outVirtSize, outPhysSize, recursive);
                }
            }
        }
    }

    /**
     * @brief Orchestrates streaming tasks based on priority and dependency cost.
     */
    void Orchestrate(StreamingPrioritySystem& prioritySystem, std::vector<StrIndex>& pendingIndices, const std::vector<StreamingInfo>& infoMap) {
        // 1. Sort by LSN and Priority
        prioritySystem.SortByLSN(pendingIndices, infoMap);

        // 2. Further refine or trigger tasks based on dependency graph
        // (Simplified placeholder for PDG integration)
        for (StrIndex index : pendingIndices) {
            uint32_t vSize = 0, pSize = 0;
            ComputeResourceCost(index, vSize, pSize, true);

            // Log or use cost to decide execution batching
        }
    }

private:
    std::unordered_map<StrIndex, ResourceNode> m_nodes;
};

} // namespace interlock::core::streaming
