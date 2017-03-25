#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "dir.h"
#include "usage.h"


const int BUF_LEN = 64;

const char *QUIT_COMMAND_LF = "QUIT\n";
const char *QUIT_COMMAND_CRLF = "QUIT\r\n";
const char *USER_COMMAND = "USER";

const char *CONNECTED_MESSAGE = "220 Service ready.\n";
const char *LOGIN_MESSAGE = "230 User name ok.\n";
const char *INVALID_USERNAME_MESSAGE = "530 Invalid user name.\n";
const char *UNSUPPORTED_COMMAND_RESPONSE = "500 Unsupported Command.\n";

const char *USER_CS317_LF = "cs317\n";
const char *USER_CS317_CRLF = "cs317\r\n";


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
    send(fd, CONNECTED_MESSAGE, strlen(CONNECTED_MESSAGE), 0);

    char buf[BUF_LEN];
    char *command;

    while (1) {
        // receive a message
        memset(&buf, 0, sizeof(buf)/sizeof(char));
        int bytesReceived = recv(fd, buf, BUF_LEN, 0);
        printf("bytesReceived = %d\n", bytesReceived);

        if (bytesReceived == -1) {
            return;
        }

        if (bytesReceived == 0) {
            // quit
            break;
        }

        // printf("buf = %s, strlen(buf) = %d\n", buf, strlen(buf));
        command = strtok(buf, " ");
        // printf("command = %s, strlen(command) = %d\n", command, strlen(command));

        if (strcmp(command, QUIT_COMMAND_LF) == 0 || strcmp(command, QUIT_COMMAND_CRLF) == 0) {
            // quit
            break;
        } else if (strcmp(command, USER_COMMAND) == 0) {
            char *username = strtok(NULL, " ");
            // printf("username = %s, strlen(username) = %d\n", username, strlen(username));

            if (strcmp(username, USER_CS317_LF) == 0 || strcmp(username, USER_CS317_CRLF) == 0) {
                send(fd, LOGIN_MESSAGE, strlen(LOGIN_MESSAGE), 0);
            } else {
                send(fd, INVALID_USERNAME_MESSAGE, strlen(INVALID_USERNAME_MESSAGE), 0);
            }
        } else {
            // unsupported command
            send(fd, UNSUPPORTED_COMMAND_RESPONSE, strlen(UNSUPPORTED_COMMAND_RESPONSE), 0);
        }
    }
}
