#Author: Alex Grasley
#Class: CS_271 Intro to Networks
#References and ideas: hhttps://www.geeksforgeeks.org/simple-chat-room-using-python/

import socket
import select
import sys

#function from users: JadedTuna and trilobyte on https://stackoverflow.com/questions/17667903/python-socket-receive-large-amount-of-data
def recvall(sock):
    buffer = 4096
    data = b''
    while True:
        part = sock.recv(buffer)
        data += part
        if(data == "2\0"):
            print "File not found"
            serverControl.close()
            serverData.close()
            sys.exit()
        else:
            if( len(part) ) < buffer:
                break
    return data

serverControl = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
serverData = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
if len(sys.argv) < 4:
    print  "Correct usage: script name, IP address, port number, command, <optional: filename>,<optional: data port>"
    exit()

IP_Address = str(sys.argv[1])
Port = int(sys.argv[2])
Command = str(sys.argv[3])

#if user specifies a file name with no data port, throw error and exit.
if len(sys.argv) == 5:
    print "Invalid number of arguments. Please ensure you have specified the desired filename and local port for receipt of data"
    exit()

#create message struture to sent command and establish data port
if len(sys.argv) == 4:
    message = Command
if len(sys.argv) == 6:
    FileName = sys.argv[4]
    DataPort = sys.argv[5]
    dataAddress = ("127.0.0.1",int(DataPort))
    message = Command + ";" + FileName + ";" + DataPort
    serverData.bind(dataAddress)
    serverData.listen(1)
    print message

#send message to server on control connection
serverControl.connect((IP_Address,Port))
#send message to server
serverControl.send(message)
recMessage = serverControl.recv(4096)
if len(recMessage) > 0:
    if(recMessage == "1\0"):
        print "Invalid command, please use \"-l\" to list files, or \"-g\" to get files"
        serverControl.close()
        sys.exit()
    else:
        print recMessage

#if specifying a file name and port, set options in variables
if len(sys.argv) == 6:
    #create listening port to receive data
    print "waiting on server for file..."
    conn, addr = serverData.accept()
    recFile = recvall(conn)

    if len(recFile) >0:
        print FileName
        newFile = open(FileName,"a+")
        newFile.write(recFile)
        conn.close()
    else:
        print "Nothing Received"

serverControl.close()

