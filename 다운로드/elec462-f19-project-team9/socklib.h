#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <string.h>
#include <arpa/inet.h>

int make_server_socket(char *);
int make_server_socket_q(char *, int);
int connect_to_server(char *, char *);
void handling_error(char *);

