#pragma once
#include "fltKernel.h"
#include "common.h"
#define SIMPLE_TAG 'SIMT'
#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, OnSimpleFilterInstanceSetup)
#pragma alloc_text(PAGE, OnPreCreateFile)
#endif

//�̴����Ͱ������ʿ����ʼ�����
FILTER_DATA g_filterData = { 0 };

//���Ͱ����ڿ�������ݹ�鱸��
const FLT_OPERATION_REGISTRATION g_callbacks[] =
{
	{
		IRP_MJ_CREATE, //������IRP �޽���
		0,
		OnPreCreateFile, //PreEvent������Ѵ�.
		NULL
	},
	{
		IRP_MJ_OPERATION_END //�ݹ����̳��������˸�   
	}
};

const FLT_CONTEXT_REGISTRATION g_contextRegistration[] =
{
	{
		FLT_STREAMHANDLE_CONTEXT,
		0,
		NULL,
		sizeof(FILTER_STREAM_HANDLE_CONTEXT),
		SIMPLE_TAG,
		NULL,
		NULL,
		NULL
	},
	{
		FLT_CONTEXT_END        //�������˸�
	}
};

const FLT_REGISTRATION g_FilterRegistration =
{
	sizeof(FLT_REGISTRATION),					 //  Size
	FLT_REGISTRATION_VERSION,					 //  Version
	0,											 //  Flags
	(const)g_contextRegistration,                //  Context Registration.
	(const)g_callbacks,                          //  Operation callbacks
	OnSimpleFilterUnload,                        //  FilterUnload
	OnSimpleFilterInstanceSetup,                 //  InstanceSetup
	NULL,									     //  InstanceQueryTeardown
	NULL,								         //  InstanceTeardownStart
	NULL,									     //  InstanceTeardownComplete
	NULL,									     //  GenerateFileName
	NULL,										 //  GenerateDestinationFileName
	NULL										 //  NormalizeNameComponent
};

//FILE I/O��������Ȯ���ڸ�ϵ��
//�����������ؽ�Ʈ���Ͽ��������ٱ��������������ϵ����ϰڴ�.
const UNICODE_STRING g_myExtensions[] =
{
	RTL_CONSTANT_STRING(L"txt"),
	{ 0,0,NULL }, 
};

//SimpleFilter
//�������λ��ɹ̴����������Ǳ���ü����
typedef struct _FILTER_DATA{
	PFLT_FILTER pFilter;                  //���͸Ŵ�������ϵȹ̴���������
	PFLT_PORT      pIocpServerPort;       //IOCP ������Ʈ
	PFLT_PORT      pIocpClientPort;       //IOCP Ŭ���̾�Ʈ��Ʈ
	PEPROCESS      pUserProcess;          //���ø����̼����μ���
}FILTER_DATA, *PFILTER_DATA;

//�������ؽ�Ʈ�ε�ϵɱ���ü
typedef struct _FILTER_STREAM_HANDLE_CONTEXT
{
	ULONG   reserved1;     //���Ƿο����ص�
}FILTER_STREAM_HANDLE_CONTEXT, *PFILTER_STREAM_HANDLE_CONTEXT;

//���;�ε��ƾ
NTSTATUS FLTAPI OnSimpleFilterUnload(FLT_FILTER_UNLOAD_FLAGS flags)
{
	UNREFERENCED_PARAMETER(flags);
	FltUnregisterFilter(g_filterData.pFilter);

	return STATUS_SUCCESS;

}

//�̴������ν��Ͻ��������ɶ����ݹ�
NTSTATUS FLTAPI OnSimpleFilterInstanceSetup(PCFLT_RELATED_OBJECTS pFltObjects, FLT_INSTANCE_SETUP_FLAGS flags, DEVICE_TYPE volumeDeviceType, FLT_FILESYSTEM_TYPE volumeFilesystemType)
{
	UNREFERENCED_PARAMETER(pFltObjects);
	UNREFERENCED_PARAMETER(flags);
	UNREFERENCED_PARAMETER(volumeDeviceType);

	PAGED_CODE();
	ASSERT(pFltObjects->Filter == g_filterData.pFilter);

	//��������������Ʈ��ũ��ġ�ϰ�쿡�����͸���������ʴ´�.
	if (volumeDeviceType == FILE_DEVICE_NETWORK_FILE_SYSTEM)
	{
		return STATUS_FLT_DO_NOT_ATTACH;
	}
	
	return STATUS_SUCCESS;
}

