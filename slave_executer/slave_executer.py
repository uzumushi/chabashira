import socket
from subprocess import check_call
import csv
import os

#change here
host = "127.0.0.1" #server_IP
port = 55555 #same with client program
slave_path = "slave" #execute slave program path
CSV_PATH = "capacity.csv"

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

#read csv
csv_file = open(CSV_PATH, "r")
reader = csv.DictReader(csv_file)

for row in reader:
	data.append(row)

host = '[%s]' % os.uname()[1]	

if(host in data.keys()):
	capacity = data[host]
else
	capacity = "20000"

while(True):
	blocknum = slave_server()
	
	if(blocknum == -1):
		break
	check_call({slave_path,capacity,str(blocknum)})