/**********************************
tcp_ser.c: the source file of the server in tcp transmission 
***********************************/


#include "headsock.h"

#define BACKLOG 10

#define PROB_FAIL 0.9

#include <time.h>
#include <stdlib.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif
#include <stdbool.h>

extern int DATALEN_ARR[];
extern float ERRPROB_ARR[];
extern volatile int trial_num; 
void str_ser(int sockfd, int datalen);                                                        
int send_ack(int sockfd);

int is_simulated_failure() {
	float prob = ERRPROB_ARR[trial_num];
	printf("\nerrprob is %f\ntrialnum is %d\n", prob, trial_num);
	int r = rand() % 100000;      // Returns a pseudo-random integer between 0 and 9.
	
	if (r < (prob * 100000)) {
		printf("Time out due to error...\n");
		return 1;
	}
	else {
		return 0;
	}
}

int main(void)
{
	srand(time(NULL));   // Initialization, should only be called once.
	int sockfd, con_fd, ret;
	struct sockaddr_in my_addr;
	struct sockaddr_in their_addr;
	int sin_size;

//	char *buf;
	pid_t pid;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);          //create socket
	if (sockfd <0)
	{
		printf("error in socket!");
		exit(1);
	}
	
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(MYTCP_PORT);
	my_addr.sin_addr.s_addr = htonl(INADDR_ANY);//inet_addr("172.0.0.1");
	bzero(&(my_addr.sin_zero), 8);
	ret = bind(sockfd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr));                //bind socket
	if (ret <0)
	{
		printf("error in binding");
		exit(1);
	}
	
	ret = listen(sockfd, BACKLOG);                              //listen
	if (ret <0) {
		printf("error in listening");
		exit(1);
	}

	int count = 0;
	while (1)
	{
		printf("waiting for data\n");
		sin_size = sizeof (struct sockaddr_in);
		con_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);            //accept the packet
		if (con_fd <0)
		{
			printf("error in accept\n");
			exit(1);
		}

		if ((pid = fork())==0)                                         // creat acception process
		{
			close(sockfd);
			str_ser(con_fd, DATALEN_ARR[trial_num]);                                          //receive packet and response
			close(con_fd);
			exit(0);
		}
		else close(con_fd);                                         //parent process

		count++;
		if (count >= EXPERIMENT_REPEAT_NUM) {
			trial_num++;
			count = 0;
		}
	}
	close(sockfd);
	exit(0);
}

void str_ser(int sockfd, int datalen)
{
	char buf[BUFSIZE];
	FILE *fp;
	char recvs[datalen];
	struct ack_so ack;
	int end, n = 0;
	long lseek=0;
	end = 0;
	int timeout;
	int count = 3;
	int random;
	int success = 0;
	
	printf("receiving data!\n");

	while(!end)
	{
		// Simulates probability of failure of transmission by (10-PROB)/10 chance of failure
		if (is_simulated_failure()) {
			continue;
		}

		n= recv(sockfd, &recvs, DATALEN_ARR[trial_num], 0);

		if (n==-1)                                   //receive the packet
		{
			printf("error when receiving\n");
			exit(1);
		}

		printf("Received byte length is %d\n", n);
		// If nothing is received, continue to poll

		if (recvs[n-1] == '\0')									//if it is the end of the file
		{
			end = 1;
			n--;
		}
		memcpy((buf+lseek), recvs, n);
		lseek += n;

		// After receiving a frame, send an acknowledgement.
		while (!send_ack(sockfd));
	}
	

	if ((fp = fopen ("myTCPreceive.txt","wt")) == NULL)
	{
		printf("File doesn't exit\n");
		exit(0);
	}
	fwrite (buf , 1 , lseek , fp);					//write data into file
	fclose(fp);
	printf("a file has been successfully received!\nthe total data received is %d bytes\n", (int)lseek);
}

int send_ack(int sockfd) {
	// After receiving a frame, send an acknowledgement.
	struct ack_so ack;
	ack.num = 1;
	ack.len = 0;
	int n;
	if ((n = send(sockfd, &ack, 2, 0))==-1) {
		printf("send error!");
		return 0;
	} 
	else return 1;
}