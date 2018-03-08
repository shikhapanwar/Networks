all: server client
server: tcpserver.c
	gcc -Wall tcpserver.c -o server -lcrypto -lssl

client: tcpclient.c
	gcc -Wall tcpclient.c -o client -lcrypto -lssl

clean:
	rm server client