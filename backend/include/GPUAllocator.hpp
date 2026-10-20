#pragma once

#include <string>
#include <vector>
#include <map>
#include <mutex>

namespace interlock {

struct GPUMetrics {
    float vram_total_gb;
    float vram_reserved_gb;
    float vram_actual_used_gb;
    float vram_free_gb;
    float temperature_c;
    float gpu_utilization_pct;
    bool saturated;
    bool overheated;
};

class GPUAllocator {
public:
    static GPUAllocator& getInstance() {
        static GPUAllocator instance;
        return instance;
    }

    // Monitoreo de Hardware (M4 Pro Specific)
    GPUMetrics getMetrics();

    // Gestión de Memoria Unificada con Saturation Tracking
    bool reserveVRAM(const std::string& pool_id, float amount_gb);
    void releaseVRAM(const std::string& pool_id);
    float getReservedVRAM(const std::string& pool_id);
    float getPoolSaturation() const;
    float getPoolSaturationUnsafe() const;

    // Thermal-Aware Dynamic Scaling
    int calculateOptimalRDTFactor();
    void updateThermals(float temp_c, float fan_speed_rpm);
    std::string getThermalState() const;

    // Monitorización y Presión de VRAM para Enjambre
    bool isUnderHighPressure() const;
    void triggerEmergencyEviction();

    // Ajuste de Prioridad de Cómputo (Metal Performance Shaders)
    void setComputePriority(const std::string& agent_alias, int priority);

    // Metal Compute Integration
    void runSignalSimulation(const std::vector<float>& transmitters, const std::vector<float>& obstacles, std::vector<float>& results);

    void updateHardwareStats();

private:
    GPUAllocator();

    std::map<std::string, float> active_allocations;
    mutable std::mutex mtx;

    float current_temp = 40.0f;
    float current_fan_speed = 0.0f;
    const float THERMAL_THROTTLE_THRESHOLD = 85.0f;
    const float VRAM_LIMIT_GB = 44.0f;
    float system_total_ram_gb = 48.0f;
};

} // namespace interlock
