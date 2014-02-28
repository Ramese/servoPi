/*wiki example */
#define BASE		0x20000000

#define GPIO_BASE 	(BASE + 0x200000)
/* */

/* z dodatku */
#define PWM_BASE	(BASE + 0x20C000)
#define CLK_BASE	(BASE + 0x101000)

#define PWM_CTL		*(pwm)
#define PWM_RNG1	*(pwm+4)
#define PWM_DAT1	*(pwm+5)

#define PWM_CLK_CNTL	*(clk+40)
#define PWM_CLK_DIV	*(clk+41)
/* */

#define LEFT		1
#define RIGHT		2
#define STOP		3
#define GPIO_PWM	18
#define GPIO_LEFT	14
#define GPIO_RIGHT	15
#define GPIO_IRC1	4
#define GPIO_IRC2	3
#define GPIO_IRQ	2

#define PIBLASTER	"/dev/pi-blaster"

/* wiki example */
#define PAGE_SIZE 	(4*1024)
#define BLOCK_SIZE	(4*1024)

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
/* */

/*
#include <string.h>
#include <dirent.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
*/


/* wiki example: */
int mem_fd;
void *gpio_map, *pwm_map, *clk_map;
volatile unsigned *gpio;
/* */

volatile unsigned *pwm, *clk;

#define INP_GPIO(g) 		*(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) 		*(gpio+((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(g,a) 	*(gpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))

#define GPIO_SET 	*(gpio+7)
#define GPIO_CLR 	*(gpio+10)

/*
nějaka adresa na 64bit pamět či co
0x20003000
*/

void inicializacePameti()
{

	if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
		printf("can't open /dev/mem\nroot access needed\n");
		exit(-1);
	}

	gpio_map = mmap(
		NULL,             //Any adddress in our space will do
		BLOCK_SIZE,       //Map length
		PROT_READ|PROT_WRITE,// Enable reading & writting to mapped memory
		MAP_SHARED,       //Shared with other processes
		mem_fd,           //File to map
		GPIO_BASE         //Offset to GPIO peripheral
	);

	if (gpio_map == MAP_FAILED) {
		printf("mmap error %d\n", (int)gpio_map);
		exit(-1);
	}
	
	gpio = (volatile unsigned *)gpio_map;
	
	pwm_map = mmap(
		NULL,             //Any adddress in our space will do
		BLOCK_SIZE,       //Map length
		PROT_READ|PROT_WRITE,// Enable reading & writting to mapped memory
		MAP_SHARED,       //Shared with other processes
		mem_fd,           //File to map
		PWM_BASE         //Offset to GPIO peripheral
	);

	if (pwm_map == MAP_FAILED) {
		printf("mmap error %d\n", (int)pwm_map);
		exit(-1);
	}
	
	pwm = (volatile unsigned *)pwm_map;
	
	clk_map = mmap(
		NULL,             //Any adddress in our space will do
		BLOCK_SIZE,       //Map length
		PROT_READ|PROT_WRITE,// Enable reading & writting to mapped memory
		MAP_SHARED,       //Shared with other processes
		mem_fd,           //File to map
		CLK_BASE         //Offset to GPIO peripheral
	);

	if (clk_map == MAP_FAILED) {
		printf("mmap error %d\n", (int)clk_map);
		exit(-1);
	}

   	clk = (volatile unsigned *)clk_map;
	

	close(mem_fd); //No need to keep mem_fd open after mmap

} /* inicializacePameti */

void inicializacePWM(){
	INP_GPIO(GPIO_PWM);
	SET_GPIO_ALT(GPIO_PWM, 5);
	PWM_CTL = 0;
	PWM_CLK_CNTL = (PWM_CLK_CNTL&~0x10)|0x5a000000;
	while(PWM_CLK_CNTL&0x80);
	PWM_CLK_DIV = 0x5a000000|(1<<12);
	PWM_CLK_CNTL = 0x5a000001;
	PWM_CLK_CNTL = 0x5a000011;
	while(!PWM_CLK_CNTL&0x80);
	PWM_RNG1 = 384; 
	/*
	500;-38kHz => huci ale ok 
	256;-25kHz => ok
	800-24kHz => huci ale ok
	
	*/
	PWM_DAT1 = 0;
	PWM_CTL = 0x1;
} /* inicializace PWM */

void setHWPWM(float procento){
	PWM_DAT1 = (int)(384.0*procento);
}

void otaceni(int action){
	if(action == LEFT){
		/* zapne LEFT vypne RIGHT */
		GPIO_CLR = 1<<GPIO_RIGHT;
		GPIO_SET = 1<<GPIO_LEFT;
		puts("Otáčení do leva");
	}else if(action == RIGHT){
		GPIO_CLR = 1<<GPIO_LEFT;
		GPIO_SET = 1<<GPIO_RIGHT;
		puts("Otáčení do prava");
	}else if(action == STOP){
		GPIO_CLR = 1<<GPIO_RIGHT;
		GPIO_CLR = 1<<GPIO_LEFT;
		puts("Stop");
	}else{
		puts("Nedefinovaná akce");
	}
} /* otaceni */

void setPWM(float procento){
	FILE *f;
	if((f = fopen(PIBLASTER, "w")) == NULL){
		printf("Chyba PIBLASTER\n");
	}else{
		fprintf(f, "18=%f\n", procento);
		fclose(f);
	}
} /* setPWM */

int main(int argc, char **argv)
{
  	/*FILE *f;*/
	float procento = 0.0;
	int smer = 0;
	/*if((f=fopen(PIBLASTER, "w")) == NULL){
		printf("Chyba pro opevreni /dev/pi-blaster\n");
		return -1;
	}
	fclose(f);*/

  	inicializacePameti();
	inicializacePWM();
  	
	INP_GPIO(GPIO_LEFT);
	OUT_GPIO(GPIO_LEFT);
	INP_GPIO(GPIO_RIGHT);
	OUT_GPIO(GPIO_RIGHT);
/*	INP_GPIO(18);
	OUT_GPIO(18);
	GPIO_SET = 1<<18;*/
	while(smer < 5){
		puts("Procento a smer (1-L, 2-R, 3-S)");
		scanf("%f %i", &procento, &smer);
		otaceni(smer);
		setHWPWM(procento);
		/* otaceni(RIGHT);
		sleep(1);
		otaceni(STOP);
		sleep(1);*/
	}
	puts("Konec");
	return 0;
} /* main */
