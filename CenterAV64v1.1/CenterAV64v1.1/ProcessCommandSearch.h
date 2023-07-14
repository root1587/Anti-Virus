#include "CenterAV64v1.1.h"

#define SEND_BUFFER_SIZE 80

VOID CreateProcessNotifyEx(
	_Inout_   PEPROCESS Process,
	_In_      HANDLE ProcessId,
	_In_opt_  PPS_CREATE_NOTIFY_INFO CreateInfo
)
{
	UNREFERENCED_PARAMETER(Process);
	if (CreateInfo)
	{
		if (CreateInfo->FileOpenNameAvailable == TRUE)
		{			
			NTSTATUS status2;

			// IRP 통신작업
			SendMessage.Buffer = NULL;
			SendMessage.Buffer = ExAllocatePool(NonPagedPool, SEND_BUFFER_SIZE);
			SendMessage.MaximumLength = SEND_BUFFER_SIZE;
			status2 = RtlStringCchPrintfExA(SendMessage.Buffer, SendMessage.MaximumLength, NULL,NULL, STRSAFE_FILL_BEHIND_NULL | STRSAFE_IGNORE_NULLS,
				"%wZ", CreateInfo->CommandLine);
			
			if (!(NT_SUCCESS(status2))) {
				DbgPrint("Fail RtlStringCbPrintfExA\n");
			}
			else {
				DbgPrint("SUCCESS RtlStringCbPrintfExA\n");
				DbgPrint("CmdLine: %s\n", SendMessage.Buffer);
			}
			ExFreePool(SendMessage.Buffer);
	
			DbgPrintEx(
				DPFLTR_IHVDRIVER_ID,
				DPFLTR_INFO_LEVEL,
				"PID : 0x%X (%d)  ImageName :%wZ CmdLine : %wZ \n",
				ProcessId, ProcessId,
				CreateInfo->ImageFileName,
				CreateInfo->CommandLine
			);
		}
	}
}

NTSTATUS ProcessCommandSearch()
{
	NTSTATUS status = PsSetCreateProcessNotifyRoutineEx(CreateProcessNotifyEx, FALSE);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Faild to PsSetCreateProcessNotifyRoutineEx .status : 0x%X \n", status);
		return status;
	}
	else {
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Success to PsSetCreateProcessNotifyRoutineEx");
	}

	return STATUS_SUCCESS;
}

NTSTATUS UnProcessCommandSearch()
{
	NTSTATUS status = PsSetCreateProcessNotifyRoutineEx(CreateProcessNotifyEx, TRUE);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Faild to UnPsSetCreateProcessNotifyRoutineEx .status : 0x%X \n", status);
	}
	else {
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Success to UnPsSetCreateProcessNotifyRoutineEx");
	}

	return STATUS_SUCCESS;
}