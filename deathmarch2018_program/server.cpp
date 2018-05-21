// for socket
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>

#include <vector>
#include "./PicoSHA2/picosha2.h"
#include <iomanip>

#include <chrono>

const int BUFFER_SIZE = 256;
const int MAXTEAMNUM = 1;
const int PORTNUM = 10050;

using namespace std;

//---------------------------------------------
class Block{
public:
    int index_;
    int timestamp_;
    string data_;
    string previous_hash_;
    string hash_;
    string nonce_;

    Block(){index_ = 0; timestamp_ = 0; data_ = ""; previous_hash_ = "";nonce_ ="";} //メンバ変数の初期化

    Block(int index, int timestamp, string data, string previous_hash,string nonce){
        index_ = index;
        timestamp_ = timestamp;
        data_ = data;
        previous_hash_ = previous_hash;
        nonce_ = nonce;
        hash_ = GenerateHash();
    }

public:
    string GenerateHash(){
        //ID、タイムスタンプ、データ、一つ前のハッシュ値から新たなハッシュ値を生成する
        string header;
        string header_hash;
        string double_hash;

        header = to_string(index_) + to_string(timestamp_) + data_ + previous_hash_ + nonce_;
        picosha2::hash256_hex_string(header, header_hash);
        picosha2::hash256_hex_string(header_hash, double_hash);

        return  double_hash;
    }

public:
    void CheckBlockInfo(){
        //ブロックに保存されている内容を表示する
        cout << "index:         " << index_ << endl;
        cout << "timestamp:     " << timestamp_ << endl;
        cout << "data:          " << data_ << endl;
        cout << "previous hash: " << previous_hash_ << endl;
        cout << "nonce: " << nonce_ << endl;
        cout << "hash:          " << hash_ << endl;
        cout << endl;
    }
};

// ブロックチェーンの最初のブロックを生成
Block CreateGenesisBlock(){
    Block genesis_block(0, time(NULL), "Genesis Block", "0","1234");
    return genesis_block;
}

// 次のブロックを生成
Block CreateNextBlock(Block last_block, string nonce){
    int32_t this_index = last_block.index_ + 1;
    int64_t this_timestamp = time(NULL);
    string this_data;
    picosha2::hash256_hex_string(to_string(time(NULL)), this_data);
    string this_hash = last_block.hash_;
    string this_nonce =nonce;

    Block next_block(this_index, this_timestamp, this_data, this_hash,this_nonce);
    return next_block;
}

//-------------------------

// socket通信の初期化
int InitializeSocket(struct sockaddr_in &addr, int port){
    int addr_size;
    int sock;
    
    /*ソケットの作成(TCP/IP通信)*/
    sock = socket(AF_INET, SOCK_STREAM, 0);

    if(sock<0){
        perror("ERROR opening socket");
        exit(1);
    }
    /*ソケットの設定*/
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;//どれでも要求を受け付ける
    bind(sock,(struct sockaddr*)&addr, sizeof(addr));

    /*TCPクライアントからの接続要求を持てる状態にする*/
    listen(sock,1);
    /*TCPクライアントからの要求を受付*/
    addr_size = sizeof(addr);
    sock = accept(sock,(struct sockaddr*)&addr, (socklen_t*)&addr_size);
    if(sock<0){
        cout << "Connection Failed. " << "\n";
        exit(1);
    }else{
        cout << "Conneted Clients."<< "\n";
    }

    string connect_message("connection success");
    cout << "connect_message: " << connect_message << endl;
    sendto(sock, connect_message.c_str(), connect_message.size()+1,
           0,(struct sockaddr*)&addr,sizeof(addr));
    
    return sock;
}

//最も早かったプレイヤーを見つけてnonceを返す
string ChooseFastestNonce(string* client_nonce, double* elapsed_time){
    double fastest_time = elapsed_time[0];
    int fastest_team = 0;
    for (int i = 1; i < MAXTEAMNUM; i++) {
        if(elapsed_time[i] < fastest_time){
            fastest_time = elapsed_time[i];
            fastest_team = i;
        }
    }
    return client_nonce[fastest_team];
}


int main(){
    int sock[MAXTEAMNUM];
    char buf[BUFFER_SIZE];
    int read_size;
    string  snd_str;

    struct sockaddr_in addr[MAXTEAMNUM];
    
    vector<Block> blockchain; //ブロックを保持していくための動的配列
    Block previous_block;
    Block current_block;
    const string dummy_nonce = "00000000";
    
    string client_nonce[MAXTEAMNUM];
    int block_num;// 生成するブロックの個数
    string judge_zero_num;// ハッシュ値の条件となる0の個数
    
    double elapsed_time[MAXTEAMNUM];
    vector<string> team_name;
    
    cout << "generate_block_num: ";
    cin >> block_num;
    cout << "judge_zero_num: ";
    cin >> judge_zero_num;
    
    // 各クライアントのソケットを初期化
    for (int i = 0; i < MAXTEAMNUM; i++) {
        sock[i] = InitializeSocket(addr[i], PORTNUM+i);
        read_size = read(sock[i], buf, sizeof(buf));
        team_name.push_back(buf);
        cout << "team " << i << ": " << team_name[i] << endl;
        sendto(sock[i], judge_zero_num.c_str(), judge_zero_num.size()+1, 0,
               (struct sockaddr*)&addr[i],sizeof(addr[i]));
    }

    blockchain.push_back(CreateGenesisBlock()); // 最初のブロックを用意、追加
    cout << "Create genesis Block" << endl;
    previous_block = blockchain[0];
    previous_block.CheckBlockInfo();
    cout << endl;
    
//---blockchain
    for (int i = 0; i < block_num; i++) {

        current_block = CreateNextBlock(previous_block, dummy_nonce);
        for (int team_num = 0; team_num < MAXTEAMNUM; team_num++) {

            // clientに送るデータの生成
            memset(buf, 0, sizeof(buf));
            snd_str = to_string(current_block.index_) +
                to_string(current_block.timestamp_) +
                current_block.data_ +
                current_block.previous_hash_;
            //----計測開始
            auto start = chrono::system_clock::now();

            sendto(sock[team_num],snd_str.c_str(),snd_str.size()+1,0,
                   (struct sockaddr*)&addr[team_num],sizeof(addr[team_num]));
            memset(buf, 0, sizeof(buf));
            read_size = read(sock[team_num], buf, sizeof(buf));
            
            //---- 計測終了
            auto end = chrono::system_clock::now();
            
            elapsed_time[team_num] = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
            client_nonce[team_num]=buf;
            cout<< team_name[team_num] <<": nonce: "<<client_nonce[team_num]
                <<" elapsed_time: " << elapsed_time[team_num] << "msec"
                << endl;
        }
        // ブロックチェーンの生成に用いるnonceの決定
        string current_nonce = ChooseFastestNonce(client_nonce, elapsed_time);
        
        //nonce を用いてブロックチェーンを更新
        current_block.nonce_=current_nonce;
        current_block.hash_=current_block.GenerateHash();
        current_block.CheckBlockInfo();

        previous_block = current_block;

        // ブロックチェーンに追加
        blockchain.push_back(current_block);
    }

    // 競技終了のメッセージを各クライアントに送信
    for (int team_num = 0; team_num < MAXTEAMNUM; team_num++) {
        sendto(sock[team_num],"FINISH",7,0,
               (struct sockaddr*)&addr[team_num],sizeof(addr[team_num]));
    }
    
    return 0;
}
