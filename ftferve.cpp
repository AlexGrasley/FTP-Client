#include <iostream>
#include <dirent.h>
#include <vector>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <fstream>

/*
some code and ideas from Sloan Kelly https://gist.github.com/codehoose/020c6213f481aee76ea9b096acaddfaf
*/
using namespace std;

//function from user "jtshaw" on https://www.linuxquestions.org/questions/programming-9/c-list-files-in-directory-379323/. Slight modifications made
int getdir (string dir, vector<string> &files)
{
    DIR *dp;
    struct dirent *dirp;
    if((dp  = opendir(dir.c_str())) == NULL) {
        cout << "Error(" << errno << ") opening " << dir << endl;
        return errno;
    }

	string fileName;
    while ((dirp = readdir(dp)) != NULL) {
		if (string(dirp->d_name) != "." || string(dirp->d_name) != ".."){
		fileName = string(dirp->d_name) + "\n";
        files.push_back(fileName);
		}
    }
    closedir(dp);
    return 0;
}

//function from http://www.codecodex.com/wiki/Read_a_file_into_a_byte_array
char* readFileBytes(const char *name)  
{  
    ifstream fl(name);  
    fl.seekg( 0, ios::end );  
    size_t len = fl.tellg();  
    char *ret = new char[len];
    fl.seekg(0, ios::beg);   
    fl.read(ret, len);  
    fl.close();  
    return ret;  
} 

int main() {

	//create socket
	int listening = socket(AF_INET, SOCK_STREAM, 0);
	if (listening == -1) {
		cerr << "Can't create socket";
		return -1;
	}

	//get port from user 
	int port = 0;
	cout << "Enter Port: ";
	cin >> port;


	//bind socket to IP and port
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(port);
	inet_pton(AF_INET, "0.0.0.0", &hint.sin_addr);

	if (bind(listening, (sockaddr*)&hint, sizeof(hint)) == -1) {
		cerr << "Can't bind to IP/Port";
			return -2;
	}
	//Mark socket for listening
	if (listen(listening, SOMAXCONN) == -1) {
		cerr << "Can't listen";
		return -3;
	}
	

	//accept call
	sockaddr_in client;
	socklen_t clientSize = sizeof(client);
	char host[NI_MAXHOST];
	char svc[NI_MAXSERV];

while(true){
	cout << "Waiting on client...." << endl;
	int clientSocket = accept(listening, (sockaddr*)&client, &clientSize);

	if (clientSocket == -1) {
		cerr << "Problem with client connecting";
		return -4;
	}
	//close listening socket
	
	memset(host, 0, NI_MAXHOST);
	memset(svc, 0, NI_MAXSERV);

	int result = getnameinfo((sockaddr*)&client, sizeof(client), host, NI_MAXHOST, svc, NI_MAXSERV, 0);
	if (result) {
		cout << host << " connected on " << svc << endl;
	}
	else {
		inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
		cout << host << " connected on " << ntohs(client.sin_port) << endl;
	}
	//while receiving - display message and echo
	char buf[4096];
	
	//clear buffer
	memset(buf, 0, 4096);
	//wait for message
	int bytesRecv = recv(clientSocket, buf, 4096, 0);
	if (bytesRecv == -1) {
		cerr << "There was a connection issue" << endl;
	}

	if (bytesRecv == 0) {
		cout << "Client disconnected" << endl;
	}

	//parse message
	string message = string(buf, 0, bytesRecv);
	cout << message << endl;
	string command = message.substr(0,message.find(";")); //get the first bit of the message
		message.erase(0,message.find(';') + 1); // remove it from the message
	const char* fileName = message.substr(0,message.find(';')).c_str();
		message.erase(0,message.find(';') + 1);
	int clientDataPort = atoi(message.c_str());

	

//command to list files, get files from home directory and send back to client
	if(command == "-l"){
		string dir = ".";
		vector<string> files = vector<string>();
		getdir(dir,files);
		string directory;

		for(unsigned int i = 0; i< files.size(); ++i) {
			directory += files[i];
		}
		strcpy(buf,directory.c_str());
		send(clientSocket, buf, directory.length() + 1, 0);
		continue;
	}
	else if(command == "-g"){ // received command to get file, create data connection and send desired file.
		//create new hint structure for the data port connection back to the client
		string fileMessage = "locating and sending files";
		strcpy(buf,fileMessage.c_str());
		send(clientSocket, buf, (unsigned)strlen(buf), 0);
		int sock = socket(AF_INET, SOCK_STREAM, 0);
		sockaddr_in dataHint;
		dataHint.sin_family = AF_INET;
		dataHint.sin_port = htons(clientDataPort);
		inet_pton(AF_INET, host, &dataHint.sin_addr);
	//connect to dataport
		int connectResult = connect(sock, (sockaddr*)& dataHint, sizeof(dataHint));
		if (connectResult == -1) {
			cout << "Failure Connecting to data port..." << endl;
			return 1; // error, bail out
		}
		else {
			cout << "data port connected!" << endl;
		}
		//valid get command received, data port connected, get file name and send it. 
		char* file;
		try
		{
			file = readFileBytes(fileName);
		}
		catch(bad_alloc & ba)
		{
			memset(buf, 0, 4096);
			buf[0] = '2';
			send(sock,buf,2,0);
			close(sock);
			continue;
		}
			//check length of file, if larger than buffer, create larger buffer.
			if((unsigned)strlen(file) > 4096){
				char newBuffer[(unsigned)strlen(file) + 1];
				strcpy(newBuffer,file);
				send(sock,newBuffer,(unsigned)strlen(newBuffer),0); 
			}
			else{
			strcpy(buf,file);
			send(sock,buf,(unsigned)strlen(file),0);
			close(sock);
			continue;
			}
			
		}
	else{
		string errorMessage = "1";
		strcpy(buf,errorMessage.c_str());
		send(clientSocket, buf, errorMessage.length() + 1, 0);
		continue;
	}
	close(clientSocket);


}
	close(listening);

	return 0;
}