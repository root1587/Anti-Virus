#include "CenterAV64v1.1.h"

VOID PloadImageNotifyRoutine(
	PUNICODE_STRING FullImageName,
	HANDLE ProcessId,
	PIMAGE_INFO ImageInfo
) 
{
	UNREFERENCED_PARAMETER(ImageInfo);
	
	WCHAR *pwsName = NULL;
	if (FullImageName == NULL) {
		goto exit;
	}
	pwsName = (WCHAR *)ExAllocatePool(NonPagedPool, FullImageName->Length + sizeof(WCHAR));
	memcpy(pwsName, FullImageName->Buffer, FullImageName->Length);
	pwsName[FullImageName->Length / sizeof(WCHAR)] = 0;
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "(PID:0x%X) Process, (%ws) is loading ImageName :%wZ ImageBase: 0x%X \n", ProcessId, pwsName, FullImageName, ImageInfo->ImageBase);

	if (ImageInfo->SystemModeImage)
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "(PID:0x%X) Process, Driver:(%ws) is loading ImageName :%wZ ImageBase: 0x%X \n", ProcessId, pwsName, FullImageName, ImageInfo->ImageBase);
	else
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "(PID:0x%X) Process, DLL:(%ws) is loading ImageName :%wZ ImageBase: 0x%X \n", ProcessId, pwsName, FullImageName, ImageInfo->ImageBase);

exit:
	if (pwsName) {
		ExFreePool(pwsName);
	}
}

NTSTATUS ImageSearch()
{
	NTSTATUS status = PsSetLoadImageNotifyRoutine(PloadImageNotifyRoutine);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Faild to PsRemoveLoadImageNotifyRoutine .status : 0x%X \n", status);
		return status;
	}
	else {
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Success to PsRemoveLoadImageNotifyRoutine");
	}

	return STATUS_SUCCESS;
}

NTSTATUS UnImageSearch() 
{
	NTSTATUS status = PsRemoveLoadImageNotifyRoutine(PloadImageNotifyRoutine);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Faild to UnPsRemoveLoadImageNotifyRoutine .status : 0x%X \n", status);
	}
	else {
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Success to UnPsRemoveLoadImageNotifyRoutine");
	}

	return STATUS_SUCCESS;
}