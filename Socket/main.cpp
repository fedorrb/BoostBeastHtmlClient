//	Федорчук Р.Б.
//--------------------------------------
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <string>
#include <exception>
#include <stdio.h>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <cstdlib>

#include <normalize.h>
#include "klovr.h"
#include "xstay.h"
#include "interface.h"
#include "common.h"
#include "mat.hpp"
#include "singleton.h"
#include <stdlib.h>
#include "serveraddr.h"
#include "busyindc.h"//BUSY_FORM
#include "moduls_check.h"
using namespace std;
using std::string;
using std::vector;

namespace beast = boost::beast;     // from <boost/beast.hpp>
namespace http = beast::http;       // from <boost/beast/http.hpp>
namespace net = boost::asio;        // from <boost/asio.hpp>
namespace pt = boost::property_tree;
using tcp = net::ip::tcp;           // from <boost/asio/ip/tcp.hpp>


vector<string> messageToSend;
vector<string> receivedLines;
vector<string> errorMessage;
StayEventProc BOSBusyForm; //BUSY_FORM
StayEventProc BOSWSetAddr;
void SaveJsonToFile (const char *out_filename, int code, vector<string> & receivedLines);
int SaveLogFileAll (vector<string> & messageToSend, int direction);
void DeleteSrvrAdr();
const int MAXMESSAGE = 131072; //#define DEFAULT_BUFLEN 1024 in socketcl.h
const unsigned long MAXLOGFILESIZE = 20000000; //20Mb
void replaceAll(std::string& str, const std::string& from, const std::string& to);
int HttpClientSync(string ip, string port, vector<string>& messageToSend, vector<string>& receivedMessage);
bool LoadJsonFromFileHtml(const char* in_filename, vector<string>& messageToSend);
void GetSystemInfo();
string MakeSysInfoJson();

USETOOLS;USESHELL;USETECH;

ASOPDMAIN("Обмін через сокет.");

int main(int argc, char** argv)
{
	int result = 1;

	INITTOOLS(); INITSHELL(); INITTECH();
	Singleton& glb = Singleton::getInstance(); 	//создание экземпляра класса глобальных переменных
	StackReset();
	SetDateDelim('.');
	Initiate();
	glb.debug = true;
	//glb.debug = false;
	if (!StartProcSet(&glb.insCode, NULL, glb.insFio, NULL))
	{
		glb.rayon = 3225;
		glb.uzel = 3225;
	}
	else
	{
		glb.rayon = fGetTech("район");
		glb.uzel = fGetTech("узел");
	}
	GetSystemInfo();
	if(glb.debug)
		MakeSysInfoJson();
	if(argc == 4) {
		glb.pathAdmin = argv[0];
		glb.fileNameIn = argv[1]; //файл с входящим сообщением
		glb.fileNameOut = argv[2]; //файл для выходящего сообщения
		glb.reqCode = argv[3];//код обміну
		glb.vidpov = 1;
		Display (WndBusy, BOSBusyForm);//BUSY_FORM
		result = glb.vidpov;
	}
	else {
		glb.pathAdmin = argv[0];
		Display (WndSetAddr, BOSWSetAddr);
	}
	if(B_SvrAdr->bs)
		Close(B_SvrAdr);
	Terminate();
	TERMTOOLS();TERMSHELL();TERMTECH();
	return result; 
}

//возвращает содержимое входного файла в вектор
bool LoadJsonFromFileHtml(const char* in_filename, vector<string>& messageToSend) {
	Singleton& glb = Singleton::getInstance();
	bool result = false;
	char buf[MAXMESSAGE];
	char bufNew[MAXMESSAGE];
	memset(bufNew, 0, MAXMESSAGE);
	messageToSend.clear();
	StayFile fJsonIn;
	fJsonIn = FOpen(in_filename, RD | ANRD);
	if (fJsonIn) {
		FSeek(fJsonIn, 0);
		while (FReadText(fJsonIn, buf, MAXMESSAGE) > 0) {
			messageToSend.push_back(buf);
		}
		//messageToSend.push_back("\r\n\1");
		FClose(fJsonIn);
		result = true;
	}
	return result;
}

