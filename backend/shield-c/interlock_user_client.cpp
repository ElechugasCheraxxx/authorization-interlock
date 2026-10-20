#include <IOKit/IOLib.h>
#include <IOKit/IOUserClient.h>
#include "interlock_user_client.hpp"
#include "interlock_shared_types.h"

#define Log(fmt, ...) IOLog("InterlockUserClient: " fmt "\n", ##__VA_ARGS__)

OSDefineMetaClassAndStructors(com_interlock_ShieldUserClient, IOUserClient)

// Method dispatch table
static const IOExternalMethodDispatch sMethods[kInterlockNumberOfCommands] = {
    [kInterlockGetStatus] = {
        (IOExternalMethodAction) &com_interlock_ShieldUserClient::sGetStatus,
        0,                                   // scalarInputCount
        0,                                   // structInputSize
        0,                                   // scalarOutputCount
        sizeof(InterlockStatusResponse)       // structOutputSize
    },
    [kInterlockReadPhysMemory] = {
        (IOExternalMethodAction) &com_interlock_ShieldUserClient::sReadPhysMemory,
        0,                                   // scalarInputCount
        sizeof(InterlockMemoryArgs),          // structInputSize
        0,                                   // scalarOutputCount
        0                                    // structOutputSize (Uses out-of-line memory or arguments->structureOutput)
    },
    [kInterlockWritePhysMemory] = {
        (IOExternalMethodAction) &com_interlock_ShieldUserClient::sWritePhysMemory,
        0,                                   // scalarInputCount
        sizeof(InterlockMemoryArgs),          // structInputSize
        0,                                   // scalarOutputCount
        0                                    // structOutputSize
    },
    [kInterlockHideProcess] = {
        (IOExternalMethodAction) &com_interlock_ShieldUserClient::sHideProcess,
        1,                                   // scalarInputCount (pid)
        0,                                   // structInputSize
        0,                                   // scalarOutputCount
        0                                    // structOutputSize
    },
    [kInterlockProtectProcess] = {
        (IOExternalMethodAction) &com_interlock_ShieldUserClient::sProtectProcess,
        1,                                   // scalarInputCount (pid)
        0,                                   // structInputSize
        0,                                   // scalarOutputCount
        0                                    // structOutputSize
    },
    [kInterlockHidePort] = {
        (IOExternalMethodAction) &com_interlock_ShieldUserClient::sHidePort,
        1,                                   // scalarInputCount (port)
        0,                                   // structInputSize
        0,                                   // scalarOutputCount
        0                                    // structOutputSize
    },
    [kInterlockInstallNetFilter] = {
        (IOExternalMethodAction) &com_interlock_ShieldUserClient::sInstallNetFilter,
        0,                                   // scalarInputCount
        64,                                  // structInputSize (interface name)
        0,                                   // scalarOutputCount
        0                                    // structOutputSize
    },
    [kInterlockSetWifiSovereignty] = {
        (IOExternalMethodAction) &com_interlock_ShieldUserClient::sSetWifiSovereignty,
        1,                                   // scalarInputCount (enable)
        0,                                   // structInputSize
        0,                                   // scalarOutputCount
        0                                    // structOutputSize
    },
    [kInterlockBypassCaptive] = {
        (IOExternalMethodAction) &com_interlock_ShieldUserClient::sBypassCaptive,
        0,                                   // scalarInputCount
        0,                                   // structInputSize
        0,                                   // scalarOutputCount
        0                                    // structOutputSize
    },
    [kInterlockApplyJITPunch] = {
        (IOExternalMethodAction) &com_interlock_ShieldUserClient::sApplyJITPunch,
        1,                                   // scalarInputCount (pid)
        0,                                   // structInputSize
        0,                                   // scalarOutputCount
        0                                    // structOutputSize
    },
    [kInterlockHideFile] = {
        (IOExternalMethodAction) &com_interlock_ShieldUserClient::sHideFile,
        0,                                   // scalarInputCount
        1024,                                // structInputSize (path)
        0,                                   // scalarOutputCount
        0                                    // structOutputSize
    },
    [kInterlockForgeEntitlements] = {
        (IOExternalMethodAction) &com_interlock_ShieldUserClient::sForgeEntitlements,
        1,                                   // scalarInputCount (pid)
        0,                                   // structInputSize
        0,                                   // scalarOutputCount
        0                                    // structOutputSize
    },
    [kInterlockPostKernelEvent] = {
        (IOExternalMethodAction) &com_interlock_ShieldUserClient::sPostKernelEvent,
        1,                                   // scalarInputCount (eventType)
        1024,                                // structInputSize (data)
        0,                                   // scalarOutputCount
        0                                    // structOutputSize
    },
    [kInterlockGeneratePayload] = {
        (IOExternalMethodAction) &com_interlock_ShieldUserClient::sGeneratePayload,
        1,                                   // scalarInputCount (type)
        0,                                   // structInputSize
        0,                                   // scalarOutputCount
        1024                                 // structOutputSize (artifact)
    },
    [kInterlockRunValidation] = {
        (IOExternalMethodAction) &com_interlock_ShieldUserClient::sRunValidation,
        1,                                   // scalarInputCount (level)
        0,                                   // structInputSize
        0,                                   // scalarOutputCount
        0                                    // structOutputSize
    },
    [kInterlockUpdateKnowledge] = {
        (IOExternalMethodAction) &com_interlock_ShieldUserClient::sUpdateKnowledge,
        0,                                   // scalarInputCount
        0,                                   // structInputSize
        0,                                   // scalarOutputCount
        0                                    // structOutputSize
    }
};

