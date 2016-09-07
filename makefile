cc=gcc
flags=-Wall
clientLibraries=-pthread
flagsFormat=-sob -bad -bap -br -nce -cli4 -npcs -cs -nsaw -nsai -nsaf -nbc -di1 -cdb -sc -brs -brf -i4 -lp -ppi 4 -l100 --ignore-newlines -nbbo -nut -ncdw -nbc -npsl
source=./src/
files=$(source)server.c $(source)client.c $(source)chat.h
serverOut=server
clientOut=client

all: compile

compile: client server

client: $(source)client.c
	$(cc) $(flags) $(clientLibraries) $(source)client.c -o $(clientOut)

server: $(source)server.c
	$(cc) $(flags) $(source)server.c -o $(serverOut)

clean:
	rm $(clientOut) $(serverOut)

format:
	indent $(files) $(flagsFormat)

