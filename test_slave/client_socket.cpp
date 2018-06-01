#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <cstring>


#include "client_socket.h"
using namespace std;

CLIENT_SOCKET::CLIENT_SOCKET(){
	initSocket();
}

CLIENT_SOCKET::CLIENT_SOCKET(int port,const char* ip_addr){
	initSocket();
	setAddr(port,ip_addr);
}

CLIENT_SOCKET::~CLIENT_SOCKET(){
	closeSocket();
}

//initialize socket
void CLIENT_SOCKET::initSocket(){
	sock = socket(AF_INET, SOCK_STREAM, 0);
}

//set up address
void CLIENT_SOCKET::setAddr(int port,const char* ip_addr){

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(ip_addr);
}

/* get socket */
int CLIENT_SOCKET::getSocket(){
	return sock;
}

//connect to server
bool CLIENT_SOCKET::connectSocket(){
    if(connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0)
    	return true;
    else
    	return false;
}

//get from server
int CLIENT_SOCKET::readSocket(){
	memset(buff, 0, sizeof(buff)); // èâä˙âª
 	return read(sock, buff, sizeof(buff));
}

const char* CLIENT_SOCKET::getBuff(){
	return buff;
}

bool CLIENT_SOCKET::sendSocket(string message){
	sendto(sock, message.c_str(), message.size()+1, 0,(struct sockaddr*)&addr,sizeof(addr));
}

bool CLIENT_SOCKET::sendSocket(const char* message){
	sendto(sock, message, strlen(message)+1, 0,(struct sockaddr*)&addr,sizeof(addr));
}

void CLIENT_SOCKET::closeSocket(){
	close(sock);
}