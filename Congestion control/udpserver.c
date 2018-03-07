/* 
 * udpserver.c - A UDP echo server 
 * usage: udpserver <port_for_server>
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/md5.h>

#define BUFSIZE 1024
#define SEQ_NUM_SIZE 4
#define TIME_OUT 1

/*
 * error - wrapper for perror
 */
void error(char *msg) {
  perror(msg);
  exit(1);
}

struct MESSAGE
{
    int seq_no;
  char buf[BUFSIZE];
};



void MD5_checksum(char *filename,char *output)
{
    int n;
    MD5_CTX c;
    char buf[BUFSIZE];
    char temp[7];
    ssize_t bytes;
    unsigned char out[MD5_DIGEST_LENGTH];
    FILE *fp  = fopen(filename,"r");
    if(!fp) printf("error readinf file for MD5 checksum\n");
    MD5_Init(&c);
    bytes=fread(buf,1, BUFSIZE, fp);
    while(!feof(fp))
    {
        MD5_Update(&c, buf, bytes);
        bytes=fread(buf,1, BUFSIZE, fp);
    }

    MD5_Final(out, &c);

    for(n=0; n<MD5_DIGEST_LENGTH; n++)
    {
        snprintf(temp, 6, "%02x", out[n]);
        strcat(output,temp);
    }
    printf("\n");
    fclose(fp);
}


