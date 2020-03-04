/***************************************************************
Name: Lauren Boone
Date: 3/1/20
Description: This is a server the receives request from a client
to either send a file or send the directory list. To run the server

******************************************************************/
#define _GNU_SOURCE
#include <ctype.h>
#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>




void error(const char *msg) { perror(msg); exit(1); } // Error function used for reporting issues


/***************************************************
startConnection()
Description: This function takes the IP(possiblyNULL)
and port as parameters. It calls the getaddrinfo
to identify a host a service. A addrinfo struct
is returned.
//https://beej.us/guide/bgnet/html/#socket
******************************************************/
struct addrinfo* startConnection(char *port, char IP[]) {
	//This is taken mostly from beej's guide 
	//printf("getting address info");
	struct addrinfo *serveinfo;
	struct addrinfo hints;
	int socketfd;//used to see return value
	memset(&hints, 0, sizeof hints);//clear memory
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM; //use TCP
	if (strcmp(IP, "none")==0) {
		//printf("Ip is null\n");
		hints.ai_flags = AI_PASSIVE; //use my IP (from beej's guide)
		socketfd = getaddrinfo(NULL, port, &hints, &serveinfo);//get the address info with NULL IP

	}
	else {
		//printf("IP: %s\n", IP);
		socketfd = getaddrinfo(IP, port, &hints, &serveinfo);//get the address info with IP
	}
	
	//sourcehttps://beej.us/guide/bgnet/html/#getaddrinfoprepare-to-launch

		
	if (socketfd != 0) {//exit if error
		error("Error connecting to port/ip");
	}

	return serveinfo;//return the struct

}


/**********************************************************
getSocket
This function tries to create a socket useing the serve name
The socket file descripter is returned
************************************************************/
int getSocket(struct addrinfo *sockName) {
	int ret;
	//create the socket and assign the file descriptor value to ret
	// /*https://beej.us/guide/bgnet/html/#socket */
	ret = socket(sockName->ai_family, sockName->ai_socktype, sockName->ai_protocol);
	if ( ret == -1) {
		error("Error in creating socket");
	}
	
	//printf("return: %d\n", ret);
	return ret;
}





/*******************************************************************
list_of_files()
This function is called to create a list of all the file in the
current directory
****************************************************************/
int list_of_files(char* fileName) {
	DIR *dirtocheck; //starting dir(current)
	struct dirent *fileInDir; //current subdir 
	
	int i = 0;

	dirtocheck = opendir(".");//open the current directory
	if (dirtocheck > 0) { //make sure its open
		while ((fileInDir = readdir(dirtocheck))) {//read list of files/directorys
			if (strstr(fileInDir->d_name, fileName) != NULL) {
				if (strcmp(fileInDir->d_name, fileName) == 0) {//If this is the file name then return 1 for ture
					closedir(dirtocheck);
					return 1;
				}
			}

		}
	}
	closedir(dirtocheck);
	return 0;


}


/******************************************************************
sendFile()
This function sends the file to the client
*****************************************************************/
void sendFile(char* file, char* IP, char* port) {
	sleep(2); //Needed to allow python client to set up
	struct addrinfo *res = startConnection(port, IP);//The IP is the data port we are sending the file
	int dataFD = getSocket(res);//Get the socket
	if (connect(dataFD, res->ai_addr, res->ai_addrlen) == -1) {
		error("Issue connecting");
	}
	char output[1000];//Size of output buffer
	memset(output, 0, sizeof(output));
	int fd = open(file, O_RDONLY);//Open the file
	int byte = -1;
	
	while (1) {
		byte = read(fd, output, sizeof(output) - 1); //read from the file put in output
		if (byte < 0) {//error catching
			error("Error reading from file");
		}
		if (byte == 0) {//end of find
			break;
		}
		void *index = output;//keep track of location in file
		while (byte > 0) {//begin sending output
			int bsent = send(dataFD, index, sizeof(output), 0);
			if (bsent < 0) {//error catching
				error("error writing to socket");
			}
			byte = byte - bsent;//subtract byte sent until byte = 0
			index += bsent;//increment index
		}
		memset(output, 0, sizeof(output));

	}
	
	char finished[100];//send end of file message
	memset(finished, 0, 100);
	strcpy(finished, "eof");
	send(dataFD, finished, 100, 0);
	close(dataFD);//close file, socket and free socket 
	freeaddrinfo(res);
	printf("File Sent\n");

}

/********************************************************************
sendDirectory()
********************************************************************/
void sendDirectory(char* port, char* IP) {
	sleep(2);
	printf("Sending directory:\n");
	char fileName[100];//holds name of file to be sent
	struct addrinfo * res = startConnection(port, IP);
	//printf("Getting socket: prot: %s\n", port);
	int dataFD = getSocket(res);
	//printf("fd: %d", dataFD);
	if (connect(dataFD, res->ai_addr, res->ai_addrlen) == -1) {
		error("Issue connecting");
	}
	DIR *dirtocheck; //starting dir(current)
	struct dirent *fileInDir; //current subdir 
	struct stat dirStats; //hold info about dir
	int i = 0;

	dirtocheck = opendir(".");//open the current directory
	if (dirtocheck > 0) { //make sure its open
		while ((fileInDir = readdir(dirtocheck))) {//read list of files/directorys
			memset(fileName, 0, sizeof(fileName));
			strcpy(fileName, fileInDir->d_name);//copy the name to the variable to be sent
			send(dataFD, fileName, 100, 0);
			printf("%s\n", fileInDir->d_name);
		}
	}
	printf("\n");
	//char finished[100];
	//memset(finished, 0, 100);
	//strcpy(finished, "eof");
	//11send(dataFD, finished, 100, 0);
	closedir(dirtocheck);
	close(dataFD);
	freeaddrinfo(res);
}



