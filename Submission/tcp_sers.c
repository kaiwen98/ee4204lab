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
int send_ack(int sockfd, struct sockaddr *addr, int addrlen);
int compareFile(char path1[], char path2[], int * line, int * col);

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
	int sockfd, ret;
	struct sockaddr_in my_addr;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);          //create socket
	if (sockfd <0)
	{
		printf("error in socket!");
		exit(1);
	}
	
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(MYUDP_PORT);
	my_addr.sin_addr.s_addr = INADDR_ANY;//inet_addr("172.0.0.1");
	bzero(&(my_addr.sin_zero), 8);
	ret = bind(sockfd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr));                //bind socket
	if (ret <0)
	{
		printf("error in binding");
		exit(1);
	}

	int count = 0;
	while (1)
	{
		printf("waiting for data\n");


		str_ser(sockfd, DATALEN_ARR[trial_num]);                                          //receive packet and response

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
	
	struct sockaddr_in addr;
	int len = sizeof (struct sockaddr_in);
	printf("receiving data!\n");

	char file_in[100] = "myfile.txt";
	char file_out[100] = "myTCPreceive.txt";
	int line, col;
	while(!end)
	{
		// Simulates probability of failure of transmission by (10-PROB)/10 chance of failure
		if (is_simulated_failure()) {
			continue;
		}
		printf("hi\n");
		n= recvfrom(sockfd, &recvs, DATALEN_ARR[trial_num], 0, (struct sockaddr *)&addr, &len);

		if (n==-1)                                   //receive the packet
		{
			printf("error when receiving\n");
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
		while (!send_ack(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)));
	}
	

	if ((fp = fopen ("myTCPreceive.txt","wt")) == NULL)
	{
		printf("File doesn't exit\n");
		exit(0);
	}
	fwrite (buf , 1 , lseek , fp);					//write data into file
	fclose(fp);

	if (compareFile(file_in, file_out, &line, &col) != 0) {
		printf("Diff found in line %d col %d!!!!\n", line, col);
	} else {
		printf("No file loss!!!\n");
	}
	printf("a file has been successfully received!\nthe total data received is %d bytes\n", (int)lseek);
}

int send_ack(int sockfd, struct sockaddr *addr, int addrlen) {
	// After receiving a frame, send an acknowledgement.
	struct ack_so ack;
	ack.num = 1;
	ack.len = 0;
	int n;
	if ((n = sendto(sockfd, &ack, 2, 0, addr, addrlen))==-1) {
		printf("send error!");
		return 0;
	} 
	else return 1;
}

int compareFile(char path1[], char path2[], int * line, int * col)
{
    char ch1, ch2;

    *line = 1;
    *col  = 0;

	/*  Open all files to compare */
    FILE *fPtr1 = fopen(path1, "r");
    FILE *fPtr2 = fopen(path2, "r");

    /* fopen() return NULL if unable to open file in given mode. */
    if (fPtr1 == NULL || fPtr2 == NULL)
    {
        /* Unable to open file hence exit */
        printf("\nUnable to open file.\n");
        printf("Please check whether file exists and you have read privilege.\n");
        exit(EXIT_FAILURE);
    }

    do
    {
        // Input character from both files
        ch1 = fgetc(fPtr1);	
        ch2 = fgetc(fPtr2);
        
        // Increment line 
        if (ch1 == '\n')
        {
            *line += 1;
            *col = 0;
        }

        // If characters are not same then return -1
        if (ch1 != ch2)
            return -1;

        *col  += 1;

    } while (ch1 != EOF && ch2 != EOF);


    /* If both files have reached end */
    if (ch1 == EOF && ch2 == EOF)
        return 0;
    else
        return -1;
}