void SaveJsonToFile (const char *out_filename, int code, vector<string> & receivedLines) {
	StayFile fJsonOut;
	int len = 0;
	char ansCode[4];
	memset(ansCode, 0, sizeof(ansCode));
	if(code < 0)
		StrForm(ansCode, 3, "-1|");//error
	else
		StrForm(ansCode, 2, "0|"); //no error
    fJsonOut=FCreate(out_filename, RDWR);
	if(fJsonOut) {
		FWrite(fJsonOut, ansCode, (int) strlen(ansCode)); 
		for(std::vector<string>::iterator it = receivedLines.begin(); it != receivedLines.end(); ++it) {
			FWrite(fJsonOut, (*it).c_str(), (int) strlen((*it).c_str()));
		}
	}
    FClose(fJsonOut);
}

int SaveLogFileAll (vector<string> & messageToSend, int direction) {
	int result = 0;
	Singleton &glb = Singleton::getInstance();
	char buf[MAXMESSAGE];
	char buf_inout[4];
	char name_log_file[256];
	if(direction)
		StrCpy(buf_inout, "in");
	else
		StrCpy(buf_inout, "out");
	StayDate dtNow = GetSysDate();
	StayTime tmNow = GetSysTime();

	vector<string> splitLines;
	splitLines.clear();
	Normalize *Norm = new Normalize();
	for(std::vector<string>::iterator it = messageToSend.begin(); it != messageToSend.end(); ++it) {
		Norm->SplitStringLine((*it), splitLines);
	}
	Norm->ModifyLengthCol(splitLines);
	delete Norm;
	int size_v = static_cast<int>(splitLines.size());

	unsigned long fileSize = 0;
	StrForm(buf, MAXMESSAGE, "\r\n%10v %5t %3s ", dtNow, tmNow, buf_inout);
	StayFile logFile;
	int len = StrLen(buf);
	StrForm(name_log_file, 256, "SOK:socket%u.log", glb.insCode);
	try {
		if(FFind(name_log_file, NULL)) {
			logFile = FOpen(name_log_file, RDWR | ANRD);
		} else {
			logFile = FCreate(name_log_file, RDWR | ANRD);
		}
	} catch (...){
		if(logFile)
			FClose(logFile);
		result = 1;
	}
	if(result)
		return result;
	//if log file bigger than MAXLOGFILESIZE
	fileSize = FSize(logFile);
	if(logFile && fileSize > MAXLOGFILESIZE) {
		FClose(logFile);
		char name2[L_tmpnam];
		if(std::tmpnam(name2)) {
			name2[0] = ':';
			std::string name1 = name2;
			std::string nameFull = "SOK" + name1 + "log";
			FCopy(name_log_file, nameFull.c_str());
			logFile = FCreate(name_log_file, RDWR | ANRD);
	    }
	}
	if(logFile) {
		FSeek(logFile,FSize(logFile));
		FWrite(logFile, buf, len);
		int i = 0;
		while(i < size_v) {
			FWrite(logFile, splitLines[i].c_str(), (int) strlen(splitLines[i].c_str()));
			i++;
		}
		FFlush(logFile);
		FClose(logFile);
	}
	return result;
}

