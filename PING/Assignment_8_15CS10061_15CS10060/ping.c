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

#define pack_ET_SIZE     4096

#define MAX_WAIT_TIME   5

#define MAX_NO_pack_ETS  1
#define INF 100
#define MAX_TIME 1000
char sender_buf[pack_ET_SIZE];

char recv_buf[pack_ET_SIZE];

double rtt = 0, t_rtt, mx_rtt = 0, mn_rtt = INF;
double vec[MAX_TIME];

int sockfd, datalen = 56;
int nsend = 0, nrecv = 0;

struct sockaddr_in addr_dest;

pid_t pid;

struct sockaddr_in from;

struct timeval tv_recv;

unsigned short check_sum( int len, unsigned short *addr);


void my_send_pack_et(int attempt);


void show_stats(int signo);
double my_recv_pack_et();

double un_pack_(int len, char *buf);

double calculateSD();

void show_stats(int signo)

{

    printf("\n------------------PING STATISTICS-----------------\n");

    printf("%d packets transmitted, %d received , %%%3f lost\n", nsend, nrecv, (float)(nsend - nrecv) / (float)nsend * 100.0);
    if(nrecv>0)printf("rtt min/avg/max/mdev = %3f/%3f/%3f/%3lf\n", mn_rtt, t_rtt/nrecv,mx_rtt,calculateSD());
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


unsigned short check_sum(int len, unsigned short *addr)

{

    int n_left = len;

    int sum = 0;

    unsigned short *w = addr;

    unsigned short answer = 0;





    while (n_left > 1)

    {

        sum +=  *w++;

        n_left -= 2;

    }



    if (n_left == 1)

    {

        *(unsigned char*)(&answer) = *(unsigned char*)w;

        sum += answer;

    }

    sum = (sum >> 16) + (sum &0xffff);

    sum += (sum >> 16);

    answer = ~sum;

    return answer;

}








void my_send_pack_et(int attempt)

{
    signal(SIGINT, show_stats);
    int pack_size;

    struct icmp *icmp = (struct icmp*)sender_buf;

    struct timeval *tval;

    ;
    icmp->icmp_cksum = 0;

    icmp->icmp_seq = attempt;

    icmp->icmp_id = pid;

    icmp->icmp_type = ICMP_ECHO;

    icmp->icmp_code = 0;

    pack_size = 8+datalen;  

    tval = (struct timeval*)icmp->icmp_data;

    gettimeofday(tval, NULL); 

    icmp->icmp_cksum = check_sum(pack_size,(unsigned short*)icmp);  

        if (sendto(sockfd, sender_buf, pack_size, 0, (struct sockaddr*)

            &addr_dest, sizeof(addr_dest)) < 0)

        {

            perror("sendto error");

            return;

        }
    nsend++;

}





double my_recv_pack_et()

{

    int n, fromlen;

    extern int errno;

    signal(SIGALRM, show_stats);
    signal(SIGINT, show_stats);
    alarm(MAX_WAIT_TIME);
    fromlen = sizeof(from);

        

        if ((n = recvfrom(sockfd, recv_buf, sizeof(recv_buf), 0, (struct

            sockaddr*) &from, &fromlen)) < 0)

        {

                return;


        } 
        gettimeofday(&tv_recv, NULL); 
        
        rtt = un_pack_(n, recv_buf);
        if(rtt != -1)
            nrecv++;
        return rtt;
    

}





double un_pack_( int len, char *buf)

{
    signal(SIGINT, show_stats);

    int i, iphdrlen;

    struct ip *ip;

    struct icmp *icmp;

    struct timeval *tvsend;

    double rtt;

    ip = (struct ip*)buf;

    iphdrlen = ip->ip_hl << 2; 

    icmp = (struct icmp*)(buf + iphdrlen);
 
    len = len - iphdrlen; 

    if (!(len >= 8))

    {

        printf("ICMP packets \'s leng is less than 8\n");

        return  - 1;

    } 

    if ( (icmp->icmp_id == pid) && (icmp->icmp_type != ICMP_ECHOREPLY) )
    {
        my_recv_pack_et();
        return -1;
    }
    if ( (icmp->icmp_id == pid) && (icmp->icmp_type == ICMP_ECHOREPLY) )

    {

        tvsend = (struct timeval*)icmp->icmp_data;
        if ((tv_recv.tv_usec -= tvsend->tv_usec) < 0)

            {
                --tv_recv.tv_sec;
                tv_recv.tv_usec += 1000000;

            } 
            tv_recv.tv_sec -= tvsend->tv_sec;

        rtt = (double)tv_recv.tv_sec * 1000.0+(double)tv_recv.tv_usec / 1000.0;

        printf("%d byte from %s: icmp_seq = %u ttl=%d time=%3f ms\n", len,

            inet_ntoa(from.sin_addr), icmp->icmp_seq, ip->ip_ttl, rtt);

        return rtt;


    }

    else

        return  - 1;

}



int main(int argc, char *argv[])

{

    int waittime = MAX_WAIT_TIME;
    unsigned long inaddr = 0l;
    int size = 50 * 1024;
    int attempt;
    struct hostent *host;
    struct protoent *prtcol;

    

    

    if (argc < 2)

    {

        printf("usage :%s hostname/ IP address \n", argv[0]);

        exit(1);

    } if ((prtcol = getprotobyname("icmp")) == NULL)

    {

        perror("getprotobyname");

        exit(1);

    }



    if ((sockfd = socket(AF_INET, SOCK_RAW, prtcol->p_proto)) < 0)

    {

        perror("socket error");

        exit(1);

    }



    setuid(getuid());



    setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));

    bzero(&addr_dest, sizeof(addr_dest));

    addr_dest.sin_family = AF_INET;



    if (inaddr = inet_addr(argv[1]) == INADDR_NONE)

    {

        if ((host = gethostbyname(argv[1])) == NULL)

       

        {

            perror("gethostbyname  error ");

            exit(1);

        }

        memcpy((char*) &addr_dest.sin_addr, host->h_addr, host->h_length);

    }

    else

        addr_dest.sin_addr.s_addr = inet_addr(argv[1]);


    pid = getpid();

    printf("PING %s(%s): %d bytes data in ICMP pack_ets..\n", argv[1], inet_ntoa

        (addr_dest.sin_addr), datalen);

    attempt = 0;
    while(1)
    {
        attempt ++;

        my_send_pack_et(attempt); 
        
       rtt = my_recv_pack_et(); 
       if(rtt > (double)0)
       {
            vec[nsend]= rtt;
            t_rtt += rtt;

            if(rtt > mx_rtt) mx_rtt = rtt;
            if(rtt < mn_rtt) mn_rtt = rtt;
       }

       sleep(1);
    }
    show_stats(SIGALRM); 

    return 0;

}

