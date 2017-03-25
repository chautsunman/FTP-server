#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "dir.h"
#include "usage.h"


void process(int);


// Here is an example of how to use the above function. It also shows
// one how to get the arguments passed on the command line.

int main(int argc, char **argv) {

    // This is some sample code feel free to delete it
    // This is the main program for the thread version of nc

    int i;

    // Check the command line arguments
    if (argc != 2) {
      usage(argv[0]);
      return -1;
    }

    char *port = argv[1];


    int sockfd;
    int clientfd;

    struct addrinfo hints;
    struct addrinfo *res;

    struct sockaddr_storage clientAddr;
    socklen_t clientAddrLen;

    // initialize all members to 0
    memset(&hints, 0, sizeof hints);
    // set members
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    // get the address information
    if (getaddrinfo(NULL, port, &hints, &res) != 0) {
        return 1;
    }

    // create a socket
    // TODO: loop through all results to create a socket
    if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
        return 1;
    }

    // bind the socket to the port
    if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
        return 1;
    }

    // release the address information memory
    freeaddrinfo(res);

    // listen on the socket
    if (listen(sockfd, 1) == -1) {
        return 1;
    }

    while (1) {
        // accept a connectiion
        clientAddrLen = sizeof clientAddr;
        if ((clientfd = accept(sockfd, (struct sockaddr *) &clientAddr, &clientAddr)) == -1) {
            return 1;
        }

        process(clientfd);

        // close the client socket
        close(clientfd);

        // close the socket
        close(sockfd);
        return 0;
    }


    // This is how to call the function in dir.c to get a listing of a directory.
    // It requires a file descriptor, so in your code you would pass in the file descriptor
    // returned for the ftp server's data connection

    printf("Printed %d directory entries\n", listFiles(1, "."));
    return 0;

}


void process(int fd) {
    // send a connected message
    char *connectedMessage = "Connected.\n";
    send(fd, connectedMessage, strlen(connectedMessage), 0);
}
