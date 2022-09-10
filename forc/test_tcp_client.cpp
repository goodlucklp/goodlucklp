#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
 
#define BUFFSIZE 1024
 
int main(int argc, char *argv[])
{
    int client_sockfd = 0;
    int len = 0;
    struct sockaddr_in server_addr;
    char buf[BUFFSIZE] = {0};
    bzero(&server_addr, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(8000);
 
    if((client_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket error!\n");
        return -1;
    }
 
    if(connect(client_sockfd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr)) < 0)
    {
        perror("client error!\n");
        return -1;
    }
 
    printf("connect success!\n");
 
    len = recv(client_sockfd, buf, BUFFSIZE, 0);
    buf[len] = '\0';
    printf("client_buf = %s\n",buf);
 
    while(1)
    {
        printf("Enter string to send:");
        scanf("%s",buf);
        if(!strcmp(buf,"quit"))
        {
            break;
        }
 
        len = send(client_sockfd, buf, strlen(buf), 0);
        len = recv(client_sockfd, buf, BUFFSIZE, 0);
        buf[len] = '\0';
        printf("received: %s\n",buf);
    }
 
    close(client_sockfd);
 
    return 0;
}

