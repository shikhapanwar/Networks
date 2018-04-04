#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <bits/stdc++.h>
using namespace std;

#define BUFSIZE 1024
#define max_window_size_1 102400	// size of sender buffer
#define max_window_size_2 102400	//size of receiver buffer
#define MSS_bytes 1024

/*Timepass Variable */
int optval;
int n;


int sockfd;	//global socket through which data is received or sent
struct sockaddr_in senderaddr; /* server's addr */
struct sockaddr_in receiveraddr;	/*client address */
socklen_t senderlength,receiverlength;

/* Threads and semaphore declarations */
pthread_t sender;
pthread_t receiver;
pthread_mutex_t sender_mutex;
// pthread_mutex_t receiver_mutex;

/* servermode = 1 server is running */
int servermode;

/*sender and receiver buffer */
char sender_queue[max_window_size_1];
char recver_queue[max_window_size_2];

/* curr_ptr_sender = index till which bytes has been ack
   base_ptr_sender = index till which bytes had been sent */

int curr_ptr_sender = 0;
int base_ptr_sender = 0;

/* unack_notsent = bytes_unack + bytes_not_sent */
int unack_notsent = 0;

struct timeval timeout={1,0};

typedef struct
{
	char data[BUFSIZE];
	int size;	//for 'DATA' packet it is the size of data and for 'ACK' packet it is the receiver advertised window size
	int type;	//0 for ACK, 1 for Data, 2 for Sync, 3 for Fin
	int seq_no;
}packet;

/*initial values */
int recv_advr_cwnd = max_window_size_2;
int ssthresh = max_window_size_1;

int current_cwnd = 1024;
int slow_start = 1; // 1 = true, 0 = false
 
int count_dup_ack = 0;


int i = 0;
int j = 0;

int curr_ptr_recver = 0;	//max index upto which seqence number has been received
int base_ptr_recver = 0;	//index till which bytes has been acknowledged


int seq_no = 0;	//seq_no at the sender side
int seq_no_recv = 0;	// seq no at the receiver side
int not_read = 0;	// index till not_read -1 has been read by the app

// for handling out of order packets
bool bitmap_recv_buff[max_window_size_2];


packet packet_data_send,packet_data;

int filesize = -1;

/**Helpler functions**/


void error(const char *msg) {
  perror(msg);
  exit(0);
}


void make_connection_packet()
{
	bzero(packet_data.data, BUFSIZE);;
	packet_data.seq_no = 0;
	packet_data.type = 2;
	packet_data.size = filesize;	//gives the filesize
}


// gives the difference between base and current pointer at the sender buffer
int getdiff()
{
	int getdiff;
	if(base_ptr_sender >= curr_ptr_sender)
		getdiff = base_ptr_sender - curr_ptr_sender;
	else
		getdiff = max_window_size_1 - (curr_ptr_sender - base_ptr_sender);
	
	return getdiff;
}

// gives the difference between not read pointer and current pointer at the receiver buffer
int diff2()
{
	if(not_read<=curr_ptr_recver)	
		return curr_ptr_recver - not_read;
	else
		return max_window_size_2 - (not_read - curr_ptr_recver);
}


int min(int a,int b)
{
	if(a>=b)	return b;
	else return a;
}

// set bits
void update_bit_map(int offset,int length)
{
	int i;
	// cout<<"\nOffset is :- "<<offset<<" "<<"length is :- "<<length<<"\n";
	for(i=0;i<length;i++)
		bitmap_recv_buff[(offset+i)%(max_window_size_2)] = true;
}


// unset bits
void update_bit_map_2(int offset,int length)
{
	int i;
	for(i=0;i<length;i++)
		bitmap_recv_buff[(offset+i)%(max_window_size_2)] = false;
}



/**Functions called by the sender thread **/



void create_packet(packet *sent_packet,char *data,int bytes,int packet_type,int sequence_no)
{
	memcpy((*sent_packet).data,data,bytes);
	(*sent_packet).size = bytes;
	(*sent_packet).seq_no = sequence_no;
	(*sent_packet).type = packet_type;
}





