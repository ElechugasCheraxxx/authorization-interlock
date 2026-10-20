#include "SurfaceMapper.hpp"
#include <fstream>
#include <regex>
#include <iostream>
#include <algorithm>

namespace interlock {

std::vector<SurfaceMapper::SurfaceEntry> SurfaceMapper::mapDirectory(const std::string& root_path) {
    std::vector<SurfaceEntry> entries;
    try {
        if (!std::filesystem::exists(root_path)) return entries;

        for (const auto& entry : std::filesystem::recursive_directory_iterator(root_path)) {
            if (entry.is_regular_file()) {
                auto extension = entry.path().extension();
                if (extension == ".c" || extension == ".cpp" || extension == ".h" || extension == ".hpp" || extension == ".py" || extension == ".rb") {
                    entries.push_back(analyzeFile(entry.path()));
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[SurfaceMapper] Error mapping directory: " << e.what() << std::endl;
    }

    // Sort by criticality (Descending)
    std::sort(entries.begin(), entries.end(), [](const SurfaceEntry& a, const SurfaceEntry& b) {
        return a.criticality > b.criticality;
    });

    return entries;
}

SurfaceMapper::SurfaceEntry SurfaceMapper::analyzeFile(const std::filesystem::path& path) {
    SurfaceEntry entry;
    entry.path = path.string();
    entry.criticality = 1; // Default low

    std::ifstream file(path);
    if (!file.is_open()) return entry;

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    // Patterns for High-Criticality Entry Points
    std::map<std::string, std::string> patterns = {
        {"recv\\(", "Network Input (recv)"},
        {"read\\(", "File/Socket Read"},
        {"socket\\(", "Network Initialization"},
        {"strcpy\\(", "Potential Buffer Overflow (strcpy)"},
        {"system\\(", "Command Injection Point (system)"},
        {"exec\\(", "Process Execution"},
        {"mmap\\(", "Memory Mapping"},
        {"ptrace\\(", "Anti-Debug/Trace Logic"}
    };

    for (const auto& [pattern, description] : patterns) {
        std::regex re(pattern);
        if (std::regex_search(content, re)) {
            entry.entry_points.push_back(description);
            entry.criticality += 1;
        }
    }

    if (entry.criticality > 5) entry.criticality = 5;

    if (entry.criticality >= 4) {
        entry.reasoning = "High concentration of sensitive syscalls and data input handlers.";
    } else if (entry.criticality >= 2) {
        entry.reasoning = "Standard application logic with some external data interaction.";
    } else {
        entry.reasoning = "Low risk boilerplate or utility code.";
    }

    return entry;
}

} // namespace interlock
