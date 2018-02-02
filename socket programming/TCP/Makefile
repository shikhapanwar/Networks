all: server client
server: TCP_Server/tcpserver.c
	gcc -Wall TCP_Server/tcpserver.c -o server -lcrypto -lssl

client: TCP_Client/tcpclient.c
	gcc -Wall TCP_Client/tcpclient.c -o client -lcrypto -lssl

clean:
	rm server client