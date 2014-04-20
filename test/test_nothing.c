#include <stdio.h>
/*#include <unistd.h>*/
#include <pthread.h>
#include <sched.h>
#include <time.h>
#include <stdint.h>

#define MS	1000000

int prumer(int pole[]){
	int i;
	int sum = 0;
	for(i = 0; i < 100; i++){
		sum += pole[i];
	}
	
	return sum/100;
}

void timespec_add (struct timespec *sum, const struct timespec *left,
	      const struct timespec *right)
{
  sum->tv_sec = left->tv_sec + right->tv_sec;
  sum->tv_nsec = left->tv_nsec + right->tv_nsec;

  if (sum->tv_nsec >= 1000000000){
     ++sum->tv_sec;
     sum->tv_nsec -= 1000000000;
  }
} /* timespect_add*/

int main(void){
	const struct timespec period = {0, MS};
	struct timespec time_to_wait;
	struct timespec time_save;
	FILE *soubor;
	uint32_t pom, last;
	int i = 0;
	int pole[100];
	if((soubor = fopen("/dev/irc0", "r")) == NULL){
		printf("chyba otevreni souboru /dev/irc0\n");
		return 0;
	}
	
	clock_gettime(CLOCK_MONOTONIC,&time_to_wait);
	clock_gettime(CLOCK_MONOTONIC,&time_save);
	while(1){
		timespec_add(&time_to_wait,&time_to_wait,&period);
		
		if(i == 100){
			i = 0;
		}
		
		if(fread(&pom,1,sizeof(uint32_t),soubor) == -1){
			printf("chyba pri cteni ze souboru /dev/irc0\n");
			fclose(soubor);
			return 0;
		}
		pole[i] = (int)(pom-last);
		printf("act: %5i avg: %5i %12i %10li\r", pole[i], prumer(pole), (int)(time_to_wait.tv_sec-time_save.tv_sec), (time_to_wait.tv_nsec - time_save.tv_nsec));
		
		last = pom;
		time_save.tv_sec = time_to_wait.tv_sec;
		time_save.tv_nsec = time_to_wait.tv_nsec;
		i++;
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &time_to_wait,NULL);
	}

}
