/* server */

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "chat.h"

typedef struct client {
    struct sockaddr_in address; //will hold the client's address information
    char username[USERNAME_LEN];        //will hold the clients username and will be initialized when each client starts and sends it's username and a special code informative of joining server
    struct client *next;
} client;

int sockfd;

client clientList;
char requestBuffer[BUF_SIZE];
char responseBuffer[BUF_SIZE + USERNAME_LEN];

char sender_name[USERNAME_LEN];

char *colors[NCOLORS] = { RED, GREEN, YELLOW, BLUE, MAGENTA,
    CYAN, LRED, LGREEN, LYELLOW, LBLUE, LMAGENTA,
    LCYAN
};

void userColor(int n) {
    strcat(responseBuffer, colors[(n % NCOLORS)]);
}

/* function returns true if the two passes internet address are identical */
int clientCompare(struct sockaddr_in client1, struct sockaddr_in client2) {
    if(strncmp
       ((char *) &client1.sin_addr.s_addr, (char *) &client2.sin_addr.s_addr,
        sizeof(unsigned long)) == 0) {
        if(strncmp((char *) &client1.sin_port, (char *) &client2.sin_port, sizeof(unsigned short))
           == 0) {
            if(strncmp
               ((char *) &client1.sin_family, (char *) &client2.sin_family,
                sizeof(unsigned short)) == 0) {

                return TRUE;
            }
        }
    }
    return FALSE;
}

/* sends message to all clients except for the sender */
/* will send to all clients if second argument `global` is set to TRUE */
void broadcast(struct sockaddr_in sender, int global) {
    client *cli = clientList.next;      //client list iterator

    while(cli != NULL) {
        //if sender isn't the client send out the message // may need separate clientCompare function // use username for comparisons so noone can connect with the same username
        if(clientCompare(sender, cli->address) == FALSE || global) {
            printf("Sending message to %s\n", cli->username);
            if((sendto
                (sockfd, responseBuffer, strlen(responseBuffer), 0,
                 (struct sockaddr *) &cli->address, sizeof(struct sockaddr))) == SYSERR) {

                perror("sendto");
                close(sockfd);
                exit(EXIT_FAILURE);
            }
        }

        cli = cli->next;
    }
}

int isConnected(struct sockaddr_in newClient) {
    client *element = &clientList;

    while(element != NULL) {
        if(clientCompare(element->address, newClient)) {
            strncpy(sender_name, element->username, USERNAME_LEN);
            printf("Client is already connected\n");
            return TRUE;
        }
        element = element->next;
    }
    printf("Client is not already connected\n");
    return FALSE;
}

/* Used to add new clients dynamically to our server's client linked list */
/* connects a client to the server */
int connectClient(struct sockaddr_in newClient, char *username) {
    printf("Attempting to connect client: %s\n", username);
    client *element = &clientList;

    while(element != NULL) {
        if(strcmp(element->username, username) == 0) {
            printf("Cannot connect client user already exists\n");
            strcpy(responseBuffer, "");
            strcat(responseBuffer, ERROR);
            if((sendto
                (sockfd, responseBuffer, strlen(responseBuffer), 0, (struct sockaddr *) &newClient,
                 sizeof(struct sockaddr))) == SYSERR) {
                perror("sendto");
                close(sockfd);
                exit(EXIT_FAILURE);
            }
            return SYSERR;
        }
        element = element->next;
    }

    element = &clientList;
    while(element->next != NULL) {
        element = element->next;
    }
    element->next = malloc(sizeof(client));
    element = element->next;

    element->address = newClient;
    strncpy(element->username, username, USERNAME_LEN);
    element->next = NULL;
    printf("Client connected\n");
    return OK;
}

/* Used to remove clients dynamically from our server's client linked list */
/* disconnects a client from the server */
int disconnectClient(struct sockaddr_in oldClient) {
    printf("Attempting to disconnect client\n");
    client *temp;
    client *element = &clientList;

    while(element->next != NULL) {
        if(clientCompare(oldClient, element->next->address)) {
            temp = element->next->next;
            free(element->next);
            element->next = temp;
            printf("Client disconnected\n");
            return OK;
        }
        element = element->next;
    }

    printf("Client was not disconnected properly\n");
    return SYSERR;
}

/* Prints out the server's client list for debugging */
void printClientList() {
    client *cli = clientList.next;
    int count = 1;

    while(cli != NULL) {
        printf("Client %d\n", count);
        printf("%s\n", cli->username);
        printf("ip: %s\n", inet_ntoa(cli->address.sin_addr));
        printf("port: %d\n\n", ntohs(cli->address.sin_port));
        cli = cli->next;
        count++;
    }
}

