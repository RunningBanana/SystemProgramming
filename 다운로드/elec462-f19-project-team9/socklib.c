#include "socklib.h"

#define HOSTLEN 256
#define BACKLOG 5

int make_server_socket(char *portnum){
	return make_server_socket_q(portnum, BACKLOG);
}


int make_server_socket_q(char *portnum, int backlog){
	struct sockaddr_in	saddr;
	int    sock_id;

	sock_id = socket(PF_INET, SOCK_STREAM, 0);
	
	if(sock_id == -1) handling_error("socket");
	
	bzero((void*)&saddr, sizeof(saddr));
	saddr.sin_addr.s_addr=htonl(INADDR_ANY);
	saddr.sin_port = htons(atoi(portnum));
	saddr.sin_family = AF_INET;

	if(bind(sock_id, (struct sockaddr*)&saddr, sizeof(saddr)) != 0)
		handling_error("bind");

	if(listen(sock_id, backlog) != 0) handling_error("listen");
	return sock_id;
}

int connect_to_server(char* host, char* portnum){
	int sock;
	struct sockaddr_in	servadd;

	sock = socket(PF_INET, SOCK_STREAM, 0);
	if(sock == -1) handling_error("socket");

	bzero(&servadd, sizeof(servadd));
	servadd.sin_addr.s_addr=inet_addr(host);
	servadd.sin_port = htons(atoi(portnum));
	servadd.sin_family = AF_INET;
	if(connect(sock, (struct sockaddr*)&servadd, sizeof(servadd)) != 0)
		handling_error("connect");
	return sock;
}

void handling_error(char *msg){
	perror(msg);
	exit(1);
}

