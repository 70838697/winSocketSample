#include <winsock2.h>
#include <stdio.h>
#include <windows.h>
// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")

#define PORT     5150
#define MSGSIZE 1024
typedef enum
{
	RECV_POSTED
}OPERATION_TYPE;       //ö��,��ʾ״̬

typedef struct
{
	WSAOVERLAPPED   overlap;      
	WSABUF          Buffer;        
	char            szMessage[MSGSIZE];
	DWORD           NumberOfBytesRecvd;
	DWORD           Flags;
	OPERATION_TYPE OperationType;
}PER_IO_OPERATION_DATA, *LPPER_IO_OPERATION_DATA;    //����һ���ṹ�屣��IO����

DWORD WINAPI ThreadProc(
	__in  LPVOID lpParameter
	)
{
	HANDLE                   CompletionPort=(HANDLE)lpParameter;
	DWORD                    dwBytesTransferred;
	SOCKET                   sClient=INVALID_SOCKET;
	LPPER_IO_OPERATION_DATA lpPerIOData = NULL;

	while (TRUE)
	{
		if(!GetQueuedCompletionStatus( //�������Խ��������򷵻أ�����ȴ�
			CompletionPort,
			&dwBytesTransferred, //���ص�����
			(PULONG_PTR) &sClient,           //����Ӧ���ĸ��ͻ��׽��֣�
			(LPOVERLAPPED *)&lpPerIOData, //�õ����׽��ֱ����IO��Ϣ
			INFINITE)){						//���޵ȴ���������ʱ�����֡�
				if(WSAGetLastError()==64){
					// Connection was closed by client
					//�����������ע�͵�����Ϊ�����ߵ�����رյ�����
					closesocket(sClient);
					HeapFree(GetProcessHeap(), 0, lpPerIOData);        //�ͷŽṹ��
					continue;
				}
				else
				{
				wprintf(L"GetQueuedCompletionStatus: %ld\n", WSAGetLastError());
				return -1;
				}
		}
		if (dwBytesTransferred == 0xFFFFFFFF)
		{
			return 0;
		}

		if (lpPerIOData->OperationType == RECV_POSTED) //����ܵ�����
		{
			if (dwBytesTransferred == 0)
			{
				// Connection was closed by client
				closesocket(sClient);
				HeapFree(GetProcessHeap(), 0, lpPerIOData);        //�ͷŽṹ��
			}
			else
			{
				lpPerIOData->szMessage[dwBytesTransferred] = '\0';
				
				//�����յ�����Ϣ����
				int sendCount,currentPosition=0,count=dwBytesTransferred;
				while( count>0 && (sendCount=send(sClient ,lpPerIOData->szMessage+currentPosition,count,0))!=SOCKET_ERROR)
				{
					count-=sendCount;
					currentPosition+=sendCount;
				}
				//------------------------------------------------------
				//if(sendCount==SOCKET_ERROR)break; there is error here


				// Launch another asynchronous operation for sClient
				memset(lpPerIOData, 0, sizeof(PER_IO_OPERATION_DATA));
				lpPerIOData->Buffer.len = MSGSIZE;
				lpPerIOData->Buffer.buf = lpPerIOData->szMessage;
				lpPerIOData->OperationType = RECV_POSTED;
				WSARecv(sClient,               //ѭ������
					&lpPerIOData->Buffer,
					1,
					&lpPerIOData->NumberOfBytesRecvd,
					&lpPerIOData->Flags,
					&lpPerIOData->overlap,
					NULL);
			}
		}
	}
	return 0;
}

