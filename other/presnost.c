#define PRESNOST	32

#include <stdlib.h>
#include <stdio.h>
/*#include <unistd.h>*/
#include <stdint.h>

void vypisPoleDouble(double pole[], int k){
	int i;
	for(i=0; i<k; i++){
		printf("%.010f x %d\n", pole[i], i);
	}
}

int main(void){
	int i;
	double pole[PRESNOST];
	int64_t pozRych=0;
	double cislo=0.3333333333333333333333333333333;
	pole[0] = 0.5;
	for(i=1; i<PRESNOST; i++){
		pole[i] = pole[i-1]/2.0;
	}
/*	vypisPoleDouble(pole, PRESNOST);*/
/*	ulozeni vrchni casti cisla*/
	pozRych = ((int64_t)cislo)<<(PRESNOST);
	
	printf("pozadovanaRychlost je %lx%lx\n",(long unsigned int)(pozRych>>32), (long unsigned int)pozRych);
	
	cislo = cislo - (int)cislo;
	printf("cislo %.010f\n", cislo);
	
	for(i=31; i>-1; i--){
		if(pole[PRESNOST-i-1] <= cislo){
			pozRych = pozRych | ((unsigned int)1 << i);
			cislo -= pole[PRESNOST-i-1];
		}
		if(cislo < pole[PRESNOST-1]){
			break;
		}
	}
	
	printf("pozadovanaRychlost je %lx%lx\n",(long unsigned int)(pozRych>>32), (long unsigned int)pozRych);
	
	return 0;
}
