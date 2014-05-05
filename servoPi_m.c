/*
servoPi
Autor: Radek Mečiar

*/
#define BASE		0x20000000
#define GPIO_BASE 	(BASE + 0x200000)
#define PWM_BASE	(BASE + 0x20C000)
#define CLK_BASE	(BASE + 0x101000)

#define PWM_CTL		*(pwm)
#define PWM_RNG1	*(pwm+4)
#define PWM_DAT1	*(pwm+5)

#define PWM_CLK_CNTL	*(clk+40)
#define PWM_CLK_DIV	*(clk+41)

#define LEFT		1
#define RIGHT		2
#define STOP		3
#define GPIO_PWM	18
#define GPIO_LEFT	14
#define GPIO_RIGHT	15

#define GPIO_IRC1	4
#define GPIO_IRC2	3
#define GPIO_IRQ	2

#define PAGE_SIZE 	(4*1024)
#define BLOCK_SIZE	(4*1024)

#define IN_PIN          8
#define IN_PIN2         9

#define MS		1000000

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

/*
#include <string.h>
#include <dirent.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
*/

int mem_fd;
void *gpio_map, *pwm_map, *clk_map;
volatile unsigned *gpio, *pwm, *clk;

static volatile int oldError = 14;
static volatile int integrator = 0;

volatile uint32_t pozice = 0;
volatile int orientace = 0;
volatile uint64_t pozadovanaPozice = 0;
volatile int64_t pozadovanaRychlost = 0;
/*volatile int direction = LEFT;*/

#define INP_GPIO(g) 		*(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) 		*(gpio+((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(g,a) 	*(gpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))

#define GPIO_SET 	*(gpio+7)
#define GPIO_CLR 	*(gpio+10)

void inicializacePameti() {
	if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
		printf("can't open /dev/mem\nroot access needed\n");
		exit(-1);
	}

	gpio_map = mmap(NULL, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, GPIO_BASE);

	if (gpio_map == MAP_FAILED) {
		printf("mmap error %d\n", (int)gpio_map);
		exit(-1);
	}
	
	gpio = (volatile unsigned *)gpio_map;
	
	pwm_map = mmap(NULL, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, PWM_BASE);

	if (pwm_map == MAP_FAILED) {
		printf("mmap error %d\n", (int)pwm_map);
		exit(-1);
	}
	
	pwm = (volatile unsigned *)pwm_map;
	
	clk_map = mmap(NULL, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, CLK_BASE);

	if (clk_map == MAP_FAILED) {
		printf("mmap error %d\n", (int)clk_map);
		exit(-1);
	}

   	clk = (volatile unsigned *)clk_map;
	
	close(mem_fd);

} /* inicializacePameti */

void inicializacePWM(){	
	INP_GPIO(GPIO_PWM);
	OUT_GPIO(GPIO_PWM);
	INP_GPIO(GPIO_PWM);
	SET_GPIO_ALT(GPIO_PWM, 5);
	/* pocatecni staveni - nefunguje bez toho */
	PWM_CTL = 0;
	/* vypne PWM */
	PWM_CLK_CNTL = (PWM_CLK_CNTL&~0x10)|0x5a000000;
	/* vypne hodiny */
	while(PWM_CLK_CNTL&0x80);
	/* pocka dokud BUSY neni 0 */
	PWM_CLK_DIV = 0x5a000000|(5<<12);
	/* nastaveni dalsi delicky */
	PWM_CLK_CNTL = 0x5a000016; 
	/* zapnout kanal a source je PLLD (500MHz) */
	while(!(PWM_CLK_CNTL&0x80));
	/* pocka dokud nenabehna BUSY na 1 */
	PWM_RNG1 = 4000;
	/* externi delic - pocet urovni */ 
	PWM_DAT1 = 0;   
	/* strida = 0 */
	PWM_CTL = 0x81; 
	/* zapnout MSEN=1 & ENA=1 */
} /* inicializace PWM */

