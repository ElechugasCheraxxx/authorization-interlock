#include <iostream>
#include <vector>
#include <cassert>
#include <thread>
#include "../core-cpp/Memory/PoolAllocator.hpp"
#include "interop/FormalValidators.hpp"

using namespace interlock::memory;
using namespace interlock::formal;

void test_pool_basic() {
    PoolAllocator pool(100, 10);
    uint32_t h1, h2;

    void* p1 = pool.allocate(h1);
    assert(p1 != nullptr);
    assert(PoolValidator::verifyAlignment(reinterpret_cast<uintptr_t>(p1), 64));
    assert(PoolValidator::isHandleValid(h1, 10));

    void* p2 = pool.allocate(h2);
    assert(p2 != nullptr);
    assert(h1 != h2);

    pool.deallocate(h1);

    // Test Anti-DKOM (InstanceID increment)
    uint32_t h3;
    void* p3 = pool.allocate(h3);
    assert(p3 == p1); // Reused memory
    assert(h3 != h1); // But different handle (different InstanceID)

    // Stale handle resolution should fail
    assert(pool.resolve(h1) == nullptr);
    assert(pool.resolve(h3) == p3);

    std::cout << "[PASS] Pool Basic & Anti-DKOM (Certified)\n";
}

void test_pool_stress() {
    const int num_threads = 8;
    const int iterations = 1000;
    PoolAllocator pool(128, 1000);

    std::vector<std::thread> threads;
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&pool]() {
            std::vector<uint32_t> handles;
            for (int i = 0; i < iterations; ++i) {
                uint32_t h;
                void* p = pool.allocate(h);
                if (p) {
                    handles.push_back(h);
                    // Verify every allocation formally
                    assert(PoolValidator::isHandleValid(h, 1000));
                }

                if (i % 5 == 0 && !handles.empty()) {
                    uint32_t target = handles.back();
                    handles.pop_back();
                    pool.deallocate(target);
                }
            }
            for (uint32_t h : handles) pool.deallocate(h);
        });
    }

    for (auto& t : threads) t.join();
    assert(pool.getUsedCount() == 0);
    std::cout << "[PASS] Pool Multithreaded Stress (Certified)\n";
}

int main() {
    test_pool_basic();
    test_pool_stress();
    std::cout << "All Pool Memory tests passed successfully.\n";
    return 0;
}
