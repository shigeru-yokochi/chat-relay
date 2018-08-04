all:
	gcc -g chat-relay.c -o chat-relay -L/usr/local/lib -I/usr/local/include -lssl -lcrypto -lwiringPi
clean:
	rm -f  chat-relay
