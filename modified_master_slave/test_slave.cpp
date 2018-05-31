#include <iostream>
#include <iomanip>
#include <string.h>
#include <sstream>
#include <random>
#include <limits.h>
#include <unistd.h> // added by tomoya
#include <pthread.h>    /* pthread_create(), pthread_detach() */

#include "./PicoSHA2/picosha2.h"
#include "client_socket.h"

const int PORTNUM = 50000;
const char SERVER_IP[] = "127.0.0.1";
const int CAPACITY = 1;
const int BLOCK_NUM = 10;
const char READY[] = "ready";
bool flag = 0;

using namespace std;

//the thread function
void *get_message_interruption(void *);

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
	pthread_t thread_id;//for get message thread 

    //random setting
    random_device rnd;
    mt19937 mt(rnd());
    
    //make socket connection
	cout << "PORTNUM is " << PORTNUM << endl; // for Debug
    if(!socket.connectSocket()){
        printError(__LINE__);
        return -1;
    }

    //get connection mes from server
    if(socket.readSocket() == -1){
        printError(__LINE__);
        return -1;
    }
    cout << "Master Message :" << socket.getBuff() <<endl;

    //send capacity
    socket.sendSocket(to_string(CAPACITY));

    //get rangeof rand
    if(socket.readSocket()==-1){
        printError(__LINE__);
        return -1;
    }
    cout << "Range :" << socket.getBuff()<<endl;
    istringstream iss(socket.getBuff());
    unsigned rand_begin,rand_end;
    iss >> rand_begin >> rand_end;
    std::uniform_int_distribution<unsigned> rand_unsigned(rand_begin, rand_end);


    //get number of continuous zero
    if(socket.readSocket()==-1){
        printError(__LINE__);
        return -1;
    }
    n_zeros = atoi(socket.getBuff());
    cout << "n_zeros : " << n_zeros << endl;
    

	///// Nonce Culculate /////

    for(int i=0; i<BLOCK_NUM; i++){
		cout << "================ START NONCE CALCULATION ==============" << endl;
		cout << "Now : " << i + 1 << endl; // for Debug
		//flag reset
		flag = 0;
		////send ready message
		//socket.sendSocket(READY);
		//cout << "I send READY!" << endl;

	    //get block data
	    if(socket.readSocket()==-1){
	        printError(__LINE__);
	        return -1;
	    }
		cout << "Block :" << socket.getBuff() << endl; // for Debug

		/* create thread for getting message another slave got nonce */
		int sock = socket.getSocket();
		if (pthread_create(&thread_id, NULL, get_message_interruption, (void*) &sock) < 0){
			perror("could not create thread");
			return -1;
		}

	    //get right nonce
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

			//break
			if (flag == 1){
				break;
			}

	    }while(checkContinuousZeros(double_hash.c_str(),n_zeros)==false);

		/* join thread */
		cout << "My flag is " << flag << endl; // for Debug
		if (flag == 0) {
			pthread_cancel(thread_id); // thread cancel

			cout << "nonce : " << nonce << endl;
			cout << "hash  : " << double_hash << endl;

			socket.sendSocket(nonce);
		}
		else if (flag == 1) {
			flag = 0;
			cout << "I changed flag 0" << endl;
			cout << "I didn't make a new nonce" << endl;
		}
		pthread_join(thread_id, NULL);
	}

	cout << "FINISH!" << endl; // for Debug

	return 0;
}

/* This is interruption function */
void *get_message_interruption(void *thread_data){
	char buff[BUFF_SIZE];
	int *sock = (int *)thread_data;
	read(*sock, buff, sizeof(buff));
	cout << "From Master :" << buff << endl;
	write(*sock, "OK, I stopped.", sizeof("OK, I stopped.") + 1);
	flag = 1;

	return 0;
}