#include "socklib.h"
#include <pthread.h>
#include <signal.h> 
#define MSG_SIZE 100
#define BUF_SIZE 20
#define NAME_SIZE 21

char name[NAME_SIZE]; 
char msg[MSG_SIZE];     
char server_ip[BUF_SIZE];
char port[BUF_SIZE];
int roomNumber;

void* send_msg(void* arg);
void* recv_msg(void* arg); 
void initMenu(void);
int joinMenu(int);
void printRoom(int);
int main(int argc, char *argv[]){
	int sock; //server와 연결할 소켓
	char dupCheck[10]; //name 중복체크에 필요한 문자열
	pthread_t snd_thread, rcv_thread; // msg를 보내는 쓰레드, 받는 쓰레드

	if (argc!=4){
	printf(" Usage : %s <server_ip> <port> <name>\n", argv[0]);
	exit(1);
	}
	//Ctrl+C, Ctrl+\ 시그널을 무시
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);

	//각각의 string에 server_ip, port, name을 차례대로 저장
	sprintf(server_ip, "%s", argv[1]);
	sprintf(port, "%s", argv[2]);
	sprintf(name, "%s", argv[3]);

	//server_ip, port로 connect
	sock = connect_to_server(argv[1], argv[2]);

	//server에 자신의 name을 보내줌(Name 중복체크를 위함)
	write(sock, argv[3], 21);

	//server에서 중복체크의 결과를 받아옴
	read(sock, dupCheck, 10);

	if(strcmp(dupCheck,"Accept")){//서버 접속이 허용되지 않았다면(거절 메세지를 받음)
		//메세지를 띄우고 소켓을 닫음.
		printf("Name is Duplicated. Please enter another name.\n");
		close(sock);
		return 0;
	}
	while(1){
		//initMenu 출력
		initMenu();
		//stdin으로부터 메뉴를 결정하는 문자열을 받음
		fgets(msg, BUF_SIZE, stdin);	
	
		//:I나 :i를 받았다면 joinMenu로 들어감
		if(!strcmp(msg, ":i\n") || !strcmp(msg, ":I\n")){
			if(joinMenu(sock)){//joinMenu의 리턴값이 0이면 initMenu로 돌아감
				//joinMenu의 리턴값이 1이면 쓰레드 실행
				pthread_create(&snd_thread, NULL, send_msg, (void*)&sock);
				pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock);
				//두 쓰레드가 모두 종료될 때까지 기다림
				pthread_join(snd_thread, NULL);
				pthread_join(rcv_thread, NULL);
			}	
		}
		//-list를 받았다면 현재 server에 생성된 방의 정보를 출력 후, joinMenu로 들어감
		else if(!strcmp(msg, "-list\n")){
			printRoom(sock);
			if(joinMenu(sock)){//joinMenu의 리턴값이 0이면 initMenu로 돌아감
				//joinMenu의 리턴값이 1이면 쓰레드 실행
				pthread_create(&snd_thread, NULL, send_msg, (void*)&sock);
				pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock);
				//두 쓰레드가 모두 종료될 때까지 기다림
				pthread_join(snd_thread, NULL);
				pthread_join(rcv_thread, NULL);
			}
		}
		//:Q나 :q를 받았다면, 반복문을 빠져나간 후 소켓을 닫고 프로그램 종료.
		else if(!strcmp(msg, ":q\n") || !strcmp(msg, ":Q\n")){
			break;
		}
		//snd_thread, rcv_thread가 모두 종료되었다면 initMenu로 돌아감.
		//만약, 메뉴에 해당하지 않는 잘못된 문자를 받았다면 무시 후 다시 initMenu출력
	}
	close(sock);	
	return 0;
}

