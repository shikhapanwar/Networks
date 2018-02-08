all: server client

server: udpserver.c
	gcc -Wall udpserver.c -o server  -lm

client: udpclient.c
	gcc -Wall udpclient.c -o client -lm

clean:
	rm server client