void otaceni(int action){
	if(action == LEFT){
		/* zapne LEFT vypne RIGHT */
		GPIO_CLR = 1<<GPIO_RIGHT;
		GPIO_SET = 1<<GPIO_LEFT;
/*		puts("Otáčení do leva");*/
	}else if(action == RIGHT){
		GPIO_CLR = 1<<GPIO_LEFT;
		GPIO_SET = 1<<GPIO_RIGHT;
/*		puts("Otáčení do prava");*/
	}else if(action == STOP){
		GPIO_CLR = 1<<GPIO_RIGHT;
		GPIO_CLR = 1<<GPIO_LEFT;
/*		puts("Stop");*/
	}else{
		puts("Nedefinovaná akce");
	}
} /* otaceni */

void setHWPWM(int hodnota){
	if(hodnota < 0){
		otaceni(RIGHT*orientace);
		hodnota *= -1;
	}else if(hodnota > 0){
		otaceni(LEFT*orientace);
	}
	
	if(hodnota > 4000){
                PWM_DAT1 = 4000;
        }else if(hodnota < 0){
        	PWM_DAT1 = 0;
        }else{
                PWM_DAT1 = hodnota;
        }
} /* setHWPWM */

#define KON	2

void regulatorPID(int error){
        int akce = 0;
        int P = 80*KON, I = 1*KON, D = 6*KON;
        integrator += I*error;
/*        printf(" integratr %05i", integrator);*/
	if(integrator > 40000) {
		integrator = 40000;
	}
	
	if(integrator < -40000) {
		integrator = -40000;
	}
        akce = P * error + integrator + D*(oldError - error);
        printf("\r%08i", akce);
        oldError = error;
        setHWPWM(akce/10);
/*	printf("\r%5d %5d %5d", akce, oldError, integrator);*/
/*	printf(" akce %05i", akce);*/
} /* regulatorPID */

void timespec_add (struct timespec *sum, const struct timespec *left, const struct timespec *right) {
  sum->tv_sec = left->tv_sec + right->tv_sec;
  sum->tv_nsec = left->tv_nsec + right->tv_nsec;

  if (sum->tv_nsec >= 1000000000){
     ++sum->tv_sec;
     sum->tv_nsec -= 1000000000;
  }
} /* timespect_add*/

static void setprio(int prio, int sched) {
   	struct sched_param param;
/*   	 Set realtime priority for this thread*/
   	param.sched_priority = prio;
   	if (sched_setscheduler(0, sched, &param) < 0)
   		perror("sched_setscheduler");
} /* setprio */

void setSpeed(){
	uint32_t pom = 65;
	printf("pozadovanaRychlost je %lx%lx velikost je %d\n",(long unsigned int)(pozadovanaRychlost>>4), (long unsigned int)pozadovanaRychlost,sizeof(uint64_t));
	pozadovanaRychlost = (pom << 31);
	
/*	if(pozadovanaRychlost != (uint32_t)2147483648){*/
/*		puts("eee");*/
/*	}*/
	printf("pozadovanaRychlost je %lx%lx velikost je %d\n",(long unsigned int)(pozadovanaRychlost>>32), (long unsigned int)pozadovanaRychlost,sizeof(uint64_t));
} /* setSpeed */

void *thread_controller(void *arg){
	const struct timespec period = {0, MS};
	struct timespec time_to_wait;
	FILE *soubor;
/*	uint32_t pozice = 0;*/
	int pom = 0;
	uint32_t pomm = 0;
        if((soubor = fopen("/dev/irc0", "r")) == NULL){
		printf("chyba otevreni souboru /dev/irc0\n");
		return 0;
	}
	clock_gettime(CLOCK_MONOTONIC,&time_to_wait);
	setprio(95, SCHED_FIFO);
	
	while(1) {	  
		timespec_add(&time_to_wait,&time_to_wait,&period);
		if(fread(&pomm,1,sizeof(uint32_t),soubor) == -1){
			printf("chyba pri cteni ze souboru /dev/irc0\n");
			fclose(soubor);
			return 0;
		}
		pozice = pomm;
		pozadovanaPozice += pozadovanaRychlost;
		pom = (int)((pozadovanaPozice>>32) - pozice);
/*		printf ("\r%010u: %010i", (unsigned int)(pozadovanaPozice>>32), pom);*/
		regulatorPID(pom);
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &time_to_wait,NULL);		
	}
	
	return 0;

} /* thread_controller */

