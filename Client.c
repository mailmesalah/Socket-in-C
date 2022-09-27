#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h> 

#define BUF_SIZE 128

int main(int argc, char *argv[])
{
	//checks if corrent number of arguments
	if (argc != 5) {
		printf("Usage: ./Client <PORT_NUMBER> [SL]|[AL] <NUM> [N]|[F]\n");
		return 1;
	}

	int portNo =atoi(argv[1]);
	char* optionV =argv[2];//option for Segment or ACK loss
	int optionNum =atoi(argv[3]);//value for Segment or ACK loss
	char* retransOption =argv[4];//Retransmission Option, F for fast retransmission and N for Timed out retransmission

	int SegAckLoss=-1;
	if(strcmp(optionV,"SL")==0){//checks if Segment Loss
		SegAckLoss=0;
	}else if(strcmp(optionV,"AL")==0){//checks if ACK loss 
		SegAckLoss=1;
	}
	
	//Create file where data will be stored 
	FILE *fp = fopen("output.txt", "wb+");
	if (NULL == fp)
	{
		printf("Error opening file");
		return 1;
	}

	//Create a socket first
	int sockfd = 0;
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0))< 0)
	{
		printf("\n Error : Could not create socket \n");
		return 1;
	}

	//Initialize sockaddr_in data structure 
	struct sockaddr_in serv_addr;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portNo); // port
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); //providing IP Address

	//Attempt a connection 
	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0)
	{
		printf("\n Error : Connect Failed \n");
		return 1;
	}


	//Receive data in chunks of BUF_SIZE bytes
	int bytesReceived = 0;
	char buff[BUF_SIZE];
	memset(buff, '0', sizeof(buff));//allocate memory

	
	int bSize=0;//size of bytes
	//int index=0;//index for adding buff data to segment
	int totalByte =0;
	int checked =0;//For skiping SL/AL loss after checked once
	int count =0;
	if(strcmp(retransOption,"N")==0){
		while ((bytesReceived = read(sockfd, buff, BUF_SIZE)) > 0)
		{
			totalByte+=bytesReceived;
			//segment loss
			if((SegAckLoss==0 || SegAckLoss==1) && totalByte>optionNum && checked==0 && strcmp(retransOption,"N")==0){//normal retransmission
		
				totalByte-=bytesReceived;
				checked=1;//set as already checked
				printf("Segment Or ACK Loss\n");
				continue;//skip any code below until loop end
			}			

			fwrite(&buff, 1, bytesReceived, fp);// Save the segment to file

			write (sockfd, &totalByte, sizeof(totalByte));//Sending the segment no as ACK
			printf("ACK %d\n", totalByte);

		}
	}
	
	int nextSeg=0;
	unsigned char bData[100][BUF_SIZE];
	int bDataLen[100];
	int in=0;
	int lossIndex=-1;
	if(strcmp(retransOption,"F")==0){
		while ((bytesReceived = read(sockfd, buff, BUF_SIZE)) > 0)
		{

			totalByte+=bytesReceived;
			//segment loss
			if((SegAckLoss==0 || SegAckLoss==1) && totalByte>optionNum && checked<3 && strcmp(retransOption,"F")==0){//Fast retransmission
			
				
				//Buffering
				bDataLen[in]=bytesReceived;
				for(int k=0;k<bytesReceived;++k){				
					bData[in][k]=buff[k];				
				}
				++in;

				lossIndex=in-1;
				checked++;

				//Fast Retransmission
				write (sockfd, &nextSeg, sizeof(nextSeg));//Sending the segment no as ACK
				printf("Segment Or ACK Loss\n");
				continue;//skip any code below until loop end					
			}else if(checked==3){
				//resend received
				//Buffering
				bDataLen[lossIndex]=bytesReceived;
				for(int k=0;k<bytesReceived;++k){
					bData[lossIndex][k]=buff[k];
				}
				totalByte-=bytesReceived;
				checked=4;
				write (sockfd, &totalByte, sizeof(totalByte));//Sending the segment no as ACK
				printf("ACK %d\n", totalByte);
				continue;//skip any code below until loop end					
			}else{
				
				//Buffering
				bDataLen[in]=bytesReceived;
				for(int k=0;k<bytesReceived;++k){				
					bData[in][k]=buff[k];				
				}
				++in;
			}
			

			nextSeg+=128;
			write (sockfd, &totalByte, sizeof(totalByte));//Sending the segment no as ACK
			printf("ACK %d\n", totalByte);
			
		}
		
		for(int k=0;k<in;++k){			
			fwrite(&bData[k], 1, bDataLen[k], fp);// Save the segment to file
		}
	}
	

	fclose(fp);//close the file
	
	if (bytesReceived < 0)
	{
		//printf("\n Read Error \n");
	}
	return 0;
}