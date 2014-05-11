#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
/*#include <sockLib.h>*/
#include <string.h>
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
/* 2.5 kHz */

#define PERIODA 5000000
#define S	1000000000
#define DELKA	100
#define TRUE	1
#define FALSE	2
int pole[DELKA];

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

void *thread_sberDat(void *arg){
	struct timespec period = {0, PERIODA};
	struct timespec time_to_wait;
	FILE *soubor;
	int i;
	uint32_t pom = 0;
	uint32_t pomm = 0;
        if((soubor = fopen("/dev/irc0", "r")) == NULL){
		printf("chyba otevreni souboru /dev/irc0\n");
		return 0;
	}
	clock_gettime(CLOCK_MONOTONIC,&time_to_wait);
	while(1) {
		if(fread(&pomm,1,sizeof(uint32_t),soubor) == -1){
			printf("chyba pri cteni ze souboru /dev/irc0\n");
			fclose(soubor);
			return 0;
		}
		
		for(i = DELKA - 1; i > 0; i--){
			pole[i] = pole[i-1];
		}
		pole[0] = pomm - pom;
		pom = pomm;
		
		sectiCasy(&time_to_wait,period);
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &time_to_wait,NULL);		
	}
	
	return 0;

} 
/* thread_sberDat */


int main() {
	pthread_t sberDat;
	pthread_attr_t sberDat_attr;
	int server_sockfd, client_sockfd;
	int server_len, client_len;
	struct sockaddr_in server_address;
	struct sockaddr_in client_address;
	FILE *fd;
	int i, jePrvni = TRUE, poslatGraf = FALSE, poslatWeb = FALSE;
	char buff[2];
	char buff2[50];
	char c = ' ', cc = ' ';
	
	for (i = 0; i < DELKA; i++) {
		pole[i] = 50;
	}
	
	buff[0] = ' ';

	puts("WebServer start");	
	unlink("server_socket");
	server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	server_address.sin_port = htons(8080);
	server_len = sizeof(server_address);
	bind(server_sockfd, (struct sockaddr *)&server_address, server_len);
	
	if (pthread_attr_init(&sberDat_attr)){
   		puts("Chyba pthread_attr_init");
   		return 1;
   	}
   	pthread_create(&sberDat, &sberDat_attr, &thread_sberDat, NULL);
/*   	pthread_join(sberDat, NULL);*/
	/* fronta pro pripojeni a cekani na klienty */
	puts("cekam na klienty");
	listen(server_sockfd, 5);
	while(1) {
	/* prijmuti pripojeni */
		client_len = sizeof(client_address);
		client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_address, (socklen_t *)&client_len);
/*		puts("Dalsi klient");*/
		jePrvni = TRUE;
		fd = fdopen(client_sockfd, "w+");
		while(1){
			if(jePrvni == TRUE){
				jePrvni = FALSE;
				fscanf(fd, "GET %s ", buff2);
				if(strcmp(buff2, "/graph.svg") == 0){
					poslatGraf = TRUE;
				}else if(strcmp(buff2, "/") == 0){
					poslatWeb = TRUE;
				}else {
/*					printf("\nbuff2 %s\n", buff2);*/
				}
			}else{
				fgets(buff, 2, fd);
				if(cc == '\n' && buff[0] == '\n'){
					cc = ' ';
					c = ' ';
					break;
				}
				cc = c;
				c = buff[0];
/*				if(c == '\n'){*/
/*					puts("\\n");*/
/*				}else if(c == '\r'){*/
/*					printf("\\r");*/
/*				}else{*/
/*					printf("%s", buff);*/
/*				}*/
			}
		}
		  	 
		fprintf(fd, "HTTP/1.0 200 OK\r\n");
		if(poslatWeb == TRUE){
			poslatWeb = FALSE;
			fprintf(fd, "Content-Type: text/html\r\n\r\n");
		
			fprintf(fd, "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">\r\n");
			fprintf(fd, "<html>\r\n");
			fprintf(fd, "<head>\r\n");
			fprintf(fd, "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\r\n");
			fprintf(fd, "<meta http-equiv=\"refresh\" content=\"1\" />\r\n");
			fprintf(fd, "<title>Motor Status Example</title>\r\n");
			fprintf(fd, "</head>\r\n");

			fprintf(fd, "<body>\r\n");
			fprintf(fd, "<h1>Motor Status Example</h1>\r\n");

			fprintf(fd, "<object type=\"image/svg+xml\" data=\"graph.svg\">\r\n");

			fprintf(fd, "</body></html>\r\n");
			fprintf(fd, "\r\n");
		}else if(poslatGraf == TRUE){
			poslatGraf = FALSE;
			fprintf(fd, "Content-Type: image/svg+xml\r\n\r\n");
			
			fprintf(fd, "<?xml version=\"1.0\"?>\n");
			fprintf(fd, "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.0//EN\" \"http://www.w3.org/TR/2001/REC-SVG-20010904/DTD/svg10.dtd\">\n");
			fprintf(fd, "<svg width=\"500\" height=\"300\" xmlns='http://www.w3.org/2000/svg'>\n");
			fprintf(fd, "<g transform=\"translate(50,130) scale(1)\">\n");
    			fprintf(fd, "<!-- Now Draw the main X and Y axis -->\n");
    			fprintf(fd, "<g style=\"stroke-width:2; stroke:black\">\n");
      			fprintf(fd, "<!-- X Axis -->\n");
	      		fprintf(fd, "<path d=\"M 0 0 L 400 0 Z\"/>\n");
	      		fprintf(fd, "<!-- Y Axis -->\n");
	      		fprintf(fd, "<path d=\"M 0 -100 L 0 100 Z\"/>\n");
	    		fprintf(fd, "</g>\n");
	    		fprintf(fd, "<g style=\"fill:none; stroke:#B0B0B0; stroke-width:1; stroke-dasharray:2 4;text-anchor:end; font-size:30\">\n");
	      		fprintf(fd, "<text style=\"fill:black; stroke:none\" x=\"-1\" y=\"70\" >-70</text>\n");
	      		fprintf(fd, "<text style=\"fill:black; stroke:none\" x=\"-1\" y=\"0\" >0</text>\n");
	      		fprintf(fd, "<text style=\"fill:black; stroke:none\" x=\"-1\" y=\"-70\" >70</text>\n");
	      		fprintf(fd, "<g style=\"text-anchor:middle\">\n");
			fprintf(fd, "<text style=\"fill:black; stroke:none\" x=\"100\" y=\"20\" >100</text>\n");
			fprintf(fd, "<text style=\"fill:black; stroke:none\" x=\"200\" y=\"20\" >200</text>\n");
			fprintf(fd, "<text style=\"fill:black; stroke:none\" x=\"300\" y=\"20\" >300</text>\n");
			fprintf(fd, "<text style=\"fill:black; stroke:none\" x=\"400\" y=\"20\" >400</text>\n");
	      		fprintf(fd, "</g>\n");
	    		fprintf(fd, "</g>\n");
	    		fprintf(fd, "<polyline\n");
		     	fprintf(fd, "points=\"\n");
		     	for(i = 0; i < DELKA; i++){
		     		fprintf(fd, "%i, %i\n", i*5, -pole[i]/2);
		     	}
			fprintf(fd, "\"\n");
		        fprintf(fd, "style=\"stroke:red; stroke-width: 1; fill : none;\"/>\n");
	         	fprintf(fd, "</g>\n");
		 	fprintf(fd, "</svg>\n");
		}else{
			fprintf(fd, "Content-Type: text/html\r\n\r\n");
			fprintf(fd, "\r\n");
		}
		fclose(fd);
		close(client_sockfd);
	}
} /* main */
