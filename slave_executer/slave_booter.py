import socket
import csv

PORT = 55555
CSV_PATH = "slaves.csv"

data = []

def send_mes(addr,mes):
	client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	client.connect((addr,PORT))
	client.send(mes)

csv_file = open(CSV_PATH, "r")
#reader = csv.DictReader(csv_file)
reader = csv.reader(csv_file)

for row in reader:
	data.append(row)
	print row #for Debug

data_dict = dict(data) #list to dict
print data #for Debug
print data_dict #for Debug

while(True):
    	print "input command"
	command = raw_input() #if python3, you should use input()
	print command #for Debug
	cmd_lst = command.split()
	print cmd_lst #for Debug

	if cmd_lst[0] in ["boot","kill"]:
		#message setting
		if cmd_lst[0] == "boot":
    			print "I'm boot!" #for Debug
			mes = cmd_lst[2] if len(cmd_lst) == 3 else "10" #Is this right?
			print mes #for Debug
		else:
    			print "I'm kill!" #for Debug
			mes = "kill"
			print mes #for Debug

		#boot(kill) all slaves
		if cmd_lst[1] == "all":
    			print "I'm all!" #for Debug
			for slave in data_dict.values():
				send_mes(slave,mes)
				print slave #for Debug
				print mes #for Debug

			continue

		#boot single slave
		elif cmd_lst[1] in data_dict.keys():
			print "I'm single slave!" #for Debug
			addr = data_dict[cmd_lst[1]]
			send_mes(addr,mes)
			print addr #for Debug
			continue
			
	elif command == "exit":
		break;
	
	print "invalid command"

	
