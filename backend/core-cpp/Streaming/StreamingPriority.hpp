#pragma once

#include <cstdint>
#include <algorithm>
#include <vector>
#include "../Memory/PoolAllocator.hpp"

namespace interlock::core::streaming {

using StrIndex = uint32_t;

enum StreamingFlags : uint32_t {
    STRFLAG_NONE = 0,
    STRFLAG_PRIORITY_LOAD = 1 << 0,
    // Add other flags as needed
};

struct StreamingInfo {
    uint32_t flags;
    uint32_t cdPos;

    uint32_t GetFlags() const { return flags; }
    uint32_t GetCdPos() const { return cdPos; }
};

/**
 * @brief LSN-based streaming priority system optimized for M4 Pro.
 *
 * Features:
 * - 4-channel parallel streaming simulation.
 * - LSN (Logical Sector Number) ordering for artifact exfiltration.
 * - Cache-aligned priority entries.
 */
class StreamingPrioritySystem {
public:
    static constexpr size_t NUM_CHANNELS = 4;
    uint32_t m_currentHeadPos;

    StreamingPrioritySystem(uint32_t currentHeadPos = 0) : m_currentHeadPos(currentHeadPos) {}

    void SetCurrentHeadPos(uint32_t pos) { m_currentHeadPos = pos; }

    struct PriorityEntry {
        StrIndex index;
        uint32_t lsn;
        uint32_t priority;
        uint32_t channel;
    };

    /**
     * @brief Sorts and distributes load requests across parallel channels.
     */
    void DistributeRequests(std::vector<StrIndex>& indices, const std::vector<StreamingInfo>& infoMap,
                            std::vector<PriorityEntry> channels[NUM_CHANNELS]) {

        // 1. Sort by priority and LSN first
        std::sort(indices.begin(), indices.end(), [&](StrIndex i, StrIndex j) {
            const auto& info_i = infoMap[i];
            const auto& info_j = infoMap[j];

            if ((info_i.flags & STRFLAG_PRIORITY_LOAD) != (info_j.flags & STRFLAG_PRIORITY_LOAD)) {
                return (info_i.flags & STRFLAG_PRIORITY_LOAD) != 0;
            }

            uint32_t lsn_i = info_i.cdPos - m_currentHeadPos;
            uint32_t lsn_j = info_j.cdPos - m_currentHeadPos;
            return lsn_i < lsn_j;
        });

        // 2. Distribute to channels (Round-robin for now, could be load-balanced)
        for (size_t k = 0; k < indices.size(); ++k) {
            StrIndex idx = indices[k];
            size_t channelIdx = k % NUM_CHANNELS;

            channels[channelIdx].push_back({
                idx,
                infoMap[idx].cdPos,
                infoMap[idx].flags,
                static_cast<uint32_t>(channelIdx)
            });
        }
    }

private:
    uint32_t m_currentHeadPos;
};

} // namespace interlock::core::streaming
