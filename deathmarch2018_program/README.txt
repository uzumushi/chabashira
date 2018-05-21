コンパイル方法（server）
    $ g++ server.cpp -o server --std=c++11

実行方法(server)
    $ ./server
    generate_block_num: //生成するブロックの個数
    judge_zero_num: //nonceの条件となるハッシュ値の先頭の0の個数

実行方法(client)
    $ ./client
      dst_ip: //serverのIPアドレス　ローカルならば127.0.0.1
      dst_port: //serverのポート　デフォルトなら１チーム目は10050
                                          2チーム目は10051
      team_name: // チーム名　ご自由に

※注意点
defaultの設定だとserverと通信できるclientは2つとなっている
同時に接続するチーム数を変えたいときはserver.cppのMAXPLAYERNUMの数を変更すること
(その際port等にも注意すること)
