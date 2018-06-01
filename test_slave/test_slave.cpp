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
const char SERVER_IP[] = "160.12.172.211"; //pi
const int CAPACITY = 1;
const int BLOCK_NUM = 10;
bool flag = 0;

using namespace std;

//the thread function
void *get_message_interruption(void *);

void printError(int line){
    printf("LINE: %d error !\n", line);
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
	printf("PORTNUM is %d\n", PORTNUM); // for Debug
    if(!socket.connectSocket()){
        printError(__LINE__);
        return -1;
    }

	//send name
	socket.sendSocket("I am a human.");

    //get connection mes from server
    if(socket.readSocket() == -1){
        printError(__LINE__);
        return -1;
    }
    printf("Master Message : %s\n", socket.getBuff());

    //send capacity
    socket.sendSocket(to_string(CAPACITY));

    //get rangeof rand
    if(socket.readSocket()==-1){
        printError(__LINE__);
        return -1;
    }
    printf("Range : %s\n", socket.getBuff());
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
    printf("n_zeros : %d\n", n_zeros);
    

	///// Nonce Culculate /////

    for(int i=0; i<BLOCK_NUM; i++){
		puts("================ START NONCE CALCULATION ==============");
		printf("Now : %d\n", i+1); // for Debug
		//flag reset
		flag = 0;

	    //get block data
	    if(socket.readSocket()==-1){
	        printError(__LINE__);
	        return -1;
	    }
		printf("Block : %s\n", socket.getBuff()); // for Debug

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
		printf("My flag is %d\n", flag); // for Debug
		if (flag == 0) {
			pthread_cancel(thread_id); // thread cancel

			printf("nonce : %s\n", nonce.c_str());
			printf("hash  : %s\n", double_hash.c_str());

			socket.sendSocket(nonce);
		}
		else if (flag == 1) {
			flag = 0;
			puts("I changed flag to 0");
			puts("I didn't make a new nonce");
		}
		pthread_join(thread_id, NULL);
	}

	//get finish message
	if (socket.readSocket() == -1){
		printError(__LINE__);
		return -1;
	}
	puts(socket.getBuff()); //for Debug

	return 0;
}

/* This is interruption function */
void *get_message_interruption(void *thread_data){
	char buff[BUFF_SIZE];
	int *sock = (int *)thread_data;

	read(*sock, buff, sizeof(buff));
	printf("From Master : %s\n", buff); //for Debug

	write(*sock, "OK, I stopped.", sizeof("OK, I stopped.") + 1);
	flag = 1;

	return 0;
}