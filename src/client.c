#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <server-tools.h>

void *HandleIncomingMessages();

typedef struct clientConnectionStruct {
    pthread_t threadId;
    int keepRunning;
    int clientSocket;
    char clientId[10];
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    char clientName[25];
    char port[10];
    char clientThreadId[25];
    char serverMessageBuffer[1024];
    char clientMessageBuffer[1024];
    int clientMessageLength;
    int waitingForInput;
} clientConnection_t;

clientConnection_t clientConnection;

void TerminateClient() {
    clientConnection.keepRunning = 0;
    close(clientConnection.clientSocket);
    pthread_cancel(clientConnection.threadId);
    StatusSuccess("Disconnected from Server. Session Ended.");
    exit(0);
}

void signalHandler(int sig) {
    char  c;

    signal(sig, SIG_IGN);
    
    do {
        printf("\n\nTERMINATE: Did you hit Ctrl-C?\nDo you really want to quit? [y/n] : ");
        c = getchar();
    } while((c != 'y') && (c != 'Y') && (c != 'N') && (c != 'n'));
    
    if (c == 'y' || c == 'Y') {
       TerminateClient();
    } else {
        signal(SIGINT, signalHandler);
    }
}

void PrintConnectionInformation(char *title) {
    StatusSuccess(title);
    printf("%-50s%s%s\n", "Client ID", " : ", clientConnection.clientId);
    printf("%-50s%s%s\n", "Client Name", " : ", strtrim(clientConnection.clientName));
    printf("%-50s%s%s\n", "IP Address", " : ", inet_ntoa(clientConnection.clientAddr.sin_addr));
    printf("%-50s%s%s\n", "Port", " : ", clientConnection.port);
    printf("%-50s%s%s\n\n", "Thread ID", " : ", clientConnection.clientThreadId);
}

int main(void) {
    int waitingForInput;
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    clientConnection.keepRunning = 1;

    signal(SIGINT, signalHandler);
    
    printf("Message Client\n");
    printf("Version 1.0\n");
    printf("Starting...\n\n");
    
    clientConnection.clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(clientConnection.clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*) &tv, sizeof tv);
    
    if (clientConnection.clientSocket < 0) {
        printf("Unable to create socket\n");
        return -1;
    }
    
    StatusSuccess("Socket created successfully");
    printf("%-50s%s%i\n", "Scoket ID", " : ", clientConnection.clientSocket);
    
    clientConnection.serverAddr.sin_family = AF_INET;
    clientConnection.serverAddr.sin_port = htons(2000);
    clientConnection.serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    if (connect(clientConnection.clientSocket, (struct sockaddr*) &clientConnection.serverAddr, sizeof(clientConnection.serverAddr)) < 0) {
        printf("Unable to connect\n");
        return -1;
    }

    bzero((char *) &clientConnection.clientName, sizeof(clientConnection.clientName));
    
    do {
        printf("%-50s%s", "What's your name? ", " : ");
        fgets(clientConnection.clientName, sizeof(clientConnection.clientName), stdin);
    } while(strlen(clientConnection.clientName) <= 1);

    printf("\n");

    send(clientConnection.clientSocket, clientConnection.clientName, strlen(clientConnection.clientName) - 1, 0);
    recv(clientConnection.clientSocket, clientConnection.clientId, sizeof(clientConnection.clientId), 0);
    recv(clientConnection.clientSocket, clientConnection.port, sizeof(clientConnection.port), 0);
    recv(clientConnection.clientSocket, clientConnection.clientThreadId, sizeof(clientConnection.clientThreadId), 0);

    PrintConnectionInformation("Connected to Server");

    if (pthread_create(&clientConnection.threadId , NULL, HandleIncomingMessages, NULL) != 0) 
        DieWithError("Unable to create thread.\n");
    
    while (clientConnection.keepRunning != 0) {
        bzero((char *) &clientConnection.clientMessageBuffer, sizeof(clientConnection.clientMessageBuffer));
        bzero((char *) &clientConnection.serverMessageBuffer, sizeof(clientConnection.serverMessageBuffer));
        
        clientConnection.waitingForInput = 1;

        do {
            printf("%s%-47s%s", "Me \033[1;33m", strtrim(clientConnection.clientName), "\033[0m : ");
            fgets(clientConnection.clientMessageBuffer, sizeof(clientConnection.clientMessageBuffer), stdin);
        } while(strlen(clientConnection.clientMessageBuffer) <= 1);

        clientConnection.waitingForInput = 0;

        if(strcmp(clientConnection.clientMessageBuffer, ":EXIT\n") == 0){
            send(clientConnection.clientSocket, clientConnection.clientMessageBuffer, strlen(clientConnection.clientMessageBuffer), 0);
            TerminateClient();
        } else if(clientConnection.keepRunning != 0) {
            if (send(clientConnection.clientSocket, clientConnection.clientMessageBuffer, strlen(clientConnection.clientMessageBuffer), 0) < 0) 
                    TerminateClient();

            clientConnection.clientMessageLength = recv(clientConnection.clientSocket, clientConnection.serverMessageBuffer, sizeof(clientConnection.serverMessageBuffer), 0);
            
            if(clientConnection.clientMessageLength > -1 && strlen(strtrim(clientConnection.serverMessageBuffer)) > 0)
                printf("%s\n", clientConnection.serverMessageBuffer);
        }
    } 

    close(clientConnection.clientSocket);
}

void sighandler(int signum) {
   printf("Caught signal %d, coming out...\n", signum);
   exit(1);
}

void *HandleIncomingMessages() {
    signal(SIGINT, signalHandler);
    
    while (clientConnection.keepRunning != 0) {
        do {
            bzero((char *) &clientConnection.clientMessageBuffer, sizeof(clientConnection.clientMessageBuffer));
            clientConnection.clientMessageLength = recv(clientConnection.clientSocket, clientConnection.serverMessageBuffer, sizeof(clientConnection.serverMessageBuffer), 0);
            
            if (clientConnection.clientMessageLength > -1 && strlen(strtrim(clientConnection.serverMessageBuffer)) > 0) {
                if (clientConnection.waitingForInput == 1) 
                    printf("\n");

                printf("%s\n", clientConnection.serverMessageBuffer);
                clientConnection.waitingForInput == 0;
            }
        } while (clientConnection.clientMessageLength > -1);

        sleep(1);
    }

    pthread_exit(NULL);
}