bool com_interlock_ShieldUserClient::initWithTask(task_t owningTask, void *securityID, UInt32 type) {
    if (!IOUserClient::initWithTask(owningTask, securityID, type)) return false;
    if (clientHasPrivilege(securityID, kIOClientPrivilegeAdministrator) != kIOReturnSuccess) {
        return false;
    }
    return true;
}

bool com_interlock_ShieldUserClient::start(IOService *provider) {
    fProvider = OSDynamicCast(com_interlock_Shield, provider);
    if (!fProvider || !IOUserClient::start(provider)) return false;
    return true;
}

void com_interlock_ShieldUserClient::stop(IOService *provider) {
    IOUserClient::stop(provider);
}

IOReturn com_interlock_ShieldUserClient::clientClose() {
    terminate();
    return kIOReturnSuccess;
}

IOReturn com_interlock_ShieldUserClient::clientDied() {
    return clientClose();
}

IOReturn com_interlock_ShieldUserClient::externalMethod(uint32_t selector, IOExternalMethodArguments *arguments,
                                                       IOExternalMethodDispatch *dispatch, OSObject *target, void *reference) {
    if (selector >= kInterlockNumberOfCommands) return kIOReturnBadArgument;

    dispatch = (IOExternalMethodDispatch *) &sMethods[selector];
    target = this;
    reference = nullptr;

    return IOUserClient::externalMethod(selector, arguments, dispatch, target, reference);
}

IOReturn com_interlock_ShieldUserClient::sWritePhysMemory(com_interlock_ShieldUserClient *target, void *reference, IOExternalMethodArguments *arguments) {
    if (!target) return kIOReturnBadArgument;

    InterlockMemoryArgs *args = (InterlockMemoryArgs *) arguments->structureInput;
    IOMemoryDescriptor *memDesc = arguments->structureInputDescriptor;

    if (!args || !memDesc) return kIOReturnBadArgument;

    // Map the input descriptor into kernel space to read the source data
    IOMemoryMap* map = memDesc->map();
    if (!map) return kIOReturnVMError;

    void* srcBuffer = (void*)map->getVirtualAddress();
    IOReturn kr = target->fProvider->writePhysicalMemory(args->address, srcBuffer, args->size);

    map->release();
    return kr;
}

IOReturn com_interlock_ShieldUserClient::sReadPhysMemory(com_interlock_ShieldUserClient *target, void *reference, IOExternalMethodArguments *arguments) {
    if (!target) return kIOReturnBadArgument;

    InterlockMemoryArgs *args = (InterlockMemoryArgs *) arguments->structureInput;
    IOMemoryDescriptor *outDesc = arguments->structureOutputDescriptor;

    if (!args || !outDesc) return kIOReturnBadArgument;

    // Map the output descriptor into kernel space to write the data
    IOMemoryMap* map = outDesc->map();
    if (!map) return kIOReturnVMError;

    void* destBuffer = (void*)map->getVirtualAddress();
    IOReturn kr = target->fProvider->readPhysicalMemory(args->address, destBuffer, args->size);

    map->release();
    return kr;
}

IOReturn com_interlock_ShieldUserClient::sHideProcess(com_interlock_ShieldUserClient *target, void *reference, IOExternalMethodArguments *arguments) {
    if (!target) return kIOReturnBadArgument;
    pid_t pid = (pid_t)arguments->scalarInput[0];
    return target->fProvider->hideProcess(pid);
}

IOReturn com_interlock_ShieldUserClient::sProtectProcess(com_interlock_ShieldUserClient *target, void *reference, IOExternalMethodArguments *arguments) {
    if (!target) return kIOReturnBadArgument;
    pid_t pid = (pid_t)arguments->scalarInput[0];
    return target->fProvider->protectProcessFromSignals(pid);
}

