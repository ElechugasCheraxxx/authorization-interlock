#pragma once

#include <cstdint>
#include <vector>
#include <mutex>
#include <algorithm>

namespace interlock::memory {

class TrackableEntity;

/**
 * @brief Reference Tracking System (GC-lite).
 * Ported and modernized from GTA's CReferences system.
 *
 * Prevents dangling pointers by maintaining a list of active references
 * to entities that might be deleted or remapped in memory.
 */
struct Reference {
    Reference* next;
    TrackableEntity* entity;
};

class ReferenceTracker {
public:
    static constexpr size_t MAX_REFERENCES = 10000;

    static ReferenceTracker& getInstance() {
        static ReferenceTracker instance;
        return instance;
    }

    void init() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_refs.resize(MAX_REFERENCES);
        m_emptyList = &m_refs[0];
        for (size_t i = 0; i < MAX_REFERENCES - 1; ++i) {
            m_refs[i].next = &m_refs[i + 1];
            m_refs[i].entity = nullptr;
        }
        m_refs[MAX_REFERENCES - 1].next = nullptr;
    }

    Reference* addReference(TrackableEntity* entity) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_emptyList) return nullptr;

        Reference* ref = m_emptyList;
        m_emptyList = m_emptyList->next;

        ref->entity = entity;
        ref->next = nullptr; // This would be linked to the entity's ref list in a full implementation
        return ref;
    }

    void removeReference(Reference* ref) {
        std::lock_guard<std::mutex> lock(m_mutex);
        ref->entity = nullptr;
        ref->next = m_emptyList;
        m_emptyList = ref;
    }

private:
    ReferenceTracker() { init(); }
    std::vector<Reference> m_refs;
    Reference* m_emptyList = nullptr;
    std::mutex m_mutex;
};

class TrackableEntity {
public:
    virtual ~TrackableEntity() {
        // In a real implementation, we would prune all references pointing here
    }
};

} // namespace interlock::memory
