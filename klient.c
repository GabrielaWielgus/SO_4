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

int id_kolejki, iwys, iod, run;
msglen_t pojemnosc;
struct msqid_ds w;
komunikat kom1, kom2;
int i=0;
char wiadomosc[MAX];

void obsluga_sygnalu(int sygnal)
{
	komunikat ob1;
	printf("\nZamykanie konsumenta...\n");
	while(1)
	{
		msgrcv(id_kolejki, &ob1, MAX+sizeof(long), getpid(), IPC_NOWAIT);//zdejmowanie komunikatow
		if(errno) break;
	}
	printf("\nZamknieto\n");
	exit(1);
}
void *odbieranie_komunikatu()
{
	signal(SIGINT, obsluga_sygnalu);
	signal(SIGQUIT, obsluga_sygnalu);
	
	while(run)
	{
		if(iwys!=iod)
		{
			kom1.typ = getpid();
		
			memset(kom1.tekst,0,MAX); //Wypelnia kolejne bajty w pamieci ustalona wartoscia(NULL),
		//MAX - ile bajtow zapisac, .tekst - adres poczatkowy

			msgrcv(id_kolejki,(komunikat *)&kom1,MAX+1*sizeof(long),kom1.typ,MSG_NOERROR);// odbiera info z kolejki 
	//(kolejka, adres bufora, rozmiar struktury bufora, typ wiadomosci do odbioru) 0->proces zawieszony,czeka na kom
			if(errno)
			{
				if(errno == EINVAL || errno==EIDRM)
				{
					printf("Serwer zostal wylaczony\n");
					run = 0;
				}
				else if(errno!=ENOMSG)
				{
					printf("Blad przy odczycie z kolejki. Zamykanie \n");
					exit(1);
				}
				else
				{
					printf("Blad przy odbieraniu komunikatu: %i (%s)\n", errno,strerror(errno));
					exit(1);
				}
			}
			else
			{
				printf("\nKlient PID: %d odebral komunikat: %s\n",getpid(),kom1.tekst);
				iod++;
			}
			
		}
	}
pthread_exit((void *) 0);
}

void *wysylanie_komunikatu()
{
	signal(SIGINT,obsluga_sygnalu);
	signal(SIGQUIT, obsluga_sygnalu);
	kom2.typ = 1;
	kom2.pid_nadawcy = getpid();
	unsigned long zajete;
	sleep(1);

	while(run)
	{
		//wprowadzanie ciagu znakow
			printf("\nKlient PID: %d - Podaj tekst do wyslania: ", getpid() );
			i = 0;
				while(1)
				{
					wiadomosc[i] = getchar();
					if( (wiadomosc[i] == '\n') || ( i>(MAX-1) ))
					{
						wiadomosc[i] = '\0';
						break;
					}
					++i;
				}
		
		strcpy(kom2.tekst,wiadomosc);
		
		do
		{
			msgctl(id_kolejki,IPC_STAT, &w);
			if(errno)
			{
				if(errno==EINVAL || errno==EIDRM)
				{
					printf("Serwer zostal wylaczony.\n");
					run = 0;
					break;
				}
				else
				{
					printf("Blad przy pobieraniu informacji o kolejce\n");
					exit(-1);
				}
			}
			zajete = w.__msg_cbytes;
		}while(pojemnosc-zajete<3*(strlen(kom2.tekst)+1*sizeof(long) - sizeof(long)));

	//jezeli wolnego miejsca jest mniej niz 3 komunikaty to zapetla sie while
	//wysylanie
	msgsnd(id_kolejki, (komunikat *)&kom2,strlen(kom2.tekst)+1*sizeof(long),0);//wysyla wiadomosc do kolejki
	//(identyfikator, wskaznik bufora wiadomosci, rozmiar wiadomosci w bajtach, flaga)
		if(errno)
		{
			if(errno==EINVAL || errno==EIDRM)
			{
				printf("Serwer zostal wylaczony\n");
				run =0;
				break;
			}
			else
			{
				printf("Blad wysylania komunikatu %i (%s)\n",strerror(errno));
				exit(2);
			}
		}
		else 
		{
			printf("Klient PID: %d wysylal komunikat: \"%s\" do serwera\n",getpid(),kom2.tekst);
			iwys++;
		}

	}
	pthread_exit((void *) 0);
}


int main() 
{
	signal(SIGINT, obsluga_sygnalu);
	signal(SIGQUIT, obsluga_sygnalu);
	iwys = iod = 0;
	run = 1;
	key_t klucz;
	if(! (klucz = ftok(".",'A')))
    {
    	printf("Blad tworzenia klucza /klient\n");
       	exit(-1);
    }

//uzyskuje dostep do kolejki 
	id_kolejki = msgget(klucz, 0600 );
	if(errno)
	{
		if(errno==ENOENT)
		{
			printf("Usluga niedostepna. Serwer wylaczony.\n");
		}
		else
		{
    		printf("Blad uzyskania dostepu do kolejki komunikatow %i (%s)\n", errno, strerror(errno));
		}
		exit(-1);
	}
	else
	{	
		printf("\nKlient %d uzyskal dostep do kolejki komunikatow o ID: %d\n", getpid(),id_kolejki);
	}

	msgctl(id_kolejki, IPC_STAT, &w);
	if(errno)
	{
		if(errno==EINVAL || errno==EIDRM)
		{
			printf("Serwer zostal wylaczony.\n");
			run=0;
		}
		else
		{
			printf("Blad przy podbieraniu informacji o kolejce\n");
			exit(-1);
		}
	}
	
	pojemnosc = w.msg_qbytes; // zapisuje ile bajtow moze byc w kolejce

	pthread_t tid1, tid2;

if(pthread_create(&tid1,NULL,odbieranie_komunikatu, NULL) == -1)
{
	printf("Blad tworzenia watku (odbieranie_komunikatu): %i (%s)\n", errno, strerror(errno));
	exit(-1);
}

if(pthread_create(&tid2,NULL,wysylanie_komunikatu, NULL) == -1)
{
        printf("Blad tworzenia watku (wysylanie_komunikatu): %i (%s)\n", errno, strerror(errno));
        exit(-1);
}

if(pthread_join(tid1, NULL)==-1) 
{       
	printf("Blad przylaczenia watku (wysylanie_komunikatu):  %i (%s)\n", errno, strerror(errno));
	exit(-1);
}
        
if(pthread_join(tid2, NULL)==-1) 
{       
	printf("Blad przylaczenia watku (wysylanie_komunikatu):  %i (%s)\n", errno, strerror(errno));
	exit(-1);
}

	return 0;
}

