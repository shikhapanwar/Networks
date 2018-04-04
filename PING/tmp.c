//A simple Ping program to ping any address such as google.com in Linux 
//run program as : gcc -o ping ping.c
// then : ./ping google.com
//can ping localhost addresses 
//see the RAW socket implementation
/*.....Ping Application....*/
/*   Note Run under root priveledges  */
/*  On terminal 
  $ gcc -o out ping.c 
  $ sudo su
  # ./out 127.0.0.1 
   OR
  # ./out google.com */
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
#include <math.h>

#define PACKET_SIZE     4096

#define MAX_WAIT_TIME   5

#define MAX_NO_PACKETS  1
#define INF 100
#define MAX_TIME 1000
char sendpacket[PACKET_SIZE];

char recvpacket[PACKET_SIZE];

double rtt = 0, t_rtt, mx_rtt = 0, mn_rtt = INF;
double vec[MAX_TIME];

int sockfd, datalen = 56;
int nsend = 0, nrecv = 0;

struct sockaddr_in dest_addr;

pid_t pid;

struct sockaddr_in from;

struct timeval tvrecv;

void statistics(int signo);

unsigned short cal_chksum(unsigned short *addr, int len);

int pack(int pack_no);

void send_packet(int attempt);

double recv_packet(void);

double unpack(char *buf, int len);

void tv_sub(struct timeval *out, struct timeval *in);
double calculateSD();

void statistics(int signo)

{

    printf("\n--------------------PING statistics-------------------\n");

    printf("%d packets transmitted, %d received , %%%d lost\n", nsend,

        nrecv, (nsend - nrecv) / nsend * 100);
    printf("rtt min/avg/max/mdev = %3f/%3f/%3f/%3lf\n", mn_rtt, t_rtt/nrecv,mx_rtt,calculateSD());
// to be done 
    close(sockfd);

    exit(1);

} 



double calculateSD()
{
    double sum = 0.0, mean, standardDeviation = 0.0;

    int i;

    for(i=0; i<nrecv; ++i)
    {
        sum += vec[i];
    }

    mean = sum/10;

    for(i=0; i<10; ++i)
        standardDeviation += pow(vec[i] - mean, 2);

    return sqrt(standardDeviation/10);
}


unsigned short cal_chksum(unsigned short *addr, int len)

{

    int nleft = len;

    int sum = 0;

    unsigned short *w = addr;

    unsigned short answer = 0;





    while (nleft > 1)

    {

        sum +=  *w++;

        nleft -= 2;

    }



    if (nleft == 1)

    {

        *(unsigned char*)(&answer) = *(unsigned char*)w;

        sum += answer;

    }

    sum = (sum >> 16) + (sum &0xffff);

    sum += (sum >> 16);

    answer = ~sum;

    return answer;

}





int pack(int pack_no)

{

    int i, packsize;

    struct icmp *icmp;

    struct timeval *tval;

    icmp = (struct icmp*)sendpacket;

    icmp->icmp_type = ICMP_ECHO;

    icmp->icmp_code = 0;

    icmp->icmp_cksum = 0;

    icmp->icmp_seq = pack_no;

    icmp->icmp_id = pid;

    packsize = 8+datalen;

    tval = (struct timeval*)icmp->icmp_data;

    gettimeofday(tval, NULL); 

    icmp->icmp_cksum = cal_chksum((unsigned short*)icmp, packsize); 

    return packsize;

}





void send_packet(int attempt)

{
    signal(SIGINT, statistics);
    int packetsize = pack(attempt); 

        if (sendto(sockfd, sendpacket, packetsize, 0, (struct sockaddr*)

            &dest_addr, sizeof(dest_addr)) < 0)

        {

            perror("sendto error");

            return;

        }
    nsend++;

}





double recv_packet()

{

    int n, fromlen;

    extern int errno;

    signal(SIGALRM, statistics);
    signal(SIGINT, statistics);

    fromlen = sizeof(from);

        alarm(MAX_WAIT_TIME);

        if ((n = recvfrom(sockfd, recvpacket, sizeof(recvpacket), 0, (struct

            sockaddr*) &from, &fromlen)) < 0)

        {

                return;


        } 
        gettimeofday(&tvrecv, NULL); 
        nrecv++;
        rtt = unpack(recvpacket, n);

        return rtt;
    

}





double unpack(char *buf, int len)

{
    signal(SIGINT, statistics);

    int i, iphdrlen;

    struct ip *ip;

    struct icmp *icmp;

    struct timeval *tvsend;

    double rtt;

    ip = (struct ip*)buf;

    iphdrlen = ip->ip_hl << 2; 

    icmp = (struct icmp*)(buf + iphdrlen);

    len -= iphdrlen; 

    if (len < 8)

    {

        printf("ICMP packets\'s length is less than 8\n");

        return  - 1;

    } 


    if ((icmp->icmp_type == ICMP_ECHOREPLY) && (icmp->icmp_id == pid))

    {

        tvsend = (struct timeval*)icmp->icmp_data;
/*        printf("plz%d\n%d\nplzz\n",tvrecv.tv_usec, tvrecv.tv_sec );
*/
        tv_sub(&tvrecv, tvsend); 

        rtt = (double)tvrecv.tv_sec * 1000.0+(double)tvrecv.tv_usec / 1000.0;

        printf("%d byte from %s: icmp_seq=%u ttl=%d time=%.3f ms\n", len,

            inet_ntoa(from.sin_addr), icmp->icmp_seq, ip->ip_ttl, rtt);

        return rtt;


    }

    else

        return  - 1;

}



int main(int argc, char *argv[])

{

    struct hostent *host;

    struct protoent *protocol;

    unsigned long inaddr = 0l;

    int waittime = MAX_WAIT_TIME;

    int size = 50 * 1024;

    int attempt;

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


    pid = getpid();

    printf("PING %s(%s): %d bytes data in ICMP packets.\n", argv[1], inet_ntoa

        (dest_addr.sin_addr), datalen);

    attempt = 0;
    while(1)
    {
        attempt ++;

        send_packet(attempt); 
        
       rtt = recv_packet(); 
       printf("rtt %lf\n",rtt );
       if(rtt > (double)0)
       {
            vec[nsend]= rtt;
            t_rtt += rtt;

            if(rtt > mx_rtt) mx_rtt = rtt;
            if(rtt < mn_rtt) mn_rtt = rtt;
       }

       sleep(1);
    }
    statistics(SIGALRM); 

    return 0;

}


void tv_sub(struct timeval *out, struct timeval *in)

{
    

    if ((out->tv_usec -= in->tv_usec) < 0)

    {

        --out->tv_sec;

        out->tv_usec += 1000000;


    } out->tv_sec -= in->tv_sec;

}


