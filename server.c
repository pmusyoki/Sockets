#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>

void *HandleTCPConnection(void *clientThreadNode);
void TerminateTCPConnection(void *threadNode);

int is_white_space(char c) {
    return (c == ' ' || c == '\t' || c == '\n');
}

int get_first_position(char const *str) {
    int i = 0;
    while (is_white_space(str[i])) {
        i += 1;
    }
    return (i);
}

int get_str_len(char const *str) {
    int len = 0;
    while (str[len] != '\0') {
        len += 1;
    }
    return (len);
}

int get_last_position(char const *str) {
    int i = get_str_len(str) - 1;
    while (is_white_space(str[i])) {
        i -= 1;
    }
    return (i);
}

int get_trim_len(char const *str) {
    return (get_last_position(str) - get_first_position(str));
}

char *strtrim(char const *str) {
    char *trim = NULL;
    int i, len, start, end;
    if (str != NULL) {
        i = 0;
        len = get_trim_len(str) + 1;
        trim = (char *)malloc(len);
        start = get_first_position(str);
        while (i < len) {
            trim[i] = str[start];
            i += 1;
            start += 1;
        }
        trim[i] = '\0';
    }
    return (trim);
}

pthread_mutex_t lock;

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

void InitClientSocketThreads() {
   clientSocketThreads.keepRunning = 1;
   clientSocketThreads.head = NULL; 
   clientSocketThreads.threadCount = 0; 
}

threadNode_t *AddClientSocketThread(pthread_t threadId, int socketId, struct sockaddr_in addr) {
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
       return newThreadNode;
    }

    while (lastThreadNode->next != NULL)
        lastThreadNode = lastThreadNode->next;
    
    lastThreadNode->next = newThreadNode;
    return newThreadNode;
}

int UserName(int clientId, char *userName) {
    threadNode_t *lastThreadNode;
    lastThreadNode = clientSocketThreads.head;
   
    while (lastThreadNode->next != NULL) {
        if (lastThreadNode->id == clientId) {
            strcpy(userName, lastThreadNode->clientName);
            return lastThreadNode->id;
        }

        lastThreadNode = lastThreadNode->next;
    }

    return -1;
}

void DieWithError(char *message) {
    printf("\033[1;31mTERMINATING\033[0m: Error: [%s]\n", message);
    exit(EXIT_FAILURE);
}

void StatusSuccess(char *message) {
    printf("\033[1;32mSUCCESS\033[0m: Message: [%s]\n", message);
}

void TerminateServer() {
    clientSocketThreads.keepRunning = 0;
    printf("\n");
    StatusSuccess("Terminate Server");
    threadNode_t *lastThreadNode;
    lastThreadNode = clientSocketThreads.head;

    while (lastThreadNode != NULL) {
        TerminateTCPConnection((void *) lastThreadNode);
        lastThreadNode = lastThreadNode->next;
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
    

int main(void)
{
    pthread_t threadId;
    char* currentTime;
   
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
    
    StatusSuccess("Listening for incoming connections ....");
    printf("\nIP Address\t\t: %s\nPort\t\t\t: %i\nListen Socket ID\t: %i\n", inet_ntoa(clientSocketThreads.serverAddr.sin_addr), ntohs(clientSocketThreads.serverAddr.sin_port),  clientSocketThreads.serverSocket);
    
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

void TerminateTCPConnection(void *clientThreadNode) {
    threadNode_t *threadNode = (threadNode_t*) clientThreadNode;
    threadNode->keepRunning = 0;
    printf("\n");
    StatusSuccess("Client Disconnected");
    printf("IP Address\t\t\t: %s\nPort\t\t\t\t: %i\nClient Socket ID\t\t: %i\nThread ID\t\t\t: %lu\nClient Number\t\t\t: %i\nClient Name\t\t\t: %s\n\n", inet_ntoa(threadNode->addr.sin_addr), ntohs(threadNode->addr.sin_port), threadNode->socketId, (unsigned long) threadNode->threadId, threadNode->id, threadNode->clientName);
    close (threadNode->socketId);
    pthread_cancel(threadNode->threadId);
}

void *HandleTCPConnection(void *clientThreadNode) {
    int clientMessageLength;
    char clientName[25];
    char port[10];
    char clientThreadId[25];
    threadNode_t *threadNode = (threadNode_t*) clientThreadNode;
    threadNode_t *lastThreadNode;
    char serverMessageBuffer[1024], clientMessageBuffer[1024];

    signal(SIGINT, signalHandler);

    bzero((char *) &clientMessageBuffer, sizeof(clientMessageBuffer));
    bzero((char *) &serverMessageBuffer, sizeof(serverMessageBuffer));
        
    if (threadNode->socketId < 0)
        DieWithError("Can't accept. Connection may have been closed.\n");
    
    printf("\n");
    StatusSuccess("Client Connected");

    recv(threadNode->socketId, threadNode->clientName, sizeof(threadNode->clientName), 0);
    sprintf(port, "%i", ntohs(threadNode->addr.sin_port));
    send(threadNode->socketId, port, sizeof(port), 0);

    sprintf(clientThreadId, "%li", (unsigned long)threadNode->threadId);
    send(threadNode->socketId, clientThreadId, sizeof(clientThreadId), 0);
    
    printf("IP Address\t\t\t: %s\nPort\t\t\t\t: %i\nClient Socket ID\t\t: %i\nThread ID\t\t\t: %lu\nClient Number\t\t\t: %i\nClient Name\t\t\t: %s\n\n", inet_ntoa(threadNode->addr.sin_addr), ntohs(threadNode->addr.sin_port), threadNode->socketId, (unsigned long) threadNode->threadId, threadNode->id, threadNode->clientName);
    
    while (threadNode->keepRunning == 1) {
        bzero((char *) &clientMessageBuffer, sizeof(clientMessageBuffer));
        bzero((char *) &serverMessageBuffer, sizeof(serverMessageBuffer));
        
        if (clientMessageLength = recv(threadNode->socketId, clientMessageBuffer, sizeof(clientMessageBuffer), 0) < 0) 
            TerminateTCPConnection((void *) threadNode);
        
        if (strlen(clientMessageBuffer) <= 0) 
            TerminateTCPConnection((void *) threadNode);

        if(strcmp(clientMessageBuffer, ":EXIT") == 0) {
            TerminateTCPConnection((void *) threadNode);
        } else if (strlen(strtrim(clientMessageBuffer) > 0)) {
            printf("\033[1;33mClient [%s]\t\t: \033[0m%s\n", threadNode->clientName, strtrim(clientMessageBuffer));
            
            lastThreadNode = clientSocketThreads.head;
            
            while (lastThreadNode != NULL) {
                bzero((char *) &serverMessageBuffer, sizeof(serverMessageBuffer));
                strcpy(serverMessageBuffer, "Server [");
                
                strcat(serverMessageBuffer, threadNode->clientName);
                strcat(serverMessageBuffer, "] \t: ");
                strcat(serverMessageBuffer, strtrim(clientMessageBuffer));
                
                send(lastThreadNode ->socketId, serverMessageBuffer, sizeof(serverMessageBuffer), 0); 
                lastThreadNode = lastThreadNode->next;
            }
        }
    }

    TerminateTCPConnection(threadNode);
}

