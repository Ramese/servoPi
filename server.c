#define PRESNOST	32

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
/*#include <unistd.h>*/
#include <stdint.h>
#include <fcntl.h>
#include <sys/stat.h>

double pole[PRESNOST];

void cleanArray(char *c, int size){
	int i;
	for(i=0; i<size; i++){
		c[i]=' ';
	}
} /* cleanArray */

void inicializacePole(void){
	int i;
	pole[0] = 0.5;
	for(i=1; i<PRESNOST; i++){
		pole[i] = pole[i-1]/2.0;
	}
}

int64_t getRequestedSpeed(double cislo){
	int64_t pozRych = 0;
	int i;
	int znamenko = 1;
	if(cislo < 0){
		znamenko = -1;
	}
	pozRych = ((int64_t)znamenko*((int64_t)cislo))<<(PRESNOST);
	cislo = cislo - (int)cislo;
	cislo *= znamenko;
	for(i=31; i>-1; i--){
		if(pole[PRESNOST-i-1] <= cislo){
			pozRych = pozRych | ((unsigned int)1 << i);
			cislo -= pole[PRESNOST-i-1];
		}
		if(cislo < pole[PRESNOST-1]){
			if(znamenko == -1){
				pozRych = ~pozRych;
			}
			break;
		}
	}
	if(znamenko == -1 && pozRych > 0){
		pozRych *= znamenko;
	}
	return pozRych;
}

int main() {
	int server_sockfd, client_sockfd;
	int server_len, client_len;
	struct sockaddr_in server_address;
	struct sockaddr_in client_address;
	char ch[10];
	int otackyZaMin;
	int64_t pozadovanaRychlost;
	FILE *fd;
	char * myfifo = "/media/ramdisk/otackyFIFO";
	puts("Inicializuju FIFO");
	mkfifo(myfifo, 0666);	
	
	puts("Vyplnuji pole");
	inicializacePole();
	
	/* odstraníme všechny staré sockety a vytvoříme nový nepojmenovaný socket  */
	unlink("server_socket");
	server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
	/* pojmenovani socketu */
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	server_address.sin_port = htons(9050);
	server_len = sizeof(server_address);
	bind(server_sockfd, (struct sockaddr *)&server_address, server_len);
	
	/* fronta pro pripojeni a cekani na klienty */
	listen(server_sockfd, 5);
	while(1) {
	/* prijmuti pripojeni */
		client_len = sizeof(client_address);
		client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_address, (socklen_t *)&client_len);
	/* cteme a piseme klientovi na client_sockfd */
		cleanArray(ch, 10);
		if(read(client_sockfd, ch, 10) == -1){
			puts("navratova hodnota je -1");
		}
		close(client_sockfd);
		/*printf("pozice kurzoru %d\n", (int)lseek(client_sockfd, 0, SEEK_CUR));*/
		/*ch[0]++;
		write(client_sockfd, &ch, 1);*/
		sscanf(ch, "%d", &otackyZaMin);
/*		printf("Otacky za min: %d\n", otackyZaMin);*/
		pozadovanaRychlost = getRequestedSpeed((double)otackyZaMin/150.0);
/*		printf("Pozadovana rychlost: 0x%08x%08x\n", (unsigned int)(pozadovanaRychlost>>32), (unsigned int)pozadovanaRychlost);*/
/*		puts("pokousim se o zapis");*/
/*		puts("Oteviram FIFO");*/
		fd = fopen(myfifo, "w");
		if(fwrite(&pozadovanaRychlost,sizeof(int64_t),1,fd) != 1){
			puts("Chyba pri zapisu");
		}
/*		puts("konec zapisu");*/
		fclose(fd);
		
	}
	
	unlink(myfifo);
} /* main */
