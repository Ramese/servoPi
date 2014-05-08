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

#define NS	1
#define US	1000
#define MS	1000000
#define S	1000000000
#define velikostPole	55000
#define SOUBOR	"vystup.csv"

int kladne[velikostPole];
int zaporne[velikostPole];
int nula = 0;

void vypisDoSouboru(){
	FILE *f;
	int i;
	f = fopen(SOUBOR, "w");
	
	for(i=0; i<velikostPole; i++){
		if(zaporne[velikostPole - i -1] != 0){
			fprintf(f, "%i, %i\n", -(velikostPole)+i, zaporne[velikostPole - i -1]);
		}
	}
	fprintf(f, "%i, %i\n", 0, nula);
	for(i=0; i<velikostPole; i++){
		if(kladne[i] != 0){
			fprintf(f, "%i, %i\n", i+1, kladne[i]);
		}
	}
	
	fclose(f);
	
}

void vynulujPole(int pole[], int velikost){
	int i;
	for(i = 0; i < velikost; ++i){
		pole[i] = 0;
	}
}

/*
pouze pro kladne pricitani casu !!!
*/
void sectiCasy(struct timespec *t1, struct timespec t2){
	t1->tv_sec += t2.tv_sec;
	t1->tv_nsec += t2.tv_nsec;
	
	if(t1->tv_nsec >= S){
		++t1->tv_sec;
		t1->tv_nsec -= S;
	}
}

/*
vraci kladné i zaporne rozdily v nanosekundach
*/
int rozdilCasuNano(struct timespec t1, struct timespec t2){
	int vystup;
	vystup = (t2.tv_sec - t1.tv_sec);
	vystup *= S;
	if(vystup == 0){
		vystup = t2.tv_nsec - t1.tv_nsec;
	}else if(vystup > 0){
		vystup += t2.tv_nsec;
		vystup += (S - t1.tv_nsec);
		vystup -= S;
	}else{
		vystup -= t1.tv_nsec;
		vystup -= (S - t2.tv_nsec);
		vystup += S;
	}
	return vystup;
}

void *thread_vlakno(void *arg){
	struct sched_param param;
	const struct timespec period = {0, MS};
	struct timespec time_to_wait;
	struct timespec time_pom, time_last;
	int i = 0;
	int pom = 0;
	
   	param.sched_priority = 98;
   	
   	if (sched_setscheduler(0, SCHED_FIFO, &param) < 0)
   		perror("sched_setscheduler");
   	
   	puts("Vlakno nastaveno na prioritu 98");
   	
   	clock_gettime(CLOCK_MONOTONIC,&time_to_wait);
	clock_gettime(CLOCK_MONOTONIC, &time_last);
	while(i < 60*1000*10) {	  
		clock_gettime(CLOCK_MONOTONIC, &time_pom);
		pom = rozdilCasuNano(time_last, time_pom);
		pom = MS - pom;
		if(pom == 0){
			nula++;
		}else if(pom > 0){
			if(pom-1 >= velikostPole){
				kladne[velikostPole-1]++;
			}else{
				kladne[pom-1]++;
			}
		}else{
			if(pom*(-1)-1 >= velikostPole){
				zaporne[velikostPole-1]++;
			}else{
				zaporne[pom*(-1)-1]++;
			}
		}
		
		
		time_last = time_pom;
		sectiCasy(&time_to_wait,period);
 		i++;
 		printf("\r%03i procent", i/6000);
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &time_to_wait,NULL);		
		
	}
	puts("");
	vypisDoSouboru();
	
	return 0;

} /* thread_controller */

int main(void){
	pthread_t vlakno;
	pthread_attr_t vlakno_attr;
	vynulujPole(kladne, velikostPole);
	vynulujPole(zaporne, velikostPole);
	puts("Start test vlakna");
	if (pthread_attr_init(&vlakno_attr)){
   		puts("Chyba pthread_attr_init");
   		return 1;
   	}
   	pthread_create(&vlakno, &vlakno_attr, &thread_vlakno, NULL);
	pthread_join(vlakno, NULL);

	return 0;
}
