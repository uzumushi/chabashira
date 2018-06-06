#include <iostream>
#include <iomanip>
#include <string.h>
#include <sstream>
#include <limits.h>

#include "picosha2.h"

using namespace std;

void getDoubleHash(string& header,string& double_hash){
    string header_hash;

    picosha2::hash256_hex_string(header, header_hash);
    picosha2::hash256_hex_string(header_hash, double_hash);
}

int main(void){

    string nonce;//for nonce
    string double_hash;//for hash


    cout << "nanikashira\n" <<endl;



        //set header value
        string header = "51528210305818912a0c5065e04921ae30a162641517c58dce4d4b4931e8853c5246820fa0d0000000896a97a80e4b869a93706ac86cc1cf8718f59fb5e4ffab78fc79c247e";
 
	header += "00062f5c";
       
        //get hash
        getDoubleHash(header, double_hash);


    cout << "header :" << header << endl;
    cout << "hash  : " << double_hash <<endl;

	
	return 0;
}
