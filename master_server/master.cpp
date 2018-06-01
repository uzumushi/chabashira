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

const char SERVER_IP[]="160.12.172.43"; //giren
const int SERVER_PORT = 10050;
const int PORT = 50000;
const char TEAM_NAME[] = "Chabashira";
const int N_SLAVE = 3;
const int BLOCK_NUM = 10;
const char TO_SLAVE_MES[] = "Stop Calculation!";

CLIENT_SOCKET server_sock(SERVER_PORT,SERVER_IP);
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond;

unsigned capacity_total = 0;
unsigned capacity[N_SLAVE];
unsigned RandRange[N_SLAVE];
int client_sock[N_SLAVE]; //from main function
 
string block_data;
int n_zeros = 0;
int block_count = 0;
int slave_count = 0; //for count wait slaves
bool flag = 0; //for stop slave

//the thread function
void *connection_handler_initial(void *);
void *connection_handler_execute(void *);
void *recv_ready_message(void *);

struct THREAD_DATA{
    int id;
	char client_name[BUFF_SIZE];
    int client_sock;
};

int main(int argc , char *argv[])
{
    int socket_desc , c;
    struct sockaddr_in server , client;
    THREAD_DATA thread_data[N_SLAVE];
 
	//initialize mutex and cond
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);

	puts("================= SLAVE CONNECTION ==================");
    
    //create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1){
        printf("Could not create socket");
        return -1;
    }
    puts("Socket created");
     
    //prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( PORT );
     
    //bind
	printf("PORT is %d.\n", PORT); //for Debug
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0){
        //print the error message
        perror("bind failed. Error");
        return -1;
    }
     
    //listen
    listen(socket_desc , N_SLAVE); 
    c = sizeof(struct sockaddr_in);
     
    pthread_t thread_id[N_SLAVE];
    
    for(int i=0;i < N_SLAVE; i++){
		//accept
        client_sock[i] = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
        if (client_sock[i] < 0){
            perror("accept failed");
            return -1;
        }

        thread_data[i].id = i;
        thread_data[i].client_sock = client_sock[i];

		//make thread (connection_handler_initial)
        if( pthread_create( &thread_id[i] , NULL ,  connection_handler_initial , (void*) &thread_data[i]) < 0){
            perror("could not create thread");
            return -1;
        }
		printf("This thread is %x.\n", &thread_id[i]); // for Debug
    }

	//join thread (connection_handler_initial)
	for (int i = 0; i < N_SLAVE; i++){
		pthread_join(thread_id[i], NULL);
        printf("Join thread is %x.\n", &thread_id[i]); // for Debug
	}
    puts("All slaves connection success!");

    //decide range of rand
    for (int i = 0; i < N_SLAVE; ++i){
        RandRange[i] = UINT_MAX / capacity_total * capacity[i];
        if(i!=0)RandRange[i] += RandRange[i-1];
    }

    //make thread to get answer (connection_handler_execute)
    for(int i=0;i<N_SLAVE;i++){
        if( pthread_create( &thread_id[i] , NULL ,  connection_handler_execute , (void*) &thread_data[i]) < 0){
            perror("could not create thread");
            return -1;
        }
    }

	puts("================= SERVER CONNECTION ==================");

    //connect to server
    server_sock.connectSocket();
    if(server_sock.readSocket() == -1){
        perror("server connection error");
        return -1;
    }
    printf("Server Message : %s\n", server_sock.getBuff());

    //send team name
    server_sock.sendSocket(TEAM_NAME);

    //get number of continuous zeros
    if(server_sock.readSocket() == -1){
        perror("server connection error");
        return -1;
    }
    printf("Number of continuous zeros : %s\n", server_sock.getBuff());
    n_zeros = atoi(server_sock.getBuff());
    pthread_cond_broadcast(&cond); // bloadcast continuous zeros

	puts("================= START CALCULATION (LOOP SECTION) ==================");

    block_count =0;
    for(int i=0; i<BLOCK_NUM; i++){
		//wait all threads
		while (1){
			if (slave_count == N_SLAVE){
				slave_count = 0; // reset
				break;
			}
		}

        //get blockdata
        if(server_sock.readSocket() == -1){
            perror("server connection error");
            return -1;
        }
        block_data = server_sock.getBuff();
		printf("Block : %s\n", block_data.c_str());

		flag = 0; // flag reset
        pthread_cond_broadcast(&cond); // bloadcast block

		puts("============================================"); //for Debug
        puts("Bloadcast Block");
    }

	//get finish message from server
	if (server_sock.readSocket() == -1){
		perror("server finish error");
		return -1;
	}

	//join thread (connection_handler_execute)
    for (int i = 0; i < N_SLAVE; i++){
    	pthread_join(thread_id[i], NULL);
        printf("FINISH Join thread is %x\n", &thread_id[i]); // for Debug
	}

    puts(server_sock.getBuff()); //for Debug

    return 0;
}
 

 // This will handle connection for each client
