// cd /home/hork/cuda-workspace/CudaSHA256/Debug/files
// time ~/Dropbox/FIIT/APS/Projekt/CpuSHA256/a.out -f ../file-list
// time ../CudaSHA256 -f ../file-list


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <cuda.h>
#include "sha256_revision.cuh"
#include <dirent.h>
#include <ctype.h>

#define THREAD_TOTAL 1052

dim3 grid_num(6);
dim3 block_num(192);
  

__device__ bool checkContinuousZeros(const char* str,int n_zeros){

    for (int i = 0; i < n_zeros; ++i){
        if(str[i] != '0')
            return false;
    }

    return true;
}

__device__ void addNonceToBlock(BLOCK_DATA* myblock,unsigned nonce){
	
	for(int i=7;i >= 0 ;i--){
		unsigned tmp = nonce >> (4*i);
		tmp &= 0x0f;
		myblock->data[myblock->size-8+(7-i)] = (10 <= tmp)*('a' + (tmp - 10)) + (10 > tmp)*('0' + tmp);

		
	}

}

__global__ void sha256_cuda(BLOCK_DATA * block,unsigned nonce_start,unsigned n_zero,bool * f_finish ,unsigned *d_answers) {
	int i = blockIdx.x * blockDim.x + threadIdx.x;

	BYTE previous_hash[64];
	BYTE double_hash[64];
	BYTE digest[64];
	bool get_ans =false;
	unsigned  nonce= nonce_start + i;
	BLOCK_DATA myblock;

	memcpy(myblock.data,block->data,block->size);
	myblock.size = block->size + 8;	

	do{
		SHA256_CTX ctx;
		
		addNonceToBlock(&myblock,nonce);		

		sha256_init(&ctx);
		sha256_update(&ctx, myblock.data, myblock.size);
		sha256_final(&ctx, digest);
		
		hashStrCpy(previous_hash,digest);		

		sha256_init(&ctx);
		sha256_update(&ctx, previous_hash , 64);
		sha256_final(&ctx, digest);

		hashStrCpy(double_hash,digest);

		get_ans = checkContinuousZeros((char*)double_hash,n_zero);

	}while( get_ans == false && *f_finish == false && (nonce = nonce +THREAD_TOTAL));

	
	if(*f_finish ==false){
		*f_finish = true;
		d_answers[i] = nonce;
	}
}


void pre_sha256() {
	// compy symbols
	checkCudaErrors(cudaMemcpyToSymbol(dev_k, host_k, sizeof(host_k), 0, cudaMemcpyHostToDevice));
}



// * JOB_init(BYTE * data, long size) {
//	JOB * j;
//	checkCudaErrors(cudaMallocManaged(&j, sizeof(JOB)));	//j = (JOB *)malloc(sizeof(JOB));
//	checkCudaErrors(cudaMallocManaged(&(j->data), size));
//	j->data = data;
//	j->size = size;
//	for (int i = 0; i < 64; i++)
//	{
//		j->dige1152 (SM:6)st[i] = 0xff;
//	}
//	return j;
//}



int main() {
	
	int  i , n;
	BLOCK_DATA block;
	int nonce_start = 0;
	int n_zero = 3;
	unsigned answers[THREAD_TOTAL];	

	unsigned *d_answers;
	BLOCK_DATA *d_block;
	
	
	char data[] = "51528210305818912a0c5065e04921ae30a162641517c58dce4d4b4931e8853c5246820fa0d0000000896a97a80e4b869a93706ac86cc1cf8718f59fb5e4ffab78fc79c247e";

	strcpy((char*)(block.data),data);
	block.size = strlen(data);

	cudaMalloc((void**)&d_block,sizeof(BLOCK_DATA) );
	cudaMemcpy(d_block,&block,sizeof(BLOCK_DATA),cudaMemcpyHostToDevice);
	
	cudaMalloc((void**)&d_answers,sizeof(unsigned)*THREAD_TOTAL);
	cudaMemset(d_answers,0,sizeof(unsigned)*THREAD_TOTAL); 	


	bool *f_finish;
	cudaMalloc((void**)&f_finish,sizeof(bool));
	cudaMemset(f_finish,false,1);
	sha256_cuda <<< grid_num,block_num  >>> (d_block,nonce_start,n_zero,f_finish,d_answers);

	cudaDeviceSynchronize();

	cudaMemcpy(answers,d_answers,sizeof(unsigned)*THREAD_TOTAL,cudaMemcpyDeviceToHost);
		
	for(int i=0;i<THREAD_TOTAL;i++){
		printf("%x\n",answers[i]);
	}
	cudaDeviceReset();
	return 0;
}
