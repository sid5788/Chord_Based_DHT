
default: client
	gcc -o dhtmain dhtmain.c -lcrypto -g -lm

client:
	gcc -o client client.c -lreadline -g -lm
	
clean:
	rm dhtmain client
debug: clientD
	gcc -o dhtmain dhtmain.c -lcrypto -DDEBUG -lm
clientD:
	gcc -o client client.c -lreadline -DDEBUG

