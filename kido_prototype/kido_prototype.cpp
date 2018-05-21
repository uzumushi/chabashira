#include <iostream>
#include <iomanip>
#include <string.h>
#include <sstream>
#include <random>
#include <limits.h>

#include "./PicoSHA2/picosha2.h"
#include "client_socket.h"

const int PORTNUM = 10050;
const char SERVER_IP[] = "127.0.0.1";

using namespace std;

void printError(int line){
    cout << "LINE:" << line << " error !" << endl;
}

bool checkContinuousZeros(const char* str,int n_zeros){

    for (int i = 0; i < n_zeros; ++i){
        if(str[i] != '0')
            return false;
    }

    return true;
}

void getDoubleHash(string& header,string& double_hash){
    string header_hash;

    picosha2::hash256_hex_string(header, header_hash);
    picosha2::hash256_hex_string(header_hash, double_hash);
}

int main(void){

    CLIENT_SOCKET socket(PORTNUM,SERVER_IP);//socket for server connection
    int n_zeros;//condition of continuous zeros
    string nonce;//for nonce
    string double_hash;//for hash

    //random setting
    random_device rnd;
    mt19937 mt(rnd());
    std::uniform_int_distribution<unsigned> rand_unsigned(0, UINT_MAX);

    //make socket connection
    if(!socket.connectSocket()){
        printError(__LINE__);
        return -1;
    }

    //get connection mes from server
    if(socket.readSocket() == -1){
        printError(__LINE__);
        return -1;
    }
    cout << socket.getBuff() <<endl;

    //send team name
    string team_name("Chabashira");
    cout << "team_name: " << team_name << endl;
    socket.sendSocket(team_name);

    //get number of continuous zero
    if(socket.readSocket()==-1){
        printError(__LINE__);
        return -1;
    }
    n_zeros = atoi(socket.getBuff());
    cout << "n_zeros : " << n_zeros << endl;

    //get block data
    if(socket.readSocket()==-1){
        printError(__LINE__);
        return -1;
    }

    //get nonce
    do{
        //get rundom for nonce
        int nonce_value = rand_unsigned(mt);
        ostringstream os;
        os<< hex << setw(8) <<setfill('0')<< nonce_value;

        //exchange nonce_value(unsigned) to nonce(string)
        nonce = os.str();

        //set header value
        string header = socket.getBuff() + nonce;
        
        //get hash
        getDoubleHash(header,double_hash);

    }while(checkContinuousZeros(double_hash.c_str(),n_zeros)==false);

    cout << "nonce : " << nonce <<endl;
    cout << "hash  : " << double_hash <<endl;

    socket.sendSocket(nonce);
	
	return 0;
}