#include "CenterAV64v1.1.h"
#include "stdio.h"

char * PsGetProcessImageFileName(PEPROCESS p);

typedef struct _GLOBAL_CONTEXT {
	PDRIVER_OBJECT DriverObject;
	UNICODE_STRING Altitude;
	LARGE_INTEGER Cookie;
} GLOBAL_CONTEXT, *PGLOBAL_CONTEXT;

GLOBAL_CONTEXT g_GlobalContext = { 0 };

UNICODE_STRING g_PolicyKeyArray[] = {
	RTL_CONSTANT_STRING(L"\\REGISTRY\\MACHINE\\System\\CurrentControlSet\\Services\\TAN1"),
	RTL_CONSTANT_STRING(L"\\REGISTRY\\MACHINE\\System\\ControlSet001\\Services\\TAN1"),
	RTL_CONSTANT_STRING(L"\\REGISTRY\\MACHINE\\System\\ControlSet002\\Services\\TAN1"),
	RTL_CONSTANT_STRING(L"\\REGISTRY\\MACHINE\\System\\ControlSet003\\Services\\TAN1")
};
ULONG g_PolicyKeyCount = sizeof(g_PolicyKeyArray) / sizeof(UNICODE_STRING);


BOOLEAN CheckProcess(VOID) {
	PEPROCESS  Process;
	PCHAR ImageFileName;

	Process = PsGetCurrentProcess();

	ImageFileName = PsGetProcessImageFileName(Process);

	if (_stricmp(ImageFileName, "services.exe") == 0) {
		return TRUE;
	}

	if (_stricmp(ImageFileName, "svchost.exe") == 0) {
		return TRUE;
	}

	return FALSE;
}


BOOLEAN CheckPolicy(PUNICODE_STRING KeyFullPath) {

	BOOLEAN Matched = FALSE;
	ULONG Idx;

	for (Idx = 0; Idx < g_PolicyKeyCount; Idx++) {
		if (RtlEqualUnicodeString(KeyFullPath, &g_PolicyKeyArray[Idx], TRUE)) {
			Matched = TRUE;
			break;
		}
	}

	if (Matched) {
		DbgPrint("[ RegMonitor ] pid(%x) and tid(%x) Block %wZ\n",
			PsGetCurrentProcessId(), PsGetCurrentThreadId(), KeyFullPath);
	}

	return Matched;
}


NTSTATUS RegPreDeleteKey(PVOID RootObject, PUNICODE_STRING CompleteName)
{
	PUNICODE_STRING RootObjectName;
	ULONG_PTR RootObjectID;
	BOOLEAN Matched = FALSE;
	NTSTATUS Status;
	UNICODE_STRING KeyPath = { 0 };

	// CompleteName은 절대 경로 또는 상대 경로를 가짐
	// RootObject가 유효하지 않으면 CompleteName에 전체 경로가 있음을 의미
	// RootObject가 유효하면 더 많이 작업이 필요

	if (RootObject) {

		// RootObject에서 RootObjectName으로의 경로를 저장 CmCallbackGetKeyObjectID()
		if (!NT_SUCCESS(Status = CmCallbackGetKeyObjectID(&g_GlobalContext.Cookie, RootObject, &RootObjectID, &RootObjectName)))
		{
			DbgPrint("[ RegMonitor ] [ ERROR ] CmCallbackGetKeyObjectID : %x\n", Status);
			goto Exit;
		}

		// 유효한 CompleteName이 있으면 RootObjectName과 CompleteName을 연결해야 함
		// 없으면 RootObjectName만 사용
		if (CompleteName->Length && CompleteName->Buffer) {

			KeyPath.MaximumLength = RootObjectName->Length + CompleteName->Length + (sizeof(WCHAR) * 2);

			KeyPath.Buffer = (PWCH)ExAllocatePoolWithTag(NonPagedPool, KeyPath.MaximumLength, 'pkMC');

			if (!KeyPath.Buffer) {
				DbgPrint("[ RegMonitor ] [ Error ] ExAllocatePool() FAIL\n");
				goto Exit;
			}

			swprintf(KeyPath.Buffer, L"%wZ\\%wZ", RootObjectName, CompleteName);
			KeyPath.Length = RootObjectName->Length + CompleteName->Length + (sizeof(WCHAR));

			Matched = CheckPolicy(&KeyPath);
		}
		else {
			Matched = CheckPolicy(RootObjectName);

		}
	}
	else {
		Matched = CheckPolicy(CompleteName);
	}

Exit:
	// KeyPath.Buffer에 버퍼가 할당 된 경우 해제
	if (KeyPath.Buffer) {
		ExFreePool(KeyPath.Buffer);
	}
	return Matched;
}

NTSTATUS RegistryFilterCallback(
	IN PVOID CallbackContext,
	IN PVOID Argument1,
	IN PVOID Argument2
)
{
	NTSTATUS Status = STATUS_SUCCESS;
	REG_NOTIFY_CLASS NotifyClass = (REG_NOTIFY_CLASS)(ULONG_PTR)Argument1;

	UNREFERENCED_PARAMETER(CallbackContext);

	if (CheckProcess()) {
		return STATUS_SUCCESS;
	}

	if (RegNtPreCreateKeyEx == NotifyClass || RegNtPreOpenKeyEx == NotifyClass)
	{
		PREG_CREATE_KEY_INFORMATION RegInformation = (PREG_CREATE_KEY_INFORMATION)Argument2;

		if (RegPreDeleteKey(RegInformation->RootObject, RegInformation->CompleteName))
		{
			DbgPrint("[ RegMonitor ] Prevent Opening Handle\n");
			Status = STATUS_ACCESS_DENIED;
		}
	}

	return Status;
}



NTSTATUS RegisterProtect(IN PDRIVER_OBJECT DriverObject)
{
	NTSTATUS Status = STATUS_SUCCESS;
	RtlInitUnicodeString(&g_GlobalContext.Altitude, L"371000");
	g_GlobalContext.DriverObject = DriverObject;

	NTSTATUS status =CmRegisterCallbackEx(
		RegistryFilterCallback,
		&g_GlobalContext.Altitude,
		DriverObject,
		&g_GlobalContext,
		&g_GlobalContext.Cookie,
		NULL
	);

	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[ RegMonitor ] [ ERROR ] CmRegisterCallbackEx Failed : (%x)\n", status);
		return status;
	}
	else {
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[ RegMonitor ] [ SUCCESS ] CmRegisterCallbackEx Success\n");
	}

	return STATUS_SUCCESS;
}

NTSTATUS UnRegisterProtect()
{
	NTSTATUS status;

	if (!NT_SUCCESS(status = CmUnRegisterCallback(g_GlobalContext.Cookie))) {
		DbgPrint("[ RegMonitor ] [ ERROR ] CmUnRegisterCallback Failed (%x)\n", status);
		return status;
	}
	else {
		DbgPrint("[ RegMonitor ] [ SUCCESS ] CmUnRegisterCallback Success\n");
	}
	return STATUS_SUCCESS;
}