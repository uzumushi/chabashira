#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>    /* pthread_create(), pthread_detach() */
#include <string>
#include <cstring>
#include <limits.h>
#include <string>
#include <iostream>

#include "client_socket.h"

using namespace std;

const int PORT = 50000; // Free Number 49151-65535
const int N_SLAVE = 2;
const char TEAM_NAME[] = "Chabashira";

CLIENT_SOCKET server_sock(PORT,SERVER_IP);
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond;

unsigned capacity_total = 0;
unsigned capacity[N_SLAVE];
unsigned RandRange[N_SLAVE];
 
int n_zeros=0;
string block_data;
int block_count = 0;

//the thread function
void *connection_handler_initial(void *);
void *connection_handler_execute(void *);

struct THREAD_DATA{
    int id;
    int client_sock;
};

int main(int argc , char *argv[])
{
    int socket_desc , client_sock , c;
    struct sockaddr_in server , client;
    THREAD_DATA thread_data[N_SLAVE];
    
    //Create socket (For Master-Slave)
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1){
        printf("Could not create socket");
        return -1;
    }
    puts("Master-Slave Socket created");
     
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( PORT );
     
    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0){
        //print the error message
        perror("bind failed. Error");
        return -1;
    }
     
    //Listen
    listen(socket_desc , 3); // 3ヵ所の接続待ち
    c = sizeof(struct sockaddr_in);
     
    pthread_t thread_id; // スレッドを管理する変数->1スレッドあたり1つ必要らしい
    
    for(int i=0;i < N_SLAVE; i++){
        client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
        
        if (client_sock < 0){
            perror("accept failed");
            return -1;
        }

        thread_data[i].id = i;
        thread_data[i].client_sock = client_sock;

        if( pthread_create( &thread_id , NULL ,  connection_handler_initial , (void*) &thread_data[i]) < 0){ // <0でなく!=0かな
            perror("could not create thread");
            return -1;
        }
    }

    pthread_join( thread_id , NULL); // これも管理しているスレッドの数分実行する
    cout << "all connection success" <<endl;

}
 

 // This will handle connection for each client
void *connection_handler_initial(void *thread_data)
{
    //Get the socket descriptor
    int id = ((THREAD_DATA*)thread_data) -> id;
    int sock = ((THREAD_DATA*)thread_data) -> client_sock;
    int read_size;
    char *message , client_message[BUFF_SIZE];

    //Send connection mes
    message = "Connection success";
    write(sock , message , strlen(message));
    
    //read capacity
    read_size = recv(sock , client_message , BUFF_SIZE , 0);
    capacity[id] = atoi(client_message);
    
    pthread_mutex_lock( &mutex);
    capacity_total += capacity[id];
    pthread_mutex_unlock( &mutex);

    puts(client_message);
         
    return 0;
} 

void *connection_handler_execute(void *thread_data){
    int id = ((THREAD_DATA*)thread_data) -> id;
    int sock = ((THREAD_DATA*)thread_data) -> client_sock;
    int read_size;
    char *message , client_message[BUFF_SIZE];
    int ans_count = 0;

    //send Range of Rand
    string send_message="";
    if (id == 0) send_message += "0";
    else send_message += to_string(RandRange[id-1]);
    send_message += " ";
    send_message += to_string(RandRange[id]);
    cout << send_message << endl;
    write(sock, send_message.c_str(),send_message.length());

    //send number of continuous zeros
    pthread_cond_wait(&cond, &mutex);
    write(sock,to_string(n_zeros).c_str(),to_string(n_zeros).length());

    for(ans_count = 0; 1 ;ans_count++){
        //send blockdata
        pthread_cond_wait(&cond, &mutex);
        write(sock,to_string(n_zeros).c_str(),to_string(n_zeros).length());
        
        //get ans
        read_size = recv(sock , client_message , BUFF_SIZE , 0);
        pthread_mutex_lock(&mutex);
        if(ans_count == block_count){
            server_sock.sendSocket(client_message);
            block_count++;
        }
        pthread_mutex_unlock(&mutex);
    }
}