/*******************************************************************
handleCommands()
This function handles the commands send from the client. 
The commands are -g or -l.
*********************************************************************/
void handleCommands(int fd, char* IP) {
	char command[100];//holds command value
	char port[100]; //hold value to establish port connection with client
	char validCommand[100]; //sends good to client
	memset(validCommand, 0, 100);
	strcpy(validCommand, "good");
	memset(port, 0, sizeof(port));
	int bytesrec = -1;
	
	bytesrec=recv(fd, port, 100, 0);//receive port
	send(fd, port, 100, 0);//send values to clear buffer, prevents client from sending all info at once
	//printf("Recieved port %s: \n", port);
	memset(command, 0, sizeof(command));
	bytesrec=recv(fd, command, 100, 0);
	//printf("command: %s\n", command);
	send(fd, command, 100, 0);
	//if (command == "Bad") {
		//error("Bad Command");
	//}
	char dataPort[50];
	memset(dataPort, 0, 50);
	recv(fd, dataPort, 50, 0);//receive dataprot
	//send(fd, validCommand, 100, 0);
	//printf("Recieved dataprot %s: \n",dataPort);
	if (strcmp(command, "-g") == 0) {//if command is -g then see if file exist to send
		char fileName[100];//assuming file name not larger than 100
		memset(fileName, 0, sizeof(fileName));
		send(fd, validCommand, 100, 0);//send good to client
		recv(fd, fileName, sizeof(fileName) - 1, 0);//receive file name
		printf("Filename: %s\n...Checking for File", fileName);
		if (list_of_files(fileName)) {//Check for files if (1) file found
			char fileFound[100];
			memset(fileFound, 0, 100);
			strcpy(fileFound, "File Found");
			send(fd, fileFound, 100, 0);//send notice that file was foudn
			printf("File Found...Sending File\n");
			char dupFileName[100];//send file
			memset(dupFileName, 0, 100);
			strcpy(dupFileName, "./");
			sprintf(dupFileName, "%s%s", dupFileName , fileName);
			sendFile(dupFileName,IP, dataPort); //Function to send the file written above

		}
		else {//otherwise the file was not found
			printf("File not Found");
			char fileError[100];
			memset(fileError, 0, 100);
			strcpy(fileError, "Bad File");
			send(fd, fileError, strlen(fileError), 0);//send error message to client
		}
	}
	else if (strcmp(command, "-l") == 0) {//Otherwise send the directory
		//printf("IP: %s, dataport: %s, port: %s\n", IP, dataPort, port);
		send(fd, validCommand, 100, 0);//Send "good" to client
		sendDirectory(dataPort, IP);//Funciton to send directory list
	}
	else {//other wise the command was invalid
		char invalid[100];
		memset(invalid, 0, 100);
		strcpy(invalid, "Invalid Request");
		//printf("Sending invalid command: %s\n", invalid);
		send(fd, invalid, strlen(invalid), 0);//send "invalid request" to client
		printf("Invalid command\n");
	}
	

}

/***************************************************************
listen_for_client
THis function waits and listens for a client to connect. When a client
tries to connect we handle the commands
https://beej.us/guide/bgnet/html/#getaddrinfoprepare-to-launch
(taken from accept() section)
***********************************************************************/
void listen_for_client(int socketfd) {
	if (listen(socketfd, 5) == -1) {//make sure we can listen
		close(socketfd);
		error("Could not listen");
	}
	struct sockaddr_storage their_addr;
	socklen_t addr_size;
	int new_fd;
	while (1) {
		printf("Listening for clients\n");
		addr_size = sizeof their_addr;
		//This is taken from beejs guide. It gets the IP address of the client and converts 
		//it to a string name IP
		new_fd = accept(socketfd, (struct sockaddr *)&their_addr, &addr_size);
		//This is taken from https://stackoverflow.com/questions/3060950/how-to-get-ip-address-from-sock-structure-in-c
		//This converts the sock structur to an ip address
		//Need to create second conneciton
		struct sockaddr_in* pV4Addr = (struct sockaddr_in*)&their_addr;
		struct in_addr ipAddr = pV4Addr->sin_addr;
		char IP[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &ipAddr, IP, INET_ADDRSTRLEN);
		handleCommands(new_fd,IP);
	}
}

/******************************************************************
bindSocket()
This function binds the socket with a port. 
Taken from https://beej.us/guide/bgnet/html/#getaddrinfoprepare-to-launch
bind() section
returns 1 if able to bind else exit
******************************************************************/
int bind_socket(int socketfd, struct addrinfo * server) {
	int ret; //return value
	if ((ret == (bind(socketfd, server->ai_addr, server->ai_addrlen))) == -1) {
		close(socketfd);
		error("Error on binding");
	}
	return 1;
}



int main(int argc, char *argv[]) {
	

	int portNumber;//defined in argv[2]

	struct addrinfo *server;//struct for tcp server
	int socketfd = 0;//socket file descriptor
	if (argc != 2) {//check for valid number of arguments
		error("Invalid number of arguments");
	}

	
	
	//portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
	printf("Starting connection\n");
	char IP[100];
	memset(IP, 0, 100);
	strcpy(IP, "none");
	server = startConnection( argv[1],IP);//starts a connection uses the port number and server name
	socketfd = getSocket(server); //create a socket and assign it to the file descriptor
	//printf("%i", socketfd);
	int status;
	
	//sourcehttps: beej.us/guide/bgnet/html/#connect
	bind_socket(socketfd, server);//if we can bind to the socket begin listening
	listen_for_client(socketfd);
		
	
	
	freeaddrinfo(server);//free the memory for the struct

	return 0;
}