void send_packets(int actual_bytes)
{
	int n;
	int bytes_sent;
	char data[1025];
	int sequence_no;
	int packet_type = 1;	//data packet
	int temp;

	if(actual_bytes <= 0)	return;
	else
	{
		while(actual_bytes>0)
		{
			
			if(actual_bytes > MSS_bytes)	bytes_sent = MSS_bytes;
			else bytes_sent = actual_bytes;
			actual_bytes -= bytes_sent;
			sequence_no = seq_no + getdiff(); 
			

			temp = max_window_size_1 - (base_ptr_sender);
			
			if( temp >= bytes_sent)
			{
				memcpy(data,sender_queue + base_ptr_sender,bytes_sent);
				base_ptr_sender+=bytes_sent;
			}
			else
			{
			
				memcpy(data,sender_queue + base_ptr_sender,temp);
				base_ptr_sender = 0;
				memcpy(data+temp,sender_queue + base_ptr_sender,bytes_sent-temp);
				base_ptr_sender+=(bytes_sent-temp);
			}

			create_packet(&packet_data_send,data,bytes_sent,packet_type,sequence_no);

			n = sendto(sockfd, &packet_data_send, sizeof(packet), 0, (struct sockaddr *) &receiveraddr, receiverlength);
			cout<<"\nSent packet sequence number is :-"<<packet_data_send.seq_no<<"\n";
		}

	}

}

void * rate_control(void *useless)
{
	int max_bytes,actual_bytes,bytes_remaining;
	while(1)
	{
		// Busy Waiting
		while(getdiff() == unack_notsent) ;	//no data to send
		
		// if data present in the buffer, lock the buffer and send packets
		pthread_mutex_lock(&sender_mutex);

		max_bytes = current_cwnd - getdiff();	//max bytes that can be sent now
		bytes_remaining = unack_notsent - getdiff();	//bytes that actually needs to be sent
		actual_bytes = min(max_bytes,bytes_remaining);
		send_packets(actual_bytes);

		pthread_mutex_unlock(&sender_mutex);
	}

}



/**Functions called by receiver thread **/


/* function overloading */

// when timeout occurs

void update_window()
{
	
	// reset base pointer
	base_ptr_sender = curr_ptr_sender;

	// set the new ssthresh and cwnd values
	if(current_cwnd/2 < MSS_bytes)	ssthresh = MSS_bytes;
	else ssthresh = current_cwnd/2;
	current_cwnd = min(MSS_bytes,recv_advr_cwnd);

	if(ssthresh > current_cwnd)	slow_start = 1;
	else 	slow_start = 0;

	cout<<"\nTimeout occured\n";
	cout<<"\nNew ssthresh is :-"<<ssthresh;
	cout<<"\n Cwnd is :-"<<current_cwnd<<"\n";
	
	count_dup_ack = 0;
}


// flag = 1 new ACK received, flag = 2 dup ACK received

void update_window(int flag)	
{
	int temp;

	if(flag == 1)	//new ACK received
	{

		cout<<"\nNew ACK received. Seq no is :- "<<packet_data.seq_no<<"\n";
		
		// if cummulative ACK is received
		if(packet_data.seq_no - seq_no > getdiff())	base_ptr_sender = ( curr_ptr_sender  + (packet_data.seq_no - seq_no))%(max_window_size_1);
		
		// update values
		curr_ptr_sender = ( curr_ptr_sender  + (packet_data.seq_no - seq_no))%(max_window_size_1);
		unack_notsent-= (packet_data.seq_no - seq_no);
		seq_no = packet_data.seq_no;
		recv_advr_cwnd = packet_data.size;
		count_dup_ack = 0;

		//update current window
		if(slow_start)
		{
			current_cwnd = min(current_cwnd+MSS_bytes,recv_advr_cwnd);
			if(current_cwnd >= ssthresh)	slow_start = 0;
		}
		else 	current_cwnd = min(current_cwnd+(MSS_bytes*MSS_bytes)/current_cwnd,recv_advr_cwnd);
		
		cout<<"\n Cwnd is :-"<<current_cwnd<<"\n";
	}

	else if(flag == 2)	//dup ACK received
	{
		cout<<"\nDup ACK received. Seq no is :- "<<packet_data.seq_no<<"\n";

		//increase dup count
		count_dup_ack++;

		if(count_dup_ack == 3)
		{
			// reset base pointer
			base_ptr_sender = curr_ptr_sender;

			// set the new ssthresh and cwnd values
			if(current_cwnd/2 < MSS_bytes)	temp = MSS_bytes;
			else temp = current_cwnd/2;
			ssthresh = temp;
			current_cwnd = min(temp,recv_advr_cwnd);
			
			slow_start = 0;
			count_dup_ack = 0;
		}
	}
}





