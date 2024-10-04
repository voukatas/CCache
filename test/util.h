#ifndef UTIL_H
#define UTIL_H
#include <stdio.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "../include/connection.h"
#include "../include/server.h"
#include "../include/signal_handler.h"
#include "../unity/unity.h"



#define TEST_PORT 8080

void connect_disconnect_client(int port, char *ip);
void print_msg(char *msg);
void wait_for_no_active_connections(unsigned int waiting_time);
void shutdown_server(void);
void wait_for_server_to_start(unsigned int waiting_time);
void start_server(void);
void *run_server_thread(void *arg);
int connect_client(int port, char *ip);
void disconnect_client(int sockfd);
void connect_disconnect_client(int port, char *ip);
void send_client_msg_and_wait_response(int sockfd, char *msg, char *buffer);
void send_client_msg(int sockfd, char *msg);
void send_client_msg_in_new_conn_and_wait_response(char *msg, int port,
                                                   char *ip, char *buffer);

extern const unsigned int WAITING_SERVER_TIMEOUT;
extern const unsigned int WAITING_SERVER_INIT;
extern char *TEST_ADDRESS;

#endif // UTIL_H
