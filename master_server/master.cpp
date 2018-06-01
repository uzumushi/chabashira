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

const char SERVER_IP[]="127.0.0.1";
const int SERVER_PORT = 10050;
const int PORT = 50000;
const int N_SLAVE = 2;
const char TEAM_NAME[] = "Chabashira";
const int BLOCK_NUM = 10;
const char TO_SLAVE_MES[] = "Stop Calculation!";

CLIENT_SOCKET server_sock(SERVER_PORT,SERVER_IP);
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond;

unsigned capacity_total = 0;
unsigned capacity[N_SLAVE];
unsigned RandRange[N_SLAVE];
int client_sock[N_SLAVE]; // from main function
 
int n_zeros=0;
string block_data;
int block_count = 0;
bool flag = 0; //for stop slave

//the thread function
void *connection_handler_initial(void *);
void *connection_handler_execute(void *);
void *recv_ready_message(void *);

struct THREAD_DATA{
    int id;
    int client_sock;
};

int main(int argc , char *argv[])
{
    int socket_desc , c;
    struct sockaddr_in server , client;
    THREAD_DATA thread_data[N_SLAVE];
 
	//Initialize mutex and cond
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
    
    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1){
        printf("Could not create socket");
        return -1;
    }
    puts("Socket created");
     
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( PORT );
     
    //Bind
	cout << "PORT is " << PORT << endl; //for Debug
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0){
        //print the error message
        perror("bind failed. Error");
        return -1;
    }
     
    //Listen
    listen(socket_desc , 2); 
    c = sizeof(struct sockaddr_in);
     
    pthread_t thread_id[N_SLAVE];
    
    for(int i=0;i < N_SLAVE; i++){
        client_sock[i] = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
        
        if (client_sock < 0){
            perror("accept failed");
            return -1;
        }

        thread_data[i].id = i;
        thread_data[i].client_sock = client_sock[i];

		//Make thread (connection_handler_initial)
        if( pthread_create( &thread_id[i] , NULL ,  connection_handler_initial , (void*) &thread_data[i]) < 0){
            perror("could not create thread");
            return -1;
        }
		cout << "This thread is " << &thread_id[i] << endl; // for Debug
    }

	//Join thread (connection_handler_initial)
	for (int i = 0; i < N_SLAVE; i++){
		pthread_join(thread_id[i], NULL);
        cout << "Join thread is " << &thread_id[i] << endl; // for Debug
	}
    cout << "all connection success" <<endl;

    //Decide range of rand
    for (int i = 0; i < N_SLAVE; ++i){
        RandRange[i] = UINT_MAX / capacity_total * capacity[i];
        if(i!=0)RandRange[i] += RandRange[i-1];
    }

    //Make thread to get answer (connection_handler_execute)
    for(int i=0;i<N_SLAVE;i++){
        if( pthread_create( &thread_id[i] , NULL ,  connection_handler_execute , (void*) &thread_data[i]) < 0){
            perror("could not create thread");
            return -1;
        }
    }

    //Connect to server
    server_sock.connectSocket();
    if(server_sock.readSocket() == -1){
        perror("server connection error");
        return -1;
    }
    cout << "Server Message :" << server_sock.getBuff() << endl;

    //Send team name
    server_sock.sendSocket(TEAM_NAME);

    //Get number of continuous zeros
    if(server_sock.readSocket() == -1){
        perror("server connection error");
        return -1;
    }
    cout << "Number of continuous zeros :" << server_sock.getBuff() << endl;
    n_zeros = atoi(server_sock.getBuff());
    pthread_cond_broadcast(&cond); // bloadcast continuous zeros

	cout << "================= START CALCULATION (LOOP SECTION) ==================" << endl;

    block_count =0;
    for(int i=0; i<BLOCK_NUM; i++){
        //Get blockdata
        if(server_sock.readSocket() == -1){
            perror("server connection error");
            return -1;
        }
        block_data = server_sock.getBuff();
		cout << "Block :" << block_data << endl;

		// ここにready信号を受け取る処理を記述する
		//char ready[BUFF_SIZE]; // for ready
		//for (int i = 0; i < N_SLAVE; i++){
		//		recv(client_sock[i], ready, BUFF_SIZE, 0); // send cancel message to each slaves
		//		cout << "Slave ready! " << "(" << i + 1 << ")" << endl;
		//}
		//cout << "i = " << i + 1 << "(READY)" << endl; //for Debug 

		//Make thread to get ready message (recv_ready_message)
		//pthread_t thread_ready[N_SLAVE];
		//for (int i = 0; i<N_SLAVE; i++){
		//	if (pthread_create(&thread_ready[i], NULL, recv_ready_message, (void*) &client_sock[i]) < 0){
		//		perror("could not create thread");
		//		return -1;
		//	}
		//	cout << "Create thread_ready is " << &thread_ready[i] << endl; // for Debug
		//}

		////Join thread (recv_ready_message)
		//for (int i = 0; i < N_SLAVE; i++){
		//	pthread_join(thread_ready[i], NULL);
		//	cout << "Join thread_ready is " << &thread_ready[i] << endl; // for Debug
		//}

		flag = 0; // flag reset
        pthread_cond_broadcast(&cond); // bloadcast block

		cout << "============================================" << endl; //for Debug
        cout << "Bloadcast Block" << endl;
    }

    for (int i = 0; i < N_SLAVE; i++){
    	pthread_join(thread_id[i], NULL);
        cout << "FINISH Join thread is " << &thread_id[i] << endl; // for Debug
	}
    cout << "FINISH" << endl;

    return 0;
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
    cout << "Capacity : " << client_message << " (thread id = " << id << ")" << endl; // for Debug
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
    cout << "Range : " << send_message << " (thread id = " << id << ")" << endl; // for Debug
    write(sock, send_message.c_str(),send_message.length());

    //send number of continuous zeros
    pthread_cond_wait(&cond, &mutex);
    pthread_mutex_unlock(&mutex);
    write(sock,to_string(n_zeros).c_str(),to_string(n_zeros).length());


	///// Send Block /////

    for(ans_count = 0; ans_count<BLOCK_NUM ;ans_count++){
        //send blockdata

        pthread_cond_wait(&cond, &mutex); //mutex unlock and wait cond (sleep), last mutex lock
        pthread_mutex_unlock(&mutex);

        write(sock,block_data.c_str(),block_data.length());
        
        //get ans from Slave
        read_size = recv(sock, client_message, BUFF_SIZE, 0);
		pthread_mutex_lock(&mutex);
		if (flag == 0) {
			flag = 1;
			cout << "I am " << id << " thread !" << "And Now ans_count is " << ans_count+1 << endl;
			cout << "Recv Answer :" << client_message << "(id = " << id << ")" << endl;
			for (int i = 0; i < N_SLAVE; i++){
				if (sock != client_sock[i]){
					write(client_sock[i], TO_SLAVE_MES, strlen(TO_SLAVE_MES)); // send cancel message to each slaves
				}
			}
		}
		else if (flag == 1) {
			cout << client_message << endl; // for Debug
		}
		pthread_mutex_unlock(&mutex);

        //send ans to Server
        pthread_mutex_lock(&mutex);
        if(ans_count == block_count){
            server_sock.sendSocket(client_message);
            cout << "I send " << client_message << endl;
            block_count++;
        } else {
            cout << "Nonce reject" << "(id = " << id << ")" << endl;
        }
        pthread_mutex_unlock(&mutex);
    }

    return 0;
}

/* This is function to recieve ready message from slaves */
void *recv_ready_message(void *thread_data){
	char ready[BUFF_SIZE]; // for ready
	int *sock = (int *)thread_data;
	recv(*sock, ready, BUFF_SIZE, 0); // recieve ready message to each slaves // ここでずっとまってる状態
	cout << ready << endl;

	return 0;
}