void* send_msg(void* arg){//server로 메세지를 보내는 쓰레드
	int sock=*((int*)arg);
	char name_msg[NAME_SIZE + MSG_SIZE];
	char join_msg[NAME_SIZE + 10];

	//안내 메세지 출력
	printf(" Join with room number %d.\n", roomNumber);
	printf(" If you want to exit, Enter ':Q' or ':q'.\n");
	printf(" ------------------------------\n");
	//server로 join_msg를 보냄(server는 해당 방에 있는 client들에게 join_msg를 보내줌)	
	sprintf(join_msg, "[%s]'s join.\n", name);
	write(sock, join_msg, strlen(join_msg));

	while(1){
		//msg를 초기화한 후, stdin으로부터 msg를 받음
		memset(msg, '\0', sizeof(msg));
		fgets(msg, MSG_SIZE, stdin); 
		if (!strcmp(msg, ":q\n") || !strcmp(msg, ":Q\n")){//msg가 quit msg였다면
			//server에게 quit_msg 전달 (채팅방에서 나가겠다고 알림)	
			write(sock, msg, strlen(msg));
			//쓰레드 종료
			return NULL;
		}
		else{
			//msg가 quit msg가 아니었다면 msg를 일정 형식을 취해 server로 보냄
			sprintf(name_msg, "[%s] %s", name, msg);
			write(sock, name_msg, strlen(name_msg));
		}
	}
}
 
void* recv_msg(void* arg){//server에서 메세지를 받는 쓰레드
    int sock=*((int*)arg);
    char name_msg[NAME_SIZE+MSG_SIZE];
    int str_len;

    while(1)
    {
	//name_msg를 초기화 한 후 server에서 client의 name을 포함한 msg를 받음
	memset(name_msg, '\0', sizeof(name_msg));
        str_len=read(sock, name_msg, NAME_SIZE+MSG_SIZE-1);
        if (str_len==-1)//read하는 것을 실패하였다면(소켓이 닫힘, EOF)
		handling_error("read");//error를 핸들링(socklib.c에 구현되어있음.)
	if(!strcmp(name_msg, ":q")){//서버에서 quit msg를 받았다면
		//이 quit msg는 send_msg 쓰레드에서 서버에 보낸 후, 서버를 통해 받음
		return NULL; //쓰레드 종료
	}
        fputs(name_msg, stdout);//quit msg나 error가 아니라면 server서 받은 msg를 stdout으로 출력
    }
}
 
 
void initMenu(void){//프로그램을 처음 실행하였을 때 나오는 Menu화면
	system("clear");
	printf("--------------------------------\n");
	printf(" Server IP   : %s\n", server_ip);
	printf(" Server Port : %s\n", port);
	printf(" User name   : %s\n", name);
	printf("--------------------------------\n");
	printf(" If you want to join a room,\n");
	printf(" Please enter ':I' or ':i'.\n");
	printf(" If you want to display a room list,\n");
	printf(" Please enter '-list'.\n");
	printf(" If you want to quit the server,\n");
	printf(" please enter ':Q' or ':q'.\n");
	printf("--------------------------------\n");
}

int joinMenu(int sock){//방에 join하고자 할 때 나오는 Menu화면
	printf("--------------------------------\n");
	printf(" Please enter the room number.\n");
	printf(" If you want to quit this menu,\n");
	printf(" please enter ':Q' or ':q'.\n");
	printf("--------------------------------\n");
	while(1){
		//방 번호를 입력 받음.
		fgets(msg, BUF_SIZE, stdin);
		//quit message가 입력되었다면 0을 리턴
		if(!strcmp(msg, ":q\n") || !strcmp(msg, ":Q\n")){
			return 0;
		}
		roomNumber = atoi(msg);//msg를 int형으로 바꿈.
		if(roomNumber <= 0){//방 번호가 0 이하거나 정수가 아닐 때 출력되는 메세지
			printf(" The number must be an integer more than 0.\n");
		}
		else if(strlen(msg) >= 10){//방 번호의 길이가 10 이상일때 출력되는 메세지
			printf(" The number must be less than 10 digits.\n");
		}
		else{//정상적인 방 번호를 입력받았다면 방 번호를 server에 전달해준 후 1을 리턴
			write(sock, msg, 10);
			return 1;
		}
	}
	return 0;
	//리턴값이 1이라면, joinMenu종료 후 쓰레드를 실행하여 채팅방 접속
	//리턴값이 0이라면, joinMenu종료 후 initMenu로 돌아감.
}

void printRoom(int sock){//server로부터 받아온 방의 정보를 출력
	char list[1000];
	printf("   num     users\n");
	write(sock, "-list", 6);//server에게 list를 요청함.
	read(sock, list, 1000);//server에게 list를 받은 후 출력
	printf("%s", list);
}	
