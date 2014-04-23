#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
/*#include <inttypes.h>*/
#include <pthread.h>
#include <sched.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <arpa/inet.h>

#define MS 1000000

void timespec_add (struct timespec *sum, const struct timespec *left, const struct timespec *right) {
  sum->tv_sec = left->tv_sec + right->tv_sec;
  sum->tv_nsec = left->tv_nsec + right->tv_nsec;

  if (sum->tv_nsec >= 1000000000){
     ++sum->tv_sec;
     sum->tv_nsec -= 1000000000;
  }
} /* timespect_add*/

int main(void){
	FILE *soubor;
	int sockfd;
	int len;
	int result;
	FILE *stream;
	struct sockaddr_in address;
	const struct timespec period = {0, 50*MS};
	struct timespec time_to_wait;
	struct timespec ts;
	uint32_t pom;
	int32_t pom1;
	if((soubor = fopen("/dev/irc0", "r")) == NULL){
		printf("chyba otevreni souboru /dev/irc0\n");
		return 0;
	}
	
	/* vytvoření socketu pro klienta */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
	/* pojmenujte socket podle serveru */
	address.sin_family = AF_INET;
/*	address.sin_addr.s_addr = inet_addr("192.168.1.23");*/
	address.sin_addr.s_addr = inet_addr("10.42.0.1");
	address.sin_port = htons(9051);
	len = sizeof(address);
	
	result = connect(sockfd, (struct sockaddr *)&address, len);
	
	if(result == -1){
		perror("opps: klient");
		exit(1);
	}
	stream=fdopen(sockfd,"w");
	puts("Nastavuji prioritu odesilacimu procesu 49");
/*	setprio(50, SCHED_FIFO);*/
	clock_gettime(CLOCK_MONOTONIC,&time_to_wait);
	while(1) {
		clock_gettime(CLOCK_MONOTONIC,&ts);
		if(fread(&pom,1,sizeof(uint32_t),soubor) == -1){
			printf("chyba pri cteni ze souboru /dev/irc0\n");
			fclose(soubor);
			return 0;
		}
		timespec_add(&time_to_wait,&time_to_wait,&period);
		
/*		pom = (int32_t)pozice;*/
		fwrite(&pom, sizeof(int32_t), 1, stream);
		pom1 = (int32_t)ts.tv_sec;
		fwrite(&pom1, sizeof(int32_t), 1, stream);
		pom1 = (int32_t)ts.tv_nsec;
		fwrite(&pom1, sizeof(int32_t), 1, stream);
		//fwrite(&newLine, sizeof(char), 1, stream);
		fflush(stream);
		
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &time_to_wait,NULL);
		
	}
	close(sockfd);
	fclose(stream);
/*	exit(0);*/
	return 0;
}
