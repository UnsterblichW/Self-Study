#include "test_iocp.h"

void PostAcceptEx(SOCKET listenSocket) {
	SOCKET acceptSock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (acceptSock == INVALID_SOCKET) {
		return;
	}
	OverlappedPerIo* overlp = new OverlappedPerIo;
	if (overlp == nullptr) {
		closesocket(listenSocket);
		return;
	}
	ZeroMemory(overlp->buff, BUFF_SIZE);
	overlp->socket = acceptSock;
	overlp->wsaBuf.buf = overlp->buff;
	overlp->wsaBuf.len = BUFF_SIZE;
	overlp->type = IO_ACCEPT;

	DWORD dwByteRecv = 0;
	AcceptEx(listenSocket,
		acceptSock,
		overlp->wsaBuf.buf,
		0,
		sizeof(SOCKADDR_IN) + 16,
		sizeof(SOCKADDR_IN) + 16, 
		&dwByteRecv,
		(LPOVERLAPPED)&overlp->overlapped);
}


// 初始化服务端监听socket，使用重叠IO
int initServer(ServerParams& params) {
	WSADATA wsaData;
	int ret;

	ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (ret == 0) {
		params.listenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (params.listenSocket != INVALID_SOCKET) {
			// 绑定地址和端口
			SOCKADDR_IN address;
			address.sin_family = AF_INET;
			address.sin_addr.s_addr = INADDR_ANY;
			address.sin_port = htons(8989);
			ret = bind(params.listenSocket, (const sockaddr*)&address, sizeof(address));
			if (ret == 0) {
				// 创建 I/O 完成端口
				params.completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
				if (params.completionPort != NULL) {
					// 把 监听端口 和 I/O 完成端口 关联在一起，此后监听端口就可以接收到来自其他地方的IO事件
					if (NULL != CreateIoCompletionPort((HANDLE)params.listenSocket, params.completionPort, NULL, 0)) {
						return 0;
					}
					CloseHandle(params.completionPort);
				}
			}

			return 0;
		}
		closesocket(params.listenSocket);
	}

	WSACleanup();
	if (ret == 0) ret == -1;
	return ret;
}