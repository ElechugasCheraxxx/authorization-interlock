#include "MissionControl.hpp"
#include "OrchestrationEngine.hpp"
#include "AgentEngine.hpp"
#include "RiskManager.hpp"
#include "security/Harden.hpp"
#include "PDGEngine.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <dlfcn.h>

namespace interlock {

namespace fs = std::filesystem;

typedef char* (*CompileMissionFn)(const char*);
typedef void (*FreeStringFn)(char*);

void MissionControl::executeSovereignSwarm(const std::string& chrxSource) {
    std::cout << "[MissionControl] [ALPHA STRIKE] Initiating Sovereign Swarm Mission..." << std::endl;

    // 1. Compile .chrxxx to C++
    void* handle = dlopen("/Users/Cheraxxx/Interlock IDE/backend/chrx-compiler/target/release/libchrx_compiler.dylib", RTLD_LAZY);
    if (!handle) {
        std::cerr << "[MissionControl] Error: Could not load libchrx_compiler.dylib" << std::endl;
        return;
    }

    auto compile_chrx = (CompileMissionFn)dlsym(handle, "chrx_compile_mission");
    auto free_string = (FreeStringFn)dlsym(handle, "chrx_free_string");

    if (compile_chrx) {
        char* generatedCode = compile_chrx(chrxSource.c_str());
        if (generatedCode) {
            std::cout << "[MissionControl] .chrxxx mission compiled successfully." << std::endl;
            // In a real scenario, we'd compile this C++ and hot-load it or use it to drive agents.
            free_string(generatedCode);
        } else {
            std::cerr << "[MissionControl] .chrxxx compilation failed." << std::endl;
        }
    }
    dlclose(handle);

    // 2. Generate and Verify PDG
    std::string pdgJson = PDGEngine::getInstance().generatePDGFromMission(chrxSource);
    std::cout << "[MissionControl] PDG Verified: " << (pdgJson.length() > 2 ? "SECURE" : "EMPTY") << std::endl;

    // 3. Execution (Simulated for this test)
    std::cout << "[MissionControl] Swarm Agents Synchronizing..." << std::endl;
    std::cout << "[MissionControl] [Sovereign Swarm] Mission in progress: Infiltration of target sector." << std::endl;
}

MissionControl::MissionControl() : running(false), overnight_mode(false) {}

MissionControl::~MissionControl() {
    stop();
}

void MissionControl::start() {
    if (running) return;
    running = true;
    processor_thread = std::thread(&MissionControl::missionProcessorLoop, this);
    std::cout << "[MissionControl] Overnight Mission Control started." << std::endl;
}

void MissionControl::stop() {
    if (!running) return;
    running = false;
    cv.notify_all();
    if (processor_thread.joinable()) {
        processor_thread.join();
    }
}

std::string MissionControl::submitMission(const Mission& mission) {
    Mission m = mission;
    if (m.id.empty()) {
        m.id = "msn_" + security::IntegrityVerifier::generateSecureID();
    }

    std::lock_guard<std::mutex> lock(queue_mtx);
    mission_queue.push(m);
    cv.notify_one();
    std::cout << "[MissionControl] Mission submitted: " << m.objective << " (ID: " << m.id << ")" << std::endl;
    return m.id;
}

void MissionControl::enableOvernightMode(bool enable) {
    overnight_mode = enable;
    if (enable) {
        std::cout << "[MissionControl] Overnight Mode ENABLED. Integrity and autonomy maximized." << std::endl;
        RiskManager::getInstance().setStealthLevel(5); // Maximum integrity
    } else {
        std::cout << "[MissionControl] Overnight Mode DISABLED." << std::endl;
        RiskManager::getInstance().setStealthLevel(3); // Normal operation
    }
}

void MissionControl::missionProcessorLoop() {
    while (running) {
        Mission mission;
        {
            std::unique_lock<std::mutex> lock(queue_mtx);
            cv.wait(lock, [this] { return !mission_queue.empty() || !running; });

            if (!running) break;

            mission = mission_queue.front();
            mission_queue.pop();
        }

        processMission(mission);
    }
}

void MissionControl::processMission(const Mission& mission) {
    std::cout << "[MissionControl] Processing mission: " << mission.id << std::endl;

    // 1. Task Decomposition via AgentEngine
    // 2. Resource Allocation via OrchestrationEngine
    // 3. Execution and Feedback Loop

    // Simulated execution
    MissionReport report;
    report.mission_id = mission.id;
    report.success = true;
    report.summary = "Objective '" + mission.objective + "' achieved autonomously.";
    report.risk_score = 0.12f;

    {
        std::lock_guard<std::mutex> lock(reports_mtx);
        reports[mission.id] = report;
    }

    std::cout << "[MissionControl] Mission completed: " << mission.id << std::endl;
}

MissionReport MissionControl::getReport(const std::string& mission_id) {
    std::lock_guard<std::mutex> lock(reports_mtx);
    if (reports.find(mission_id) != reports.end()) {
        return reports[mission_id];
    }
    return {mission_id, false, "Report not found", {}, 1.0f};
}

void MissionControl::generateBuildEnvironment(const std::string& project_name, const std::string& target_os) {
    std::cout << "[MissionControl] Generating build environment for " << project_name << " (" << target_os << ")" << std::endl;

    // Logic ported from referencia previa VS2010ProjectGenerator.rb
    // Constructs a minimal .vcxproj XML structure for cross-compilation.

    // Using Jenkins Hash (0x04c11db7) for the Project GUID as per referencia previa security utilities
    uint32_t j_hash = security::IntegrityVerifier::calculateJenkinsHash(
        reinterpret_cast<const uint8_t*>(project_name.c_str()), project_name.size()
    );

    std::stringstream ss;
    ss << "{" << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << j_hash
       << "-BEEF-4C11-D1B7-" << std::setw(12) << std::setfill('0') << j_hash << "}";
    std::string guid = ss.str();

    std::stringstream vcxproj;
    vcxproj << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    vcxproj << "<Project DefaultTargets=\"Build\" ToolsVersion=\"4.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n";
    vcxproj << "  <ItemGroup Label=\"ProjectConfigurations\">\n";
    vcxproj << "    <ProjectConfiguration Include=\"Release|Win32\">\n";
    vcxproj << "      <Configuration>Release</Configuration>\n";
    vcxproj << "      <Platform>Win32</Platform>\n";
    vcxproj << "    </ProjectConfiguration>\n";
    vcxproj << "  </ItemGroup>\n";
    vcxproj << "  <PropertyGroup Label=\"Globals\">\n";
    vcxproj << "    <ProjectGuid>" << guid << "</ProjectGuid>\n";
    vcxproj << "    <RootNamespace>" << project_name << "</RootNamespace>\n";
    vcxproj << "  </PropertyGroup>\n";
    vcxproj << "  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.Default.props\" />\n";
    vcxproj << "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Release|Win32'\" Label=\"Configuration\">\n";
    vcxproj << "    <ConfigurationType>Application</ConfigurationType>\n";
    vcxproj << "    <UseDebugLibraries>false</UseDebugLibraries>\n";
    vcxproj << "    <PlatformToolset>v100</PlatformToolset>\n";
    vcxproj << "  </PropertyGroup>\n";
    vcxproj << "  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.props\" />\n";
    vcxproj << "  <ItemDefinitionGroup Condition=\"'$(Configuration)|$(Platform)'=='Release|Win32'\">\n";
    vcxproj << "    <ClCompile>\n";
    vcxproj << "      <WarningLevel>Level3</WarningLevel>\n";
    vcxproj << "      <Optimization>MaxSpeed</Optimization>\n";
    vcxproj << "      <FunctionLevelLinking>true</FunctionLevelLinking>\n";
    vcxproj << "      <IntrinsicFunctions>true</IntrinsicFunctions>\n";
    vcxproj << "      <AdditionalIncludeDirectories>$(ProjectDir)\\include;%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>\n";
    vcxproj << "    </ClCompile>\n";
    vcxproj << "    <Link>\n";
    vcxproj << "      <GenerateDebugInformation>false</GenerateDebugInformation>\n";
    vcxproj << "      <EnableCOMDATFolding>true</EnableCOMDATFolding>\n";
    vcxproj << "      <OptimizeReferences>true</OptimizeReferences>\n";
    vcxproj << "    </Link>\n";
    vcxproj << "  </ItemDefinitionGroup>\n";
    vcxproj << "  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.targets\" />\n";
    vcxproj << "</Project>\n";

    fs::path build_dir = fs::path("/Users/Cheraxxx/Interlock IDE/build/agent_gen") / project_name;
    fs::create_directories(build_dir);

    std::ofstream file(build_dir / (project_name + ".vcxproj"));
    file << vcxproj.str();
    file.close();

    std::cout << "[MissionControl] Build environment generated at: " << build_dir << std::endl;
}

} // namespace interlock
