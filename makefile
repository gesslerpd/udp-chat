cc=gcc
flags=-Wall
flagsClient=-lpthread
source=./src/

all: compile

compile: client server

client: $(source)client.c
	$(cc) $(flags) $(flagsClient) $(source)client.c -o client

server: $(source)server.c
	$(cc) $(flags) $(source)server.c -o server

clean:
	rm client server
