# udp-chat

Chat server and client using the User Datagram Protocol (UDP)


## Building
```
$ make
```

#### Client
```bash
$ make client
```
or
```bash
$ gcc -o client src/client.c -lpthread
```

#### Server

```bash
$ make server
```
or
```bash
$ gcc -o server src/server.c
```

## Running

#### Client

```bash
$ ./client [server] [port] [username]

e.g.

$ ./client localhost 32505 gesslerpd
or
$ ./client 127.0.0.1 32505 gesslerpd
```

#### Client Commands

`list` - Will list all the other currently connected clients

`close` - Will disconnect you from the server and exit the program

`exit` - Will shutdown the server and exit the program

#### Server

```bash
$ ./server [port]

e.g.

$ ./server 32505
```
