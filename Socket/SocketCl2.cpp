#include ".\socketcl2.h"
#include <ctime>
using namespace std;
using std::string;

Socket23::Socket23()
{
	errMsg[0] = '\0';
	ptrAddrInfo = NULL;
	receivedAllData = false;
	error = false;
	ConnectSocket = INVALID_SOCKET;
	def_ip = "10.0.8.101";
	def_port = "1861";
	delay_seconds = 1;
}

Socket23::Socket23(string new_ip, string new_port, unsigned short attempt)
{
	errMsg[0] = '\0';
	ptrAddrInfo = NULL;
	receivedAllData = false;
	error = false;
	ConnectSocket = INVALID_SOCKET;
	def_ip = new_ip;
	def_port = new_port;
	delay_seconds = attempt;
}

Socket23::~Socket23(void)
{
	freeaddrinfo(ptrAddrInfo);
	WSACleanup();
}

bool Socket23::MakeSocket() {
	error = false;
	int iResult = 0;
	WSADATA wsaData;
	struct addrinfo hints;

	// есть ли на машине поддержка сокетов
	if ( WSAStartup( MAKEWORD(2,2), &wsaData) ) //ok  WS2_32.dll
	{
		strcpy(errMsg,"Відсутня підтримка сокетів.");
		error = true;
		return false;
	}

	//Creating a Socket for the Client
	ZeroMemory( &hints, sizeof(hints) );
	//hints.ai_family = AF_UNSPEC;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	//iResult = getaddrinfo(DEFAULT_IP, DEFAULT_PORT, &hints, &ptrAddrInfo);
	iResult = getaddrinfo(def_ip.c_str(), def_port.c_str(), &hints, &ptrAddrInfo);
	
	if (iResult != 0)
	{
		strcpy(errMsg, "getaddrinfo failed: ");
		strcat(errMsg, strerror(iResult));
		WSACleanup();
		error = true;
		return false;
	}
	return true;
}
//*****************************************************************************
//bool Socket::SendData(char* sendText)
bool Socket23::SendData(vector<string> & allFile)
{
	addrinfo *ptr = NULL;
	int iResult, recvbuflen = DEFAULT_BUFLEN;
	ConnectSocket = INVALID_SOCKET;
	if(!error) {

		// Attempt to connect to the first address returned by the call to getaddrinfo
		// берем данные ранее установленного соединения
		ptr = ptrAddrInfo;

		// Attempt to connect to an address until one succeeds
		// перебираем доступные порты наверно пока не установим соединение
		for(ptr=ptrAddrInfo; ptr != NULL ;ptr=ptr->ai_next)
		{
			// Create a SOCKET for connecting to server
			ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
			if (ConnectSocket == INVALID_SOCKET)
			{
				strcpy(errMsg, "Error at socket: ");
				strcat(errMsg, strerror(WSAGetLastError()));
				error = true;
				return false;
			}
			// Connect to server.
			iResult = connect( ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
			if (iResult == SOCKET_ERROR)
			{
				closesocket(ConnectSocket);
				ConnectSocket = INVALID_SOCKET;
				continue;
			}
			break;
		}

		if (ConnectSocket == INVALID_SOCKET)
		{
			strcpy(errMsg, "Unable to connect to server!");
			error = true;
			return false;
		}

		/* The getaddrinfo function is used to determine the values in the sockaddr structure.
		In this example, the first IP address returned by the getaddrinfo function is used to specify
		the sockaddr structure passed to the connect. If the connect call fails to the first IP address,
		then try the next addrinfo structure in the linked list returned from the getaddrinfo function.*/

		// Sending and Receiving Data on the Client
		// Send an initial buffer
		int size_v = static_cast<int>(allFile.size());
		for(int i = 0; i < size_v; i++) {
			//iResult = send(ConnectSocket, allFile[i].c_str(), (int) strlen(allFile[i].c_str()), 0);
			iResult = send(ConnectSocket, allFile[i].c_str(), allFile[i].size(), 0);
			if (iResult == SOCKET_ERROR)
			{
				strcpy(errMsg, "Send failed: ");
				strcat(errMsg, strerror(WSAGetLastError()));
				closesocket(ConnectSocket);
				error = true;
				return false;
			}
		}
	}
	return (!error);
}

bool Socket23::ShutdownSocket() {
	// shutdown the connection for sending since no more data will be sent
	// the client can still use the ConnectSocket for receiving data
	if(!error) {
		int iResult = shutdown(ConnectSocket, SD_SEND);
		if (iResult == SOCKET_ERROR)
		{
			strcpy(errMsg, "Shutdown failed: ");
			strcat(errMsg, strerror(WSAGetLastError()));
			closesocket(ConnectSocket);
			error = true;
		}
	}
	return (!error);
}

bool Socket23::ReceiveData(vector<string> & receivedLines) {
	receivedLines.clear();
	clock_t delay = delay_seconds * CLOCKS_PER_SEC;
	clock_t start = clock();
	clock_t finish = clock();
	char recvbuf[DEFAULT_BUFLEN];
	//recvbufAll[0] = '\0';
	recvbuf[0] = '\0';
	int recvbuflen = DEFAULT_BUFLEN - 2;
	int iResult = 0;
	int reeceivedBytes = 0;
	//int isReady = 10;
	//char readyBuf[1];
	if(!error) {
		// Receive until the peer closes the connection
		do {
			/*
			if(!reeceivedBytes) {
				isReady = recv(ConnectSocket, readyBuf, 1, MSG_PEEK);
				if(!isReady) {
					Sleep(1000);
					iResult = 1;
					continue;
				}
			}
			*/

			try {
				iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
			} catch (exception& e) {
				iResult = -1;
			}
			if(iResult > recvbuflen)
				iResult = recvbuflen;
			//strncpy(recvbufAll, recvbuf, iResult);
			if(iResult > 0) {
				reeceivedBytes += iResult;
				recvbuf[iResult] = '\0';
				receivedLines.push_back(recvbuf);
				if(recvbuf[iResult-1] == '\1' && recvbuf[iResult-2] == '\n' && recvbuf[iResult-3] == '\r') {
					receivedAllData = true;
					break;
				}
			}
			else if ( iResult == 0 ) {
				if(!receivedAllData) {
					finish = clock();
					if(finish - start < delay) {
						iResult = 1;
						Sleep(1000);
						continue;
					}
				}
				strcpy(errMsg, "Connection closed");
				error = true;
			}
			else {
				strcpy(errMsg, "Recv failed with error: ");
				strcat(errMsg, strerror(WSAGetLastError()));
				error = true;
				if(!receivedAllData) {
					finish = clock();
					if(finish - start < delay) {
						iResult = 1;
						Sleep(1000);
						continue;
					}
				}
			}
		} while( iResult > 0 );

		closesocket(ConnectSocket);
	}
	return (!error);
}

bool Socket23::IsError() {
	return error;
}

bool Socket23::IsReceiveAll() {
	return receivedAllData;
}