void create_packet()
{
	packet_data.seq_no = seq_no_recv;
	packet_data.type = 0;
	packet_data.size = max_window_size_2 - diff2();
}

void send_ack()
{	
	// check the maximum byte number received and send cumulative ACK
	while(bitmap_recv_buff[curr_ptr_recver] == true)
	{
		curr_ptr_recver = (curr_ptr_recver + 1)%(max_window_size_2);
		seq_no_recv+=1;
	}

	cout<<"\nACK packet being sent. Seq no is :-"<<seq_no_recv<<"\n";
	create_packet();
	n = sendto(sockfd, &packet_data, sizeof(packet), 0, (struct sockaddr *) &receiveraddr, receiverlength);
}



void recv_buffer_handle()
{
	int receiver_window,bytes_stored,temp,difference;
	
	
	if(packet_data.seq_no == seq_no_recv)
	{
		cout<<"\nOrdered packet received. Seq no is :- "<<packet_data.seq_no<<"\n";
		receiver_window = max_window_size_2 - diff2();

		//store maximum possible bytes in the buffer
		bytes_stored = min(receiver_window,packet_data.size);
		temp = max_window_size_2 - curr_ptr_recver;
		
		if(bytes_stored > temp)
		{
			memcpy(recver_queue+curr_ptr_recver,packet_data.data,temp);
			update_bit_map(curr_ptr_recver,temp);	
			memcpy(recver_queue,packet_data.data+temp,bytes_stored-temp);
			update_bit_map(0,bytes_stored-temp);
		}
		else
		{
			memcpy(recver_queue+curr_ptr_recver,packet_data.data,bytes_stored);
			update_bit_map(curr_ptr_recver,bytes_stored);
		}
	}
	else if(packet_data.seq_no > seq_no_recv)
	{
		cout<<"\nOut of order packets received. Seq no. is :- "<<packet_data.seq_no<<"\n";
		
		difference = packet_data.seq_no - seq_no_recv;
		receiver_window = max_window_size_2 - diff2() - difference;
		
		if(receiver_window <= 0)	return;	//overflow

		//store maximum possible bytes in the buffer
		bytes_stored = min(receiver_window,packet_data.size);
		temp = max_window_size_2 - (curr_ptr_recver + difference)%(max_window_size_2);

		if(bytes_stored > temp)
		{
			memcpy(recver_queue+curr_ptr_recver+difference,packet_data.data,temp);
			update_bit_map(curr_ptr_recver+difference,temp);
			memcpy(recver_queue,packet_data.data+temp,bytes_stored-temp);
			update_bit_map(0,bytes_stored-temp);
		}
		else
		{
			memcpy(recver_queue+(curr_ptr_recver+difference)%(max_window_size_2),packet_data.data,bytes_stored);
			update_bit_map(curr_ptr_recver+difference,bytes_stored);
		}
	}
	
	send_ack();
	
}

