#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <limits.h> // get int size limit
#include <random>
#include "./PicoSHA2/picosha2.h"

const int BUFFER_SIZE = 256;
const int PORTNUM = 10050;

int dstSocket;

using namespace std;

///* クライアント(自分)についての情報 *///
struct Status {
  char ip[80]; // IPアドレス
  int port; // ポート番号
  char teamName[40]; // チームの名前
};

///* IPおよびportの設定 *///
void Set_IP_and_port(Status &myStatus){
  /* アドレスを入力 */ // 同じコンピュータ上なら127.0.0.1のテスト用IPアドレスを指定する
  cout << "Sever IP address :";
  cin >> myStatus.ip;

  /* ポートを入力 */
  cout << "Sever port :";
  cin >> myStatus.port;
}

///* 接続を確立する *///
void Connection(sockaddr_in &dstAddr, Status &myStatus) {
  // 接続するソケットの生成
  dstSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (dstSocket < 0){
      perror("ERROR opening socket");
      exit(1);
  }

  // ポート番号、アドレスを構造体で管理
  memset(&dstAddr, 0, sizeof(dstAddr)); // dstAddrを0で埋める
  dstAddr.sin_family = AF_INET; // ドメイン指定(IPv4)
  dstAddr.sin_port = htons(myStatus.port); // 通信ポート指定
  dstAddr.sin_addr.s_addr = inet_addr(myStatus.ip); // サーバアドレス指定(int -> char)

  // サーバに接続要求を出す
  cout << "Wating for connection ...\n";
  if (connect(dstSocket, (struct sockaddr *) &dstAddr, sizeof(dstAddr)) < 0){
      perror("ERROR connecting sever");
      exit(1);
  }
}

///* 10進数を16進数に変換 *///
string Dec_to_hex(unsigned int src){
    //cout << "src :" << src << endl;
    int i = 0;
    string hex_num;
    string henkan = "0123456789ABCDEF";

    while(src > 0) {
      // 入力した数値を16で割り、その余りと商をだす
      i = src % 16;
      src = src / 16;
      hex_num = henkan[i] + hex_num;
    }

    return hex_num;
}

int main(){
  struct sockaddr_in dstAddr;
  struct Status myStatus;
  char buf[BUFFER_SIZE]; // recvのbuffer
  string recv_zero_num; // 0の個数を文字列として格納
  int zero_num; // 0の個数を数値として格納
  string zero;

  /////* 初期登録処理 *//////////////////////////////////////////////////////
  /* IPとportの設定 */
  Set_IP_and_port(myStatus);

  /* サーバと接続 */
  Connection(dstAddr, myStatus);

  /* "connection success"を受信 */
  recv(dstSocket, buf, BUFFER_SIZE, 0);
  cout << buf << endl;

  /* 名前を入力, 送信する */
  cout << "Team name :";
  cin >> myStatus.teamName;
  send(dstSocket, myStatus.teamName, strlen(myStatus.teamName)+1, 0);

  /* 条件となる0の個数を受け取る */
  recv(dstSocket, buf, BUFFER_SIZE, 0);
  recv_zero_num = buf;
  zero_num = stoi(recv_zero_num);
  cout << "judge_zero_num :" << zero_num << endl;

  for(int i=0; i<zero_num; i++){
      zero = zero + "0";
  }
 
  /////* nonce値計算時の通信 *///////////////////////////////////////////////

  unsigned int nonce; 
  string block;
  string first_hash;
  string second_hash;
  string init_data;

  /* nonce値計算 */
  random_device rnd; // 非決定的な乱数生成
  mt19937 mt(rnd()); // メルセンヌツイスタ
  uniform_int_distribution<unsigned int> dist(0, UINT_MAX); // 生成乱数の範囲指定　[0 ~ 4294967295]

  for(int i=0;i<10;i++){
    recv(dstSocket, buf, BUFFER_SIZE, 0); // ブロックの受信
    block = buf;
    for(;;){
        nonce = dist(mt);
        init_data = Dec_to_hex(nonce); // 10進数から16進数に変換
        picosha2::hash256_hex_string(block + init_data, first_hash); // 第一ハッシュ値
        picosha2::hash256_hex_string(first_hash, second_hash); // 第二ハッシュ値

        if(second_hash.substr(0,zero_num).compare(zero) == 0){ // 一致でreturn 0
            break;
        }
    }

  /* nonce値を送信 */
  cout << "========== " << i+1 << " ==========" << endl;
  cout << "nonce(0x) :" << init_data << endl;
  cout << "block :" << block << endl;
  cout << "block + nonce :" << block + init_data << endl;
  cout << "first_hash :" << first_hash << endl;
  cout << "second_hash :" << second_hash << endl;
  cout << "send nonce :" << init_data << endl;
  send(dstSocket, init_data.c_str(), init_data.size()+1, 0);
  }

  /////* 終了時処理 *////////////////////////////////////////////////////////

  /* "FINISH"を受信 */
  recv(dstSocket, buf, BUFFER_SIZE, 0);
  cout << "finish? :" << buf << endl;

  /* socket切断 */
  close(dstSocket);

}
