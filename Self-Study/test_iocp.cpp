#include "test_iocp.h"
#include <iostream>
#include <cstring>

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
	ZeroMemory(overlp->buffer, BUFF_SIZE);
	overlp->socket = acceptSock;
	overlp->wsaBuf.buf = overlp->buffer;
	overlp->wsaBuf.len = BUFF_SIZE;
	overlp->type = IO_ACCEPT;

	DWORD dwByteRecv = 0;

	// AcceptEx 的最后一个参数 LPOVERLAPPED lpOverlapped，我们传入了overlp->overlapped，
	// 这里发生了用户态到内核态的转换，overlp->overlapped会在内核态设置数据，然后将会通过 GetQueuedCompletionStatus() 把数据取出
	while (false == AcceptEx(listenSocket,
		acceptSock,
		overlp->wsaBuf.buf,
		0,
		sizeof(SOCKADDR_IN) + 16,
		sizeof(SOCKADDR_IN) + 16, 
		&dwByteRecv,
		(LPOVERLAPPED)&overlp->overlapped)) {
		if (GetLastError() == ERROR_IO_PENDING) {
			break;
		}
	}
}


DWORD WINAPI workerThread(LPVOID lpParam) {
	// LPVOID: A pointer to any type. 
	ServerParams* pms = reinterpret_cast<ServerParams*>(lpParam);
	HANDLE completionPort = pms->completionPort;
	SOCKET listenSocket = pms->listenSocket;

	DWORD bytesTrans;
	ULONG_PTR comletionKey;
	LPOverlappedPerIO overlp = nullptr;

	int ret = 0;
	while (true) {
		// 等着从completionPort这个IO完成端口里面拿出来数据，如果与completionPort端口相关联IO操作还没做完（pending状态），那就等着
		BOOL result = GetQueuedCompletionStatus(completionPort,
			&bytesTrans,
			&comletionKey,
			(LPOVERLAPPED*)&overlp, // 内核态->用户态
			INFINITE); // 一直等，一直阻塞，永不超时。 如果填 0 的话就是如果没有IO操作就立即超时。
		if (!result) {
			if ((GetLastError() == WAIT_TIMEOUT) || (GetLastError() == ERROR_NETNAME_DELETED)) {
				std::cout << "socket disconnection:" << overlp->socket << std::endl;
				closesocket(overlp->socket);
				delete overlp;
				continue;
			}
			std::cout << "GetQueuedCompletionStatus failed" << std::endl;
			return 0;
		}
		switch (overlp->type) {
		case IO_ACCEPT: {

			// SO_UPDATE_ACCEPT_CONTEXT : Updates the accepting socket with the context of the listening socket.
			setsockopt(overlp->socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&(listenSocket), sizeof(SOCKET));

			ZeroMemory(overlp->buffer, BUFF_SIZE);
			overlp->type = IO_RECV;
			overlp->wsaBuf.buf = overlp->buffer;
			overlp->wsaBuf.len = BUFF_SIZE;
			CreateIoCompletionPort((HANDLE)overlp->socket, completionPort, NULL, 0);

			DWORD dwRecv = 0, dwFlag = 0;
			ret = WSARecv(overlp->socket, &overlp->wsaBuf, 1, &dwRecv, &dwFlag, &(overlp->overlapped), 0);
			if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
				std::cout << "WSARecv failed:" << WSAGetLastError() << std::endl;
			}
		}
		break;
		case IO_RECV:
		{
			std::cout << "happed IO_RECV:" << bytesTrans << std::endl;
			if (bytesTrans == 0) {
				std::cout << "socket disconnection:" << overlp->socket << std::endl;
				closesocket(overlp->socket);
				delete overlp;
				continue;
			}

			std::cout << "recved data:" << overlp->buffer << std::endl;

			ZeroMemory(&overlp->overlapped, sizeof(OVERLAPPED));
			overlp->type = IO_SEND;

			char str[22] = "response from server\n";
			overlp->wsaBuf.buf = str;
			overlp->wsaBuf.len = 22;

			DWORD dwSend = 0;
			ret = WSASend(overlp->socket, &overlp->wsaBuf, 1, &dwSend, 0, &(overlp->overlapped), 0);
			if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
				std::cout << "WSARecv failed:" << WSAGetLastError() << std::endl;
			}
		}
		break;
		case IO_SEND:
		{
			std::cout << "happed IO_SEND:" << bytesTrans << std::endl;
			if (bytesTrans == 0) {
				std::cout << "socket disconnection:" << overlp->socket << std::endl;
				closesocket(overlp->socket);
				delete overlp;
				continue;
			}
		}
		break;


		}

	}

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