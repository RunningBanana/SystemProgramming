#include "socklib.h"
#include <net/if.h>
#include <sys/ioctl.h>
#include <pthread.h>

#define BUF_SIZE 100
#define NAME_SIZE 21
#define MAX_USER 100
#define MAX_IP 30

//client의 정보를 저장하는 struct client
typedef struct client{
	int socket;
	int roomNumber;
	char name[NAME_SIZE];
}client;

//server에 생성된 room의 정보를 저장하는 struct room
typedef struct room{
	int number;
	int users;
}room;

void *handle_clnt(void *);
void send_msg(char *, int, int);
void print_userInfo(client);
char *serverState(int);
void initSystem(char port[]);
void print_server_ip(void);
void disconnect(client); 
void joinRoom(int);
void quitRoom(int);
int chatting(client);
void printList(int);
int socket_count = 0;// accept한 client의 수
int room_count = 0; // room의 수
client socks[MAX_USER];//현재 server에 접속한 모든 client의 정보
room rooms[MAX_USER]; //현재 server에 생성된 모든 room의 정보
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
 
int main(int argc, char *argv[]){
	int i, sock_id, sock_fd, dupCheck;
	char name[NAME_SIZE];
	client sock;
	struct sockaddr_in caddr;
	socklen_t addr_len;	
	pthread_t t_id;
 
	if (argc != 2){
        printf(" Usage : %s <port>\n", argv[0]);
        exit(1);
	}

	initSystem(argv[1]);//server를 실행하였을 때 메세지 출력
	sock_id = make_server_socket(argv[1]);//client을 accept할 socket을 생성
 	
	while(1){
		//client를 accept
		addr_len = sizeof(caddr);
		sock_fd=accept(sock_id, (struct sockaddr*)&caddr, &addr_len);
	
		//client를 accept하면, client는 처음으로 name을 보내줌
		read(sock_fd, name, 21);

		//client 구조체에 현재 client의 정보 저장
		sock.socket = sock_fd;
		sock.roomNumber = 0; // 임시 방 번호
		strncpy(sock.name, name, 21);

		dupCheck = 0;//name 중복 검사
		for(i=0;i<socket_count;i++){
			if(!strcmp(sock.name, socks[i].name)){
				//name이 중복된 부분이 있다면, client에 거절 메세지를 보냄
				dupCheck = 1;
				write(sock.socket, "Decline", 10);
				//거절 메세지를 보낸 후, client에서는 server와의 연결을 끊게 됨
			}
		}
		if(!dupCheck){//name이 중복되지 않았다면, 허용 메세지를 보낸 후 client handling시작
			write(sock.socket, "Accept", 10);
       			pthread_create(&t_id, NULL, handle_clnt, (void*)&sock);
			//쓰레드를 detach시켜 종료를 기다리지 않아도 자동으로 자원이 반납되게 함
		       	pthread_detach(t_id);
		}
    	}
    	close(sock_id);
    	return 0;
}
 
