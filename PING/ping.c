#include <stdio.h>

#include <signal.h>

#include <arpa/inet.h>

#include <sys/types.h>

#include <sys/socket.h>

#include <unistd.h>

#include <netinet/in.h>

#include <netinet/ip.h>

#include <netinet/ip_icmp.h>

#include <netdb.h>

#include <setjmp.h>

#include <errno.h>

#define PACKET_SIZE     4096

#define MAX_WAIT_TIME   5

#define MAX_NO_PACKETS  3


char recvpacket[PACKET_SIZE];

int sockfd, datalen = 56;

int nsend = 0, nreceived = 0;

struct sockaddr_in dest_addr;

pid_t pid;

struct sockaddr_in from;

struct timeval tvrecv;

void my_send()
{
	char sendpacket[PACKET_SIZE];

	struct *icmp = (struct icmp*)sendpacket;
	icmp->icmp_type = ICMP_ECHO;

    icmp->icmp_code = 0;

    icmp->icmp_cksum = 0;

    
    icmp->icmp_id = getpid();
    packsize = 8+datalen;


	for (int i = 0; i < NO_OF_TIMES; ++i)
	{
		icmp->icmp_seq = i;
		tval = (struct timeval*)icmp->icmp_data;

	    gettimeofday(tval, NULL); 

	    icmp->icmp_cksum = cal_chksum((unsigned short*)icmp, packsize); 

	    if (sendto(sockfd, sendpacket, packetsize, 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr)) < 0)

        {

            perror("sendto error");

            continue;

        } 

        sleep(1);

		/* code */
	}
}

void my_recv()
{
	
}


int main(int argc, char *argv[])

{

    struct hostent *host;

    struct protoent *protocol;

    unsigned long inaddr = 0l;

    int waittime = MAX_WAIT_TIME; 

    int size = 50 * 1024;

    if (argc < 2)

    {

        printf("usage:%s hostname/IP address\n", argv[0]);

        exit(1);

    } if ((protocol = getprotobyname("icmp")) == NULL)

    {

        perror("getprotobyname");

        exit(1);

    }



    if ((sockfd = socket(AF_INET, SOCK_RAW, protocol->p_proto)) < 0)

    {

        perror("socket error");

        exit(1);

    }



    setuid(getuid());



    setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));

    bzero(&dest_addr, sizeof(dest_addr));

    dest_addr.sin_family = AF_INET;



    if (inaddr = inet_addr(argv[1]) == INADDR_NONE)

    {

        if ((host = gethostbyname(argv[1])) == NULL)

       

        {

            perror("gethostbyname error");

            exit(1);

        }

        memcpy((char*) &dest_addr.sin_addr, host->h_addr, host->h_length);

    }

    else

        dest_addr.sin_addr.s_addr = inet_addr(argv[1]);

    printf("Pinging!\n");

	my_send();
	my_recv();
}