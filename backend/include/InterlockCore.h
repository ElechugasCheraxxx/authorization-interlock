#ifndef INTERLOCK_CORE_H
#define INTERLOCK_CORE_H

#ifdef __cplusplus
#include <string>
#endif

#ifndef __swift__
#include <thread>
#include <atomic>

// Fix for CFBase.h enum conflicts with C++23
#ifdef __APPLE__
#include <CoreFoundation/CFBase.h>
#endif

#include "../core-cpp/KernelBridge.hpp"
#endif

namespace interlock {

/**
 * InterlockCore is the high-performance C++ engine for the IDE.
 * It is imported into Swift via C++ Interop.
 */
struct InterlockCore {
    // Lifecycle
    static void boot();
    static void shutdown();
    static void setBasePath(const char* path);

    // High-level Actions
    static void createOperation(const char* name, const char* preset);
    static void joinOperation(const char* op_id);
    static void advancePhase();
    static void injectMessage(const char* to, const char* content);
    static void createCheckpoint(const char* op_id, const char* label);

    // Snapshot & Metrics (Direct JSON for Qt Bridge)
    static std::string getSnapshotJSON();
    static std::string getHardwareMetricsJSON();
    static std::string getKernelShieldStatusJSON();
    static std::string getReportDraft(const char* op_id);
    static std::string searchIntelligence(const char* query);
    static std::string triggerZigPayload(uint32_t seed);
    static bool distributeTicket(uint32_t type, const char* base64_data, const char* pids_csv);

    // Removed XML methods for performance - C++23 Pure Mode

    // Model Management
    static void loadModel(const char* model_id, const char* model_path);
    static void unloadModel(const char* model_id);
    static void selectModel(const char* model_id);

    // Performance Monitoring
    static double benchmarkInterop(int iterations);

    // Operations Signal Simulation (Metal-Accelerated)
    static std::string simulateSignalPropagation(const std::string& transmitters_json, const std::string& obstacles_json);

    // .chrxxx DSL Sovereign Compiler & Choreography
    static std::string compileChrx(const char* source);
    static bool validateChoreography(const char* chrx_json);

    // Event Handling
    typedef void (*NativeEventCallback)(const char* json_event);
    static void setEventCallback(NativeEventCallback callback);

#ifndef __swift__
    // Members hidden from Swift to ensure the struct remains "trivially importable"
    // and to avoid issues with non-copyable types like std::thread/std::atomic.
private:
    InterlockCore();
    ~InterlockCore();
    static InterlockCore& getInstance();

    void maintenanceLoop();

    NativeEventCallback event_callback = nullptr;
    std::thread maintenance_thread;
    std::atomic<bool> maintenance_running{false};
    KernelBridge kernel_bridge;
#endif
};

} // namespace interlock

#endif
