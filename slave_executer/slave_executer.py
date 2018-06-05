import socket
from subprocess import check_call
import csv
import os

#change here
host = "160.12.172.3" #server_IP(now deep)
port = 55555 #same with client program
slave_path = "/home/fss3/tomoya/test_slave" #execute slave program path
CSV_PATH = "capacity.csv"

data = []

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
	return blocknum # added by tomoya

#read csv
csv_file = open(CSV_PATH, "r")
#reader = csv.DictReader(csv_file)
reader = csv.reader(csv_file)

for row in reader:
	data.append(row)
	print row #for Debug

data_dict = dict(data) #list to dict
print data #for Debug
print data_dict #for Debug

host = '%s' % os.uname()[1]	
print host #for Debug
host_split = host.split(".")
print host_split[0] #for Debug

if(host_split[0] in data_dict.keys()):
	capacity = data_dict[host_split[0]]
	print capacity #for Debug
else:
	capacity = "20000"
	print capacity #for Debug

while(True):
	blocknum = slave_server()
	print blocknum #for Debug

 	if(blocknum == -1):
		print "I'm killed!" #for Debug
		break
	print '========== SLAVE PROCESS START ==========' #for Debug
 	check_call([slave_path,capacity,str(blocknum)]) #give capacity and blocknum to slaves
	print '========== SLAVE PROCESS FINISH ==========' #for Debug
