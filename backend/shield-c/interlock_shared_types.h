#ifndef INTERLOCK_SHARED_TYPES_H
#define INTERLOCK_SHARED_TYPES_H

enum InterlockCommandID {
    kInterlockGetStatus          = 0,
    kInterlockHideProcess        = 1,
    kInterlockReadPhysMemory     = 2,
    kInterlockWritePhysMemory    = 3,
    kInterlockSetStealthMode     = 4,
    kInterlockProtectProcess     = 5,
    kInterlockHidePort           = 6,
    kInterlockInstallNetFilter   = 7,
    kInterlockSetWifiSovereignty = 8,
    kInterlockBypassCaptive      = 9,
    kInterlockApplyJITPunch      = 10,
    kInterlockHideFile           = 11,
    kInterlockForgeEntitlements   = 12,
    kInterlockPostKernelEvent    = 13,
    kInterlockGeneratePayload    = 14,
    kInterlockRunValidation      = 15,
    kInterlockUpdateKnowledge    = 16,
    kInterlockNumberOfCommands
};

struct InterlockStatusResponse {
    uint32_t version;
    uint32_t status_flags; // bit 0: SIP_OFF, bit 1: LOCKDOWN_ON
    uint64_t kernel_base;
};

struct InterlockMemoryArgs {
    uint64_t address;
    uint64_t size;
};

// Mach IPC artifact type (synchronized with interlock_ipc.defs)
typedef char tact_msg_payload_t[1024];

#endif
