#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stddef.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <string.h>
#include <time.h>

//size of array "clients_socket_list"
#define _SIZE_OF_ARRAY_CLIENTS_SOCKET_LIST 5
#define MAX_CLIENTS _SIZE_OF_ARRAY_CLIENTS_SOCKET_LIST

//sleep time for epoll_wait
#define EWAIT 1000


//_START_Support defines
#define DEBUG(dbgmsg) printf(dbgmsg)
#define DEBUG_TIME(dbgmsg) printf("%s : %ld", dbgmsg, time(NULL))
#define DEBUG_TIME_NL(dbgmsg) printf("%s : %ld\n", dbgmsg, time(NULL))
#define DEBIG_PUT_VAR(spec, var) printf(spec, var)
#define nl_c putchar('\n')
//_END_Support defines

int proto_echo_serv(int sockfd);

int main(int argc, char **argv)
{
    int client_sock, listener_sock;
    struct sockaddr_in addr;
    char buf[1024];
    int bytes_read = 0;
    //epoll listener vars
    struct epoll_event listener_ev, listener_wait_ev;
    int listener_epollfd;
    //epoll clients vars
    struct epoll_event client_ev, clients_wait_events[MAX_CLIENTS];
    int clients_epollfd;

    listener_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listener_sock < 0)
    {
        perror("Error create listener socket.");
        exit(EXIT_FAILURE);
    }
    // Set nonblock listener socket.
    fcntl(listener_sock, F_SETFL, O_NONBLOCK);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(5500);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(listener_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("Bind error.");
        exit(EXIT_FAILURE);
    }

    listen(listener_sock, 10);

    //epoll start init listener
    listener_epollfd = epoll_create(1);
    if (listener_epollfd == -1)
    {
        perror("epoll_create failure.");
        exit(EXIT_FAILURE);
    }
    listener_ev.events = EPOLLIN;
    listener_ev.data.fd = listener_sock;
    if (epoll_ctl(listener_epollfd, EPOLL_CTL_ADD, listener_sock, &listener_ev) == -1)
    {
        perror("epoll_ctl error.");
        exit(EXIT_FAILURE);
    }

    //epoll start init clients
    clients_epollfd = epoll_create(MAX_CLIENTS);
    if (clients_epollfd == -1)
    {
        perror("Failure create clients epoll_create()");
        exit(EXIT_FAILURE);
    }

    //main for() vars
    int ep_ret = 0, clients_count = 0, clients_socket_list[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        clients_socket_list[i] = 0;
    }
    //main for()
    for (;;)
    {
        //wait new client socket
        ep_ret = epoll_wait(listener_epollfd, &listener_wait_ev, 1, EWAIT);
        if (ep_ret == -1)
        {
            perror("Wait listeners error.");
            exit(EXIT_FAILURE);
        }

        //TODO: Add check client lost or recv
        //add new client socket
        if (ep_ret > 0 && clients_count < MAX_CLIENTS - 1)
        {
            client_sock = accept(listener_sock, NULL, NULL);
            if (client_sock < 0)
            {
                perror("Accept error.");
                exit(EXIT_FAILURE);
            }
            //Add to the end of the list(array) if there is space.
            if (clients_socket_list[clients_count] == 0)
            {
                clients_socket_list[clients_count] = client_sock;
                clients_count++;
            }
            else //Else, search of near free space(0) in array
            {
                for (int i = 0; i < MAX_CLIENTS; i++)
                {
                    if (clients_socket_list[i] == 0)
                    {
                        clients_socket_list[i] = client_sock;
                        clients_count++;
                        break;
                    }
                }
            }

            client_ev.events = EPOLLIN;
            client_ev.data.fd = client_sock;
            if (epoll_ctl(clients_epollfd, EPOLL_CTL_ADD, client_sock, &client_ev) == -1)
            {
                perror("epoll_ctl add client socket error.");
                exit(EXIT_FAILURE);
            }
            DEBUG("New client.\n");
        }
        else if (clients_count >= MAX_CLIENTS - 1)
        {
            client_sock = accept(listener_sock, NULL, NULL);
            close(client_sock);
            perror("Limit clients.");
        }

        DEBUG_TIME_NL("point_001");
        //ping clients
        if (clients_count > 0)
        {
            DEBUG_TIME_NL("point_002");
            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                DEBUG_TIME_NL("point_003");
                if (clients_socket_list[i] == 0)
                {
                    DEBIG_PUT_VAR("i = %d\n", i);
                    continue;
                }
                DEBUG_TIME_NL("point_004");
                if (proto_echo_serv(clients_socket_list[i]) == -1)
                {
                    client_ev.events = EPOLLIN;
                    client_ev.data.fd = clients_socket_list[i];
                    if (epoll_ctl(clients_epollfd, EPOLL_CTL_DEL, clients_socket_list[i], &client_ev) == -1)
                    {
                        perror("epoll_ctl add client socket error.");
                        exit(EXIT_FAILURE);
                    }
                    close(clients_socket_list[i]);
                    clients_socket_list[i] = 0;
                    clients_count--;

                    DEBUG("Delete client.\n");
                }
            }
        }

        //receive and send messages
        for (;;)
        {
            ep_ret = epoll_wait(clients_epollfd, clients_wait_events, 1, EWAIT);
            if (ep_ret == -1)
            {
                perror("epoll clients wait error.");
                exit(EXIT_FAILURE);
            }
            else if (ep_ret == 0) // If no messages from clients
            {
                break;
            }

            bytes_read = recv(clients_wait_events[0].data.fd, buf, sizeof(buf), 0);
            if (bytes_read == -1 || bytes_read == 0)
            {
                break;
            }
            buf[bytes_read] = '\0';

            /*
            if (bytes_read > 0 && bytes_read < 2)
            {
                buf[0] = '\n';
                buf[1] = '\0';
            }
            */
            DEBUG("point_001\n");

            if (bytes_read > 0)
            {
                DEBUG("point_002\n");
                printf("%s\n", buf);
                for (int i = 0; i < clients_count; i++)
                {
                    if (clients_socket_list[i] == clients_wait_events[0].data.fd) //no send message for sender
                        continue;
                    if (send(clients_socket_list[i], buf, bytes_read, 0) == -1) //send msg, if error del client from clients_socket_list and epoll
                    {
                        client_ev.events = EPOLLIN;
                        client_ev.data.fd = clients_socket_list[i];
                        if (epoll_ctl(clients_epollfd, EPOLL_CTL_DEL, clients_socket_list[i], &client_ev) == -1)
                        {
                            perror("epoll_ctl add client socket error.");
                            exit(EXIT_FAILURE);
                        }
                        close(clients_socket_list[i]);
                        clients_socket_list[i] = 0;
                        clients_count--;

                        DEBUG("Delete client.\n");
                    }
                }
            }
        }

        //close(client_sock);
    }

    close(listener_sock);
    return 0;
}

int proto_echo_serv(int sockfd)
{
    char hello_serv[] = "HELLO_FROM_SERV: HELLO";
    char hello_client[] = "HELLO_FROM_CLIENT: HELLO";
    int max_len = (sizeof(hello_serv) > sizeof(hello_client)) ? sizeof(hello_serv) : sizeof(hello_client);
    int recv_len = 0;
    char msg_buf[max_len];
    DEBUG_TIME_NL("point_in_echo_001");
    send(sockfd, hello_serv, sizeof(hello_serv), 0);
    DEBUG_TIME_NL("point_in_echo_002");
    recv_len = recv(sockfd, msg_buf, max_len, MSG_PEEK);
    DEBUG_TIME_NL("point_in_echo_003");
    if (strcmp(hello_client, msg_buf) == 0)
    {
        recv(sockfd, msg_buf, max_len, 0);
        DEBUG("HELLO CLIENT\n");
        return 0;
    }
    else
    {
        DEBUG("CLIENT LOST\n");
        return -1;
    }
}