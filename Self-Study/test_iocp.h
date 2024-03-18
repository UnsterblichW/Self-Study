#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <MSWSock.h>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "MsWSock.Lib")

#define BUFF_SIZE 1024
#define THREAD_COUNT 4
#define START_POST_ACCEPTEX 4
#define PORT 8989

enum IO_TYPE {
	IO_ACCEPT,
	IO_SEND,
	IO_RECV,
	IO_CONNECT,
	IO_DISCONNECT,
};

struct ServerParams {
	SOCKET listenSocket;
	HANDLE completionPort;
};

typedef struct OverlappedPerIo {
	OVERLAPPED overlapped;  // ���OVERLAPPED���͵ĳ�Ա������ڵ�һ��
	SOCKET socket;
	WSABUF wsaBuf;
	IO_TYPE type;
	char buffer[BUFF_SIZE];
} *LPOverlappedPerIO;


// Ͷ������
void PostAcceptEx(SOCKET listenSocket);

// ÿһ�������߳���Ҫ���������
DWORD WINAPI workerThread(LPVOID lpParam);

// ��ʼ������˼���socket��ʹ���ص�IO
int initServer(ServerParams& params);

void test_iocp_demo();

