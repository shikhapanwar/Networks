/* 
 * udpclient.c - A simple UDP client
 * usage: udpclient <host> <port>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <math.h>
#include <openssl/md5.h>

#define TIME_OUT 1
#define BUFSIZE 1024
#define SEQ_NUM_SIZE 4
/* 
 * error - wrapper for perror
 */


struct MESSAGE
{  
  int seq_no;
  char buf[BUFSIZE];
};

    int no_of_packets;
  int sockfd, portno, n;
    int serverlen;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname;
    char buf[BUFSIZE];
    fd_set readfds, masterfds;
    struct timeval timeout;
    struct  MESSAGE *message;

    int WIN_SIZE;
    int currptr, baseptr;
    FILE *fp;
    char tmo_buf[BUFSIZE+1];


void error(char *msg) {
    perror(msg);
    exit(0);
}


/*function to compute to MD5_checksum*/
void MD5_checksum(char *filename,char *output)
{
    strcpy(output,"");
    int n;
    MD5_CTX c;
    char buf[BUFSIZE];
    char temp[7];
    ssize_t bytes;
    unsigned char out[MD5_DIGEST_LENGTH];
    FILE *fp  = fopen(filename,"r");
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
    fclose(fp);
}




int send_and_wait_for_ack(int seq, int sz)
{
    FD_ZERO(&masterfds);
    FD_SET(sockfd, &masterfds);
    memcpy(&readfds, &masterfds, sizeof(fd_set));
   // printf("attempting to send packet %d\n", seq);
    int seq_;
    char *pch;
    message->seq_no = seq;
    n = sendto(sockfd, message, sz, 0, &serveraddr, serverlen);
    //printf("packet sent\n");
    
    if (n < 0) 
      error("ERROR in sendto");

  if (select(sockfd +1, &readfds, NULL, NULL, &timeout) < 0)
   {
     perror("on select");
     exit(1);
   }

    if (FD_ISSET(sockfd, &readfds))
   {
         n = recvfrom(sockfd, message, sizeof(message), 0, &serveraddr, &serverlen);
        if (n < 0) 
            error("ERROR in recvfrom");
        seq_ = message->seq_no;
        if(seq_ != seq)
        {
            //printf("seq didn't match, %d %d\n", seq, seq_);
          return -1;
        }
       // printf("sending successful seq number did match with ack %d\n", seq_);

        return 0; //success
     // read from the socket
   }
   else
   {
        //printf("timeout error\n");
        return -1; // ack packet not received
   }
    
}

