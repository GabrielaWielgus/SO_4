all: serwer klient

klient: klient.c
	gcc -o klient klient.c -lpthread

serwer: serwer.c
	gcc -o serwer serwer.c

clean:
	rm -rf *~
