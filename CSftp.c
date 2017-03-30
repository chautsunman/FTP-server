#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "dir.h"
#include "usage.h"


const int BUF_LEN = 64;

// commands
const char *QUIT_COMMAND = "QUIT";
const char *USER_COMMAND = "USER";
const char *CWD_COMMAND = "CWD";
const char *CDUP_COMMAND = "CDUP";
const char *TYPE_COMMAND = "TYPE";
const char *MODE_COMMAND = "MODE";
const char *STRU_COMMAND = "STRU";
const char *RETR_COMMAND = "RETR";
const char *PASV_COMMAND = "PASV";
const char *NLST_COMMAND = "NLST";

// parameters
const char *CURRENT_DIRECTORY = "./";
const char *PARENT_DIRECTORY = "../";
const char *TYPE_A = "A";
const char *TYPE_I = "I";
enum {ASCII_TYPE, IMAGE_TYPE};
const char *MODE_S = "S";
enum {STREAM_MODE};
const char *STRU_F = "F";
enum {FILE_STRU};
const char *DATA_PORT = "45815";
const int DATA_PORT1 = 178;
const int DATA_PORT2 = 247;

// success messages
const char *CONNECTED_MESSAGE = "220 Service ready.\n";
const char *LOGIN_MESSAGE = "230 User logged in, proceed.\n";
const char *CHANGE_DIRECTORY_MESSAGE = "200 Command OK.\n";
const char *SET_TYPE_MESSAGE = "200 Command OK.\n";
const char *SET_MODE_MESSAGE = "200 Command OK.\n";
const char *SET_STRU_MESSAGE = "200 Command OK.\n";
const char *PASV_MESSAGE = "227 Entering Passive Mode (%s,%d,%d).\n";
const int PASV_MESSAGE_LEN = 64;
const char *DATA_CONNECTION_OPEN_MESSAGE = "150 Here comes the directory listing.\n";
const char *DATA_CONNECTION_CLOSE_MESSAGE = "226 Closing data connection.\n";
const char *QUIT_MESSAGE = "226 Closing data connection.\n";

enum {GET_ADDR_INFO_ERROR = -1000, CREATE_SOCKET_ERROR, BIND_SOCKET_ERROR};

// error messages
const char *INVALID_USERNAME_MESSAGE = "530 Invalid user name.\n";
const char *NOT_SIGN_IN_MESSAGE = "530 Not logged in.\n";
const char *INVALID_PATH_MESSAGE = "550 Invalid path.\n";
const char *CWD_FAIL_MESSAGE = "550 CWD failed.\n";
const char *CDUP_FAIL_MESSAGE = "550 CDUP failed.\n";
const char *INVALID_TYPE_MESSAGE = "504 Command not implemented for that parameter.\n";
const char *INVALID_MODE_MESSAGE = "504 Command not implemented for that parameter.\n";
const char *INVALID_STRU_MESSAGE = "504 Command not implemented for that parameter.\n";
const char *PASV_ERROR_MESSAGE = "500 Error.\n";
const char *DATA_OPEN_CONNECTION_ERROR_MESSAGE = "425 Can't open data connection.\n";
const char *DATA_UNAVAILABLE_ERROR_MESSAGE = "450 Requested file action not taken.\n";
const char *DATA_LOCAL_ERROR_MESSAGE = "451 Requested action aborted: local error in processing.\n";
const char *DATA_ERROR_MESSAGE = "500 Error.\n";
const char *UNSUPPORTED_COMMAND_RESPONSE = "500 Unsupported Command.\n";

// user name
const char *USER_CS317 = "cs317";


