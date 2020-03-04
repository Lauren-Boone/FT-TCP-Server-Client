

ftserver: ftserver.c
	gcc -o ftserver -g ftserver.c

make: ftserver

clean:
	rm ftserver
