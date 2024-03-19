#include "test_iocp.h"
#include <iostream>
#include <cstring>
#include <thread>

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

	//ZeroMemory(overlp->buffer, BUFF_SIZE); 
	// ���ֻ���overlp->buffer���ᷢ������AcceptEx()һֱʧ�ܣ�Ȼ��GetLastError()������6��
	// ERROR_INVALID_HANDLE 6 (0x6) �þ����Ч��
	// ������overlp->overlapped���������⣬overlapped��һ��OVERLAPPED����OVERLAPPED::hEvent������û�б����
	// ������ȷ������Ӧ�����������overlp
	ZeroMemory(overlp, sizeof(OverlappedPerIo));

	overlp->socket = acceptSock;
	overlp->wsaBuf.buf = overlp->buffer;
	overlp->wsaBuf.len = BUFF_SIZE;
	overlp->type = IO_ACCEPT;

	DWORD dwByteRecv = 0;

	// AcceptEx �����һ������ LPOVERLAPPED lpOverlapped�����Ǵ�����overlp->overlapped��
	// ���﷢�����û�̬���ں�̬��ת����overlp->overlapped�����ں�̬�������ݣ�Ȼ�󽫻�ͨ�� GetQueuedCompletionStatus() ������ȡ��
	while (false == AcceptEx(listenSocket,
		acceptSock,
		overlp->wsaBuf.buf,
		0,
		sizeof(SOCKADDR_IN) + 16,
		sizeof(SOCKADDR_IN) + 16, 
		&dwByteRecv,
		&overlp->overlapped)) {
		if (WSAGetLastError() == WSA_IO_PENDING) {
			break;
		}
		std::cout << WSAGetLastError() << std::endl;
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
		// ���Ŵ�completionPort���IO��ɶ˿������ó������ݣ������completionPort�˿������IO������û���꣨pending״̬�����Ǿ͵���
		BOOL result = GetQueuedCompletionStatus(completionPort,
			&bytesTrans,
			&comletionKey,
			(LPOVERLAPPED*)&overlp, // �ں�̬->�û�̬
			INFINITE); // һֱ�ȣ�һֱ������������ʱ�� ����� 0 �Ļ��������û��IO������������ʱ��
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
			PostAcceptEx(listenSocket);

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


// ��ʼ������˼���socket��ʹ���ص�IO
int initServer(ServerParams& params) {
	WSADATA wsaData;
	int ret;

	ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (ret == 0) {
		params.listenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (params.listenSocket != INVALID_SOCKET) {
			// �󶨵�ַ�Ͷ˿�
			SOCKADDR_IN address;
			address.sin_family = AF_INET;
			address.sin_addr.s_addr = INADDR_ANY;
			address.sin_port = htons(PORT);
			ret = bind(params.listenSocket, (const sockaddr*)&address, sizeof(address));
			if (ret == 0) {
				ret = listen(params.listenSocket, SOMAXCONN);
				if (ret == 0) {
					// ���� I/O ��ɶ˿�
					params.completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
					if (params.completionPort != NULL) {
						// �� �����˿� �� I/O ��ɶ˿� ������һ�𣬴˺�����˿ھͿ��Խ��յ����������ط���IO�¼�
						if (NULL != CreateIoCompletionPort((HANDLE)params.listenSocket, params.completionPort, NULL, 0)) {
							return 0;
						}
						CloseHandle(params.completionPort);
					}
				}
			}
		}
		closesocket(params.listenSocket);
	}

	WSACleanup();
	if (ret == 0) ret == -1;
	return ret;
}


void test_iocp_demo() {
	ServerParams pms;
	int ret;
	ret = initServer(pms);
	if (ret != 0) {
		std::cout << "initServer Error" << std::endl;
		return;
	}

	// �ȴ��������߳����ȴ�����PostAcceptEx����Ͷ����������
	for (int i = 0; i < THREAD_COUNT; i++) {
		// ��ʵ���������C++11֮��͸�����֧�ֶ�ƽ̨�Ľӿ�std::thread()����Ϊ�˻�ԭwindowsԭ֭ԭζ��������windows�Լҵ�CreateThread
		CreateThread(NULL, 0, workerThread, &pms, 0, NULL);
	}

	for (int i = 0; i < START_POST_ACCEPTEX; i++) {
		PostAcceptEx(pms.listenSocket);
	}

	std::cin.get();

	closesocket(pms.listenSocket);
	CloseHandle(pms.completionPort);
	WSACleanup();

}