#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#define TRUE 1 

int main(void)
{
    int socket_desc;
    struct sockaddr_in server_addr, client_addr, temp_addr;
    char server_message[2000], client_message[2000];
    
    printf("Message Client\n");
    printf("Version 1.0\n");
    printf("Starting...\n\n");
    
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    
    if (socket_desc < 0) {
        printf("Unable to create socket\n");
        return -1;
    }
    
    printf("Socket created successfully\n");
    printf("Socket ID: %i\n", socket_desc);
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(2000);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    if (connect(socket_desc, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0) {
        printf("Unable to connect\n");
        return -1;
    }

    printf("\nConnected to server\n");
    printf("Client IP Address: %s\n", inet_ntoa(client_addr.sin_addr));
    printf("Client Port: %i\n", ntohs(client_addr.sin_port));
    printf("Server IP Address: %s\n", inet_ntoa(server_addr.sin_addr));
    printf("Server Port: %i\n\n", ntohs(server_addr.sin_port));
    
    
    while (TRUE) {
        memset(server_message,'\0',sizeof(server_message));
        memset(client_message,'\0',sizeof(client_message));

        printf("Message: ");
        fgets(client_message, sizeof(client_message) - 1, stdin);
        
        if (send(socket_desc, client_message, strlen(client_message) - 1, 0) < 0) {
            printf("Unable to send message\n");
            return -1;
        }
        
        if (recv(socket_desc, server_message, sizeof(server_message), 0) < 0) {
            printf("Error while receiving server's msg\n");
            return -1;
        }
        
        printf("Server: %s\n",server_message);
    }
    
    close(socket_desc);
    
    return 0;
}