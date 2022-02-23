#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>
#include <server-tools.h>

typedef struct threadNode {
    int id;
    pthread_t threadId;
    int socketId;
    struct sockaddr_in addr;
    char clientName[25];
    int keepRunning;
    struct threadNode *next;
} threadNode_t;

typedef struct threadInfo {
    pthread_mutex_t mut;
    threadNode_t *head;
    int serverSocket;
    struct sockaddr_in serverAddr;
    int clientSocket;
    struct sockaddr_in clientAddr;
    int clientAddrSize;
    int threadCount;
    int keepRunning;
    char timeBuffer[8];
} threadInfo_t;

threadInfo_t clientSocketThreads;

int TerminateTCPConnection(threadNode_t *threadNode);
void *HandleTCPConnection(void *clientThreadNode);

void InitClientSocketThreads() {
   clientSocketThreads.keepRunning = 1;
   clientSocketThreads.head = NULL; 
   clientSocketThreads.threadCount = 0; 
}

threadNode_t *AddClientSocketThread(pthread_t threadId, int socketId, struct sockaddr_in addr) {
    pthread_mutex_lock(&clientSocketThreads.mut);

    threadNode_t *newThreadNode;
    threadNode_t *lastThreadNode;
    lastThreadNode = clientSocketThreads.head;
    newThreadNode = (threadNode_t *) malloc(sizeof(threadNode_t));

    newThreadNode->id = clientSocketThreads.threadCount;
    newThreadNode->threadId = threadId;
    newThreadNode->socketId = socketId;
    newThreadNode->addr = addr;
    newThreadNode->keepRunning = 1;
    newThreadNode->next = NULL;
    clientSocketThreads.threadCount = clientSocketThreads.threadCount + 1;

    if (clientSocketThreads.head == NULL) {   
       clientSocketThreads.head = newThreadNode;
    } else {
        while (lastThreadNode->next != NULL)
            lastThreadNode = lastThreadNode->next;
        
        lastThreadNode->next = newThreadNode;
    }

    pthread_mutex_unlock(&clientSocketThreads.mut);
    return newThreadNode;
}

void TerminateServer() {
    clientSocketThreads.keepRunning = 0;
    printf("\n");
    StatusSuccess("Terminate Server");
    threadNode_t *lastThreadNode;
    threadNode_t *nextThreadNode;
    lastThreadNode = clientSocketThreads.head;

    while (lastThreadNode != NULL) {
        nextThreadNode = lastThreadNode->next;
        TerminateTCPConnection(lastThreadNode);
        lastThreadNode = nextThreadNode;
    }

    close(clientSocketThreads.serverSocket);
    StatusSuccess("Server Terminated Successfully");
    pthread_exit(NULL);
    exit(0);
}

void signalHandler(int sig) {
    char  c;

    signal(sig, SIG_IGN);
    
    printf("\nTERMINATE: did you hit Ctrl-C?\nDo you really want to quit? [y/n] : ");
    c = getchar();
    
    if (c == 'y' || c == 'Y') {
        TerminateServer();
    } else {
        printf("Sig");
        signal(SIGINT, signalHandler);
    }
}

void GetDateAndTime(char *timeFormat) {
    time_t currentTime;
    struct tm *timeInfo;
    char timeBuffer[80];

    time(&currentTime);
    timeInfo = localtime(&currentTime);
       strftime(timeBuffer,80,timeFormat, timeInfo);

    strcpy(timeBuffer, clientSocketThreads.timeBuffer);
}

void PrintSocketInformation() {
    StatusSuccess("Listening for incoming connections ....");
    printf("%-50s%s%s\n", "IP Address", " : ", inet_ntoa(clientSocketThreads.serverAddr.sin_addr));
    printf("%-50s%s%i\n", "Port", " : ", ntohs(clientSocketThreads.serverAddr.sin_port));
    printf("%-50s%s%i\n\n", "Listen Socket ID", " : ", clientSocketThreads.serverSocket);
}

void PrintConnectionInformation(threadNode_t *threadNode, char *title) {
    StatusSuccess(title);
    printf("%-50s%s%i\n", "Client ID", " : ", threadNode->id);
    printf("%-50s%s%s\n", "Client Name", " : ", threadNode->clientName);
    printf("%-50s%s%s\n", "IP Address", " : ", inet_ntoa(threadNode->addr.sin_addr));
    printf("%-50s%s%i\n", "Port", " : ", ntohs(threadNode->addr.sin_port));
    printf("%-50s%s%lu\n\n", "Thread ID", " : ", (unsigned long) threadNode->threadId);
}
    