IOReturn com_interlock_ShieldUserClient::sHidePort(com_interlock_ShieldUserClient *target, void *reference, IOExternalMethodArguments *arguments) {
    if (!target) return kIOReturnBadArgument;
    uint16_t port = (uint16_t)arguments->scalarInput[0];
    return target->fProvider->hideSocket(port);
}

IOReturn com_interlock_ShieldUserClient::sGetStatus(com_interlock_ShieldUserClient *target, void *reference, IOExternalMethodArguments *arguments) {
    if (!target) return kIOReturnBadArgument;

    InterlockStatusResponse *res = (InterlockStatusResponse *)arguments->structureOutput;
    if (!res) return kIOReturnBadArgument;

    res->version = 0x0100;
    res->status_flags = 0x2; // LOCKDOWN_ON by default for Sovereign mode

    // Call the provider's internal state directly
    InterlockStatusResponse internalStatus = {0};
    // We assume there's a way to get the kernel base from the provider
    // Since we are inside the UserClient, we can access fProvider's private members if needed (friend class)
    // or use a public getter.

    // For now, let's just use the fact that fProvider is initialized.
    return target->fProvider->postKernelEvent(0xFF, res, sizeof(InterlockStatusResponse));
}

IOReturn com_interlock_ShieldUserClient::sInstallNetFilter(com_interlock_ShieldUserClient *target, void *reference, IOExternalMethodArguments *arguments) {
    if (!target) return kIOReturnBadArgument;
    const char* ifname = (const char*)arguments->structureInput;
    return target->fProvider->installNetworkFilter(ifname);
}

IOReturn com_interlock_ShieldUserClient::sSetWifiSovereignty(com_interlock_ShieldUserClient *target, void *reference, IOExternalMethodArguments *arguments) {
    if (!target) return kIOReturnBadArgument;
    bool enable = arguments->scalarInput[0] != 0;
    return target->fProvider->setWifiSovereignty(enable);
}

IOReturn com_interlock_ShieldUserClient::sBypassCaptive(com_interlock_ShieldUserClient *target, void *reference, IOExternalMethodArguments *arguments) {
    if (!target) return kIOReturnBadArgument;
    return target->fProvider->bypassCaptivePortal();
}

IOReturn com_interlock_ShieldUserClient::sApplyJITPunch(com_interlock_ShieldUserClient *target, void *reference, IOExternalMethodArguments *arguments) {
    if (!target) return kIOReturnBadArgument;
    pid_t pid = (pid_t)arguments->scalarInput[0];
    return target->fProvider->applyJITPunchHole(pid);
}

IOReturn com_interlock_ShieldUserClient::sHideFile(com_interlock_ShieldUserClient *target, void *reference, IOExternalMethodArguments *arguments) {
    if (!target) return kIOReturnBadArgument;
    const char* path = (const char*)arguments->structureInput;
    if (!path) return kIOReturnBadArgument;
    return target->fProvider->hideFile(path);
}

IOReturn com_interlock_ShieldUserClient::sForgeEntitlements(com_interlock_ShieldUserClient *target, void *reference, IOExternalMethodArguments *arguments) {
    if (!target) return kIOReturnBadArgument;
    pid_t pid = (pid_t)arguments->scalarInput[0];
    return target->fProvider->forgeEntitlements(pid);
}

IOReturn com_interlock_ShieldUserClient::sPostKernelEvent(com_interlock_ShieldUserClient *target, void *reference, IOExternalMethodArguments *arguments) {
    if (!target) return kIOReturnBadArgument;
    uint32_t eventType = (uint32_t)arguments->scalarInput[0];
    void* data = (void*)arguments->structureInput;
    uint32_t size = arguments->structureInputSize;
    return target->fProvider->postKernelEvent(eventType, data, size);
}

IOReturn com_interlock_ShieldUserClient::sGeneratePayload(com_interlock_ShieldUserClient *target, void *reference, IOExternalMethodArguments *arguments) {
    if (!target) return kIOReturnBadArgument;
    uint32_t type = (uint32_t)arguments->scalarInput[0];
    void* output = (void*)arguments->structureOutput;
    uint32_t size = (uint32_t)arguments->structureOutputSize;
    return target->fProvider->generatePayload(type, output, &size);
}

IOReturn com_interlock_ShieldUserClient::sRunValidation(com_interlock_ShieldUserClient *target, void *reference, IOExternalMethodArguments *arguments) {
    if (!target) return kIOReturnBadArgument;
    uint32_t level = (uint32_t)arguments->scalarInput[0];
    return target->fProvider->runValidationProtocol(level);
}

IOReturn com_interlock_ShieldUserClient::sUpdateKnowledge(com_interlock_ShieldUserClient *target, void *reference, IOExternalMethodArguments *arguments) {
    if (!target) return kIOReturnBadArgument;
    target->fProvider->updateKnowledgeBase();
    return kIOReturnSuccess;
}
