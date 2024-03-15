#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <MSWSock.h>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "MsWSock.Lib")

#define BUFF_SIZE 1024

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
	OVERLAPPED overlapped;  // 这个OVERLAPPED类型的成员必须放在第一个
	SOCKET socket;
	WSABUF wsaBuf;
	IO_TYPE type;
	char buff[BUFF_SIZE];
} *LPOverlappedPerIO;

void PostAcceptEx(SOCKET listenSocket);

int initServer(ServerParams& params);

