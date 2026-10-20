#include <IOKit/IOUserClient.h>
#include "interlock_shield.hpp"

class com_interlock_ShieldUserClient : public IOUserClient {
    OSDeclareDefaultStructors(com_interlock_ShieldUserClient);

public:
    virtual bool initWithTask(task_t owningTask, void *securityID, UInt32 type) override;
    virtual bool start(IOService *provider) override;
    virtual void stop(IOService *provider) override;
    virtual IOReturn clientClose() override;
    virtual IOReturn clientDied() override;

    virtual IOReturn externalMethod(uint32_t selector, IOExternalMethodArguments *arguments,
                                    IOExternalMethodDispatch *dispatch, OSObject *target, void *reference) override;

    // Static dispatchers for external methods
    static IOReturn sGetStatus(com_interlock_ShieldUserClient *target, void *reference, IOExternalMethodArguments *arguments);
    static IOReturn sReadPhysMemory(com_interlock_ShieldUserClient *target, void *reference, IOExternalMethodArguments *arguments);
    static IOReturn sWritePhysMemory(com_interlock_ShieldUserClient *target, void *reference, IOExternalMethodArguments *arguments);
    static IOReturn sHideProcess(com_interlock_ShieldUserClient *target, void *reference, IOExternalMethodArguments *arguments);
    static IOReturn sProtectProcess(com_interlock_ShieldUserClient *target, void *reference, IOExternalMethodArguments *arguments);
    static IOReturn sHidePort(com_interlock_ShieldUserClient *target, void *reference, IOExternalMethodArguments *arguments);
    static IOReturn sInstallNetFilter(com_interlock_ShieldUserClient *target, void *reference, IOExternalMethodArguments *arguments);
    static IOReturn sSetWifiSovereignty(com_interlock_ShieldUserClient *target, void *reference, IOExternalMethodArguments *arguments);
    static IOReturn sBypassCaptive(com_interlock_ShieldUserClient *target, void *reference, IOExternalMethodArguments *arguments);
    static IOReturn sApplyJITPunch(com_interlock_ShieldUserClient *target, void *reference, IOExternalMethodArguments *arguments);
    static IOReturn sHideFile(com_interlock_ShieldUserClient *target, void *reference, IOExternalMethodArguments *arguments);
    static IOReturn sForgeEntitlements(com_interlock_ShieldUserClient *target, void *reference, IOExternalMethodArguments *arguments);
    static IOReturn sPostKernelEvent(com_interlock_ShieldUserClient *target, void *reference, IOExternalMethodArguments *arguments);
    static IOReturn sGeneratePayload(com_interlock_ShieldUserClient *target, void *reference, IOExternalMethodArguments *arguments);
    static IOReturn sRunValidation(com_interlock_ShieldUserClient *target, void *reference, IOExternalMethodArguments *arguments);
    static IOReturn sUpdateKnowledge(com_interlock_ShieldUserClient *target, void *reference, IOExternalMethodArguments *arguments);

protected:
    com_interlock_Shield *fProvider;
};