void *connection_handler_initial(void *thread_data)
{
    //set the socket descriptor
    int id = ((THREAD_DATA*)thread_data) -> id;
    int sock = ((THREAD_DATA*)thread_data) -> client_sock;
    int read_size;
    char *message , client_message[BUFF_SIZE];

	//get slave name
	read_size = recv(sock, client_message, BUFF_SIZE, 0);
	//((THREAD_DATA*)thread_data)->client_name = client_message;
	printf("Connection success : %s\n", client_message); //for Debug
	memset(client_message, 0, sizeof(client_message)); // 初期化

    //gend connection mes to slave
    message = "Connection success";
    write(sock , message , strlen(message));
    
    //read capacity from slave
    read_size = recv(sock , client_message , BUFF_SIZE , 0);
	printf("Capacity : %s (thread id = %d)\n", client_message, id); //for Debug
    capacity[id] = atoi(client_message);
    
    pthread_mutex_lock(&mutex);
    capacity_total += capacity[id];
    pthread_mutex_unlock(&mutex);
         
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
	printf("Range : %s (thrad id = %d)\n", send_message.c_str(), id); //for Debug
    write(sock, send_message.c_str(),send_message.length());

    //send number of continuous zeros
    pthread_cond_wait(&cond, &mutex);
    pthread_mutex_unlock(&mutex);
    write(sock,to_string(n_zeros).c_str(),to_string(n_zeros).length());


    for(ans_count = 0; ans_count<BLOCK_NUM ;ans_count++){

		//wait block data
		printf("I wait block data. (id = %d)\n", id); // for Debug
		slave_count += 1; //tell wait
        pthread_cond_wait(&cond, &mutex); //mutex unlock and wait cond (sleep), last mutex lock
        pthread_mutex_unlock(&mutex);

		//send block data
        write(sock,block_data.c_str(),block_data.length());
        
        //get ans from Slave
        read_size = recv(sock, client_message, BUFF_SIZE, 0);
		pthread_mutex_lock(&mutex);
		if (flag == 0) {
			flag = 1;

			printf("I am %d thread! And Now ans_count is %d.\n", id, ans_count+1); //for Debug
			printf("Recv Answer : %s (id = %d)\n", client_message, id); //for Debug

			server_sock.sendSocket(client_message);
			printf("I sent %s\n", client_message); //for Debug

			for (int i = 0; i < N_SLAVE; i++){
				if (sock != client_sock[i]){
					write(client_sock[i], TO_SLAVE_MES, strlen(TO_SLAVE_MES)); // send cancel message to each slaves
					printf("%s (id = %d)\n", TO_SLAVE_MES, i); // for Debug
				}
			}
		}
		else if (flag == 1) {
			printf("%s (id = %d)\n", client_message, id); // for Debug
		}
		pthread_mutex_unlock(&mutex);
    }

	//send finish message
	write(sock, "finish", sizeof("finish")+1);

    return 0;
}