void fill_window_and_send()
{

    //fp  + 1 -> baseptr
    //ap + 1->currptr
    int sz;
    
        while( baseptr < no_of_packets-1  && baseptr - currptr < WIN_SIZE )
        {
            baseptr ++;
            fseek(fp, (baseptr)*BUFSIZE, SEEK_SET);
            bzero(buf, BUFSIZE);
            bzero(message->buf, BUFSIZE);
            sz = fread(message -> buf,1, BUFSIZE, fp);
            message->seq_no = baseptr;
            
/*            while(send_and_wait_for_ack(baseptr-1, check + sizeof(int)) == -1);
*/               
         n = sendto(sockfd, message, sz+ sizeof(int), 0, &serveraddr, serverlen);
         printf("sending message %d which has %d bytes\n",  message->seq_no, sz);

        if (n < 0) 
          error("ERROR in sendto");
        }

}
void receive_and_set_ptrs()
{

    int seq;
    FD_ZERO(&masterfds);
    FD_SET(sockfd, &masterfds);
        memcpy(&readfds, &masterfds, sizeof(fd_set));

    if (select(sockfd +1, &readfds, NULL, NULL, &timeout) < 0)
   {
     perror("on select");
     exit(1);
   }
   //printf("in receive_and_set_ptrs\n");

    if (FD_ISSET(sockfd, &readfds)) //message received
   {
        //printf("b4 recvfrom\n");
         n = recvfrom(sockfd, message, sizeof(message), 0, &serveraddr, &serverlen); 
        if (n < 0) 
            error("ERROR in recvfrom");
/*        if(baseptr == no_of_packets - 2)
        {
            printf("check here\n");
            message[n] ='\0';
            seq = atoi(message + n -4);
            printf("seq %d\n", seq);
        }
*/        seq = message->seq_no;
        printf("received ack %d\n", seq);
        if(seq > currptr)
        {
            WIN_SIZE = WIN_SIZE + seq - currptr;
            //printf("b4 modifying ap\n");
            //printf("ap now = %d\n",  ap);
           /* if(seq == no_of_packets -1) fseek(ap,0, SEEK_END);
                else fseek(ap,(seq - currptr)*BUFSIZE , SEEK_CUR);*/
            printf("ap + =%d\n", (seq - currptr)*BUFSIZE);
            currptr = seq ;

        }
   }
   else
   {
        printf("timeout error\n");
/*        fp = ap;
*/        baseptr = currptr;
        WIN_SIZE = WIN_SIZE/2;
        //fill_window_and_send();
      //  return -1; // ack packet not received
   }
   //printf("Ahem\n");

}
int main(int argc, char **argv) 
{   
    int prob_by_100 = 80;

    currptr = -1;
    baseptr = -1;
    WIN_SIZE = 3;

    message = (struct MESSAGE *) malloc(sizeof(struct  MESSAGE));
    int  i, tmp;
    char filename[BUFSIZE];
    char size_str[BUFSIZE];
    char filename_temp[BUFSIZE];
    int size, check;
    char MD5_checksum_val[200];


    timeout.tv_sec = TIME_OUT;                    /*set the timeout to 1 seconds*/
    timeout.tv_usec = 0;

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

        /* get message line from the user */
    printf("\nPlease enter file name:\n");

   
    /* get the filename */ 
    bzero(filename, BUFSIZE);
    fgets(filename, BUFSIZE, stdin);

    /*remove '\n' character from filename */
    filename[strlen(filename)-1] = '\0';  
    strcpy(message->buf,filename);
  


    /* Open the file to be send */
    fp = fopen(filename,"r");
    if(fp == NULL)
   {
      printf("Error!");   
      exit(1);             
   }

       /* Get the MD5_checksum hash value of the file */
    MD5_checksum(filename,MD5_checksum_val);


    /* Get the file size */
    fseek(fp, 0, SEEK_END); 
    size = ftell(fp); 
    fseek(fp, 0, SEEK_SET);
    no_of_packets = (int)ceil(1.0*size/(BUFSIZE));

    /* Attach the file name and size of the file and store it in 'filename_temp' */
    
    int cx = snprintf ( size_str, BUFSIZE, " %04d %d", no_of_packets, size );  // 5 <- SEQ_NUM_SIZE  
    strcat(message->buf,size_str);
    message->seq_no = prob_by_100;


    serverlen = sizeof(serveraddr);
    //printf("The file name is %s\n", filename);        

    while(send_and_wait_for_ack(0, 1+strlen(message->buf)+sizeof(int)) == -1);


    bzero(buf, BUFSIZE);
    bzero(message->buf, BUFSIZE);
    i = 1;
    /*while(!feof(fp))
    {
        bzero(buf, BUFSIZE);
        bzero(message->buf, BUFSIZE);
        check = fread(message -> buf,1, BUFSIZE, fp);
        message->seq_no = i;
        while(send_and_wait_for_ack(i, check + sizeof(int)) == -1);
        i++;
    }
*/
    int tp;
    while( currptr < no_of_packets -1  )
    {
        printf("currptr %d, baseptr %d \n", currptr, baseptr );        
        fill_window_and_send();
        receive_and_set_ptrs();
      //  printf("currptr %d, baseptr %d \n", currptr, baseptr );

    }
    printf("outside currptr %d, baseptr %d \n", currptr, baseptr );        


    fclose(fp);


    bzero(buf, BUFSIZE);

    /* Receive MD5_checksum hash value from the server */
    n = recvfrom(sockfd, buf, BUFSIZE, 0, &serveraddr, &serverlen);
    //printf("received:%s\n",buf );
    if (n < 0) 
       error("ERROR in recvfrom");
    //printf("%d bytes received of checksum received", n);
        

    /* Compare the hash values */
   printf("MD5 received%s\n", buf);
   printf("MD5 calculated%s\n",MD5_checksum_val );

    if(strcmp(MD5_checksum_val,buf) == 0)
    {
      printf("\nMD5 Matched\n");
   }
   else    printf("\nMD5 Not Matched\n");
   strcpy(buf, "matched");


   n = sendto(sockfd, buf, strlen(buf)+1, 0, &serveraddr, serverlen);
    close(sockfd);

    printf("total no. of packets %d\n", no_of_packets);


    
    return 0;
}
