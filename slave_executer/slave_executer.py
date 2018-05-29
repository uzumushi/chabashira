import socket
from subprocess import check_call

#change here
host = "127.0.0.1" #server_IP
port = 55555 #same with client program
slave_path = "slave" #execute slave program path

def slave_server():
	serversock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	serversock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
	serversock.bind((host,port)) 
	serversock.listen(1)

	print 'Waiting for connections...'
	clientsock, client_address = serversock.accept()

	rcvmsg = clientsock.recv(1024)
	print 'Received -> %s' % (rcvmsg)
	if rcvmsg == 'kill':
	    blocknum = -1
	else:
	    blocknum = int(rcvmsg)
	clientsock.close()


while(True):
	blocknum = slave_server()
	
	if(blocknum == -1):
		break
	check_call({slave_path,str(blocknum)})