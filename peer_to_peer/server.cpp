#include <stdio.h>
#include <string.h>   //strlen
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>   //close
#include <arpa/inet.h>    //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include <string.h>
#include <netdb.h> 
#include <string.h>

#define TRUE   1
#define FALSE  0
#define BUFSIZE 1024


/*print error message*/
void error(char *msg) {
  perror(msg);
  exit(1);
}

/*details of the peer */
typedef struct peer_details
{
    char name[100];
    int port_no;
    char ip_addr[50];
};

 
int main(int argc , char *argv[])
{
    int PORT;
    struct hostent *server;
    struct sockaddr_in serveraddr; 
    peer_details table[5];
    int no_of_peers;
    int opt = TRUE;
    int master_socket , addrlen , new_socket , client_socket[5] , sender_socket[5], max_clients = 5 , activity, i , valread , sd;
    int max_sd;
    struct sockaddr_in address;
    char buff[BUFSIZE],buff_temp[BUFSIZE];  //data buffer of 1K
    char buffer[BUFSIZE];
    int n;
    char *msg,*msg_2;
    char *friend_name,*friend_name_2;
    int port_number;
    char ip_address[50];
    int flag = 0;
    int len;
    int no_of_users;
    //set of socket descriptors
    fd_set readfds;
    PORT = atoi(argv[1]);


    no_of_users = 2;
    strcpy(table[0].name,"hardik");
    table[0].port_no = 8000;
    strcpy(table[0].ip_addr,"10.109.65.37");
    strcpy(table[1].name,"shikha");
    table[1].port_no = 8000;
    strcpy(table[1].ip_addr,"10.112.3.159");

    printf("\n %s %d %s",table[0].name,table[0].port_no,table[0].ip_addr);
    printf("\n %s %d %s",table[1].name,table[1].port_no,table[1].ip_addr);
    // printf("\n %s %d %s",table[2].name,table[2].port_no,table[2].ip_addr);



    char *message = "ECHO Daemon v1.0 \r\n";
    
    //initialise all client_socket[] to 0 so not checked
    for (i = 0; i < max_clients; i++) 
    {
        client_socket[i] = 0;
        sender_socket[i] = 0;
    }
      
    //create a master socket
    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0) 
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
  
    //set master socket to allow multiple connections , this is just a good habit, it will work without this
    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
  
    //type of socket created
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );
      
    //bind the socket to localhost port 8888
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0) 
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    printf("Listener on port %d \n", PORT);
     
    //try to specify maximum of 3 pending connections for the master socket
    if (listen(master_socket, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
      
    //accept the incoming connection
    addrlen = sizeof(address);
    puts("Waiting for connections ...");
     
    while(TRUE) 
    {
        //clear the socket set
        FD_ZERO(&readfds);
  
        //add master socket to set
        FD_SET(master_socket, &readfds);
        max_sd = master_socket;

        //add input file descriptor
        FD_SET(0,&readfds);

        //add child sockets to set
        for ( i = 0 ; i < max_clients ; i++) 
        {
            //socket descriptor
            sd = client_socket[i];
             
            //if valid socket descriptor then add to read list
            if(sd > 0)
                FD_SET( sd , &readfds);
             
            //highest file descriptor number, need it for the select function
            if(sd > max_sd)
                max_sd = sd;
        }
  
        //wait for an activity on one of the sockets , timeout is NULL , so wait indefinitely
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
    
        if ((activity < 0) && (errno!=EINTR)) 
        {
            printf("select error");
        }
          
        //If something happened on the master socket , then its an incoming connection
        if (FD_ISSET(master_socket, &readfds)) 
        {
            if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }
          
            printf("New connection , socket fd is %d , ip is : %s , port : %d \n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
        
            for (i = 0; i < max_clients; i++) 
            {
                //if position is empty
                if( client_socket[i] == 0 )
                {
                    client_socket[i] = new_socket;
                    printf("Adding to list of sockets as %d\n" , i);
                     
                    break;
                }
            }
        }
        else if(FD_ISSET(0,&readfds))
        {
            //get the message from stdin buffer
            bzero(buff, BUFSIZE);
            bzero(buff_temp,BUFSIZE);
            fgets(buff, sizeof(buff), stdin);
            strcpy(buff_temp,buff);
            friend_name = strtok(buff, "/");
            msg = strtok(NULL, "/");

            // find the ip and port of the destination server
            for(i=0; i<no_of_users; i++)
            {
                // printf("\n%s %s\n",friend_name,table[i].name);
                if(strcmp(friend_name,table[i].name) == 0)
                {
                    port_number = table[i].port_no;
                    strcpy(ip_address,table[i].ip_addr);
                    flag = 1;
                    break;
                }
            }

            //if friend name doesnot exist in the table
            if(flag==0)
            {
                printf("\nFriend_name doesnot exist. Sorry \n");
                continue;
            }
            else flag = 0;

            //check if the port to the destination server already exists
            for(i=0; i<max_clients; i++)
            {
                sd = sender_socket[i];
                if(sd > 0)
                {
                    getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen);
                    if(strcmp(ip_address,"localhost") == 0)
                        strcpy(ip_address,"127.0.0.1");
                    // printf("\n%d %d %s %s\n",port_number,ntohs(address.sin_port),ip_address,inet_ntoa(address.sin_addr));
                    if(port_number == ntohs(address.sin_port) && strcmp(ip_address,inet_ntoa(address.sin_addr)) == 0)
                    {
                        flag = 1;
                        break;
                    }
                }

            }


            //if the port does not exist, then create one
            if(flag == 0)
            {

                server = gethostbyname(ip_address);
                if (server == NULL) {
                    fprintf(stderr,"ERROR, no such host as %s\n", ip_address);
                    exit(0);
                }

                /* build the server's Internet address */
                bzero((char *) &serveraddr, sizeof(serveraddr));
                serveraddr.sin_family = AF_INET;
                bcopy((char *)server->h_addr, 
                  (char *)&serveraddr.sin_addr.s_addr, server->h_length);
                serveraddr.sin_port = htons(port_number);

                /* connect: create a connection with the server */
                for(i=0;i<max_clients;i++)
                {
                    if(sender_socket[i] == 0)
                    {
                        sender_socket[i] = socket(AF_INET, SOCK_STREAM, 0);
                        if (sender_socket[i] < 0)
                            error("ERROR opening socket");
                        sd = sender_socket[i];
                        break; 
                    }
                }

                if (connect(sender_socket[i],(struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) 
                  error("ERROR connecting");
                flag =0;
            }

            //send the message
            n = send(sd, buff_temp, strlen(buff_temp),0);
                if (n < 0) 
                  error("ERROR writing to socket");

        }   
        else
        {
            for (i = 0; i < max_clients; i++) 
            {
                sd = client_socket[i];
                  
                if (FD_ISSET( sd , &readfds)) 
                {
                    bzero(buffer, BUFSIZE);
                    // printf("\nsd value is :- %d\n",sd);
                    //Check if it was for closing , and also read the incoming message
                    if ((valread = read( sd , buffer, 1024)) == 0)
                    {
                        //Somebody disconnected , get his details and print
                        getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen);
                        printf("Host disconnected , ip %s , port %d \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
                          
                        //Close the socket and mark as 0 in list for reuse
                        close( sd );
                        client_socket[i] = 0;
                    }
                      
                    else
                    {
                        //receive data
                        friend_name_2 = strtok(buffer, "/");
                        msg_2 = strtok(NULL, "/");
                        getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen);
                        if(strcmp(inet_ntoa(address.sin_addr),"127.0.0.1") == 0)
                        {
                            printf("\nMessaged received from localhost is :- %s\n ",msg_2);
                        }
                        else
                        {
                            for(i=0;i<no_of_users;i++)
                            {
                                if(strcmp(table[i].ip_addr,inet_ntoa(address.sin_addr)) == 0 )
                                {
                                    printf("\nMessaged received from %s is :- %s\n ",table[i].name,msg_2);
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
      
    return 0;
}                   