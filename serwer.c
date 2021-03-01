#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include <unistd.h>


#define MAX 256

typedef struct k 
{
   long typ; 
   long pid_nadawcy;
   char tekst[MAX];

} komunikat;


void obsluga_sigint(int sig_n);


int main()
{
	
int id_kolejki;
komunikat kom;
int i;

key_t klucz;
if(! (klucz = ftok(".",'A')))
{
	printf("Blad tworzenia klucza");
	exit(-1);
}

if( (id_kolejki = msgget(klucz, 0600 | IPC_CREAT | IPC_EXCL)) == -1 )
{
	if( (id_kolejki = msgget(klucz, 0600 | IPC_CREAT )) == -1 )
	{
        printf("Blad uzyskania dostepu do kolejki komunikatow /klient: %i (%s)\n", errno, strerror(errno));
        exit(-1);
    }
    else
		printf("\nSerwer uzyskal dostep do kolejki komunikatow o ID:%d \n",id_kolejki);
}
else
  printf("\nSerwer utworzyl kolejke komunikatow o ID: %d \n",id_kolejki);	



signal(SIGINT,obsluga_sigint);
printf(" ^C konczy prace serwera \n\n");


while(1)
{
	printf("\nSerwer oczekuje na komunikat...\n");
	kom.typ = 1;
		
		memset(kom.tekst,0,MAX); //Wypelnia kolejne bajty w pamieci ustalona wartoscia(NULL),
		//MAX - ile bajtow zapisac, .tekst - adres poczatkowy
		
	//pobiera wiadomosc
	if((msgrcv(id_kolejki,(komunikat *)&kom,MAX+1*sizeof(long), kom.typ, MSG_NOERROR)) == -1) // odbiera info z kolejki 
	{
		printf("Blad pobrania komunikatu %i (%s)\n",errno,strerror(errno));
		obsluga_sigint(SIGINT);
	}
	else
		printf("Serwer odebral komunikat od procesu: %d, tresc: %s\n", kom.pid_nadawcy, kom.tekst);

		// przetwarza wiadomosc
		printf("Serwer przetwarza komunikat...\n");
		for(i=0; i<strlen(kom.tekst); i++)
		{ 
			kom.tekst[i] = toupper(kom.tekst[i]); 
		}
	//wysyla odpowiedz do klienta
	kom.typ = kom.pid_nadawcy;
	printf("Serwer wysyla komunikat do: %ld, tresc: %s\n", kom.pid_nadawcy, kom.tekst);
	
	if((msgsnd(id_kolejki,(komunikat *)&kom,strlen(kom.tekst)+1*sizeof(kom.pid_nadawcy),0))==-1) //wysyla wiadomosc do kolejki
	//(identyfikator, wskaznik bufora wiadomosci, rozmiar wiadomosci w bajtach, flaga)
	{
		printf("Blad wysylania komunikatu: %i (%s)\n",errno,strerror(errno));
		obsluga_sigint(SIGINT);
	}

}

return 0;
}

void obsluga_sigint(int sig_n)
{
	key_t klucz;
	if(! (klucz = ftok(".",'A')))
        {
        	printf("Blad tworzenia klucza");
        	exit(-2);
        }
	
		int id_kolejki;
		if(sig_n==SIGINT)
		{
			printf(" Zakonczono prace serwera \n");
			
			int id_kolejki;
			
			// uzyskiwanie dostepu
			if((id_kolejki=msgget(klucz, 0600 | IPC_CREAT))==-1)
			{
				printf(" Blad uzyskania dostepu do kolejki komunikatow /serwer: %i (%s)\n", errno, strerror(errno));
				exit(-1);
			}

				// usuwa kolejke
				if((msgctl(id_kolejki, IPC_RMID,0)) == -1)
				{
					printf(" Blad usuwania kolejki komunikatow /serwer: %i (%s)\n", errno, strerror(errno));
					exit(-1);
				}
				else
					printf(" Usunieto kolejke komunikatow o ID: %d \n", id_kolejki);
	
			exit(0);
		}
}