void process(int);
int getPasvMessage(char *);
int setupDataConnection();
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

        // printf("Client process end.\n");

        // close the client socket
        close(clientfd);

        // close the socket
        close(sockfd);

        // printf("Sockets closed.\n");

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

    int signIn = 0;

    int dataFd = -1;
    int clientDataFd = -1;

    struct sockaddr_storage clientDataAddr;
    socklen_t clientDataAddrLen;

    char projectDirectory[4096];
    if (getcwd(projectDirectory, 4096) == NULL) {
      // TODO: send error message
    }
    int projectDirectoryLen = strlen(projectDirectory);
    // printf("projectDirectory = %s, projectDirectoryLen = %d\n", projectDirectory, projectDirectoryLen);

    int type = ASCII_TYPE;
    // printf("type = %d\n", type);

    int mode = STREAM_MODE;
    // printf("mode = %d\n", mode);

    int stru = FILE_STRU;
    // printf("stru = %d\n", stru);

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
            send(fd, QUIT_MESSAGE, strlen(QUIT_MESSAGE), 0);

            signIn = 0;

            // close the client data socket
            close(clientDataFd);

            // close the data socket
            close(dataFd);

            // quit
            break;
        }

        if (strcmp(command, USER_COMMAND) == 0) {
            char *username = strtok(NULL, " ");
            // printf("username = %s, strlen(username) = %d\n", username, strlen(username));

            if (strcmp(username, USER_CS317) == 0) {
                signIn = 1;
                send(fd, LOGIN_MESSAGE, strlen(LOGIN_MESSAGE), 0);
            } else {
                send(fd, INVALID_USERNAME_MESSAGE, strlen(INVALID_USERNAME_MESSAGE), 0);
            }

            continue;
        }

        if (!signIn) {
            send(fd, NOT_SIGN_IN_MESSAGE, strlen(NOT_SIGN_IN_MESSAGE), 0);
            continue;
        }

        // user signed in

        if (strcmp(command, CWD_COMMAND) == 0) {
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
                // printf("currentWorkingDirectory = %s\n", currentWorkingDirectory);

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
                // printf("newWorkingDirectory = %s\n", newWorkingDirectory);

                if (chdir(newWorkingDirectory) != 0) {
                    send(fd, CDUP_FAIL_MESSAGE, strlen(CDUP_FAIL_MESSAGE), 0);
                    continue;
                }

                send(fd, CHANGE_DIRECTORY_MESSAGE, strlen(CHANGE_DIRECTORY_MESSAGE), 0);
            } else {
                send(fd, CDUP_FAIL_MESSAGE, strlen(CDUP_FAIL_MESSAGE), 0);
            }
        } else if (strcmp(command, TYPE_COMMAND) == 0) {
            char *t = strtok(NULL, " ");
            // printf("t = %s\n", t);

            if (strcmp(t, TYPE_A) == 0) {
                type = ASCII_TYPE;
                send(fd, SET_TYPE_MESSAGE, strlen(SET_TYPE_MESSAGE), 0);
            } else if (strcmp(t, TYPE_I) == 0) {
                type = IMAGE_TYPE;
                send(fd, SET_TYPE_MESSAGE, strlen(SET_TYPE_MESSAGE), 0);
            } else {
                send(fd, INVALID_TYPE_MESSAGE, strlen(INVALID_TYPE_MESSAGE), 0);
            }

            // printf("type = %d\n", type);
        } else if (strcmp(command, MODE_COMMAND) == 0) {
            char *m = strtok(NULL, " ");
            // printf("m = %s\n", m);

            if (strcmp(m, MODE_S) == 0) {
                mode = STREAM_MODE;
                send(fd, SET_MODE_MESSAGE, strlen(SET_MODE_MESSAGE), 0);
            } else {
                send(fd, INVALID_MODE_MESSAGE, strlen(INVALID_MODE_MESSAGE), 0);
            }

            // printf("mode = %d\n", mode);
        } else if (strcmp(command, STRU_COMMAND) == 0) {
            char *s = strtok(NULL, " ");
            // printf("s = %s\n", s);

            if (strcmp(s, STRU_F) == 0) {
                stru = FILE_STRU;
                send(fd, SET_STRU_MESSAGE, strlen(SET_STRU_MESSAGE), 0);
            } else {
                send(fd, INVALID_STRU_MESSAGE, strlen(INVALID_STRU_MESSAGE), 0);
            }

            // printf("stru = %d\n", stru);
        } else if (strcmp(command, PASV_COMMAND) == 0) {
            char pasvFormatMessage[PASV_MESSAGE_LEN];

            if (getPasvMessage(pasvFormatMessage) != 0) {
                send(fd, PASV_ERROR_MESSAGE, strlen(PASV_ERROR_MESSAGE), 0);
                continue;
            }

            send(fd, pasvFormatMessage, strlen(pasvFormatMessage), 0);

            // setup the data connection
            if ((dataFd = setupDataConnection()) < 0) {
                if (dataFd == GET_ADDR_INFO_ERROR) {
                    send(fd, DATA_LOCAL_ERROR_MESSAGE, strlen(DATA_LOCAL_ERROR_MESSAGE), 0);
                    continue;
                } else if (dataFd == CREATE_SOCKET_ERROR || dataFd == BIND_SOCKET_ERROR) {
                    send(fd, DATA_OPEN_CONNECTION_ERROR_MESSAGE, strlen(DATA_OPEN_CONNECTION_ERROR_MESSAGE), 0);
                    continue;
                } else {
                    send(fd, DATA_LOCAL_ERROR_MESSAGE, strlen(DATA_LOCAL_ERROR_MESSAGE), 0);
                    continue;
                }
            }

            // listen on the socket
            if (listen(dataFd, 1) == -1) {
                send(fd, DATA_OPEN_CONNECTION_ERROR_MESSAGE, strlen(DATA_OPEN_CONNECTION_ERROR_MESSAGE), 0);
                continue;
            }

            // accept a connectiion
            struct sockaddr_storage clientDataAddr;
            clientDataAddrLen = sizeof clientDataAddr;
            if ((clientDataFd = accept(dataFd, (struct sockaddr *) &clientDataAddr, &clientDataAddr)) == -1) {
                send(fd, DATA_OPEN_CONNECTION_ERROR_MESSAGE, strlen(DATA_OPEN_CONNECTION_ERROR_MESSAGE), 0);
                continue;
            }
        } else if (strcmp(command, NLST_COMMAND) == 0) {
            send(fd, DATA_CONNECTION_OPEN_MESSAGE, strlen(DATA_CONNECTION_OPEN_MESSAGE), 0);

            int entriesPrinted;
            if ((entriesPrinted = listFiles(clientDataFd, ".")) < 0) {
                if (entriesPrinted == -1) {
                    send(fd, DATA_UNAVAILABLE_ERROR_MESSAGE, strlen(DATA_UNAVAILABLE_ERROR_MESSAGE), 0);
                } else if (entriesPrinted == -2) {
                    send(fd, DATA_LOCAL_ERROR_MESSAGE, strlen(DATA_LOCAL_ERROR_MESSAGE), 0);
                } else {
                    send(fd, DATA_LOCAL_ERROR_MESSAGE, strlen(DATA_LOCAL_ERROR_MESSAGE), 0);
                }
            }

            send(fd, DATA_CONNECTION_CLOSE_MESSAGE, strlen(DATA_CONNECTION_CLOSE_MESSAGE), 0);

            // close the client data socket
            close(clientDataFd);
            clientDataFd = -1;

            // close the data socket
            close(dataFd);
            dataFd = -1;
        } else {
            // unsupported command
            send(fd, UNSUPPORTED_COMMAND_RESPONSE, strlen(UNSUPPORTED_COMMAND_RESPONSE), 0);
        }
    }
}