void *handle_clnt(void *arg){//client들을 handling
	client sock=*((client*)arg);
	int i;
	int exit;
	char info[10];
	//현재 서버에 접속한 전체 client 구조체를 handling하기 때문에
	//동기화하지 않으면 여러 쓰레드에서 동시에 접근할 수가 있음.
	//동기화 후 client의 정보를 전역변수 socks 배열에 저장
	pthread_mutex_lock(&lock);
	socks[socket_count].socket = sock.socket;
	strncpy(socks[socket_count++].name, sock.name, 21);	
	pthread_mutex_unlock(&lock);
	
	//접속한 user의 정보를 server상에 출력함
	printf("------------------------------\n");	       	
	print_userInfo(sock);
	printf(" User enters.\n");
	printf("------------------------------\n");
	do{	
		//현재 client는 init menu상태
		exit = 0;// exit는 client가 예상치 못한 방법으로 연결을 끊었을 때를 handling함
		memset(info, '\0', sizeof(info));//info를 초기화함
		//init menu에서 client는 list를 요청하거나 입장할 room의 number를 보내줌.	
		if(read(sock.socket, info, 10) == -1)//read가 EOF을 반환한다면
			break;//소켓과의 연결이 끊어진 것이므로 break후 disconnect절차 진행
		if(!strcmp(info, "-list")){//client가 list를 요청한다면
			printList(sock.socket);//해당 client에게 방 정보를 write해줌.
			continue;//밑의 코드는 room에 insert하는 내용이므로 continue
		}
		sock.roomNumber= atoi(info);//roomNumber를 client의 소켓에 저장함
		joinRoom(sock.roomNumber);//roomNumber를 이용하여 room의 list를 최신화함.
		for (i=0; i<socket_count; i++){//현재 접속하고있는 client중
			if(socks[i].socket == sock.socket){//입장한 client의 socket 정보를 찾아
				socks[i].roomNumber = sock.roomNumber;//roomNumber를 바꾸어줌
			}
		}
		exit = chatting(sock); // 채팅 시작
		//채팅이 끝나면, roomNumber를 이용하여 room의 ist를 최신화함.
		quitRoom(sock.roomNumber);
		//채팅이 끝났으므로, roomNumber를 다시 임시 번호로 바꾸어줌.
		sock.roomNumber = 0;	
	}while(exit != -1);//exit가 -1이면 disconnect해야함
	disconnect(sock);
	close(sock.socket);
	return NULL;
}

void disconnect(client sock){
	int i;
	//현재 서버에 접속한 전체 client 구조체를 handling하기 때문에 lock을 걸어 동기화
	//전체 client구조체 중 현재 socket을 찾아 제거
	pthread_mutex_lock(&lock);
	for(i = 0; i < socket_count; i++){
		if(sock.socket == socks[i].socket){
			for(;i<socket_count-1;i++){//현재 socket보다 뒤에 있는 socket을 당겨줌
				socks[i] = socks[i+1];
			}
			break;
		}
	}
	socket_count--;
	//server에 접속을 해제한 client의 정보를 출력
	printf("------------------------------\n");
	print_userInfo(sock);
	printf(" User exits.\n");
	printf("------------------------------\n");
	pthread_mutex_unlock(&lock);
}
 
void send_msg(char* msg, int len, int roomNumber){//해당 roomNumber를 가진 client에게 msg 출력
	int i;
	//현재 서버에 접속한 전체 client 구조체를 handling하기 때문에 lock을 걸어 동기화
	pthread_mutex_lock(&lock);
	for (i=0; i<socket_count; i++){
		if(socks[i].roomNumber == roomNumber)//같은 roomNumber를 가진 socket에 msg를 write
			write(socks[i].socket, msg, len);
	}
	pthread_mutex_unlock(&lock);
}
 
void print_userInfo(client sock){//client의 정보와 전체 유저 수를 출력
	printf(" Name : %s\n", sock.name);
	printf(" The number of Total Users : (%d/%d)\n", socket_count, MAX_USER);
}       
 
void initSystem(char port[]){//Server IP, Port, MAX_USER 수를 출력
	system("clear");
    	printf("------------------------------\n");
    	printf(" Server IP      : ");
	print_server_ip();
    	printf(" Server Port    : %s\n", port);
    	printf(" User Capacity  : %d\n", MAX_USER);
    	printf("------------------------------\n");
}

void print_server_ip(void){//server ip를 출력
	struct ifreq ifr;
	char ipstr[40];
	int s;
	s = socket(AF_INET, SOCK_DGRAM, 0);
	strncpy(ifr.ifr_name, "enp0s3", IFNAMSIZ);
    
	if (ioctl(s, SIOCGIFADDR, &ifr) < 0) {
		handling_error("print_server_ip");
	}
	else {
		inet_ntop(AF_INET, ifr.ifr_addr.sa_data+2,
	    		    ipstr,sizeof(struct sockaddr));
		printf("%s\n", ipstr);
	}
	close(s);
}