//�����������ϱ����Ͽ�Ȯ���ڸ�üũ�ϴ��Լ�
BOOLEAN IsMyExtension(PUNICODE_STRING pExtension)
{
	const UNICODE_STRING *ext;

	//���Ҵ���ǹ��ڿ����̰�0�̶��ƹ��͵����°��̴�.
	if (pExtension->Length == 0)
	{
		return FALSE;
	}
	
	ext = g_myExtensions;

	//�Ѿ��Ȯ���ڰ�ext�Ǹ���Ʈ�͵����ϴٸ�TRUE����ȯ���ش�.
	//�ش��ϴ�Ȯ���ڸ����͸��ϱ������̴�.
	while (ext->Buffer != NULL)
	{
		if (RtlCompareUnicodeString(pExtension, ext, TRUE) == 0)
		{
			return TRUE;
		}

		ext++;
	}

	return FALSE;
}

//�츮�������ǻ����׸������������ϱ����Ͽ������ϰ�MJ_CREATE �κп���ó�������ָ�ȴ�.
//�̿����̺�Ʈ�����ؼ������Ĵٷ絵���ϰڴ�.
//IRP_MJ_CREATE �ݹ���(Pre)
FLT_PREOP_CALLBACK_STATUS FLTAPI OnPreCreateFile(PFLT_CALLBACK_DATA pData, PCFLT_RELATED_OBJECTS pFltObjects, PVOID* ppCompletionContext)
{
	NTSTATUS status;
	PFLT_FILE_NAME_INFORMATION nameInfo;
	BOOLEAN hasExtension;

	UNREFERENCED_PARAMETER(pFltObjects);
	UNREFERENCED_PARAMETER(ppCompletionContext);
	
	PAGED_CODE();
	
	//�������̸����������������´�.
	status = FltGetFileNameInformation(pData, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &nameInfo);

	//���Ͽ������������о�ü����ٸ��ش����͵���̹��¸����������ģ�����ΰ����Ѵ�.
	if (!NT_SUCCESS(status))
	{
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	FltParseFileNameInformation(nameInfo);

	//������Ȯ���ڰ��ִٸ���丮���ƴҰ��̴�. ����, �����������Ͽ����õȺκ��̶�����Ҽ��ְڴ�.
	if (nameInfo->Extension.Buffer != UNICODE_NULL)
	{
		//����������������Ȯ���ڿ����Ѻκ����κ񱳸��Ѵ�.
		hasExtension = IsMyExtension(&nameInfo->Extension);

		//�������ʹ����Ȯ���ڶ��..?
		if (hasExtension == TRUE)
		{
			//�����Ҵ��nameInfo�������ڿ���������������Ѵ�.
			FltReleaseFileNameInformation(nameInfo);

			//����׺信�ش�޽���������Ѵ�.
			DbgPrint("Is Text File.. Can not Open\n");

			//�������ڿ������������������ϰ�, ���¸��źεǾ������ε����ش�.
			FltCancelFileOpen(pFltObjects->Instance, pFltObjects->FileObject);

			pData->IoStatus.Status = STATUS_ACCESS_DENIED;
			pData->IoStatus.Information = 0;

			//����۾��̿Ϸ�Ǿ����������ش�.
			return FLT_PREOP_COMPLETE;
		}
	}

	//�Ҵ��nameInfo�������ڿ���������Ų��.
	FltReleaseFileNameInformation(nameInfo);

	//�츮������Post Callback�����������ʾ����Ƿ�
	//IRP_MJ_CREATE �κп����Ѹ���۾������������뺸�Ѵ�.
	return FLT_PREOP_SUCCESS_NO_CALLBACK;
}


NTSTATUS KernelSocket(PDRIVER_OBJECT pDriverObject)
{
	FILTER_DATA* g_filterData;
	PSECURITY_DESCRIPTOR pSD;

	NTSTATUS status = FltRegisterFilter(pDriverObject, &g_FilterRegistration, &g_filterData->pFilter);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Faild to FltRegisterFilter(Socket) .status : 0x%X \n", status);
		return status;
	}
	else {
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Success to FltRegisterFilter(Socket)");

		pSD = NULL;

		status = FltBuildDefaultSecurityDescriptor(&pSD, FLT_PORT_ALL_ACCESS);
		if (NT_SUCCESS(status))
		{
			OBJECT_ATTRIBUTES oa = { 0 };
			UNICODE_STRING myPortName = { 0 };
			RtlInitUnicodeString(&myPortName, FLT_PORT_NAME);
			InitializeObjectAttributes(&oa, &myPortName, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, pSD);

			//���ȼ���������
			FltFreeSecurityDescriptor(pSD);

			//���͸��������Ѵ�.
			status = FltStartFiltering(g_filterData->pFilter);

			if (NT_SUCCESS(status))
			{
				DbgPrint("Success Load Filter Driver!\n");
				return STATUS_SUCCESS;
			}
		} else	{
			KdPrint(("���ȼ����ڻ�������. status : 0x%X\n", status));
			//���͵���������մϴ�.
			FltUnregisterFilter(g_filterData->pFilter);
		}	
	}

	return STATUS_SUCCESS;
}