//BUSY_FORM
int STAYPROC BOSBusyForm( StayEvent s, StayEvent id )
{
	char jsonOut[MAXMESSAGE];
	jsonOut[0] = '\0';
	string srvrIP = "10.0.5.155";
	string srvrPort = "1861";
	unsigned short srvrDL = 4; 
	unsigned short srvrAT = 10;
	//создание экземпляра класса глобальных переменных
	Singleton &glb = Singleton::getInstance();
	glb.socketType = 25;
	char errorMsg[100];
	errorMsg[0] = '\0';
	int fieldlen = 0;
	short ptkCode = 0;
	unsigned short pcPort = 1861;

	switch( s )
	{
		//case _Before:
	case _BeforeWindow:
		if(!B_SvrAdr->bs)
			OpenCreate(B_SvrAdr, RDWR|ANRDWR);
		if(Size(B_SvrAdr)) {
			SetBegin(B_SvrAdr);
			GetNext(B_SvrAdr);
			srvrIP = J_SRVIP;
			srvrPort = J_SRVPORT;
			srvrDL = J_SRVDL;
			srvrAT = J_SRVAT;
			glb.socketType = 25;
			if(glb.socketType != 25) {
				if(B_SvrAdr->bs)
					Close(B_SvrAdr);
				DeleteSrvrAdr();
				if(!B_SvrAdr->bs)
					OpenCreate(B_SvrAdr, RDWR|ANRDWR);
				StrForm(J_SRVIP, 15, srvrIP.c_str());
				StrForm(J_SRVPORT, 5, srvrPort.c_str());
				J_SRVDL = srvrDL;
				J_SRVAT = srvrAT;
				J_SRVType = 25; //default
				glb.socketType = 25;
				Put(B_SvrAdr);
			}
		}
		else {
			StrForm(J_SRVIP, 15, srvrIP.c_str());
			StrForm(J_SRVPORT, 5, srvrPort.c_str());
			J_SRVDL = srvrDL;
			J_SRVAT = srvrAT;
			J_SRVType = 25; //default
			glb.socketType = 25;
			Put(B_SvrAdr);
		}

		if (LoadJsonFromFileHtml(glb.fileNameIn.c_str(), messageToSend)) {
			if (HttpClientSync(srvrIP, srvrPort, messageToSend, receivedLines)) {
				//error
				SaveJsonToFile(glb.fileNameOut.c_str(), -1, receivedLines);
				SaveLogFileAll(receivedLines, 1);
			}
			else {
				//ok
				glb.vidpov = 0;
				SaveJsonToFile(glb.fileNameOut.c_str(), 0, receivedLines);
				SaveLogFileAll(receivedLines, 1);
			}
		}
		else {
			string err = "Input File Error";
			receivedLines.clear();
			receivedLines.push_back(err);
			SaveJsonToFile(glb.fileNameOut.c_str(), -1, receivedLines);
			SaveLogFileAll(receivedLines, 0);
		}

		if(B_SvrAdr->bs)
			Close(B_SvrAdr);
		Exit(_Ok);
		break;
	}
	return 0;
}

int STAYPROC BOSWSetAddr( StayEvent s, StayEvent id )
{
	Singleton &glb = Singleton::getInstance();
	string srvrIP = "10.0.5.155";
	string srvrPort = "1861";
	char buf[MAXMESSAGE];
	unsigned short srvrDL = 4; 
	unsigned short srvrAT = 10;
	char errorMsg[100];
	memset(errorMsg, 0, sizeof(errorMsg));	
	char buf1[64];
	memset(buf1, 0, sizeof(buf1));
	StrForm(buf1,64,"rajon:%d,insp:%d,code:999|[{\"TEST\":\"TEST\"}]\r\n\1", glb.rayon, glb.insCode);
	//StrForm(buf1,64,"rajon:%d,sys:3,insp:%d,code:999|[{\"TEST\":\"TEST\"}]\r\n\1", glb.rayon, glb.insCode);
	string strJsonIn = buf1;
	string strJsonOk = "{\"TEST\":\"OK\"}";
	int loc = 0;
	vector<string> textToSend;
	unsigned short pcPort = 1861;
	string tmpStr;
	stringstream ss;

	switch( s )
	{
	case _BeforeWindow:
		if(!B_SvrAdr->bs)
			OpenCreate(B_SvrAdr, RDWR|ANRDWR);
		if(Size(B_SvrAdr)) {
			SetBegin(B_SvrAdr);
			GetNext(B_SvrAdr);
			srvrIP = J_SRVIP;
			srvrPort = J_SRVPORT;
			srvrDL = J_SRVDL;
			srvrAT = J_SRVAT;
			glb.socketType = J_SRVType;
			if(glb.socketType != 25) {
				if(B_SvrAdr->bs)
					Close(B_SvrAdr);
				DeleteSrvrAdr();
				if(!B_SvrAdr->bs)
					OpenCreate(B_SvrAdr, RDWR | ANRDWR);
				StrForm(J_SRVIP, 15, srvrIP.c_str());
				StrForm(J_SRVPORT, 5, srvrPort.c_str());
				J_SRVDL = srvrDL;
				J_SRVAT = srvrAT;
				J_SRVType = 25; //default
				glb.socketType = 25;
				Put(B_SvrAdr);
			}
		}
		else {
			if(B_SvrAdr->bs)
				Close(B_SvrAdr);
			DeleteSrvrAdr();
			if(!B_SvrAdr->bs)
				OpenCreate(B_SvrAdr, RDWR | ANRDWR);
			StrForm(J_SRVIP, 15, srvrIP.c_str());
			StrForm(J_SRVPORT, 5, srvrPort.c_str());
			J_SRVDL = 2;
			J_SRVAT = 10;
			J_SRVType = 25; //default
			glb.socketType = 25;
			Put(B_SvrAdr);
		}
		ShowWnd( NULL );
		break;
	case _Enter:
	case BUT1:
		glb.socketType = 25;
		Modify(B_SvrAdr);
		if(B_SvrAdr->bs)
			Close(B_SvrAdr);
		StrForm(buf, MAXMESSAGE, "%s %s %D %D", J_SRVIP, J_SRVPORT, _J_SRVDL, _J_SRVAT);
		errorMessage.clear();
		errorMessage.push_back(buf);
		SaveLogFileAll(errorMessage, 0);
		Exit(_Ok);
		break;
	case BUT2:
		glb.socketType = 25;
		Modify(B_SvrAdr);
		receivedLines.clear();
		if(Size(B_SvrAdr)) {
			textToSend.clear();
			receivedLines.clear();
			SetBegin(B_SvrAdr);
			GetNext(B_SvrAdr);
			srvrIP = J_SRVIP;
			srvrPort = J_SRVPORT;
			srvrDL = J_SRVDL;
			srvrAT = J_SRVAT;
			textToSend.push_back(strJsonIn);
			srvrDL = 5;
			Ldv(srvrPort.c_str()); pcPort = Sti(0);
			//

		}
		else {
			StrForm(errorMsg, 100, "Не введено ір-адресу сервера!");
			MsgBox("Помилка", errorMsg);
		}
		break;
	}
	return 0;
}

