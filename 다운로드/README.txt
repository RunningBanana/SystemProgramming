System Programming Team Project 

Team9 : 2016110097 김동환, 2016113529 이형민

Subject : chatting program
---------------------------------------------------------------------------------------------
시작하기에 앞서

가상머신으로 Server를 만들고 싶다면,
브릿지 어댑팅 방식을 사용하여
가상머신 guest_OS가 host_OS와 동등한 수준의 IP를 할당받도록 하여야 합니다.

프로젝트 폴더에 Makeserver, Makeclient 파일이 첨부되어 있습니다.
첨부한 소스코드 chatting_server.c, chatting_client.c, socklib.c, socklib.h를 같은 폴더에 위치시킨 후
make -f Makeserver, make -f Makeclient를 이용하면
자동으로 서버 프로그램과 클라이언트 프로그램을 생성해줍니다.
makefile들의 target중에는 clean이 있습니다.
clean을 실행하면 컴파일 된 파일들(*.o)이 모두 삭제됩니다.
프로그램 이름은 chatting_server, chatting_client 입니다.
-------------------------------------------------------------------------------------------

이 프로젝트는 chatting_server, chatting_client 2개의 프로그램으로 이루어져 있습니다.

chatting_server는 각각의 client들을 server에 연결시켜 주는 역할입니다.
Usage : ./chatting_server <port>	ex) ./chatting_server 1234
<port>는 서버를 열고자 하는 포트의 번호입니다.

chatting_server를 실행시키면 실행한 장비의 IP의 지정한 포트로 서버를 열어줍니다.
이후, 서버에 client가 들어오고 나갈 때 마다, 해당 client의 정보와, 서버에 접속한 client의 수 등을 출력합니다.
server프로그램은 signal을 따로 handling하지 않았으므로, Ctrl+C나, Ctrl+\키를 이용하여 서버를 종료할 수 있습니다.


chatting_client는 입력한 서버ip와 포트에 client를 연결시켜줍니다.
Usage : ./chatting_client <server_ip> <server_port> <name> ex) ./chatting_client 10.0.2.15 1234 myung4386
<server_ip>와 <server_port>는 연결하고자 하는 서버의 ip와 포트입니다.
<name>은 다른 client들에게 보여질 나의 이름입니다.(server에 이미 해당 name이 있다면, 서버에 연결되지 않습니다.)

client는 입력한 server_ip, server_port로 서버에 connect를 시도합니다.
connect에 실패하면 에러 메세지를 출력하고,
connect에 성공하면 name이 중복되었는지 검사합니다.
중복되었다면, server에게서 중복되었다는 메세지를 받고, connect를 해제합니다.
중복되지 않았다면, 현재 server에 연결된 client중, initMenu를 출력합니다.

initMenu에는 몇 가지 기능이 있습니다.
1. :I를 입력하여 채팅방에 입장, 2. -list를 입력하여 채팅방 리스트 출력, 3. :q를 입력하여 서버와의 연결 해제

1. :I를 입력하면, joinMenu로 들어갈 수 있습니다.
   joinMenu에서는 입장할 채팅방의 번호를 입력하여 입장하거나, :q를 입력하여 initMenu로 돌아갈 수 있습니다.
   입력한 번호의 채팅방이 있다면 입장하고, 없으면 채팅방을 만들어 입장합니다.

2. -list를 입력하면 현재 존재하는 채팅방의 번호와 그 채팅방에 입장한 유저 수의 리스트를 보여줍니다.
   리스트가 없다면, no rooms를 출력합니다.
    list를 출력 후, list를 보고 입장할 수 있게 자동으로 joinMenu로 들어갑니다.
    joinMenu에서는 마찬가지로, 입장할 채팅방의 번호를 입력하여 입장하거나, :q를 입력하여 initMenu로 돌아갈 수 있습니다.

3. :q를 입력하면 서버와의 연결을 해제(socket을 close)후, 프로그램을 종료합니다.


채팅방에 입장한 후에는 msg를 입력하면 자신과 같은 방에 있는 client에게 msg를 출력해줍니다.
입장하거나 퇴장할 때, 입장 메세지와 퇴장 메세지를 출력하여 자신이 입장, 퇴장했음을 방 안의 다른 client에게 알려줍니다.
채팅방에서 퇴장하고 싶다면, :q를 눌러 initMenu로 돌아갈 수 있습니다.

client는 SIGQUIT, SIGINT가 무시되도록 handling되어 있습니다.
채팅 도중이나 메뉴 선택 도중 Ctrl+C, Ctrl+\를 눌러도 프로그램이 종료되거나, server에 영향을 주지 않습니다.