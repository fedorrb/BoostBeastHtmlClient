#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <winbase.h>

//#include ".\socketcl.h"
#include <string>
#include <vector>
using namespace std;
using std::string;
using std::vector;

#pragma comment (lib, "Ws2_32.lib") //ccc
#pragma comment (lib, "Mswsock.lib")
//#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 131072
//#define DEFAULT_BUFLEN 65536

class Socket23
{
public:
	Socket23();
	Socket23(string new_ip, string new_port, unsigned short attempt);
	~Socket23(void);
	bool MakeSocket();
	bool SendData(vector<string> & allFile);
	bool ShutdownSocket();
	bool ReceiveData(vector<string> & receivedLines);
	char errMsg[100];
	//char recvbufAll[DEFAULT_BUFLEN];
	bool IsError();
	bool IsReceiveAll();
	string def_ip;
	string def_port;
	int delay_seconds;
private:
	bool receivedAllData; //ознака отримання цілого повідомлення
	bool error;
	SOCKET ConnectSocket;
	addrinfo *ptrAddrInfo;
};
