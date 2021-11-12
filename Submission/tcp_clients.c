/*******************************
tcp_client.c: the source file of the client in tcp transmission 
********************************/

#include "headsock.h"

float str_cli(FILE *fp, int sockfd, struct sockaddr *addr, int addrlen, long *len, int datalen);                       //transmission function
void tv_sub(struct  timeval *out, struct timeval *in);	    //calcu the time interval between out and in



extern int DATALEN_ARR[];
extern float ERRPROB_ARR[];
extern volatile int trial_num;
float ti_ls[EXPERIMENT_REPEAT_NUM];
float rt_ls[EXPERIMENT_REPEAT_NUM];

float *ti_avg_ls;
float *rt_avg_ls; 

const char* print_experiment_result = 
	"Experiment Trial %d, ( Error Prob:%.3f ), ( Data Unit:%.3d ):\n\t\tAvg. Transfer Time: %.3f, Avg. Throughput: %.3f\n\n";

void print_arr(float arr[], size_t size) {
	printf(" \n{");
	for (int i = 0; i < size; i++) {
		printf("%f, ", arr[i]);
	}
	printf(" }\n");
}

float mean(float arr[]) {
	float sum = 0;
	for (int i = 0; i < EXPERIMENT_REPEAT_NUM; i++) {
		sum += arr[i];
	}
	return sum/EXPERIMENT_REPEAT_NUM;
}

float harmonic_mean(float arr[]) {
	float sum = 0;
	for (int i = 0; i < EXPERIMENT_REPEAT_NUM; i++) {
		sum += 1.0/arr[i];
	}
	return EXPERIMENT_REPEAT_NUM/sum;
}

int main(int argc, char **argv)
{
	int sockfd, ret;
	float ti, rt;
	long len;
	struct sockaddr_in ser_addr;
	char ** pptr;
	struct hostent *sh;
	struct in_addr **addrs;
	FILE *fp;
	ti_avg_ls = calloc(MAX_TRIAL_NUM, sizeof(float));
	rt_avg_ls = calloc(MAX_TRIAL_NUM, sizeof(float));

	if (argc != 2) {
		printf("parameters not match");
		exit(0);
	}

	sh = gethostbyname(argv[1]);	                                       //get host's information
	if (sh == NULL) {
		printf("error when gethostby name");
		exit(0);
	}
	
	addrs = (struct in_addr **)sh->h_addr_list;
	printf("canonical name: %s\n", sh->h_name);					//print the remote host's information
	for (pptr=sh->h_aliases; *pptr != NULL; pptr++)
		printf("the aliases name is: %s\n", *pptr);
	switch(sh->h_addrtype)
	{
		case AF_INET:
			printf("AF_INET\n");
		break;
		default:
			printf("unknown addrtype\n");
		break;
	}
        
	

	ser_addr.sin_family = AF_INET;                                                      
	ser_addr.sin_port = htons(MYUDP_PORT);
	memcpy(&(ser_addr.sin_addr.s_addr), *addrs, sizeof(struct in_addr));
	bzero(&(ser_addr.sin_zero), 8);

	for (int j = 0; j < datalenarr_len; j++) {
		for (int i = 0; i < errprobarr_len; i++) {
			printf("\n>>>>>>>>>>>>> trialnum is %d, dl %d, ep %d\n", trial_num, j, i);
			for (int k = 0; k < EXPERIMENT_REPEAT_NUM; k++) {
				//connect the socket with the host
				//create the socket
				sockfd = socket(AF_INET, SOCK_DGRAM, 0);           
				if (sockfd <0)
				{
					printf("error in socket");
					exit(1);
				}                
				
				if((fp = fopen ("myfile.txt","r+t")) == NULL)
				{
					printf("File doesn't exit\n");
					exit(0);
				}
				//perform the transmission and receiving
				ti = str_cli(fp, sockfd, (struct sockaddr *)&ser_addr, sizeof(struct sockaddr_in), &len, DATALEN_ARR[j]);     
				//caculate the average transmission rate                  
				rt = (len/(float)ti);                                         
				printf("Time(ms) : %.3f, Data sent(byte): %d\nData rate: %f (Kbytes/s)\n", ti, (int)len, rt);
				close(sockfd);
				fclose(fp);

				ti_ls[k] = ti;
				rt_ls[k] = rt;
			}		
			print_arr(ti_ls, (float)(sizeof(ti_ls))/sizeof(float));
			print_arr(rt_ls, (float)(sizeof(rt_ls))/sizeof(float));
			ti_avg_ls[trial_num] = mean(ti_ls);
			rt_avg_ls[trial_num] = mean(rt_ls);
			trial_num ++;
		}
	}

	printf("\n\nExperiment Results\n===================\n");
	for (int j = 0; j < datalenarr_len; j++) {
		for (int i = 0; i < errprobarr_len; i++) {
			printf(print_experiment_result, j+i+1, ERRPROB_ARR[i], DATALEN_ARR[j], ti_avg_ls[j+i], rt_avg_ls[j+i]);
		}
	}
	printf("\n\nExcel Dump\n===================\n");
	for (int j = 0; j < MAX_TRIAL_NUM; j++) {
		printf("%.3f          %.3f\n",ti_avg_ls[j], rt_avg_ls[j]);
	}

	exit(0);
}