void joinRoom(int num){//client가 방에 들어갈 때 room List 최신화
	int i;
	room temp;
	//현재 서버에 접속한 전체 room 구조체를 handling하기 때문에 동기화
	pthread_mutex_lock(&lock);
	for(i=0; i<room_count; i++){	//현재 입력받은 roomNumber가 이미 존재하는지 확인
		if(rooms[i].number == num)
			break;
	}
	if(i == room_count){//입력받은 roomNumber가 없으면(반복문이 중간에 break하지 않았으면)
		//새로운 room을 만들어 정보 저장		
		temp.number = num;
		temp.users = 1;
		rooms[room_count++] = temp;
	}
	else{//입력받은 roomNumber가 이미 존재한다면
		//존재하는 room의 user수 증가
		rooms[i].users++;
	}
	pthread_mutex_unlock(&lock);
}

void quitRoom(int num){//client가 방에서 나올 때 room List 최신화
	int i;
	//현재 서버에 접속한 전체 room 구조체를 handling하기 때문에 동기화
	pthread_mutex_lock(&lock);
	//client를 disconnect할 때와 동일한 방법으로 room List 최신화
	for(i=0; i<room_count; i++){	
		if(rooms[i].number == num){
			rooms[i].users--;
			if(rooms[i].users == 0){
				for(;i<room_count-1;i++){
					rooms[i] = rooms[i+1];
				}
				room_count--;
			}
			break;
		}
	}
	pthread_mutex_unlock(&lock);
}

void printList(int sock){//room의 정보를 List로 만들어 client로 전송
	int i;
	char list[1000];
	char info[BUF_SIZE];
	//list와 info 초기화
	memset(list, '\0', sizeof(list));
	memset(info, '\0', sizeof(info));
	for(i=0;i<room_count;i++){;
		//현재 index의 room의 정보를 info에 저장 후 list에 이어붙임.
		sprintf(info, "%9d %3d\n", rooms[i].number, rooms[i].users);
		strcat(list, info);
		//info는 다시 비워줌.
		memset(info, '\0', sizeof(info));
	}
	if(room_count == 0){//현재 생성된 room이 없다면
		sprintf(info, "no rooms\n");
		strcat(list, info);
	}
	//room list를 client에 전송
	write(sock, list, 1000);
}

int chatting(client sock){
	int str_len = 0;
	int i;
	char msg[BUF_SIZE];
	char quit_msg[BUF_SIZE];
	while((str_len=read(sock.socket, msg, sizeof(msg)))!=0){//client와의 연결이 끊어질 때까지
		if(!strcmp(msg, ":q\n") || !strcmp(msg, ":Q\n")){
			//client가 quit_msg를 입력하였다면
			pthread_mutex_lock(&lock);
			for(i=0; i<socket_count; i++){//해당하는 socket의 roomNumber에 임시 번호 저장
				if(socks[i].socket == sock.socket){
					socks[i].roomNumber = 0;
				}
			}
			memset(quit_msg, '\0', sizeof(quit_msg));
			sprintf(quit_msg, "[%s]'s quit.\n", sock.name);
			pthread_mutex_unlock(&lock);
			//그 방에 남아있는 client들에게 quit msg전달
			send_msg(quit_msg, strlen(quit_msg), sock.roomNumber);
			//quit한 client의 recv_msg쓰레드가 종료될 수 있도록 quit msg전달
			write(sock.socket, ":q", 3);
			//정상적인 방법으로 채팅방을 나갔으므로 0 전달			
			return 0;
		}
		else{//quit msg가 아닌 msg라면
			//msg를 해당 roomnumber를 가진 client들에게 전달
			send_msg(msg, str_len, sock.roomNumber);
		}
		//msg 초기화
		memset(msg, '\0', sizeof(msg));
	}
	//client와의 연결이 비정상적인 방법으로 끊어졌다면, -1을 리턴
	return -1;
}
