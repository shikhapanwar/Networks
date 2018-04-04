/* 
 * udpclient.c - A simple UDP client
 * usage: udpclient <host> <port>
 */
#include <pthread.h>
#include <time.h>
#include <csignal.h> 
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <bits/stdc++.h>
using namespace std;

#define BUFSIZE 1024

/* 
 * error - wrapper for perror
 */

 class RDT //deliable data transfer
 {
 	private:
	 	deque<char> sender_buffer;
	 	int s_buf_sz;
	 	pthread_t s_thread,r_thread,ack_thread;
	 	int my_sockfd;
 	public:
	 	RDT(bool client, int portno = 0);// 1 -client else 0-server
	 	~RDT();
	 	int appsend(char *buff, int size, struct sockaddr_in recv_addr); // return 0 if sent, -1 if sent failed
	 	int apprecv(char *recv, int max_size, struct sockaddr_in *sender_addr); // reveive, returns the receives size
 

};

RDT::RDT()
{
/*
if client = 1 // sending
{
	initialize threads - sender data thread, receiver thread, sender ack thread
	assign my_socketfd;

}

if client = 0 // receiving,
{
	initialize threads - sender ack thread, receiver thread, sender data thread
	assign my_socketfd;
	bind port no. to mysocketfd;
}
*/
}
int appsend(char *buff, int size, struct sockaddr_in recv_addr)
{
	int flag; // flag = 0 sent to buffer, -1 not sent
	flag = send_bufer_handle(buff,size,)
}


void error(const char *msg) {
    perror(msg);
    exit(0);
}

int main(int argc, char **argv) {
    int sockfd, portno, n;
    socklen_t serverlen;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname;
    char buf[BUFSIZE];

    /* check command line arguments */
    if (argc != 3) {
       fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
       exit(0);
    }
    hostname = argv[1];
    portno = atoi(argv[2]);

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }

    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
	  (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);

    /* get a message from the user */
    bzero(buf, BUFSIZE);
    printf("Please enter msg: ");
    fgets(buf, BUFSIZE, stdin);

    /* send the message to the server */
    serverlen = sizeof(serveraddr);
    n = sendto(sockfd, buf, strlen(buf), 0, (sockaddr *)&serveraddr, serverlen);
    if (n < 0) 
      error("ERROR in sendto");
    
    /* print the server's reply */
    n = recvfrom(sockfd, buf, strlen(buf), 0, (sockaddr *)&serveraddr, &serverlen);
    if (n < 0) 
      error("ERROR in recvfrom");
    printf("Echo from server: %s", buf);
    return 0;
}
