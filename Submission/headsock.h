// headfile for TCP program
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>

#define NEWFILE (O_WRONLY|O_CREAT|O_TRUNC)
#define MYTCP_PORT 4950
#define MYUDP_PORT 5350
#define DATALEN 500
#define BUFSIZE 60000
#define PACKLEN 508
#define HEADLEN 8
#define EXPERIMENT_REPEAT_NUM 100

volatile int trial_num = 0;

int DATALEN_ARR[] = {
    250,
    500,
    750, 
    1000,
    1250,
    1500,
    1750,
    2000
};

float ERRPROB_ARR[] = {
    0.0,
    0.125,
    0.25,
    0.375,
    0.5,
    0.625,
    0.75,
    0.875,
};

const int datalenarr_len = sizeof(DATALEN_ARR)/sizeof(float);
const int errprobarr_len = sizeof(ERRPROB_ARR)/sizeof(float);
const int MAX_TRIAL_NUM = datalenarr_len * errprobarr_len;


// float ERRPROB_ARR[] = {
//     0.0001,
//     0.0005,
//     0.001,
//     0.005,
//     0.01,
//     0.05,
//     0.1,
//     0.5,
//     0.0,
//     0.0,
//     0.0,
//     0.0,
//     0.0,
//     0.0,
//     0.0,
//     0.0
// };

struct pack_so			//data packet structure
{
uint32_t num;				// the sequence number
uint32_t len;					// the packet length
char data[DATALEN];	//the packet data
};

struct ack_so
{
uint8_t num;
uint8_t len;
};
