# FT-TCP-Server-Client
2-connection client/server TCP network application

These programs emulates a simple file transfer system, a file transfer server and a file transfer client. 
The commands the the client can send the server are -g for sending a file or -l for view the directory of the server. 


To compile and run server:
1. Run command "make" with out quotations or gcc -o ftserver ftserver.c
2. To run: ./ftserver <PORTNUM>


to run client
To get a file: python3 ftclient.py <SERVERNAME> <PORTNUM> <COMMAND> <FILENAME> <DATAPORT>
or
To get the directory list: python3 ftclient.py <SERVERNAME> <PORTNUM> <COMMAND> <DATAPORT>
