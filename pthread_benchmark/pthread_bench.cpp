#include <iostream>
#include <iomanip>
#include <string.h>
#include <sstream>
#include <random>
#include <limits.h>
#include <pthread.h>
#include <unistd.h>
#include "./PicoSHA2/picosha2.h"

using namespace std;

const int THREAD_NUM = 16;
const int DEFAULT_BLOCK_NUM = 10;

void *hash_calculation(void *);
void *timer(void *);

struct THREAD_DATA{
    int id;
    int count;
};

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int n_zeros;//condition of continuous zeros
string header;
bool f_finish = false;
unsigned hash_begin = 0;
unsigned hash_end = UINT_MAX; 

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
    
    n_zeros = 30;
    
    for (int i=0;i<1;i++){

        header = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
        
            if( pthread_create( &reciever_id , NULL ,  timer , (void*) &thread_data[i]) < 0){
                perror("could not create thread");
                return -1;
            }   

        //make calcration threads
        for(int i=0;i<THREAD_NUM ;i++){
            if( pthread_create( &thread_id[i] , NULL ,  hash_calculation , (void*) &thread_data[i]) < 0){
                perror("could not create thread");
                return -1;
            }   
        }

        //make mes reciever thread

        for (int i = 0; i < THREAD_NUM; ++i){
            pthread_join( thread_id[i], NULL);           
        }
        pthread_cancel(reciever_id);
    }
  
    int total=0;
    for(int i=0;i<THREAD_NUM;i++){
        total+=thread_data[i].count;
    }
    cout << "hashrate : " << total <<endl;
    return 0;
}

void *hash_calculation(void * t_data){
    THREAD_DATA* td = (THREAD_DATA*)t_data;
    string double_hash;
    string nonce;
    int nonce_value = hash_begin + (hash_end - hash_begin) / THREAD_NUM * td->id;
    td->count =0;       
    do{
        ostringstream os;
        os<< hex << setw(8) <<setfill('0')<< nonce_value;
        nonce = os.str();

        string block = header + nonce;
        getDoubleHash(header,double_hash);
        td->count++;
    }while(checkContinuousZeros(double_hash.c_str(),n_zeros)==false && !f_finish);

    pthread_mutex_lock( &mutex);
    if(f_finish == false){
        f_finish = true;
    }
    pthread_mutex_unlock( &mutex);
}

void *timer(void* ptr){
    sleep(1);
    cout << "timer_finish" <<endl; 
    pthread_mutex_lock( &mutex);
    if(f_finish == false){
        f_finish = true;
    }
    pthread_mutex_unlock( &mutex);
}
