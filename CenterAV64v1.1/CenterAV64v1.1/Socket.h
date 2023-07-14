#pragma once
#include "fltKernel.h"
#include "common.h"
#define SIMPLE_TAG 'SIMT'
#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, OnSimpleFilterInstanceSetup)
#pragma alloc_text(PAGE, OnPreCreateFile)
#endif

//미니필터관리에필요한필수정보
FILTER_DATA g_filterData = { 0 };

//필터관리자에등록할콜백들구성
const FLT_OPERATION_REGISTRATION g_callbacks[] =
{
	{
		IRP_MJ_CREATE, //감지할IRP 메시지
		0,
		OnPreCreateFile, //PreEvent만등록한다.
		NULL
	},
	{
		IRP_MJ_OPERATION_END //콜백목록이끝났음을알림   
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
		FLT_CONTEXT_END        //목록종료알림
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

//FILE I/O를감시할확장자목록등록
//예제에서는텍스트파일에대한접근권한을전부제거하도록하겠다.
const UNICODE_STRING g_myExtensions[] =
{
	RTL_CONSTANT_STRING(L"txt"),
	{ 0,0,NULL }, 
};

//SimpleFilter
//전역으로사용될미니필터정보의구조체선언
typedef struct _FILTER_DATA{
	PFLT_FILTER pFilter;                  //필터매니저에등록된미니필터정보
	PFLT_PORT      pIocpServerPort;       //IOCP 서버포트
	PFLT_PORT      pIocpClientPort;       //IOCP 클라이언트포트
	PEPROCESS      pUserProcess;          //어플리케이션프로세스
}FILTER_DATA, *PFILTER_DATA;

//필터컨텍스트로등록될구조체
typedef struct _FILTER_STREAM_HANDLE_CONTEXT
{
	ULONG   reserved1;     //임의로예약해둠
}FILTER_STREAM_HANDLE_CONTEXT, *PFILTER_STREAM_HANDLE_CONTEXT;

//필터언로드루틴
NTSTATUS FLTAPI OnSimpleFilterUnload(FLT_FILTER_UNLOAD_FLAGS flags)
{
	UNREFERENCED_PARAMETER(flags);
	FltUnregisterFilter(g_filterData.pFilter);

	return STATUS_SUCCESS;

}

//미니필터인스턴스가생성될때의콜백
NTSTATUS FLTAPI OnSimpleFilterInstanceSetup(PCFLT_RELATED_OBJECTS pFltObjects, FLT_INSTANCE_SETUP_FLAGS flags, DEVICE_TYPE volumeDeviceType, FLT_FILESYSTEM_TYPE volumeFilesystemType)
{
	UNREFERENCED_PARAMETER(pFltObjects);
	UNREFERENCED_PARAMETER(flags);
	UNREFERENCED_PARAMETER(volumeDeviceType);

	PAGED_CODE();
	ASSERT(pFltObjects->Filter == g_filterData.pFilter);

	//볼륨의정보가네트워크장치일경우에는필터를등록하지않는다.
	if (volumeDeviceType == FILE_DEVICE_NETWORK_FILE_SYSTEM)
	{
		return STATUS_FLT_DO_NOT_ATTACH;
	}
	
	return STATUS_SUCCESS;
}

//접근제한을하기위하여확장자를체크하는함수
BOOLEAN IsMyExtension(PUNICODE_STRING pExtension)
{
	const UNICODE_STRING *ext;

	//비교할대상의문자열길이가0이라면아무것도없는것이다.
	if (pExtension->Length == 0)
	{
		return FALSE;
	}
	
	ext = g_myExtensions;

	//넘어온확장자가ext의리스트와동일하다면TRUE를반환해준다.
	//해당하는확장자만필터링하기위함이다.
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

//우리는파일의생성및모든권한을제어하기위하여간단하게MJ_CREATE 부분에서처리를해주면된다.
//이외의이벤트에대해서는차후다루도록하겠다.
//IRP_MJ_CREATE 콜백등록(Pre)
FLT_PREOP_CALLBACK_STATUS FLTAPI OnPreCreateFile(PFLT_CALLBACK_DATA pData, PCFLT_RELATED_OBJECTS pFltObjects, PVOID* ppCompletionContext)
{
	NTSTATUS status;
	PFLT_FILE_NAME_INFORMATION nameInfo;
	BOOLEAN hasExtension;

	UNREFERENCED_PARAMETER(pFltObjects);
	UNREFERENCED_PARAMETER(ppCompletionContext);
	
	PAGED_CODE();
	
	//파일의이름에대한정보를얻어온다.
	status = FltGetFileNameInformation(pData, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &nameInfo);

	//파일에대한정보를읽어올수없다면해당필터드라이버는모든할일을마친것으로간주한다.
	if (!NT_SUCCESS(status))
	{
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	FltParseFileNameInformation(nameInfo);

	//파일의확장자가있다면디렉토리가아닐것이다. 따라서, 직접적인파일에관련된부분이라생각할수있겠다.
	if (nameInfo->Extension.Buffer != UNICODE_NULL)
	{
		//얻어온파일의정보중확장자에대한부분으로비교를한다.
		hasExtension = IsMyExtension(&nameInfo->Extension);

		//만약필터대상의확장자라면..?
		if (hasExtension == TRUE)
		{
			//먼저할당된nameInfo에대한자원을해제시켜줘야한다.
			FltReleaseFileNameInformation(nameInfo);

			//디버그뷰에해당메시지를출력한다.
			DbgPrint("Is Text File.. Can not Open\n");

			//열려진자원에대한정보를해제하고, 상태를거부되었음으로돌려준다.
			FltCancelFileOpen(pFltObjects->Instance, pFltObjects->FileObject);

			pData->IoStatus.Status = STATUS_ACCESS_DENIED;
			pData->IoStatus.Information = 0;

			//모든작업이완료되었음을돌려준다.
			return FLT_PREOP_COMPLETE;
		}
	}

	//할당된nameInfo에대한자원을해제시킨다.
	FltReleaseFileNameInformation(nameInfo);

	//우리는현재Post Callback을소유하지않았으므로
	//IRP_MJ_CREATE 부분에대한모든작업을마쳤음을통보한다.
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

			//보안서술자해제
			FltFreeSecurityDescriptor(pSD);

			//필터링을시작한다.
			status = FltStartFiltering(g_filterData->pFilter);

			if (NT_SUCCESS(status))
			{
				DbgPrint("Success Load Filter Driver!\n");
				return STATUS_SUCCESS;
			}
		} else	{
			KdPrint(("보안서술자생성실패. status : 0x%X\n", status));
			//필터등록을해제합니다.
			FltUnregisterFilter(g_filterData->pFilter);
		}	
	}

	return STATUS_SUCCESS;
}