//function to know type of packet received
void * parse_packet(void *useless)
{
	int n;
	srand(time(0));

    while(1)
    {
	
    	n = recvfrom(sockfd, &packet_data, sizeof(packet), 0,(struct sockaddr *) &receiveraddr, &receiverlength);

    	// for timeout
	    if (n < 0)
	    {
	      if(curr_ptr_sender != base_ptr_sender)	//timeout has occured
	      {
	      	pthread_mutex_lock(&sender_mutex);
	      	update_window();
	      	pthread_mutex_unlock(&sender_mutex);
	      }
	  	}

	  	//ACK packet received
	  	else if(packet_data.type == 0)	
	  	{

	  		if(packet_data.seq_no > seq_no)
	  		{
	  			pthread_mutex_lock(&sender_mutex);
	  			update_window(1);	//new ACK received
	  			pthread_mutex_unlock(&sender_mutex);
	  		}
	  		else if(packet_data.seq_no == seq_no)
	  		{
	  			pthread_mutex_lock(&sender_mutex);
	  			update_window(2);	//DUP ACK received
	  			pthread_mutex_unlock(&sender_mutex);
	  		}
	  	}

	  	//Data packet received
	  	else if(packet_data.type == 1) 
	  	{
	  		// cout<<"\nData packet received\n";
	  		
	  		if((double)rand()/(double)RAND_MAX < 0.01)
	  		{
	  			cout<<"\nDropping packet\n";
	  			continue;
	  		}

	  		recv_buffer_handle();

	  	}

	  	//SYN packet received
	  	else if(packet_data.type == 2 && packet_data.seq_no == 0) 
	  	{
	  		if(servermode)
	  		{
	  			filesize = packet_data.size;
		  		make_connection_packet();
		  		n = sendto(sockfd, &packet_data, sizeof(packet), 0, (struct sockaddr *) &receiveraddr, receiverlength);
		  		cout<<"\nConnection established ...Port no is :- "<<ntohs(receiveraddr.sin_port) <<" IP is :-"<<inet_ntoa(receiveraddr.sin_addr)<<"\n";
	  		}
	  		else
	  		{
	  			cout<<"\nConnection established ...Port no is :- "<<ntohs(receiveraddr.sin_port) <<" IP is :-"<<inet_ntoa(receiveraddr.sin_addr)<<"\n";	
	  		}
	  		setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(struct timeval));
	  	}

	  	// FIN packet received
	  	else if(packet_data.type == 3)	
	  	{

	  	}
	  	
	  	else  error("\nPacket Error. Dropping the packet\n");
	}
  	pthread_exit(NULL);
}	


/**Functions called by the App process **/

int send_buffer_handle(char *data,int size)
{
	int temp,rem_space;
	// if size of data is > max_window_size return -1
	if(size>max_window_size_1)	return -1;
	// wait till the queue becomes empty
	while(size > max_window_size_1 - unack_notsent);

	//store the data in the queue
	pthread_mutex_lock(&sender_mutex);
	temp = (curr_ptr_sender  + unack_notsent)%(max_window_size_1);
	rem_space = max_window_size_1 - temp;
	if( size > rem_space)
	{
		cout<<"\nBuffer overflow.\n";
		cout<<"\nStoring data from "<<temp<<" to "<<temp+rem_space<<" and 0 to "<<size-rem_space;
		memcpy(sender_queue+temp, data, rem_space);
		memcpy(sender_queue,data + rem_space, size - rem_space);
	}
	else
	{
		cout<<"\nStoring data from "<<temp<<" to "<<temp+size;
		memcpy(sender_queue+temp, data, size);
	}

	// cout<<"\nThe starting index is :-"<<temp<<"and size is :-"<<size<<"\n";
	unack_notsent+=size;
	pthread_mutex_unlock(&sender_mutex);
	return 0;		
}


int app_Send(char *data,int size)
{
	int flag;
	flag = send_buffer_handle(data,size);
	return flag;
}


int app_Recv(char *data,int size)
{
	int size_copied = 0;
	int temp;
	
	// pthread_mutex_lock(&receiver_mutex);
	size_copied = min(diff2(),size);
	temp = max_window_size_2 - not_read;
	if(temp >= size_copied)
	{
		// cout<<"\nMemeory accessesed from "<<not_read<<" to "<<not_read + size_copied<<"\n";


		// pthread_mutex_lock(&receiver_mutex);
		memcpy(data,recver_queue+not_read,size_copied);
		update_bit_map_2(not_read,size_copied);
		not_read = not_read + size_copied;
		// pthread_mutex_unlock(&receiver_mutex);
	}
	else
	{
		// cout<<"\nMemeory accessesed from "<<not_read<<" to "<<size_copied-temp<<"\n";
		// pthread_mutex_lock(&receiver_mutex);
		memcpy(data,recver_queue+not_read,temp);
		update_bit_map_2(not_read,temp);
		not_read = 0;
		memcpy(data+temp,recver_queue,size_copied-temp);
		update_bit_map_2(0,size_copied-temp);
		not_read+=size_copied-temp;	
		// pthread_mutex_unlock(&receiver_mutex);
	}
	// total_bytes+=size_copied;
	// cout<<"\nTotal bytes received :- "<<total_bytes<<"\n";
	// pthread_mutex_unlock(&receiver_mutex);
	return size_copied;
}



