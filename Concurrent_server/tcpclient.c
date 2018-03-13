/* 
 * tcpclient.c - A simple TCP client
 * usage: tcpclient <host> <port>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <string.h>
#include <openssl/md5.h>
#define BUFSIZE 1024

/* 
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(0);
}


/*function to compute to MD5_checksum*/
void MD5_checksum(char *filename,char *output)
{
    int n;
    MD5_CTX c;
    char buf[BUFSIZE];
    char temp[7];
    ssize_t bytes;
    unsigned char out[MD5_DIGEST_LENGTH];
    FILE *fp  = fopen(filename,"r");
	if(fp == NULL)
   {
      printf("Error!");   
      exit(1);             
   }

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



int main(int argc, char **argv) {
    int sockfd, portno, n;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname;
    char buf[BUFSIZE];
    char filename[BUFSIZE];
    char filename_temp[BUFSIZE];
    char size_str[BUFSIZE];
    long long int size;
    char temp[20];
    char MD5_checksum_val[200];
    int check;
    int i;
    /* check command line arguments */
    if (argc != 4) {
       fprintf(stderr,"usage: %s <hostname> <port> <filename>\n", argv[0]);
       exit(0);
    }
    hostname = argv[1];
    portno = atoi(argv[2]);

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
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

    /* connect: create a connection with the server */
    if (connect(sockfd, &serveraddr, sizeof(serveraddr)) < 0) 
      error("ERROR connecting");

    /* get message line from the user */
   // printf("\nPlease enter file name:");

    /* get the filename */ 
    // bzero(filename, BUFSIZE);
    // fgets(filename, BUFSIZE, stdin);
    strcpy(filename,argv[3]);
    printf("\n*********%s*******\n",filename);
    /*remove '\n' character from filename */
    strcpy(filename_temp,filename);
    // filename_temp[strlen(filename_temp)-1] = '\0';    
        

    /* Open the file to be send */
    FILE *fp = fopen(filename_temp,"r");
    if(fp == NULL)
   {
      printf("Error!");   
      exit(1);             
   }


    /* Get the file size */
    fseek(fp, 0, SEEK_END); 
    size = ftell(fp); 
    fseek(fp, 0, SEEK_SET);
    printf("\n%d\n",size);

    /* Get the MD5_checksum hash value of the file */
    MD5_checksum(filename_temp,MD5_checksum_val);

    /* Attach the file name and size of the file and store it in 'filename_temp' */
    int cx = snprintf ( size_str, BUFSIZE, " %d", size );    
    strcat(filename_temp,size_str);
    strcat(filename_temp,"\n");
    // printf("\nData %s Size of filename_temp %d \n",filename_temp,sizeof(filename_temp));

    /* send 'filename_temp' to the server */
    n = send(sockfd, filename_temp, strlen(filename_temp),0);
    if (n < 0) 
      error("ERROR writing to socket");

    /* print the server's reply/acknowledgement */
    bzero(buf, BUFSIZE);
    n = recv(sockfd, buf, BUFSIZE,0);
    if (n < 0) 
      error("ERROR reading from socket");
    printf("Echo from server: %s", buf);


    bzero(buf, BUFSIZE);
    // printf("\n@@@@@@@@@@@@@@@@@@\n");
    while(!feof(fp))
    {
        // printf("\n!!!!!!!!!!!!!!!!\n");
        /* Read contents from the file */
        check = fread(buf,1, BUFSIZE, fp);

        /* send number of bytes read from the file */
        n = send(sockfd, buf, check,0);
        if (n < 0) 
          error("ERROR writing to socket");

        /* empty buffer */
        bzero(buf, BUFSIZE);
    }
    fclose(fp);


    bzero(buf, BUFSIZE);

    /* Receive MD5_checksum hash value from the server */
    n = recv(sockfd, buf, 200,0);
    if (n < 0) 
      error("ERROR writing to socket");

    /* Compare the hash values */
    if(strcmp(MD5_checksum_val,buf) == 0)   printf("\nMD5 Matched\n");
    else    printf("\nMD5 Not Matched\n");
    close(sockfd);
    
    return 0;
}
