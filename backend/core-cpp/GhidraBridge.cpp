#include "GhidraBridge.hpp"
#include "ExecutionManager.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

namespace fs = std::filesystem;

namespace interlock {

bool GhidraBridge::initialize(const std::string& ghidra_path, const std::string& project_path) {
    std::lock_guard<std::mutex> lock(mtx);

    if (!fs::exists(ghidra_path)) {
        std::cerr << "[GhidraBridge] Ghidra path not found: " << ghidra_path << std::endl;
        return false;
    }

    ghidra_home = ghidra_path;
    current_project = project_path;

    if (!fs::exists(current_project)) {
        fs::create_directories(current_project);
    }

    is_initialized = true;
    std::cout << "[GhidraBridge] Initialized with Ghidra at: " << ghidra_home << std::endl;
    return true;
}

bool GhidraBridge::analyzeBinary(const std::string& binary_path) {
    if (!is_initialized) return false;

    std::cout << "[GhidraBridge] Analyzing binary: " << binary_path << std::endl;

    // Command for Ghidra Headless Analyzer
    // analyzeHeadless <project_path> <project_name> -import <binary_path> -postScript <DecompileAll.java>

    std::string headless_cmd = ghidra_home + "/support/analyzeHeadless " +
                               current_project + " InterlockProject -import " + binary_path + " -overwrite";

    // We would use ExecutionManager here
    // ExecutionManager::getInstance().spawnProcess(headless_cmd, {}, true);

    return true;
}

std::string GhidraBridge::decompileFunction(const std::string& binary_path, const std::string& function_name) {
    if (!is_initialized) return "Error: GhidraBridge not initialized.";

    std::string output_file = "/tmp/ghidra_" + function_name + ".c";
    if (fs::exists(output_file)) fs::remove(output_file);

    // Production Command: Execute Ghidra Headless with the decompiler script.
    // The script DecompileFunction.py must exist in the Ghidra script path or the project.
    std::string script_cmd = ghidra_home + "/support/analyzeHeadless " +
                             current_project + " InterlockProject -process " + fs::path(binary_path).filename().string() +
                             " -postScript DecompileFunction.py " + function_name + " " + output_file;

    // Use ExecutionManager to spawn the analyzer process.
    // Note: We block here until the file is produced or we timeout to meet the "NOW" production requirement.
    ExecutionManager::getInstance().spawnProcess(script_cmd, {}, false);

    int timeout_sec = 120; // Binary analysis can be slow.
    while (timeout_sec-- > 0 && !fs::exists(output_file)) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    if (fs::exists(output_file)) {
        std::ifstream ifs(output_file);
        std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
        fs::remove(output_file);
        return content;
    }

    return "[PRODUCTION ERROR] Ghidra failed to decompile " + function_name + " within the allocated time.";
}

std::map<uint64_t, std::string> GhidraBridge::getMemoryMap(const std::string& binary_path) {
    std::map<uint64_t, std::string> mmap;
    // Mocking for Phase 3.1 & 5.1 integration
    mmap[0x100000000] = "__TEXT";
    mmap[0x100001000] = "__stub_helper";
    mmap[0x100002000] = "__cstring";
    return mmap;
}

} // namespace interlock
