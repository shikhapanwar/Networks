all: server client

server: udpserver.c
	gcc -Wall udpserver.c -o server  -lm -lcrypto -lssl

client: udpclient.c
	gcc -Wall udpclient.c -o client -lm -lcrypto -lssl

clean:
	rm server client
