#pragma once

#include <string>
#include <vector>
#include <map>
#include <filesystem>

namespace interlock {

/**
 * @brief Prioritizes files and assess surfaces based on data input criticality.
 * Implements Milestone 1.3 of the Glasswing Masterplan.
 */
class SurfaceMapper {
public:
    struct SurfaceEntry {
        std::string path;
        int criticality; // 1-5
        std::vector<std::string> entry_points; // functions like recv, read, etc.
        std::string reasoning;
    };

    static SurfaceMapper& getInstance() {
        static SurfaceMapper instance;
        return instance;
    }

    /**
     * @brief Scans a directory and ranks files by assess surface potential.
     */
    std::vector<SurfaceEntry> mapDirectory(const std::string& root_path);

    /**
     * @brief Analyzes a specific file for sensitive syscalls or patterns.
     */
    SurfaceEntry analyzeFile(const std::filesystem::path& path);

private:
    SurfaceMapper() = default;
};

} // namespace interlock
