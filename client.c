#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

int keepRunning = 1;
int clientSocket;

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

void DieWithError(char *message) {
    printf("\033[1;31mTERMINATING\033[0m: Error: [%s]\n", message);
    exit(EXIT_FAILURE);
}

void StatusSuccess(char *message) {
    printf("\033[1;32mSUCCESS\033[0m: Message: [%s]\n", message);
}

void TerminateClient() {
    keepRunning = 0;
    close(clientSocket);
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

int main(void)
{
    
    struct sockaddr_in server_addr, client_addr, temp_addr;
    int clientMessageCount, n;
    char clientName[25], port[10], clientThreadId[25], serverMessageBuffer[1024], clientMessageBuffer[1024];
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    signal(SIGINT, signalHandler);
    
    printf("Message Client\n");
    printf("Version 1.0\n");
    printf("Starting...\n\n");
    
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*) &tv, sizeof tv);
    
    if (clientSocket < 0) {
        printf("Unable to create socket\n");
        return -1;
    }
    
    StatusSuccess("Socket created successfully");
    printf("Socket ID\t\t\t: %i\n\n", clientSocket);
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(2000);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    if (connect(clientSocket, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0) {
        printf("Unable to connect\n");
        return -1;
    }

    //send(clientSocket, clientMessageBuffer, sizeof(clientMessageBuffer), 0);
    //recv(clientSocket, port, sizeof(port), 0);
   // send(clientSocket, clientMessageBuffer, sizeof(clientMessageBuffer), 0);
    //recv(clientSocket, clientThreadId, sizeof(clientThreadId), 0);
    
    bzero((char *) &clientName, sizeof(clientName));
    
    do {
        printf("What is your name?\t\t: ");
        fgets(clientName, sizeof(clientName), stdin);
    } while(strlen(clientName) <= 1);

    printf("\n");

    send(clientSocket, clientName, strlen(clientName) - 1, 0);
    recv(clientSocket, port, sizeof(port), 0);
    recv(clientSocket, clientThreadId, sizeof(clientThreadId), 0);
    
    //send(clientSocket, clientName, strlen(clientName) - 1, 0);
    
    StatusSuccess("Connected to server");
    printf("Client IP Address\t\t: %s\n", inet_ntoa(client_addr.sin_addr));
    printf("Client Port\t\t\t: %s\n", port);
    printf("Server IP Address\t\t: %s\n", inet_ntoa(server_addr.sin_addr));
    printf("Server Port\t\t\t: %i\n", ntohs(server_addr.sin_port));
    printf("Thread ID\t\t\t: %s\n\n", clientThreadId);

    while (keepRunning != 0) {
        bzero((char *) &clientMessageBuffer, sizeof(clientMessageBuffer));
        bzero((char *) &serverMessageBuffer, sizeof(serverMessageBuffer));
        
        do {
            printf("\033[1;33mMe [%s]\t\t: \033[0m", strtrim(clientName));
            fgets(clientMessageBuffer, sizeof(clientMessageBuffer), stdin);
        } while(strlen(clientMessageBuffer) <= 1);

        if(strcmp(clientMessageBuffer, ":EXIT\n") == 0){
            send(clientSocket, clientMessageBuffer, strlen(clientMessageBuffer), 0);
            TerminateClient();
        } else if(keepRunning != 0) {
            if (send(clientSocket, clientMessageBuffer, strlen(clientMessageBuffer), 0) < 0) 
                    TerminateClient();
            do {
                clientMessageCount = recv(clientSocket, serverMessageBuffer, sizeof(serverMessageBuffer), 0);
                
                if (clientMessageCount >=  0) 
                    printf("%s\n", serverMessageBuffer);
            } while (clientMessageCount >= 0);
        }
    } 

    close(clientSocket);
}