
#!/bin/python

#Name: LAuren Boone
#Date: 3/1/20
#Description: This is a client application used for a ftp
#A lot of the methods are taken from https://docs.python.org/2/howto/sockets.html
#https://docs.python.org/2/library/socketserver.html#socketserver-tcpserver-example
#https://docs.python.org/3/library/socket.html#creating-sockets





from socket import *
import sys
import string



#https://docs.python.org/3/howto/sockets.html
def getSocket(numArgs):
    #print("getting socket")
    port = int(sys.argv[numArgs])#num args is different for -l and -g
    new_socket=socket(AF_INET, SOCK_STREAM)
    new_socket.bind(('',port))
    #print('printing port', port)
    new_socket.listen(1)
   # print('Listening on port: ', port)
    dataSocket, address= new_socket.accept()
    return dataSocket



def getDirectory():#-l command
    #Set up the socket to receive
    ftp=getSocket(4)
    print("Getting Directory:....")
    fileToPrint=[]
   
    fileToPrint=ftp.recv(100).decode('utf-8')# Each file sends 100 bytes from server
   # print(fileToPrint)
    while len(fileToPrint)>0:
            
        #print(fileToPrint)
        print(fileToPrint)
        #fileToPrint.decode('utf-8')
        fileToPrint=ftp.recv(100).decode('utf-8')# Each file sends 100 bytes from server
        
    ftp.close()
    print("\n")
   


def getFile(name):
    #https://docs.python.org/3.4/howto/unicode.html
    ftp=getSocket(5)
    # we want to open a file to put the data in. 
    print("Receiving File")
    ofile=open(name,"w")
    buffer=ftp.recv(1000) #Size the server is sending
    while len(buffer)>0 : #eof signals end of file
        ofile.write(buffer.decode('utf-8',"ignore"))
        buffer=ftp.recv(1000)
        #print(buffer)
       
        buffer.decode('utf-8', "ignore")
    ofile.close
    ftp.close()
    print("File Recieved")





def handCommand(ftp, server, port, command):
     errorCheck=[]
     if len(sys.argv) == 5:#get the file name if -g
        fileName=sys.argv[4]
     if len(sys.argv)==6:#otherwise get the dataport
        dataPort=sys.argv[5]
     #print(command)
     ftp.send(sys.argv[2].encode('utf-8')) #send the port to the server
     #print(sys.argv[2])
     ftp.recv(1024)#clear errors(server was receiving all arguments at once)
     ftp.send(command.encode('utf-8'))#send the command
     ftp.recv(1024)#Prevent sending faster than server can receive
   
     #errorCheck=ftp.recv(1024).decode('utf-8',"ignore")
     #print(errorCheck)
     #if errorCheck == 'Bad':
      #   print("Invalid Command")
       #  exit(1)
     
     if len(sys.argv)==5:#otherwise send the dataport (receiving list -l)
        #print('Sending dataport')
        ftp.send(sys.argv[4].encode('utf-8'))
        errorCheck=(ftp.recv(100).decode('latin-1'))
        #print('error check', errorCheck)
        if errorCheck == 'Invalid Request':
            print('Invalid Command/Request Try again with -g or -l')
            exit(1)
        getDirectory()

     elif len(sys.argv)==6:
         #print('Sending dataprot and file name')
         ftp.send((sys.argv[5]).encode('utf-8'))#first send dataport
         #ftp.recv(1024)#clear errors(server was receiving all arguments at once)
         ftp.send(sys.argv[4].encode('utf-8'))#then send file
         errorCheck=(ftp.recv(100).decode('latin-1'))
         print(errorCheck)
         if 'Invalid Request' in errorCheck:
             print('Invalid Command/Request Try again with -g or -l')
             exit(1)
         elif errorCheck == 'Bad File':
             print('File Not Found')
             exit(1)
         getFile(sys.argv[4])

     
        






#MAIN
if __name__ == "__main__": 
    #make sure program arguments are correct
    if len(sys.argv) < 5 or len(sys.argv)>6:
        print('Incorrect number of arguments')
        exit(1)
   # if (sys.argv[3])!= '-l' or sys.argv[3]!='-g':
    #    print('Incorrect command')
     #   exit(1)
    serverName=sys.argv[1]
    #Get the port number
    port=int(sys.argv[2])
    command=sys.argv[3]
   

    #taken from https://docs.python.org/3/library/socket.html#creating-sockets
    ftpsocket= socket(AF_INET, SOCK_STREAM) #set up the socket with
    ftpsocket.connect((serverName, port))#connect to the socket#taken from https://docs.python.org/3/library/socket.html#creating-sockets
    #Two parenthesis becuase must be tuple https://stackoverflow.com/questions/19143091/typeerror-connect-takes-exactly-one-argument/19143174

  
    handCommand(ftpsocket, serverName, port, command)
   
  
    ftpsocket.close()