#include "CenterAV64v1.1.h"

VOID ProcessNotifyCallBackEx(
	PEPROCESS Process,
	HANDLE ProcessId,
	PPS_CREATE_NOTIFY_INFO CreateInfo
)
{
	UNICODE_STRING ExecutableBlocked[] = {
		RTL_CONSTANT_STRING(L"*OLLYDBG*.EXE"),
		RTL_CONSTANT_STRING(L"*MSPAINT*.EXE"),
		RTL_CONSTANT_STRING(L"*NOTEPAD*.EXE")
	};

	ULONG ExecutableCount = sizeof(ExecutableBlocked) / sizeof(UNICODE_STRING);

	BOOLEAN Matched = FALSE;
	ULONG Idx;

	UNREFERENCED_PARAMETER(ProcessId);
	UNREFERENCED_PARAMETER(Process);

	if (CreateInfo)
	{
		for (Idx = 0; Idx < ExecutableCount; Idx++) {
			if (FsRtlIsNameInExpression(&ExecutableBlocked[Idx], (PUNICODE_STRING)CreateInfo->ImageFileName, TRUE, NULL)){
				Matched = TRUE;
				break;
			}
		}

		if (Matched) {
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[Process Protect] Preventing Process (%wZ) Execution\n", CreateInfo->ImageFileName);
			CreateInfo->CreationStatus = STATUS_ACCESS_DENIED;
		} else {
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[Process Protect] Starting Process: %wZ\n", CreateInfo->ImageFileName);
		}
	}

	return;
}


NTSTATUS ProcessProtect()
{
	NTSTATUS status = PsSetCreateProcessNotifyRoutineEx((PCREATE_PROCESS_NOTIFY_ROUTINE_EX)ProcessNotifyCallBackEx, FALSE);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Faild to PsSetCreateProcessNotifyRoutineEx .status : 0x%X \n", status);
		return status;
	}
	else {
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Success to PsSetCreateProcessNotifyRoutineEx");
	}

	return STATUS_SUCCESS;
}

NTSTATUS UnProcessProtect()
{
	NTSTATUS status = PsSetCreateProcessNotifyRoutineEx((PCREATE_PROCESS_NOTIFY_ROUTINE_EX)ProcessNotifyCallBackEx, TRUE);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Faild to UnPsSetCreateProcessNotifyRoutineEx .status : 0x%X \n", status);
	}
	else {
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Success to UnPsSetCreateProcessNotifyRoutineEx");
	}

	return STATUS_SUCCESS;
}