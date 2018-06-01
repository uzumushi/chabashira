#include <iostream>
#include <iomanip>
#include <string.h>
#include <sstream>
#include <random>
#include <limits.h>
#include <pthread.h>

#include "./PicoSHA2/picosha2.h"
#include "client_socket.h"

using namespace std;

const int PORTNUM = 50500;
const char SERVER_IP[] = "160.12.172.211";
const int CAPACITY = 1;
const int THREAD_NUM = 16;
const int DEFAULT_BLOCK_NUM = 10;

void *hash_calculation(void *);
void *finish_mes_reciever(void *);

struct THREAD_DATA{
    int id;
};

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
CLIENT_SOCKET sock(PORTNUM,SERVER_IP);//socket for server connection
int n_zeros;//condition of continuous zeros
string header;
bool f_finish = false;
unsigned hash_begin;
unsigned hash_end;



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

int main(int argc,char** argv){

    string nonce;//for nonce
    string double_hash;//for hash
    THREAD_DATA thread_data[THREAD_NUM];
    pthread_t thread_id[THREAD_NUM];
    pthread_t reciever_id;
    int block_num;

    block_num = argc>2 ? atoi(argv[1]):DEFAULT_BLOCK_NUM;
    
    //make socket connection
    if(!sock.connectSocket()){
        printError(__LINE__);
        return -1;
    }
    sock.sendSocket("wittgenstein");    
    
    //get connection mes from server
    if(sock.readSocket() == -1){
        printError(__LINE__);
        return -1;
    }
    cout << sock.getBuff() <<endl;

    //send capacity
    sock.sendSocket(to_string(CAPACITY));

    //get rangeof rand
    if(sock.readSocket()==-1){
        printError(__LINE__);
        return -1;
    }
    cout << sock.getBuff()<<endl;
    istringstream iss(sock.getBuff());
    iss >> hash_begin >> hash_end;

    //get number of continuous zero
    if(sock.readSocket()==-1){
        printError(__LINE__);
        return -1;
    }
    n_zeros = atoi(sock.getBuff());
    cout << "n_zeros : " << n_zeros << endl;
    


    for (int i=0;i<block_num;i++){

	    //get block data
	    if(sock.readSocket()==-1){
	        printError(__LINE__);
	        return -1;
	    }
        cout << "data : " << sock.getBuff() << endl;
        header = sock.getBuff();

        //make calcration threads
        for(int i=0;i<THREAD_NUM ;i++){
            if( pthread_create( &thread_id[i] , NULL ,  hash_calculation , (void*) &thread_data[i]) < 0){
                perror("could not create thread");
                return -1;
            }   
        }


        //make mes reciever thread
        if( pthread_create( &reciever_id , NULL ,  finish_mes_reciever , NULL)){
            perror("could not create thread");
            return -1;
        }

        for (int i = 0; i < THREAD_NUM; ++i){
            pthread_join( thread_id[i], NULL);           
        }
        pthread_cancel(reciever_id);
        f_finish = false;
	}

	return 0;
}

void *hash_calculation(void * t_data){
    THREAD_DATA* td = (THREAD_DATA*)t_data;
    string double_hash;
    string nonce;
    int nonce_value = hash_begin + (hash_end - hash_begin) / THREAD_NUM * td->id;
            
    do{
        ostringstream os;
        os<< hex << setw(8) <<setfill('0')<< nonce_value;
        nonce = os.str();

        string block = header + nonce;
        getDoubleHash(header,double_hash);
    }while(checkContinuousZeros(double_hash.c_str(),n_zeros)==false && !f_finish);

    pthread_mutex_lock( &mutex);
    if(f_finish == false){
        sock.sendSocket(nonce);
        cout << nonce << endl;
        f_finish = true;
    }
    pthread_mutex_unlock( &mutex);
}

void *finish_mes_reciever(void* ptr){
    if(sock.readSocket()==-1){
        printError(__LINE__);
        exit(-1);
               
    }
    
    pthread_mutex_lock( &mutex);
    if(f_finish == false){
        sock.sendSocket("calc stop");
        f_finish = true;
    }
    pthread_mutex_unlock( &mutex);
}
