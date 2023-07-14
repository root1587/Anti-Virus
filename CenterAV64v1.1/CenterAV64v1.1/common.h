#include "CenterAV64v1.1.h"
#pragma once

#ifndef BOOLEAN
#define BOOLEAN     UCHAR
#endif

#ifndef MY_MAX_PATH
#define MY_MAX_PATH     512
#endif

// IOCP 구현..? x IRP 통신상태

// 통신 포트 이름
#define FLT_PORT_NAME   L"\\FltPort"

// 기본 파일 처리 크기 (기본 섹터 크기 * 2)
#define FLT_READ_BUFFER_SIZE    (512 * 2)

// 필터 -> 유저모드 전송 구조체
typedef struct _FLT_NOTIFICATION
{
	ULONG   bytesToScan;                        // 읽은 크기
	ULONG   reserved;                           // 8 바이트 정렬을 위한 더미
	WCHAR   filePath[MY_MAX_PATH];              // 파일 경로
	UCHAR   fileContents[FLT_READ_BUFFER_SIZE]; // 파일 내용

} FLT_NOTIFICATION, *PFLT_NOTIFICATION;

// 유저모드 -> 필터 전송 구조체
typedef struct _FLT_REPLY
{
	BOOLEAN safeToIo;   // I/O 허용 여부

} FLT_REPLY, *PFLT_REPLY;