int main(int argc, char* argv[])
{
	// ��ʼ����ɶ˿�
	HANDLE CompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if(CompletionPort==NULL){
		wprintf(L"CreateIoCompletionPort failed with error: %ld\n", WSAGetLastError());
		return 1;
	}

	//----------------------
	// Initialize Winsock.
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR) {
		wprintf(L"WSAStartup failed with error: %ld\n", iResult);
		return 1;
	}
	//----------------------
	// Create a SOCKET for listening for
	// incoming connection requests.
	SOCKET ListenSocket=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ListenSocket == INVALID_SOCKET) {
		wprintf(L"socket failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}
	//----------------------
	// The sockaddr_in structure specifies the address family,
	// IP address, and port for the socket that is being bound.
	sockaddr_in addrServer;
	addrServer.sin_family = AF_INET;
	addrServer.sin_addr.s_addr = htonl(INADDR_ANY); //ʵ������0
	addrServer.sin_port = htons(20131);


	//���׽��ֵ�һ��IP��ַ��һ���˿���
	if (bind(ListenSocket,(SOCKADDR *) & addrServer, sizeof (addrServer)) == SOCKET_ERROR) {
		wprintf(L"bind failed with error: %ld\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	//���׽�������Ϊ����ģʽ�ȴ���������
	//----------------------
	// Listen for incoming connection requests.
	// on the created socket
	if (listen(ListenSocket, 5) == SOCKET_ERROR) {
		wprintf(L"listen failed with error: %ld\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}


	// �м���CPU�ʹ��������������߳�
	SYSTEM_INFO              systeminfo;
	GetSystemInfo(&systeminfo);

	for (unsigned i = 0; i < systeminfo.dwNumberOfProcessors; i++)
	{
		DWORD dwThread;
		HANDLE hThread = CreateThread(NULL,0,ThreadProc,(LPVOID)CompletionPort,0,&dwThread);
		if(hThread==NULL)
		{
			wprintf(L"Thread Creat Failed!\n");
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}
		CloseHandle(hThread);
	}


	SOCKADDR_IN addrClient;
	int len=sizeof(SOCKADDR);
	//��һ������ѭ���ķ�ʽ����ͣ�ؽ��տͻ���socket����
	while(1)
	{
		//�������󣬽����������󣬷���һ���µĶ�Ӧ�ڴ˴����ӵ��׽���
		SOCKET AcceptSocket=accept(ListenSocket,(SOCKADDR*)&addrClient,&len);
		if(AcceptSocket  == INVALID_SOCKET)break; //����

		//��������µ����Ŀͻ��׽��ֺ���ɶ˿ڰ󶨵�һ��
		CreateIoCompletionPort((HANDLE)AcceptSocket, CompletionPort, ( ULONG_PTR)AcceptSocket, 0);
		//������������ʾ���ݵĲ���������ʹ��ݵĿͻ��׽��ֵ�ַ�����һ������Ϊ0 ��ʾ�к�CPUһ���Ľ���������1��CPUһ���߳�

		// ��ʼ���ṹ��
		LPPER_IO_OPERATION_DATA lpPerIOData = (LPPER_IO_OPERATION_DATA)HeapAlloc(
			GetProcessHeap(),
			HEAP_ZERO_MEMORY,
			sizeof(PER_IO_OPERATION_DATA));
		lpPerIOData->Buffer.len = MSGSIZE; // len=1024
		lpPerIOData->Buffer.buf = lpPerIOData->szMessage;
		lpPerIOData->OperationType = RECV_POSTED; //��������
		WSARecv(AcceptSocket,         //�첽������Ϣ�����̷��ء�
			&lpPerIOData->Buffer, //��ý��յ�����
			1,       //The number of WSABUF structures in the lpBuffers array.
			&lpPerIOData->NumberOfBytesRecvd, //���յ����ֽ�����������󷵻�0
			&lpPerIOData->Flags,       //�������Ȳ���
			&lpPerIOData->overlap,     //��������ṹ�忩��
			NULL);
	}

	//posts an I/O completion packet to an I/O completion port.
	PostQueuedCompletionStatus(CompletionPort, 0xFFFFFFFF, 0, NULL);
	CloseHandle(CompletionPort);
	closesocket(ListenSocket);
	WSACleanup();
	return 0;
}

