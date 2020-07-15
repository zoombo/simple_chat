#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <string.h>

//stdin descryptor
#define STDIN_FD 0

//sleep time for epoll_wait
#define EWAIT 100

#define DEBUG(dbgmsg) printf(dbgmsg)

int getstring(char *buf, int length);
int proto_echo_client(int sockfd);

int main(int argc, char **argv)
{

    struct sockaddr_in bind_addr, server_addr;
    int sock;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("Error create socket.");
        exit(1);
    }

    bind_addr.sin_family = AF_INET;
    bind_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(sock, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) < 0)
    {
        perror("Bind error.");
        exit(2);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    server_addr.sin_port = htons(5500);
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Connect error.");
        exit(3);
    }

    struct epoll_event ev, out_event;
    int epollfd;
    epollfd = epoll_create(2);
    ev.events = EPOLLIN;
    ev.data.fd = sock;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sock, &ev) == -1)
    {
        perror("epoll_ctl: add net socket error.");
        exit(EXIT_FAILURE);
    }
    ev.events = EPOLLIN;
    ev.data.fd = STDIN_FD;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, STDIN_FD, &ev) == -1)
    {
        perror("epoll_ctl: add stdin descriptor error.");
        exit(EXIT_FAILURE);
    }

    //main for()
    int ep_ret;
    for (;;)
    {
        proto_echo_client(sock); // Work? WORK?!! WORK!!! (WTF??? O_o)
        ep_ret = epoll_wait(epollfd, &out_event, 1, EWAIT);
        if (ep_ret == -1)
        {
            perror("epoll_wait error.");
            exit(EXIT_FAILURE);
        }
        else if (ep_ret == 0)
        {
            DEBUG("point_001\n");
            continue;
        }
        else
        {
            if (out_event.data.fd == STDIN_FD)
            {
                char msg[1024];
                int string_len = getstring(msg, sizeof(msg));
                send(sock, msg, string_len, 0);
            }
            else
            {
                //proto_echo_client(sock); // Not work :(
                char msg[1024];
                int rec_ret = recv(sock, msg, sizeof(msg), 0);
                msg[rec_ret] = '\0';
                printf(msg);
            }
        }
    }
    close(sock);
}

int getstring(char *string_buf, int length)
{
    char c = 0;
    int i = 0;
    while ((c = getchar()) != '\n' && length > i - 2)
    {
        string_buf[i] = c;
        i++;
    }
    string_buf[i++] = '\n';
    string_buf[i] = '\0';
    return i;
}

int proto_echo_client(int sockfd)
{
    char hello_serv[] = "HELLO_FROM_SERV: HELLO";
    char hello_client[] = "HELLO_FROM_CLIENT: HELLO";
    int max_len = (sizeof(hello_serv) > sizeof(hello_client)) ? sizeof(hello_serv) : sizeof(hello_client);
    char msg_buf[max_len];
    recv(sockfd, msg_buf, max_len, MSG_PEEK);

    if (strcmp(hello_serv, msg_buf) == 0)
    {
        recv(sockfd, msg_buf, max_len, 0);
        send(sockfd, hello_client, sizeof(hello_client), 0);
        DEBUG("HELLO SERVER\n");
        return 0;
    }
    /* 
    else
    {
        DEBUG("CLIENT LOST");
        return -1;
    }
    */
}