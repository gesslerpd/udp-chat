/* client */

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "chat.h"

int done = FALSE;               //boolean that tells threads when client program is finished

int sockfd;                     //socket file descriptor for client

pthread_mutex_t mutexsum = PTHREAD_MUTEX_INITIALIZER;   //mutual exclusion for our threads

void *sender();                 // thread function that will take user input and send out messages to server

void *receiver();               // thread function that will listen for received messages coming from the server

char sendBuffer[BUF_SIZE];
char receiveBuffer[BUF_SIZE + USERNAME_LEN + 2];

int main(int argc, char *argv[]) {
    bzero(sendBuffer, BUF_SIZE);        //zero out both buffers
    bzero(receiveBuffer, BUF_SIZE + USERNAME_LEN + 2);

    int portnum;
    char username[USERNAME_LEN];

    if(argc != 4) {
        fprintf(stderr, "Usage: %s [server] [portnum] [username]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    portnum = atoi(argv[2]);
    strncpy(username, argv[3], USERNAME_LEN);

    printf("server: %s\n", argv[1]);
    printf("port: %d\n", portnum);
    printf("username: %s\n", username);

    //allow server to resolve hostnames or use ip's
    struct hostent *server_host;

    if((server_host = gethostbyname(argv[1])) == NULL) {        // get the host info
        fprintf(stderr, "Failed to resolve server host information\n");
        exit(EXIT_FAILURE);
    }

    printf("Host: %s\n", server_host->h_name);
    printf("IP Address of host: %s\n", inet_ntoa((struct in_addr)
                                                 *((struct in_addr *)
                                                   server_host->h_addr)));

    struct sockaddr_in server_addr;     // server's internet address used for all sends and receives

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd == -1) {          //socket() returns -1 on error
        close(sockfd);
        fprintf(stderr, "Failed to get socket file descriptor\n");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;   // host byte order
    server_addr.sin_port = htons(portnum);      // short, network byte order
    server_addr.sin_addr = *((struct in_addr *) server_host->h_addr);
    memset(&(server_addr.sin_zero), '\0', 8);   // zero the rest of the struct

    //Make connection to server socket so we can use send() and recv() to read and write the server
    if(connect(sockfd, (struct sockaddr *) &server_addr, sizeof(struct sockaddr)) == SYSERR) {
        close(sockfd);
        fprintf(stderr, "Failed to connect to remote server!\n");
        exit(EXIT_FAILURE);
    }

    // Create and send out open message to the server so it knows our username and we are identified as a connected client
    strcpy(sendBuffer, username);
    if(send(sockfd, sendBuffer, strlen(sendBuffer), 0) == SYSERR) {
        perror("send");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    //create threads
    //Thread 1: takes in user input and sends out messages
    //Thread 2: listens for messages that are comming in from the server and prints them to screen
    // Set up threads
    pthread_t threads[2];
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    // Run the sender and receiver threads
    pthread_create(&threads[0], &attr, sender, NULL);
    pthread_create(&threads[1], &attr, receiver, NULL);

    // Wait until done is TRUE then exit program
    while(!done);

    close(sockfd);
    return OK;
}

void *sender() {
    while(1) {
        bzero(sendBuffer, BUF_SIZE);
        fgets(sendBuffer, BUF_SIZE, stdin);

        //send message to server 
        if(send(sockfd, sendBuffer, strlen(sendBuffer), 0) == SYSERR) {
            perror("send");
            done = TRUE;
            pthread_mutex_destroy(&mutexsum);
            pthread_exit(NULL);
        }

        // Check for quiting
        if(strcmp(sendBuffer, CLOSE) == 0 || strcmp(sendBuffer, EXIT) == 0) {
            done = TRUE;
            pthread_mutex_destroy(&mutexsum);
            pthread_exit(NULL);
        }

        pthread_mutex_unlock(&mutexsum);
    }
}

void *receiver() {
    int nbytes;

    while(1) {
        bzero(receiveBuffer, BUF_SIZE);

        //Receive messages from server
        if((nbytes = recv(sockfd, receiveBuffer, BUF_SIZE - 1, 0)) == SYSERR) {
            perror("recv");
            done = TRUE;
            pthread_mutex_destroy(&mutexsum);
            pthread_exit(NULL);
        }

        receiveBuffer[nbytes] = '\0';
        if(strcmp(ERROR, receiveBuffer) == 0) {
            printf("Error: The username %s is already taken.\n", sendBuffer);
            done = TRUE;
            pthread_mutex_destroy(&mutexsum);
            pthread_exit(NULL);
        }
        else {
            printf("%s", receiveBuffer);
            pthread_mutex_unlock(&mutexsum);
        }
    }
}
