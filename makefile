cc=gcc
flags=-Wall
flagsClient=-lpthread
flagsFormat=-sob -bad -bap -br -nce -cdw -cli4 -npcs -cs -nsaw -nsai -nsaf -nbc -di1 -cdb -sc -brs -brf -i4 -lp -ppi 4 -l100 --ignore-newlines -nbbo -nut
source=./src/
files= ./src/server.c ./src/client.c ./src/chat.h

all: compile

compile: client server

client: $(source)client.c
	$(cc) $(flags) $(flagsClient) $(source)client.c -o client

server: $(source)server.c
	$(cc) $(flags) $(source)server.c -o server

clean:
	rm client server

format:
	indent $(files) $(flagsFormat)

