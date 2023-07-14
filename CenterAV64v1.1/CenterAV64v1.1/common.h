#include "CenterAV64v1.1.h"
#pragma once

#ifndef BOOLEAN
#define BOOLEAN     UCHAR
#endif

#ifndef MY_MAX_PATH
#define MY_MAX_PATH     512
#endif

// IOCP ����..? x IRP ��Ż���

// ��� ��Ʈ �̸�
#define FLT_PORT_NAME   L"\\FltPort"

// �⺻ ���� ó�� ũ�� (�⺻ ���� ũ�� * 2)
#define FLT_READ_BUFFER_SIZE    (512 * 2)

// ���� -> ������� ���� ����ü
typedef struct _FLT_NOTIFICATION
{
	ULONG   bytesToScan;                        // ���� ũ��
	ULONG   reserved;                           // 8 ����Ʈ ������ ���� ����
	WCHAR   filePath[MY_MAX_PATH];              // ���� ���
	UCHAR   fileContents[FLT_READ_BUFFER_SIZE]; // ���� ����

} FLT_NOTIFICATION, *PFLT_NOTIFICATION;

// ������� -> ���� ���� ����ü
typedef struct _FLT_REPLY
{
	BOOLEAN safeToIo;   // I/O ��� ����

} FLT_REPLY, *PFLT_REPLY;

