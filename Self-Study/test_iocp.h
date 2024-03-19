#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <MSWSock.h>
#include <stdio.h>

// 下面这样写是包含静态库的手写方式，当然也可以在vs项目中的链接器目录那里添加，这两种方式是等价的
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
	OVERLAPPED overlapped;  // 这个OVERLAPPED类型的成员必须放在第一个
	SOCKET socket;
	WSABUF wsaBuf;
	IO_TYPE type;
	char buffer[BUFF_SIZE];
} *LPOverlappedPerIO;


// 投递请求
void PostAcceptEx(SOCKET listenSocket);

// 每一个工作线程中要处理的事情
DWORD WINAPI workerThread(LPVOID lpParam);

// 初始化服务端监听socket，使用重叠IO
int initServer(ServerParams& params);

void test_iocp_demo();