void initialize_threads()
{
	int l;
	l = pthread_create(&sender, NULL, rate_control,(void *)NULL);
	if(l)error("couldn't create thread");

	l = pthread_create(&receiver,NULL,parse_packet,(void *)NULL);
	if(l)error("couldn't create thread");
}



// initialize server on the given port
void init_server(char *addr[])
{
	int port_no;
	//packet *packet_data;
	port_no = atoi(addr[1]);
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	servermode = 1;

	//packet_data = (packet *)malloc(sizeof(packet));
	receiverlength = sizeof(receiveraddr);
	optval = 1;
  	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));
  	
	/*
	 * build the server's Internet address
	*/
	bzero((char *) &senderaddr, sizeof(senderaddr));
	senderaddr.sin_family = AF_INET;
	senderaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	senderaddr.sin_port = htons((unsigned short)port_no);

	/* 
	 * bind: associate the parent socket with a port 
	 */
	if (bind(sockfd, (struct sockaddr *) &senderaddr, sizeof(senderaddr)) < 0) 
	 	error("ERROR on binding");

	cout<<"\nServer is up at port no :-"<<ntohs(senderaddr.sin_port)<<"\n";

	// cout<<"\nhello\n";
    initialize_threads();

    

}




void init_client(char *addr[])	
{
	char *hostname;
	int port_no;
	//packet *packet_data;
	struct hostent *server;

	receiverlength = sizeof(receiveraddr);
	hostname = addr[1];
	port_no = atoi(addr[2]);
	//packet_data = (packet *)malloc(sizeof(packet));
	servermode = 0;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(struct timeval));

    if (sockfd < 0) 
        error("ERROR opening socket");

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }

    /* build the server's Internet address */
    bzero((char *) &receiveraddr, sizeof(receiveraddr));
    receiveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
	  (char *)&receiveraddr.sin_addr.s_addr, server->h_length);
    receiveraddr.sin_port = htons(port_no);

    make_connection_packet();

    // cout<<"\nhello\n";
    // cout<<"\n"<<packet_data.type<<" "<<packet_data.seq_no<<"\n";
    n = sendto(sockfd, &packet_data, sizeof(packet), 0, (struct sockaddr *) &receiveraddr, receiverlength);
    // cout<<"\nConnection established ...Port no is :- "<<ntohs(receiveraddr.sin_port) <<"IP is :-"<<inet_ntoa(receiveraddr.sin_addr);
    // cout<<"\nhello\n";
	initialize_threads();
    
    
}


void init_bitmap()
{
	int i;
	for(i=0;i<max_window_size_2;i++)	bitmap_recv_buff[i] = false;
}

int main(int argc, char *argv[])
{
	int size;
	FILE *fp;
	int n;
	int tot_size = 0;

	char buff[1024];
	init_bitmap();
	if(argc == 2)
	{
		init_server(argv);
		fp = fopen("out.mp4","w+");

		while(tot_size != filesize)
		{
			// cout<<"\nTot size is :- "<<tot_size<<" "<<"File size is :- "<<filesize<<"\n";
			size = app_Recv(buff,1024);
			fwrite(buff,1,size,fp);
			tot_size += size;
		}
		fclose(fp);
		pthread_kill(receiver,SIGTERM);
		pthread_kill(sender,SIGTERM);
	}
	else if(argc == 3)
	{
		fp = fopen("abc.mp4","r");
		fseek(fp, 0, SEEK_END); 
   		filesize = ftell(fp); 
    	fseek(fp, 0, SEEK_SET);
		init_client(argv);
		while(!feof(fp))
		{
			size = fread(buff,1,1024,fp);

			app_Send(buff,size);
		}
		while(seq_no != filesize);

		
		fclose(fp);
		pthread_kill(receiver,SIGTERM);
		pthread_kill(sender,SIGTERM);
		

		

	}
	else 
	{
		cout<<"\nWrong parameters given\n";
		exit(0);
	}

	pthread_exit(NULL);
	return 0;
}