int main(void) {
    pthread_t threadId;
    char* currentTime;

    pthread_mutex_init(&clientSocketThreads.mut, NULL);
   
    InitClientSocketThreads();
    signal(SIGINT, signalHandler);
    signal(SIGTRAP, signalHandler);
    signal(SIGABRT, signalHandler);
    signal(SIGALRM, signalHandler);
    signal(SIGILL, signalHandler);
    signal(SIGABRT, signalHandler);
    
    printf("Message Server\n");
    printf("Version 1.0\n");
    printf("Starting...\n\n");

    clientSocketThreads.serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (clientSocketThreads.serverSocket < 0) 
        DieWithError("Error while creating socket");
    
    StatusSuccess("Socket created successfully");
    
    clientSocketThreads.serverAddr.sin_family = AF_INET;
    clientSocketThreads.serverAddr.sin_port = htons(2000);
    clientSocketThreads.serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    if (bind(clientSocketThreads.serverSocket, (struct sockaddr*) &clientSocketThreads.serverAddr, sizeof(clientSocketThreads.serverAddr)) < 0)
        DieWithError("Couldn't bind to the port");
    
    StatusSuccess("Done with binding server to IP and Port...");
    
    if (listen(clientSocketThreads.serverSocket, 1) < 0)
        DieWithError("Error while listening\n");
    
    PrintSocketInformation();

    while (clientSocketThreads.keepRunning == 1) {
        clientSocketThreads.clientAddrSize = sizeof(clientSocketThreads.clientAddr);
        clientSocketThreads.clientSocket = accept(clientSocketThreads.serverSocket, (struct sockaddr*) & clientSocketThreads.clientAddr, &clientSocketThreads.clientAddrSize);
        if (clientSocketThreads.clientSocket < 0) {
           if (clientSocketThreads.keepRunning == 1) 
                DieWithError("Unable to Accept Connection"); 
        } else {
            threadNode_t *clientThreadNode;
            clientThreadNode = AddClientSocketThread(0, clientSocketThreads.clientSocket, clientSocketThreads.clientAddr);
            if (pthread_create(&clientThreadNode->threadId , NULL, HandleTCPConnection, (void *) clientThreadNode) != 0) 
                DieWithError("Unable to create thread.\n");
        }
    };

    TerminateServer();
}

int TerminateTCPConnection(threadNode_t *threadNode) {
    //pthread_mutex_lock(&clientSocketThreads.mut);
    int returnValue;
    threadNode_t *previousThreadNode;
    threadNode_t *lastThreadNode;
    
    threadNode->keepRunning = 0;
    PrintConnectionInformation(threadNode, "Client Disconnected");
    lastThreadNode = clientSocketThreads.head;
    
    if (lastThreadNode != NULL && lastThreadNode->id == threadNode->id) {
        clientSocketThreads.head = lastThreadNode->next; 
    } else {
        while (lastThreadNode != NULL && lastThreadNode->id != threadNode->id) {
            previousThreadNode = lastThreadNode;
            lastThreadNode = lastThreadNode->next;
        }

        previousThreadNode->next = lastThreadNode->next;
    }
    
    if (lastThreadNode == NULL)
        returnValue = -1;
    else
        returnValue = lastThreadNode->id;
    
    close(threadNode->socketId);
    pthread_cancel(threadNode->threadId);
    free(lastThreadNode);
    //pthread_mutex_unlock(&clientSocketThreads.mut);
    
    return returnValue;
}

void *HandleTCPConnection(void *clientThreadNode) {
    int clientMessageLength;
    char clientId[10];
    char clientName[25];
    char port[10];
    char clientThreadId[25];
    threadNode_t *threadNode = (threadNode_t*) clientThreadNode;
    threadNode_t *lastThreadNode;
    char serverMessageBuffer[1024], clientMessageBuffer[1024];

    signal(SIGINT, signalHandler);

    bzero((char *) &threadNode->clientName, sizeof(threadNode->clientName));
    bzero((char *) &clientMessageBuffer, sizeof(clientMessageBuffer));
    bzero((char *) &serverMessageBuffer, sizeof(serverMessageBuffer));
        
    if (threadNode->socketId < 0)
        DieWithError("Can't accept. Connection may have been closed.\n");
    
    recv(threadNode->socketId, threadNode->clientName, sizeof(threadNode->clientName), 0);

    sprintf(clientId, "%i", threadNode->id);
    send(threadNode->socketId, clientId, sizeof(clientId), 0);

    sprintf(port, "%i", ntohs(threadNode->addr.sin_port));
    send(threadNode->socketId, port, sizeof(port), 0);

    sprintf(clientThreadId, "%li", (unsigned long)threadNode->threadId);
    send(threadNode->socketId, clientThreadId, sizeof(clientThreadId), 0);
    PrintConnectionInformation(clientThreadNode, "Client Connected");
    
    while (threadNode->keepRunning == 1) {
        bzero((char *) &clientMessageBuffer, sizeof(clientMessageBuffer));
        bzero((char *) &serverMessageBuffer, sizeof(serverMessageBuffer));
        
        if (clientMessageLength = recv(threadNode->socketId, clientMessageBuffer, sizeof(clientMessageBuffer), 0) < 0) 
            TerminateTCPConnection((void *) threadNode);
        
        if (strlen(clientMessageBuffer) <= 0) 
            TerminateTCPConnection((void *) threadNode);

        if(strcmp(clientMessageBuffer, ":EXIT") == 0) {
            TerminateTCPConnection((void *) threadNode);
        } else if (strlen(strtrim(clientMessageBuffer)) > 0) {
            printf("%s%-43s%s%s\n", "Client \033[1;33m", threadNode->clientName, "\033[0m : ", strtrim(clientMessageBuffer));

            lastThreadNode = clientSocketThreads.head;
            
            while (lastThreadNode != NULL) {
                bzero((char *) &serverMessageBuffer, sizeof(serverMessageBuffer));
                sprintf(serverMessageBuffer, "%s%-43s%s%s", "Server \033[1;33m", threadNode->clientName, " \033[0m: ", strtrim(clientMessageBuffer));
                send(lastThreadNode ->socketId, serverMessageBuffer, sizeof(serverMessageBuffer), 0); 
                lastThreadNode = lastThreadNode->next;
            }
        }
    }

    TerminateTCPConnection(threadNode);
}

