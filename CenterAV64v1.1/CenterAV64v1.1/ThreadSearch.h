#include "CenterAV64v1.1.h"

VOID CreateThreadNotifyCallBack(
	IN HANDLE ProcessId,
	IN HANDLE ThreadId,
	IN BOOLEAN Create
)
{
	switch (Create)
	{
	case TRUE:
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Process(ID:0x%X) is created Thread(ID:0x%X)\n", ProcessId, ThreadId);
		break;
	case FALSE:
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Process(ID:0x%X) is deleted Thread(ID:0x%X)\n", ProcessId, ThreadId);
		break;
	}
}

NTSTATUS ThreadSearch()
{
	NTSTATUS status = PsSetCreateThreadNotifyRoutine(CreateThreadNotifyCallBack);
	if (!NT_SUCCESS(status))
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[ERROR] Faild to PsSetCreateThreadNotifyRoutine .status : 0x%X \n", status);
		return status;
	}
	else {
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "SUCCESS to PsSetCreateThreadNotifyRoutine \n");
	}

	return STATUS_SUCCESS;
}

NTSTATUS UnThreadSearch()
{
	NTSTATUS status = PsRemoveCreateThreadNotifyRoutine(CreateThreadNotifyCallBack);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[ERROR] Faild to PsRemoveCreateThreadNotifyRoutine .status : 0x%X \n", status);
	}
	else {
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "SUCCESS to PsRemoveCreateThreadNotifyRoutine \n");
	}

	return STATUS_SUCCESS;
}