void sendClientList(struct sockaddr_in sender) {
    client *cli = clientList.next;

    while(cli != NULL) {
        if(clientCompare(sender, cli->address) == FALSE) {
            strcpy(responseBuffer, "");
            userColor(cli->address.sin_port);
            strcat(responseBuffer, cli->username);
            strcat(responseBuffer, RESET "\n");
            if((sendto
                (sockfd, responseBuffer, strlen(responseBuffer), 0, (struct sockaddr *) &sender,
                 sizeof(struct sockaddr))) == SYSERR) {

                perror("sendto");
                close(sockfd);
                exit(EXIT_FAILURE);
            }
        }
        cli = cli->next;
    }
}

/* Main Server Process */
int main(int argc, char *argv[]) {

    int server_port, nbytes;
    int address_size = sizeof(struct sockaddr_in);
    struct sockaddr_in server_addr;
    struct sockaddr_in sender_addr;     //sockaddr internet structs to handle address family,port number, and internet address

    //note sockaddr_in is the same size as sockaddr and can be cast to (struct sockaddr *)

    bzero(requestBuffer, BUF_SIZE);
    bzero(responseBuffer, BUF_SIZE + USERNAME_LEN);     //zero out buffers

    if(argc != 2) {
        fprintf(stderr, "Usage: %s portnum\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    server_port = atoi(argv[1]);        //server_port now holds the user inputed port number

    //SOCK_DGRAM are datagram or connectionless sockets they use UDP and will not nessecarily arrive in order or arrive at all
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);    //returns socket file descriptor that can use send() and recv() for data transmission
    // last argumant protocol number given 0 will derive to protocol based on the 2nd argument type passed

    if(sockfd == SYSERR) {      //socket() returns -1 on error
        close(sockfd);
        fprintf(stderr, "Failed to get socket file descriptor\n");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;   //address family we should use for this assignment
    server_addr.sin_port = htons(server_port);  //flips port argument to big endian and assigns to our port
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);    //binds socket to all the computers interfaces
    //INADDR_ANY is defined as 0 but is flipped to network long for consistency

    memset(&(server_addr.sin_zero), '\0', 8);   //sets rest of sockaddr_in to 0's 8 bytes worth

    if(bind(sockfd, (struct sockaddr *) &server_addr, sizeof(struct sockaddr)) == SYSERR) {
        close(sockfd);
        fprintf(stderr, "Failed to bind on socket!\n");
        exit(EXIT_FAILURE);
    }

    while(1) {                  // main server receive/response loop
        bzero(responseBuffer, BUF_SIZE + USERNAME_LEN); //zero out buffer //we might be able to remove this to improve performance

        /*
         * LISTEN FOR PACKETS 
         */
        if((nbytes =
            recvfrom(sockfd, requestBuffer, BUF_SIZE - 1, 0, (struct sockaddr *) &sender_addr,
                     (unsigned int *) &address_size)) == SYSERR) {
            perror("recvfrom");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        requestBuffer[nbytes] = '\0';   // ensures that recieved message is null terminated
        printf("Received packet of %d bytes\n~\n%s\n~\n\n", nbytes, requestBuffer);

        if(isConnected(sender_addr)) {
            userColor(sender_addr.sin_port);
            strcat(responseBuffer, sender_name);
            if(strcmp(CLOSE, requestBuffer) == 0) {
                if(disconnectClient(sender_addr) == OK) {       //upon success of disconnect broadcast message to clients that user left

                    strcat(responseBuffer, RED " disconnected" RESET "\n");

                    broadcast(sender_addr, TRUE);       //when sender is NULL it will broadcast to everyone in the client list
                }
                printClientList();
            }
            else if(strcmp(EXIT, requestBuffer) == 0) {
                strcat(responseBuffer, RED " shutdown the server" RESET "\n");
                broadcast(sender_addr, TRUE);
                printf("Exiting Server\n");
                close(sockfd);
                exit(OK);
            }
            else if(strcmp(LIST, requestBuffer) == 0) {
                sendClientList(sender_addr);
            }
            else {
                strcat(responseBuffer, RESET);
                strcat(responseBuffer, USERNAMExMESSAGE);       //inserts string between username and message to look nice
                strcat(responseBuffer, requestBuffer);

                printf("Message:\n[%s]\n", responseBuffer);
                //go through entire linked list and echo back the message to all clients connected with proper username of the sender
                broadcast(sender_addr, FALSE);  //sends message to all except sender
            }
        }
        else {

            if(connectClient(sender_addr, requestBuffer) == OK) {
                userColor(sender_addr.sin_port);
                strcat(responseBuffer, requestBuffer);
                strcat(responseBuffer, GREEN " connected" RESET "\n");
                broadcast(sender_addr, TRUE);
                sendClientList(sender_addr);
            }
            printClientList();
        }

    }

    close(sockfd);
    return OK;
}