int getPasvMessage(char *pasvFormatMessage) {
    char ipStr[INET_ADDRSTRLEN];

    // get the local IP
    char hostname[128];
    if (gethostname(hostname, sizeof hostname) == -1) {
        return -1;
    }
    // printf("hostname = %s\n", hostname);
    struct hostent *he;
    if ((he = gethostbyname(hostname)) == NULL) {
        return -1;
    }
    struct in_addr **ip_list = (struct in_addr **) he->h_addr_list;
    if (ip_list[0] == NULL) {
        return -1;
    }
    strcpy(ipStr, inet_ntoa(*(ip_list[0])));
    // printf("local ip = %s\n", ipStr);
    /* inet_ntop(AF_INET, &(((struct sockaddr_in *) res->ai_addr)->sin_addr), ipStr, INET_ADDRSTRLEN);
    if (ipStr == NULL) {
        return -1;
    } */
    char *p;
    for (p = ipStr; *p != 0; p++) {
        if (*p == 46) {
            *p = 44;
        }
    }
    // printf("ipStr = %s, strlen(ipStr) = %d, port = %s, port1 = %d, port2 = %d\n", ipStr, strlen(ipStr), DATA_PORT, DATA_PORT1, DATA_PORT2);

    // format the message
    if (sprintf(pasvFormatMessage, PASV_MESSAGE, ipStr, DATA_PORT1, DATA_PORT2) < 0) {
        return -1;
    }

    return 0;
}


int setupDataConnection() {
    int fd;

    struct addrinfo hints;
    struct addrinfo *res;

    // initialize all members to 0
    memset(&hints, 0, sizeof hints);
    // set members
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    // get the address information
    if (getaddrinfo(NULL, DATA_PORT, &hints, &res) != 0) {
        return GET_ADDR_INFO_ERROR;
    }

    // create a socket
    // TODO: loop through all results to create a socket
    if ((fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
        return CREATE_SOCKET_ERROR;
    }

    // bind the socket to the port
    if (bind(fd, res->ai_addr, res->ai_addrlen) == -1) {
        return BIND_SOCKET_ERROR;
    }

    // release the address information memory
    freeaddrinfo(res);

    return fd;
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
