#include <winsock2.h>
#include <stdio.h>
#include <windows.h>
// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")

int main(int argc, char* argv[])
{
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
    SOCKET ListenSocket;
    ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
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
    if (listen(ListenSocket, 1) == SOCKET_ERROR) {
        wprintf(L"listen failed with error: %ld\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

	SOCKADDR_IN addrClient;
	int len=sizeof(SOCKADDR);
	//��һ������ѭ���ķ�ʽ����ͣ�ؽ��տͻ���socket����
	while(1)
	{
		//�������󣬽����������󣬷���һ���µĶ�Ӧ�ڴ˴����ӵ��׽���
		SOCKET AcceptSocket=accept(ListenSocket,(SOCKADDR*)&addrClient,&len);
		if(AcceptSocket  == INVALID_SOCKET)break; //����
		//���ջ������Ĵ�С��50���ַ�
		char recvBuf[51];
		while(1){
			int count =recv(AcceptSocket ,recvBuf,50,0);
			if(count==0)break;//���Է��ر�
			if(count==SOCKET_ERROR)break;//����count<0
			int sendCount,currentPosition=0;
			while( count>0 && (sendCount=send(AcceptSocket ,recvBuf+currentPosition,count,0))!=SOCKET_ERROR)
			{
				count-=sendCount;
				currentPosition+=sendCount;
			}
			if(sendCount==SOCKET_ERROR)break;
			
			printf("�������Կͻ���%s����Ϣ��%s/n",inet_ntoa(addrClient.sin_addr),recvBuf);
		}
		//��������
		closesocket(AcceptSocket);
	}

	closesocket(ListenSocket);
	WSACleanup();
	return 0;
}

