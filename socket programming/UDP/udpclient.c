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

#define TIME_OUT 40
#define BUFSIZE 1024
#define SEQ_NUM_SIZE 4
/* 
 * error - wrapper for perror
 */
  int sockfd, portno, n;
    int serverlen;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname;
    char buf[BUFSIZE];
    fd_set readfds, masterfds;
    struct timeval timeout;


void error(char *msg) {
    perror(msg);
    exit(0);
}


int send_and_wait_for_ack(int seq)
{
  int seq_;
  char *pch;

    n = sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
    
    if (n < 0) 
      error("ERROR in sendto");

  if (select(sockfd +1, &readfds, NULL, NULL, &timeout) < 0)
   {
     perror("on select");
     exit(1);
   }

    if (FD_ISSET(sockfd, &readfds))
   {
         n = recvfrom(sockfd, buf, BUFSIZE, 0, &serveraddr, &serverlen);
        if (n < 0) 
            error("ERROR in recvfrom");
            seq_ = atoi(buf);
            printf("d\n",(int)seq_);
        return 0; //success
     // read from the socket
   }
   else
   {
        printf("timeout error\n");
        return -1; // ack packet not received
   }
    
}


int main(int argc, char **argv) 
{   
    int no_of_packets, i, tmp;
    char filename[BUFSIZE];
    char size_str[BUFSIZE];
    char filename_temp[BUFSIZE];
    int size, check;

    timeout.tv_sec = TIME_OUT;                    /*set the timeout to 10 seconds*/
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

    FD_ZERO(&masterfds);
    FD_SET(sockfd, &masterfds);
    memcpy(&readfds, &masterfds, sizeof(fd_set));


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
    strcpy(buf,filename);
  


    /* Open the file to be send */
    FILE *fp = fopen(filename,"r");
    if(fp == NULL)
   {
      printf("Error!");   
      exit(1);             
   }


    /* Get the file size */
    fseek(fp, 0, SEEK_END); 
    size = ftell(fp); 
    fseek(fp, 0, SEEK_SET);
    no_of_packets = (int)ceil(1.0*size/(BUFSIZE-SEQ_NUM_SIZE));

    /* Attach the file name and size of the file and store it in 'filename_temp' */
    int cx = snprintf ( size_str, BUFSIZE, " %04d %d", no_of_packets, size );  // 5 <- SEQ_NUM_SIZE  
    strcat(buf,size_str);


    serverlen = sizeof(serveraddr);
    printf("The file name is %s\n", filename);        

    while(send_and_wait_for_ack(0) == -1);


    bzero(buf, BUFSIZE);
    i = 1;
    while(!feof(fp))
    {
        bzero(buf, BUFSIZE);
        //tmp = snprintf ( buf, BUFSIZE, "%d", i );    
        //strcpy(buf, atoi(i) );
        check = fread(buf,1, BUFSIZE - SEQ_NUM_SIZE, fp);
        cx = snprintf ( size_str, BUFSIZE, "%04d", i );  // 5 <- SEQ_NUM_SIZE  
        strcat(buf,size_str);
        while(send_and_wait_for_ack(i) == -1);
        /* Read contents from the file */

        /* send number of bytes read from the file */
        //n = send(sockfd, buf, check,0);
        //if (n < 0) 
        //  error("ERROR writing to socket");

        /* empty buffer */
        //bzero(buf, BUFSIZE);
        i++;
    }
    fclose(fp);

    /*
    n = sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
    
    if (n < 0) 
      error("ERROR in sendto");

  if (select(sockfd +1, &readfds, NULL, NULL, &timeout) < 0)
   {
     perror("on select");
     exit(1);
   }

    if (FD_ISSET(sockfd, &readfds))
   {
         n = recvfrom(sockfd, buf, strlen(buf), 0, &serveraddr, &serverlen);
        if (n < 0) 
            error("ERROR in recvfrom");
        printf("Echo from server: %s", buf);
     // read from the socket
   }
   else
   {
        printf("timeout error\n");
     // the socket timedout
   }
   */
    
    /* print the server's reply */
   // n = recvfrom(sockfd, buf, strlen(buf), 0, &serveraddr, &serverlen);
    //if (n < 0) 
      //error("ERROR in recvfrom");
    
    return 0;
}
