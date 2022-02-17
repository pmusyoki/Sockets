#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>

void CloseConnection(int socket_desc, int client_sock);
void *HandleTCPConnection(void *clientThreadNode);

int keepRunning = 1;
int socket_desc, client_sock;

typedef struct threadNode {
    int id;
    pthread_t threadId;
    int socketId;
    struct sockaddr_in addr;
    struct threadNode *next;
} threadNode_t;

typedef struct threadInfo {
    threadNode_t *head;
    int threadCount;
} threadInfo_t;

threadInfo_t clientSocketThreads;

void InitClientSocketThreads() {
   clientSocketThreads.head = NULL; 
   clientSocketThreads.threadCount = 0; 
}

threadNode_t InitClientThreadNode(int id, pthread_t *threadId, int socketId, struct sockaddr_in addr) {
    threadNode_t tempThreadNode;

    tempThreadNode.id = clientSocketThreads.threadCount;
    tempThreadNode.threadId = threadId;
    tempThreadNode.socketId = socketId;
    tempThreadNode.addr = addr;
    tempThreadNode.next = NULL;

    return tempThreadNode;
}

threadNode_t *AddClientSocketThread(pthread_t threadId, int socketId, struct sockaddr_in addr) {
    if (clientSocketThreads.head == NULL) {
        clientSocketThreads.head = (threadNode_t *) malloc(sizeof(threadNode_t));
        clientSocketThreads.head->next = NULL;
    }

    threadNode_t *currentThread = clientSocketThreads.head;

    while (currentThread->next != NULL) {
        currentThread  = currentThread->next;
    }

    currentThread->next = (threadNode_t *) malloc(sizeof(threadNode_t));
    currentThread->next->id = clientSocketThreads.threadCount;
    currentThread->next->threadId = threadId;
    currentThread->next->socketId = socketId;
    currentThread->next->addr = addr;
    currentThread->next->next = NULL;
    clientSocketThreads.threadCount++;

    return currentThread->next;
}

int RemoveClientSocketThread(int id) {
    int removedId;
    threadNode_t *currentThread = clientSocketThreads.head;
    threadNode_t * tempThread = NULL;

    for (int i = 0; i < id-1; i++) {
        if (currentThread->next == NULL) {
            return -1;
        }

        currentThread = currentThread->next;
    }

    if (currentThread->next == NULL) {
        return -1;
    }

    tempThread = currentThread->next;
    removedId = tempThread->id;
    currentThread->next = tempThread->next;
    free(tempThread);

    return removedId;
}

void signalHandler(int sig) {
    char  c;

    signal(sig, SIG_IGN);
    
    printf("OUCH, did you hit Ctrl-C?\nDo you really want to quit? [y/n] : ");
    c = getchar();
    
    if (c == 'y' || c == 'Y') {
        keepRunning = 0;
        CloseConnection(socket_desc, client_sock);
    } else {
        signal(SIGINT, signalHandler);
    }
}

void DieWithError(char *message) {
    printf("\033[1;31mTERMINATING\033[0m: Error: [%s]\n", message);
    exit(EXIT_FAILURE);
}

void StatusSuccess(char *message) {
    printf("\033[1;32mSUCCESS\033[0m: Message: [%s]\n", message);
}

char *GetDateAndTime(char *timeFormat) {
    time_t currentTime;
    struct tm *timeInfo;
    char timeBuffer[80];

    time(&currentTime);
    timeInfo = localtime(&currentTime);
       strftime(timeBuffer,80,timeFormat, timeInfo);

    return timeBuffer;
}

void CloseConnection(int socket_desc, int client_sock) {
    printf("Terminating connection ...\n");
    close(client_sock);
    close(socket_desc);
}



int main(void)
{
    pthread_t threadId;
    char* currentTime;
   
    int client_size;
    struct sockaddr_in server_addr, client_addr;
    struct in_addr temp_addr;

    InitClientSocketThreads();
    
    signal(SIGINT, signalHandler);
    signal(SIGSEGV, signalHandler);
    
    printf("Message Server\n");
    printf("Version 1.0\n");
    printf("Starting...\n\n");

    socket_desc = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_desc < 0) 
        DieWithError("Error while creating socket");
    
    StatusSuccess("Socket created successfully");
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(2000);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    if (bind(socket_desc, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0)
        DieWithError("Couldn't bind to the port");
    
    StatusSuccess("Done with binding server to IP and Port...");
    
    if (listen(socket_desc, 1) < 0)
        DieWithError("Error while listening\n");
    
    StatusSuccess("Listening for incoming connections ....");
    printf("IP Address: %s\nPort: %i\nListen Socket ID: %i\nServer IP Address (Binary): %i\n\n", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port), socket_desc, server_addr.sin_addr.s_addr) ;
    
    while (keepRunning) {
        threadNode_t *clientThreadNode;
        clientThreadNode = (threadNode_t *) malloc(sizeof(threadNode_t));

        client_size = sizeof(client_addr);
        client_sock = accept(socket_desc, (struct sockaddr*) &client_addr, &client_size);

        clientThreadNode = AddClientSocketThread(0, client_sock, client_addr);

        if (pthread_create(&clientThreadNode->threadId , NULL, HandleTCPConnection, clientThreadNode) != 0) 
            DieWithError("Unable to create thread.\n");
        
        //if (pthread_join(clientThreadNode->threadId, NULL) != 0) 
        //    DieWithError("Thread create error.");
    }
}

void *HandleTCPConnection(void *clientThreadNode) {
    char* currentTime;
    int EXIT = 0;
    int clientMessageLength;
    threadNode_t *threadNode = (threadNode_t*) clientThreadNode;
    char server_message[2000], client_message[2000];
    char *tempMessage;

    if (client_sock < 0)
        DieWithError("Can't accept. Connection may have been closed.\n");
    
    currentTime = GetDateAndTime("%x - %I:%M%p");
    StatusSuccess("Client Connected");
    printf("IP Address:  %s\nPort: %i\nClient Socket ID: %i\nThread ID: %lu\n, Client Number: %i\n\n", inet_ntoa(threadNode->addr.sin_addr), ntohs(threadNode->addr.sin_port), threadNode->socketId, (unsigned long) threadNode->threadId, clientSocketThreads.threadCount);
    
    while (keepRunning && !EXIT) {
        memset(server_message, '\0', sizeof(server_message));
        memset(client_message, '\0', sizeof(client_message));

        if (clientMessageLength = recv(threadNode->socketId, client_message, sizeof(client_message), 0) < 0)
            DieWithError("Couldn't receive. Connection may have been closed.");
        
        printf("Client: %s\n", client_message);
        currentTime = GetDateAndTime("%x - %I:%M%p");
        
        strcpy(server_message, "This is the server's message. Your Message: [");
        strcat(server_message, client_message);
        strcat(server_message, "]");
        
        if (send(threadNode->socketId, server_message, strlen(server_message), 0) < 0)
            DieWithError("Can't send.");

        // if (strcmp (&client_message, "EXIT")) {
        //     printf("---------------------------------");
        //     char *exitMessage;

        //     strcpy(exitMessage , "Client Exiting: IP Address: [");
        //     strcat(exitMessage, (char *) inet_ntoa(threadNode->addr.sin_addr));
        //     strcat(exitMessage, "], Port: [");
        //     strcat(exitMessage, (ntohs(threadNode->addr.sin_port)));
        //     strcat(exitMessage, "]. DONE.");

        //     strcat(server_message, exitMessage);
        //     StatusSuccess(exitMessage);
        //     EXIT = 1;
        // }
    }

    close(threadNode->socketId);
    pthread_exit(NULL);
}