float str_cli(FILE *fp, int sockfd, struct sockaddr *addr, int addrlen, long *len, int datalen)
{
	char *buf;
	long lsize, ci;
	char sends[datalen];
	struct ack_so ack = {.num = 0, .len = 0};
	int n, slen;
	float time_inv = 0.0;
	struct timeval sendt, recvt;
	ci = 0;
	int timeout;

	fseek (fp , 0 , SEEK_END);
	lsize = ftell (fp);
	rewind (fp);
	printf("The file length is %d bytes\n", (int)lsize);
	printf("the packet length is %d bytes\n", datalen);
	
	// allocate memory to contain the whole file.
	buf = (char *) malloc (lsize);
	if (buf == NULL) exit (2);

  // copy the file into the buffer.
	fread (buf,1,lsize,fp);
  /*** the whole file is loaded in the buffer. ***/
	buf[lsize] ='\0';									//append the end byte
	gettimeofday(&sendt, NULL);							//get the current time
	while(ci<= lsize)
	{
		if ((lsize+1-ci) <= datalen)
			slen = lsize+1-ci;
		else 
			slen = datalen;
		memcpy(sends, (buf+ci), slen);
		
		n = sendto(sockfd, &sends, slen, 0, addr, addrlen);
		
		if(n == -1) {
			printf("send error!");								//send the data
			exit(1);
		}
		ci += slen;

		// Wait for acknowledgement before sending the next frame.
		ack.num = 0;
		ack.len = 0;
		struct sockaddr_in addr;
		int len = sizeof (struct sockaddr_in);
		while (!(((ack.num == 1) && (ack.len == 0))))
		{
			// printf("waiting for acknowledgement...\n");
			if ((n= recvfrom(sockfd, &ack, 2, 0, (struct sockaddr *)&addr, &len))==-1)                                   //receive the ack
			{
				printf("error when receiving\n");
				exit(1);
			}
			
			printf("Read: %ld\n", ci);
		}
	}

	gettimeofday(&recvt, NULL);
	*len= ci;                                                         //get current time
	tv_sub(&recvt, &sendt);                                           // get the whole trans time
	time_inv += (recvt.tv_sec)*1000.0 + (recvt.tv_usec)/1000.0;
	return(time_inv);
}

void tv_sub(struct  timeval *out, struct timeval *in)
{
	if ((out->tv_usec -= in->tv_usec) <0)
	{
		--out ->tv_sec;
		out ->tv_usec += 1000000;
	}
	out->tv_sec -= in->tv_sec;
}
