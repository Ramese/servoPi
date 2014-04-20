/*
Testovaci vlakno 

vypisuje casy VELIKOST krat
*/

#include <stdio.h>
/*#include <unistd.h>*/
#include <pthread.h>
#include <sched.h>
#include <time.h>

#define MS	1000000
#define VELIKOST	1000

typedef struct {
	struct timespec time;
	int number;
} log;

void logFile(char *mess){
	FILE *f;
	f=fopen("/media/ramdisk/log_vlakno.txt", "a");
	fprintf(f, "%s\n", mess);
	fclose(f); 
}

void printFile(struct timespec time, int count){
	FILE *f;
	f=fopen("/media/ramdisk/print_vlakno.txt", "a");
	fprintf(f, "%10i s %10ld ns x%2i\n", (int)time.tv_sec, time.tv_nsec, count);
	fclose(f); 
}

static void setprio(int prio, int sched) {
   	struct sched_param param;
/*   	 Set realtime priority for this thread*/
   	param.sched_priority = prio;
   	if (sched_setscheduler(0, sched, &param) < 0)
   		perror("sched_setscheduler");
}

/*struct timespec getTimeDif(struct timespec start, struct timespec now){
	struct timespec dif;
	dif.tv_sec = start.tv_sec - now.tv_sec;
	if(dif.tv_sec > 0){
		dif.tv_nsec = 1000000000 - start.tv_nsec + now.tv_nsec;
	}else{
		dif.tv_nsec = start.tv_nsec - now.tv_nsec;
	}
	return dif;
}

struct timespec getTimeRemain(struct timespec lost){
	struct timespec rem;
	if(lost.tv_sec > 0 || lost.tv_nsec > 1000000){
		logFile("lost bigger then 1 sec\0");
		rem.tv_sec = 0;
		rem.tv_nsec = 0;
	}else{
		rem.tv_sec = 0;
		rem.tv_nsec = MS - lost.tv_nsec;
	}
	return rem;
}*/

void vypis(log pole[], int velikost){
	int i;
	for(i = 0; i < velikost; i++){
		printf("%10i s %10li ns x%3i\n", (int)pole[i].time.tv_sec, pole[i].time.tv_nsec, pole[i].number);
	}
} /* vypis */

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

void *thread_func(void *arg){
	const struct timespec period = {0, MS};
	struct timespec time_to_wait;

	int count = 0;
	log zaznam[VELIKOST];
	clock_gettime(CLOCK_MONOTONIC,&time_to_wait);
	setprio(95, SCHED_FIFO);
	while(count < VELIKOST) {	  
		timespec_add(&time_to_wait,&time_to_wait,&period);
		zaznam[count].time = time_to_wait;
		zaznam[count].number = count;
		++count;
		printFile(time_to_wait, count);
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &time_to_wait,NULL);		
	}
	
	vypis(zaznam, VELIKOST);
	return 0;

} /* thread_func */

int main(){
	pthread_t thr;
	pthread_attr_t attr;
	
   	if (pthread_attr_init(&attr)){
   		puts("Chyba pthread_attr_init");
   		return 1;
   	}
   	pthread_create(&thr, &attr, &thread_func, NULL);
	
   	pthread_join(thr, NULL);
	
	return 0;
} /* main */