void *thread_readValue(void *arg){
	FILE *fd;
	char * myfifo = "/media/ramdisk/otackyFIFO";
	int64_t pom = 0;
	puts("Nastavuji prioritu ctecimu procesu 49");
/*	setprio(50, SCHED_FIFO);*/
	puts("Oteviram soubor /media/ramdisk/otackyFIFO");
	
	while(1) {
		fd = fopen(myfifo, "r");
		puts("Snazim se cist hodnotu");
		if(fread(&pom, sizeof(int64_t),1, fd) != 1){
			puts("chyba cteni fifo");
		}
		fclose(fd);
		puts("Mam hodnou");
		pozadovanaRychlost = pom;
		pozadovanaPozice = ((uint64_t)pozice)<<32;
		printf("Pozadovana rychlost: 0x%08x%08x\n", (unsigned int)(pozadovanaRychlost>>32), (unsigned int)pozadovanaRychlost);
	}
	
	return 0;

} /* thread_readValue */

int diagnostikaSmeru(void){
	uint32_t next = 0;
	uint32_t pom = 0;
	int vysledek;
	FILE *soubor;
	if((soubor = fopen("/dev/irc0", "r")) == NULL){
		printf("chyba otevreni souboru /dev/irc0\n");
		return 1;
	}
	if(fread(&pom,1,sizeof(uint32_t),soubor) == -1){
		printf("chyba pri cteni ze souboru /dev/irc0\n");
		fclose(soubor);
		return 1;
	}
	pozice = pom;
	printf("soubor obsahuje cislo %u\n", pozice);
	otaceni(LEFT);
	setHWPWM(100);
	usleep(100000);
	setHWPWM(0);
	if(fread(&next,1,sizeof(uint32_t),soubor) == -1){
		printf("chyba pri cteni ze souboru /dev/irc0\n");
		fclose(soubor);
		return 1;
	}
	vysledek = (int)next - (int)pozice;
	if(vysledek < 0) {
		orientace = -1;
	}else{
		orientace = 1;
	}
	fclose(soubor);
	pozice = next;
	pozadovanaPozice = (uint64_t)(pozice) << 32;
	return 0;
} /* diagnostikaSmeru */

int main(int argc, char **argv) {
	
	int hodnota = 0;
	
	pthread_t controller;
	pthread_attr_t controller_attr;
	
	pthread_t readValue;
	pthread_attr_t readValue_attr;

	setSpeed();
	
  	puts("program servoPi bezi");
  	
  	inicializacePameti();
        inicializacePWM();
	/* nastaveni GPIO pro output - zmena smeru */
        INP_GPIO(GPIO_LEFT);
	OUT_GPIO(GPIO_LEFT);
	INP_GPIO(GPIO_RIGHT);
	OUT_GPIO(GPIO_RIGHT);
	
	if(diagnostikaSmeru() != 0){
		puts("Diagnostika smeru selhala");
		return 0;
	}
	if(orientace == 1){
		printf("orientace do leva je kladna\n");
	}else{
		printf("orientace do leva je zaporna\n");
	}
	
	while(hodnota < 5000){
		puts("Procento:");
		scanf("%i", &hodnota);
		setHWPWM(hodnota);
	}
	puts("Start ridiciho vlakna");
	/* controller thread */
	if (pthread_attr_init(&controller_attr)){
   		puts("Chyba pthread_attr_init");
   		return 1;
   	}
   	pthread_create(&controller, &controller_attr, &thread_controller, NULL);
	
	puts("Startuju vlakno pro cteni hodnoty");
	/* reading new value thread */
	if (pthread_attr_init(&readValue_attr)){
   		puts("Chyba pthread_attr_init");
   		return 1;
   	}
   	pthread_create(&readValue, &readValue_attr, &thread_readValue, NULL);
   	
	pthread_join(readValue, NULL);
	pthread_join(controller, NULL);

	return 0;
} /* main */
