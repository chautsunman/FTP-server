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

const char *QUIT_COMMAND = "QUIT";
const char *USER_COMMAND = "USER";
const char *CWD_COMMAND = "CWD";
const char *CDUP_COMMAND = "CDUP";
const char *CURRENT_DIRECTORY = "./";
const char *PARENT_DIRECTORY = "../";

const char *CONNECTED_MESSAGE = "220 Service ready.\n";
const char *LOGIN_MESSAGE = "230 User name ok.\n";
const char *INVALID_USERNAME_MESSAGE = "530 Invalid user name.\n";
const char *CHANGE_DIRECTORY_MESSAGE = "200 Command OK.\n";
const char *INVALID_PATH_MESSAGE = "550 Invalid path.\n";
const char *CWD_FAIL_MESSAGE = "550 CWD failed.\n";
const char *CDUP_FAIL_MESSAGE = "550 CDUP failed.\n";
const char *UNSUPPORTED_COMMAND_RESPONSE = "500 Unsupported Command.\n";

const char *USER_CS317 = "cs317";


void process(int);
void getBufLine(char *);


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

    char projectDirectory[4096];
    if (getcwd(projectDirectory, 4096) == NULL) {
      // TODO: send error message
    }
    int projectDirectoryLen = strlen(projectDirectory);
    printf("projectDirectory = %s, projectDirectoryLen = %d\n", projectDirectory, projectDirectoryLen);

    while (1) {
        // receive a message
        memset(&buf, 0, sizeof(buf)/sizeof(char));
        int bytesReceived = recv(fd, buf, BUF_LEN, 0);
        // printf("bytesReceived = %d\n", bytesReceived);

        if (bytesReceived == -1) {
            return;
        }

        if (bytesReceived == 0) {
            // quit
            break;
        }

        getBufLine(&buf);
        // printf("bufLine = %s, strlen(bufLine) = %d\n", buf, strlen(buf));

        // printf("buf = %s, strlen(buf) = %d\n", buf, strlen(buf));
        command = strtok(buf, " ");
        // printf("command = %s, strlen(command) = %d\n", command, strlen(command));

        if (strcmp(command, QUIT_COMMAND) == 0) {
            // quit
            break;
        } else if (strcmp(command, USER_COMMAND) == 0) {
            char *username = strtok(NULL, " ");
            // printf("username = %s, strlen(username) = %d\n", username, strlen(username));

            if (strcmp(username, USER_CS317) == 0) {
                send(fd, LOGIN_MESSAGE, strlen(LOGIN_MESSAGE), 0);
            } else {
                send(fd, INVALID_USERNAME_MESSAGE, strlen(INVALID_USERNAME_MESSAGE), 0);
            }
        } else if (strcmp(command, CWD_COMMAND) == 0) {
            char *path = strtok(NULL, " ");
            // printf("path = %s, strlen(path) = %d\n", path, strlen(path));

            if (strstr(path, CURRENT_DIRECTORY) == path) {
                send(fd, INVALID_PATH_MESSAGE, strlen(INVALID_PATH_MESSAGE), 0);
                continue;
            }

            if (strstr(path, PARENT_DIRECTORY) != NULL) {
                send(fd, INVALID_PATH_MESSAGE, strlen(INVALID_PATH_MESSAGE), 0);
                continue;
            }

            if (chdir(path) != 0) {
                send(fd, CWD_FAIL_MESSAGE, strlen(CWD_FAIL_MESSAGE), 0);
                continue;
            }

            send(fd, CHANGE_DIRECTORY_MESSAGE, strlen(CHANGE_DIRECTORY_MESSAGE), 0);
        } else if (strcmp(command, CDUP_COMMAND) == 0) {
            char currentWorkingDirectory[4096];
            char *newWorkingDirectory;

            if (getcwd(currentWorkingDirectory, 4096) != NULL) {
                printf("currentWorkingDirectory = %s\n", currentWorkingDirectory);

                if (*(currentWorkingDirectory + projectDirectoryLen) == 0) {
                    send(fd, CDUP_FAIL_MESSAGE, strlen(CDUP_FAIL_MESSAGE), 0);
                    continue;
                }

                newWorkingDirectory = currentWorkingDirectory;

                int i;
                for (i = strlen(newWorkingDirectory); i != projectDirectoryLen; i--) {
                    if (newWorkingDirectory[i] == 47) {
                      break;
                    }
                }

                *(newWorkingDirectory + i) = 0;
                printf("newWorkingDirectory = %s\n", newWorkingDirectory);

                if (chdir(newWorkingDirectory) != 0) {
                    send(fd, CDUP_FAIL_MESSAGE, strlen(CDUP_FAIL_MESSAGE), 0);
                    continue;
                }

                send(fd, CHANGE_DIRECTORY_MESSAGE, strlen(CHANGE_DIRECTORY_MESSAGE), 0);
            } else {
                send(fd, CDUP_FAIL_MESSAGE, strlen(CDUP_FAIL_MESSAGE), 0);
            }
        } else {
            // unsupported command
            send(fd, UNSUPPORTED_COMMAND_RESPONSE, strlen(UNSUPPORTED_COMMAND_RESPONSE), 0);
        }
    }
}


void getBufLine(char *buf) {
    char *i;
    for (i = buf; *i != 0; i++) {
        if (*i == 10 || *i == 13) {
            *i = 0;
            break;
        }
    }
}