void DeleteSrvrAdr() {
	if(FFind("SOK:SRVRADRS.DT", NULL)) {
		FDelete("LS:SRVRADRS.DT");
	}
	if(FFind("SOK:SRVRADRS.BTV", NULL)) {
		FDelete("LS:SRVRADRS.BTV");
	}
}

//https://stackoverflow.com/questions/3418231/replace-part-of-a-string-with-another-string
void replaceAll(std::string& str, const std::string& from, const std::string& to) {
    if(from.empty())
        return;
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

int HttpClientSync(string ip, string port, vector<string>& messageToSend, vector<string>& receivedMessage) {
	Singleton& glb = Singleton::getInstance();
	try
	{
		receivedMessage.clear();
		//auto const target = "/api/asopd/v2/r/71";
		auto const target = "/api/asopd/v2/r/" + glb.reqCode;
		int version = 11;

		// The io_context is required for all I/O
		net::io_context ioc;

		// These objects perform our I/O
		tcp::resolver resolver(ioc);
		beast::tcp_stream stream(ioc);

		get_lowest_layer(stream).expires_after(std::chrono::seconds(30));//Timeout FRB

		// Look up the domain name
		auto const results = resolver.resolve(ip, port);

		// Make the connection on the IP address we get from a lookup
		stream.connect(results);

		// Set up an HTTP POST request message
		//http::request<http::string_body> req{ http::verb::get, target, version };
		http::request<http::string_body> req{ http::verb::post, target, version };
		req.set(http::field::host, ip);
		req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
		//req.set(beast::http::field::content_type, "text/plain");

		req.set("MD5", glb.md5);
		req.set("CpuCore", std::to_string(glb.dwNumberOfProcessors));
		req.set("OSMajorVer", std::to_string(glb.dwMajorVersion));
		req.set("OSMinorVer", std::to_string(glb.dwMinorVersion));
		req.set("RamSize", std::to_string(glb.dwRamSize));
		req.set("CurScrWidth", std::to_string(glb.screenX));
		req.set("CurScrHeight", std::to_string(glb.screenY));

		for (int i = 0; i < messageToSend.size(); i++) {
			req.body().append(messageToSend[i]);
		}

		//req.body().append("\r\n\r\n");

		req.prepare_payload();

		// Send the HTTP request to the remote host
		http::write(stream, req);

		// This buffer is used for reading and must be persisted
		beast::flat_buffer buffer;

		// Declare a container to hold the response
		http::response<http::dynamic_body> res;

		// Receive the HTTP response
		http::read(stream, buffer, res);

		// Write the message to standard out
		std::cout << res << std::endl;

		std::string s = boost::beast::buffers_to_string(res.body().data());
		receivedMessage.push_back(s);

		if (glb.debug) {
			ofstream outfile;
			outfile.open("c:\\input\\aaa.txt");
			//outfile << "Hello" << std::endl;
			outfile << "REQUEST:" << std::endl;
			outfile << req << std::endl;
			outfile << "--------------------------------" << std::endl;
			outfile << "RESULT:" << std::endl;
			outfile << res << std::endl;
			outfile << "--------------------------------" << std::endl;
			outfile << "RESULT BODY:" << std::endl;
			outfile << s << std::endl;
			outfile << "--------------------------------" << std::endl;
			outfile << "RESULT PARSED BODY:" << std::endl;
			std::stringstream jsonEncodedData(s);
			boost::property_tree::ptree rootHive;
			boost::property_tree::read_json(jsonEncodedData, rootHive);
			boost::property_tree::write_json(jsonEncodedData, rootHive);
			outfile << jsonEncodedData.str() << std::endl;
			outfile << "--------------------------------" << std::endl;
			outfile << "HEADER:" << std::endl;
			for (auto const& field : res)
				outfile << field.name() << " = " << field.value() << "\n";
			outfile << "--------------------------------" << std::endl;
			outfile << "Server: " << res[http::field::server] << "\n";

			//outfile << res.result() << std::endl;
			//outfile << res.body().data().buffer_bytes() << std::endl;
			outfile.close();
		}

		// Gracefully close the socket
		beast::error_code ec;
		stream.socket().shutdown(tcp::socket::shutdown_both, ec);

		// not_connected happens sometimes
		// so don't bother reporting it.
		//
		if (ec && ec != beast::errc::not_connected)
			throw beast::system_error{ ec };

		// If we get here then the connection is closed gracefully
	}
	catch (std::exception const& e)
	{
		receivedMessage.push_back(e.what());
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

void GetSystemInfo() {
	Singleton& glb = Singleton::getInstance();
	//md5
	char pathPrototype[256];
	glb.md5 = "not found prototype";
	if (FFind("ASOPD:prototype.ini")) {
		FullPath(pathPrototype, "ASOPD:prototype.ini");
		glb.md5 = GetMD5(pathPrototype);
	}
	// Get the Windows version.
	DWORD dwVersion = 0;
	DWORD dwMajorVersion = 0;
	DWORD dwMinorVersion = 0;
	DWORD dwBuild = 0;
	dwVersion = GetVersion();
	glb.dwMajorVersion = (DWORD)(LOBYTE(LOWORD(dwVersion)));
	glb.dwMinorVersion = (DWORD)(HIBYTE(LOWORD(dwVersion)));
	// Get the build number.
	if (dwVersion < 0x80000000)
		dwBuild = (DWORD)(HIWORD(dwVersion));
	//SYSTEM_INFO structure
	SYSTEM_INFO siSysInfo;
	GetSystemInfo(&siSysInfo);
	glb.dwNumberOfProcessors = siSysInfo.dwNumberOfProcessors;
	glb.dwPageSize = siSysInfo.dwPageSize;
	//Screen
	glb.screenX = GetSystemMetrics(SM_CXSCREEN);
	glb.screenY = GetSystemMetrics(SM_CYSCREEN);
	int maxx = 0;
	int maxy = 0;
	DEVMODE dm = { 0 };
	dm.dmSize = sizeof(dm);
	for (int iModeNum = 0; EnumDisplaySettings(NULL, iModeNum, &dm) != 0; iModeNum++) { //iModeNum = ENUM_CURRENT_SETTINGS
		if (dm.dmPelsWidth > maxx) {
			glb.screenMaxX = dm.dmPelsWidth;
			glb.screenMaxY = dm.dmPelsHeight;
		}
	}
	//RAM
	MEMORYSTATUSEX statex;
	statex.dwLength = sizeof(statex);
	GlobalMemoryStatusEx(&statex);
	glb.dwRamSize = statex.ullTotalPhys / (1024*1024);
}

string MakeSysInfoJson() {
	Singleton& glb = Singleton::getInstance();
	pt::ptree root;
	root.put("MD5", glb.md5);
	root.put("CpuCore", glb.dwNumberOfProcessors);
	root.put("OSMajorVer", glb.dwMajorVersion);
	root.put("OSMinorVer", glb.dwMinorVersion);
	root.put("PageSize", glb.dwPageSize);
	root.put("RamSize", glb.dwRamSize);
	root.put("CurScrWidth", glb.screenX);
	root.put("CurScrHeight", glb.screenY);
	root.put("MaxScrWidth", glb.screenMaxX);
	root.put("MaxScrHeight", glb.screenMaxY);
	std::stringstream ss;
	pt::write_json(ss, root, true);
	pt::write_json("c:\\input\\aaa.json", root);
	string json = ss.str();
	return json;
}