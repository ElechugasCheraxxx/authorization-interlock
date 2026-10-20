#import "ProcessManager.hpp"
#import <Foundation/Foundation.h>
#include <iostream>
#include <filesystem>

namespace interlock {

void ProcessManager::setBasePath(const std::string& path) {
    base_path = path;
}

void ProcessManager::startServices() {
    @autoreleasepool {
        NSLog(@"[ProcessManager] Initializing environment at %s", base_path.c_str());

        NSString* nsBasePath = [NSString stringWithUTF8String:base_path.c_str()];
        NSString* logDir = [nsBasePath stringByAppendingPathComponent:@"backend/data/logs"];
        NSString* pgData = [nsBasePath stringByAppendingPathComponent:@"backend/data/db"];

        [[NSFileManager defaultManager] createDirectoryAtPath:logDir withIntermediateDirectories:YES attributes:nil error:nil];

        // 1. Locate Postgres Binaries with priority to Bundle
        NSMutableArray* searchPaths = [NSMutableArray array];
        if ([nsBasePath containsString:@".app"]) {
            [searchPaths addObject:[nsBasePath stringByDeletingLastPathComponent]]; // Contents/MacOS
        }
        [searchPaths addObjectsFromArray:@[
            @"/opt/homebrew/opt/postgresql@16/bin/",
            @"/opt/homebrew/opt/postgresql@17/bin/",
            @"/opt/homebrew/bin/",
            @"/usr/local/bin/"
        ]];

        NSString* pgCtlBin = @"pg_ctl";
        BOOL found = NO;
        for (NSString* path in searchPaths) {
            NSString* fullPath = [path stringByAppendingPathComponent:@"pg_ctl"];
            if ([[NSFileManager defaultManager] fileExistsAtPath:fullPath]) {
                pgCtlBin = fullPath;
                found = YES;
                break;
            }
        }

        if (!found) {
            NSLog(@"[ProcessManager] CRITICAL: PostgreSQL binaries not found.");
        }

        // 2. Database Init
        if (![[NSFileManager defaultManager] fileExistsAtPath:[pgData stringByAppendingPathComponent:@"PG_VERSION"]]) {
            NSLog(@"[ProcessManager] Initializing database cluster...");
            NSTask* initTask = [[NSTask alloc] init];
            [initTask setLaunchPath:@"/usr/bin/env"];
            [initTask setArguments:@[@"initdb", @"-D", pgData, @"--auth=trust"]];
            [initTask launch];
            [initTask waitUntilExit];
        }

        // 3. Start PostgreSQL using NSTask for better control
        NSTask* pgTask = [[NSTask alloc] init];
        [pgTask setLaunchPath:pgCtlBin];
        [pgTask setArguments:@[@"-D", pgData, @"-l", [logDir stringByAppendingPathComponent:@"postgres.log"], @"start"]];

        @try {
            [pgTask launch];
            NSLog(@"[ProcessManager] PostgreSQL service started via NSTask.");
        } @catch (NSException *exception) {
            NSLog(@"[ProcessManager] Failed to launch PostgreSQL: %@", exception.reason);
        }
    }
}

void ProcessManager::spawnService(const std::string& command, const std::string& logFile) {
    @autoreleasepool {
        NSString* nsCommand = [NSString stringWithUTF8String:command.c_str()];
        NSString* nsLogFile = [NSString stringWithUTF8String:logFile.c_str()];

        NSTask* task = [[NSTask alloc] init];
        [task setLaunchPath:@"/bin/sh"];
        [task setArguments:@[@"-c", [NSString stringWithFormat:@"%@ > %@ 2>&1", nsCommand, nsLogFile]]];

        [task launch];
        active_pids.push_back([task processIdentifier]);
        NSLog(@"[ProcessManager] Spawned service %@ (PID: %d)", nsCommand, [task processIdentifier]);
    }
}

void ProcessManager::stopServices() {
    @autoreleasepool {
        NSLog(@"[ProcessManager] Tearing down services...");
        for (int pid : active_pids) {
            kill(pid, SIGTERM);
        }
        active_pids.clear();

        // Natively stop Postgres
        NSString* nsBasePath = [NSString stringWithUTF8String:base_path.c_str()];
        NSString* pgData = [nsBasePath stringByAppendingPathComponent:@"backend/data/db"];

        NSTask* stopTask = [[NSTask alloc] init];
        [stopTask setLaunchPath:@"/usr/bin/env"];
        [stopTask setArguments:@[@"pg_ctl", @"-D", pgData, @"stop", @"-m", @"fast"]];
        [stopTask launch];
    }
}

} // namespace interlock