int main(int argc, char **argv) {

      fd_set readfds, masterfds;
    struct timeval timeout;

    struct MESSAGE *message;
    message = (struct MESSAGE *) malloc(sizeof(struct  MESSAGE));
    timeout.tv_sec = TIME_OUT;                    /*set the timeout to 1 seconds*/
    timeout.tv_usec = 0;

    float r;
  int sockfd; /* socket file descriptor - an ID to uniquely identify a socket by the application program */
  int portno; /* port to listen on */
  int clientlen; /* byte size of client's address */
  struct sockaddr_in serveraddr; /* server's addr */
  struct sockaddr_in clientaddr; /* client addr */
  struct hostent *hostp; /* client host info */
  char buf[BUFSIZE]; /* message buf */
  char *hostaddrp; /* dotted decimal host addr string */
  int optval; /* flag value for setsockopt */
  int n; /* message byte size */
  int seq, i, c, position, no_of_packets, prob_by_100;
  char str_seq[6];
  char *pch;
  char file_name[BUFSIZE], file_name_[BUFSIZE];
  char MD_checksum_val[200];

  /* 
   * check command line arguments 
   */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port_for_server>\n", argv[0]);
    exit(1);
  }
  portno = atoi(argv[1]);

  /* 
   * socket: create the socket 
   */
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) 
    error("ERROR opening socket");

    FD_ZERO(&masterfds);
    FD_SET(sockfd, &masterfds);
    memcpy(&readfds, &masterfds, sizeof(fd_set));


  /* setsockopt: Handy debugging trick that lets 
   * us rerun the server immediately after we kill it; 
   * otherwise we have to wait about 20 secs. 
   * Eliminates "ERROR on binding: Address already in use" error. 
   */
  optval = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
       (const void *)&optval , sizeof(int));

  /*
   * build the server's Internet address
   */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)portno);

  /* 
   * bind: associate the parent socket with a port 
   */
  if (bind(sockfd, (struct sockaddr *) &serveraddr, 
     sizeof(serveraddr)) < 0) 
    error("ERROR on binding");
  while (1) {

    printf("\nServer running\n");
  /* 
   * main loop: wait for a datagram, then echo it
   */

       /* Seperate out filename and filesize */

  clientlen = sizeof(clientaddr);


    bzero(buf, BUFSIZE);
    bzero(message-> buf, BUFSIZE);

    n = recvfrom(sockfd, message, sizeof(*message), 0,
     (struct sockaddr *) &clientaddr, &clientlen);
    if (n < 0)
      error("ERROR in recvfrom");
    prob_by_100 = message->seq_no;

    /* 
     * gethostbyaddr: determine who sent the datagram
     */
    hostp = gethostbyaddr((const char *)&clientaddr, 
        sizeof(clientaddr.sin_addr.s_addr), AF_INET);
    if (hostp == NULL)
      error("ERROR on gethostbyaddr");
    hostaddrp = inet_ntoa(clientaddr.sin_addr);
    if (hostaddrp == NULL)
      error("ERROR on inet_ntoa\n");
    pch = strtok (message->buf," ");
    printf("\nFilename :- %s",pch);
    strcpy(file_name,pch);
    int cx = snprintf ( file_name_, BUFSIZE, "output_%s", file_name);  // 5 <- SEQ_NUM_SIZE  
    pch = strtok (NULL," ");
   printf(" no_of_packets :- %s\n",pch);
       printf("check\n");

    no_of_packets = atoi(pch);
    printf("check\n");
    pch = strtok (NULL," ");
     printf("Size :- %s\n",pch);
    
     prob_by_100 = message->seq_no;
    /* Open file */
    FILE *fp = fopen(file_name_,"w+");



    /* 
     * sendto: echo the input back to the client 
     */
    strcpy(buf, "ACK 0");
    strcpy(message->buf, buf);
    message -> seq_no = 0;
    n = sendto(sockfd, message, sizeof(message), 0, 
         (struct sockaddr *) &clientaddr, clientlen);

    if (n < 0) 
      error("ERROR in sendto");

  i = 0; // is the expected seq no.

  while (1) {
    //printf("waiting for packet %d\n", i );

    /*
     * recvfrom: receive a UDP datagram from a client
     */
    bzero(buf, BUFSIZE);
    n = recvfrom(sockfd, message, sizeof(*message), 0,
     (struct sockaddr *) &clientaddr, &clientlen);

    if (n < 0)
      error("ERROR in recvfrom");
        if(i == message -> seq_no)
        {
          r = rand() / RAND_MAX;
          printf("%f, %f",r, prob_by_100 );
          if( (float)prob_by_100/100.00 >  r) 
          {
            printf("Dropping this packet for testing\n");
            continue;
          }  // packet dropped
          //printf("packet matched %d\n", i);
         // printf("no. of packets %d\n", no_of_packets );
          no_of_packets--;
          i++;
        }
        else
        {
          //printf("Wrong packet received\n,");
          continue;
        }


    /* 
     * gethostbyaddr: determine who sent the datagram
     */
    hostp = gethostbyaddr((const char *)&clientaddr, 
        sizeof(clientaddr.sin_addr.s_addr), AF_INET);
    if (hostp == NULL)
      error("ERROR on gethostbyaddr");
    hostaddrp = inet_ntoa(clientaddr.sin_addr);
    if (hostaddrp == NULL)
      error("ERROR on inet_ntoa\n");
    //printf("server received datagram from %s (%s)\n", hostp->h_name, hostaddrp);

  fwrite(message->buf,1,n-sizeof(int), fp);
  printf("writing %d bytes of packet  %d\n", n-sizeof(int), message->seq_no );
  //printf("server receive %d seq_no and %d bytes\n", message->seq_no, strlen(message->buf));


     n = sendto(sockfd, message, sizeof(message), 0, 
         (struct sockaddr *) &clientaddr, clientlen);
     printf("sent ack %d \n",message->seq_no);
    if (n < 0) 
      error("ERROR in sendto");
    if(!no_of_packets )  break;
    
  }
  fclose(fp);
  printf("File received\n");
  //strcpy(MD_checksum_val,"\0");

  /* compute MD5_checksum hash value */ 
  bzero(MD_checksum_val, 200);
  MD5_checksum(file_name_,MD_checksum_val);

    /*send the MD5_checksum hash value to the client */

    while(1)
    {
      n = sendto(sockfd, MD_checksum_val, strlen(MD_checksum_val)+1, 0, (struct sockaddr *) &clientaddr, clientlen);
      printf("sent MD5  %s\n", MD_checksum_val);
        if (n < 0) 
            error("ERROR in sendto");

        if (select(sockfd +1, &readfds, NULL, NULL, &timeout) < 0)
         {
           perror("on select");
           exit(1);
         }

          if (FD_ISSET(sockfd, &readfds))
         {
               n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, &clientlen);
              if (n < 0) 
                  error("ERROR in recvfrom");
              if(strcmp(buf, "matched"))
              {
                continue;
              }

              break;
         }
         else
         {
              continue;
         }
}
    }
}