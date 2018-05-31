#ifndef CLIENT_SOCKET_INCLUDE
#define CLIENT_SOCKET_INCLUDE 

#include <string>
#include <netinet/in.h>

const int BUFF_SIZE = 256;

class CLIENT_SOCKET{
public:
	CLIENT_SOCKET();
	CLIENT_SOCKET(int,const char*);
	~CLIENT_SOCKET();
	void initSocket();
	void setAddr(int,const char*);
	bool connectSocket();
	int readSocket();
	bool sendSocket(std::string);
	bool sendSocket(const char*);
	const char* getBuff();
	void closeSocket();

private:
	struct sockaddr_in addr;
	int sock;
	char buff[BUFF_SIZE];
	
};

#endif