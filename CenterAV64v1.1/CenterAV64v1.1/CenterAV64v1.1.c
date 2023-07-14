#include "CenterAV64v1.1.h"
#include "ThreadSearch.h"
#include "ProcessCommandSearch.h"
#include "ImageSearch.h" 
#include "RegisterProtect.h"
#include "ProcessProtect.h"

// Create the device.
const WCHAR deviceNameBuffer[] = L"\\Device\\MYDEVICE";
const WCHAR deviceSymLinkBuffer[] = L"\\DosDevices\\MyDevice";
PDEVICE_OBJECT g_MyDevice;

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	//NTSTATUS status = STATUS_SUCCESS;
	// 언로드자리
	// dbg프린트

	NTSTATUS status = 0;
	UNICODE_STRING deviceNameUnicodeString, deviceSymLinkUnicodeString;

	RtlInitUnicodeString(&deviceNameUnicodeString, deviceNameBuffer);
	RtlInitUnicodeString(&deviceSymLinkUnicodeString, deviceSymLinkBuffer);

	status = IoCreateDevice(DriverObject,
		0, // 드라이버 예외를 위해
		&deviceNameUnicodeString,
		FILE_DEVICE_UNKNOWN,
		FILE_DEVICE_UNKNOWN,
		FALSE,
		&g_MyDevice);

	//  심볼릭링크 생성
	status = IoCreateSymbolicLink(&deviceSymLinkUnicodeString, &deviceNameUnicodeString);

	DriverObject->DriverUnload = DriverUnload;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = Function_IRP_MJ_CREATE;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = Function_IRP_MJ_CLOSE;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = Function_IRP_DEVICE_CONTROL;
	

	DbgPrint("[ CTAV v1.1 ] Driver Loaded\n");

	if (!NT_SUCCESS(status))
	{
		DbgPrint("Status not SUCCESS");
	}
	else {
		DbgPrint("Status SUCCESS");
	}
		
	ProcessCommandSearch();
	/*
	ThreadSearch();
	RegisterProtect(DriverObject);
	ProcessProtect();
	ImageSearch();
	*/

	return STATUS_SUCCESS;
}


VOID DriverUnload(IN PDRIVER_OBJECT DriverObject)
{
	
	UnProcessCommandSearch();
	/*
	UnThreadSearch();
	UnRegisterProtect(); 
	UnProcessProtect();
	UnImageSearch();
	*/

	
	UNICODE_STRING symLink;

	RtlInitUnicodeString(&symLink, deviceSymLinkBuffer);

	IoDeleteSymbolicLink(&symLink);
	IoDeleteDevice(DriverObject->DeviceObject);
	
	DbgPrint("[ CTAV v1.1 ] Unloaded\n");
}


NTSTATUS Function_IRP_MJ_CREATE(PDEVICE_OBJECT pDeviceObject, PIRP Irp)
{
	DbgPrint("IRP MJ CREATE received.\n");
	return STATUS_SUCCESS;
}


NTSTATUS Function_IRP_MJ_CLOSE(PDEVICE_OBJECT pDeviceObject, PIRP Irp)
{
	DbgPrint("IRP MJ CLOSE received.\n");
	return STATUS_SUCCESS;
}


NTSTATUS Function_IRP_DEVICE_CONTROL(PDEVICE_OBJECT pDeviceObject, PIRP Irp)
{
	PIO_STACK_LOCATION pIoStackLocation;
	//PCHAR welcome = "Hello from kerneland.";
	SendMessage.Length = (USHORT)strlen(SendMessage.Buffer);
	PVOID pBuf = Irp->AssociatedIrp.SystemBuffer;

	pIoStackLocation = IoGetCurrentIrpStackLocation(Irp);
	switch (pIoStackLocation->Parameters.DeviceIoControl.IoControlCode)
	{
	case IOCTL_HELLO:
		DbgPrint("IOCTL HELLO.");
		DbgPrint("Message received : %s", pBuf);
		
		RtlZeroMemory(pBuf, pIoStackLocation->Parameters.DeviceIoControl.InputBufferLength);
		RtlCopyMemory(pBuf, SendMessage.Buffer, SendMessage.Length);
		 
		break;
	}


	// 단순히 패킷을 완료하고 반환하여 I / O 작업을 완료
	// 패킷 자체와 동일한 상태
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = SendMessage.Length;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}