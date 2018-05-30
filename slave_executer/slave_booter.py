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
reader = csv.DictReader(csv_file)

for row in reader:
	data.append(row)

while(True):
	print "input command"
	command = input()
	cmd_lst = command.split()

	if cmd_lst[0] in {"boot","kill"}:
		
		#message setting
		if cmd_lst[0] == "boot":
			mes = cmd_lst[2] if cmd_lst.length > 3 else "10"	
		elif:
			mes = "kill"

		#boot(kill) all slaves
		if cmd_lst[1] == all:
			for slave in data:
				send_mes(slave.value , mes)

			continue

		#boot single slave
		elif cmd_lst[1] in data.keys():
			addr = data[cmd_lst]
			send_mes(addr,mes)
			continue
			
	elif command == "exit":
		break;
	
	print "invalid command"

	