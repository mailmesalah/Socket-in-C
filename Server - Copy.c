#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h> 
#include <poll.h> 

#define BUF_SIZE 128

int main(int argc, char *argv[])
{	
	//Checks if the no of arguments are of correct
	if (argc != 4) {
		printf("Usage: ./Server <PORT_NUMBER> <WINDOW_SIZE(1-11)> <TIME_OUT(1-11)>>\n");
		return 1;
	}
	
	int portNo =atoi(argv[1]);
	int winSize =atoi(argv[2]);
	int timeOut =atoi(argv[3]);
	
	//Checking if windows size and timeout values are in correct range
	if(winSize<1||winSize>11){
		printf("Wrong Window Size <WINDOW_SIZE(1-11)>\n");
		return 1;
	}
	
	if(timeOut<1||timeOut>11){
		printf("Wrong Time Out <TIME_OUT(1-11)>\n");
		return 1;
	}
	
	//Creating server socket
	int listenfd = socket(AF_INET, SOCK_STREAM, 0);

	//Server Socket address and allocating its space
	struct sockaddr_in serv_addr;
	memset(&serv_addr, '0', sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(portNo);

	//binds server socket with the address data specified by the sockaddr_in structure variable
	bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

	//listens to the server socket
	if (listen(listenfd, 10) == -1)
	{
		printf("Failed to listen\n");
		return -1;
	}

	
	//Blocks still a client connects to the server		
	int connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);

	//Send window size to client first, usings clients socket id
	write (connfd, &winSize, sizeof(winSize));	
	
	//Open the file that we wish to transfer
	FILE *fp = fopen("input.txt", "rb");
	if (fp == NULL)
	{
		printf("File opern error");
		return 1;
	}

	//Read data from file and send it
	int segNo=0;
	for (;;)
	{
		//First read file in chunks of BUF_SIZE bytes X windows size 		
		unsigned char buff[BUF_SIZE*winSize];
		memset( buff, 0, BUF_SIZE*winSize*sizeof(unsigned char) );//allocates memory
		int nread = fread(buff, 1, BUF_SIZE*winSize, fp);//read data from the file with the size limit specifies by window size
		printf("Bytes read %d \n", nread);//prints the number of bytes read

		//If read was success, send data.
		if (nread > 0)
		{			
			for(int k=0;k<winSize;++k){
				printf("Segment No %d Sent\n",segNo);
				segNo+=128;
			}
			
			write(connfd, buff, nread); // sends data to client in connfd with data in buff with the size in nread			
		}

		/*
		* There is something tricky going on with read ..
		* Either there was error, or we reached end of file.
		*/
		if (nread < BUF_SIZE)
		{
			if (feof(fp))
				printf("End of file\n");
			if (ferror(fp))
				printf("Error reading\n");
			break;
		}
		
		
		//Timeout management		
		int length=0;//current length
		int lengthCount=0;//total length of a segment
		//while(nread>lengthCount){//loops till all the ACK of the segment last send is received
			
			struct pollfd fd;//for creating timeout
			int ret;//to store the value returned by the poll function
			int iError=0;// if error exit the loop
			fd.fd = connfd; // client socket handler 
			fd.events = POLLIN;			
			ret = poll(&fd, 1, (timeOut*1000)); // timeOut seconds for timeout
			
			//checks the return value for error time out or data from client
			switch (ret) {
				case -1:
					// Error
					printf("Error\n");
					iError=1;
					break;					
				case 0:
					// Timeout 
					printf("TimeOut Resending\n");
					lengthCount=0;
					write(connfd, buff, nread);//send the data again for timeout
					break;
				default:
					recv (connfd, &length, sizeof (length), 0); // get your data
					lengthCount+=length;
					printf("ACK = %d\n", length);
					if(length==0){
						//if length is 0 then its fast retransmission
						printf("Fast Retransmission Resending\n");
						lengthCount=0;
						write(connfd, buff, nread); // send the data again
						//iError=1;
					}
					break;
			}
			
			//if any error exit the loop
			if(iError){
				break;
			}
		//}		
		
	}
	fclose(fp);
	close(connfd);//close connection with client

}