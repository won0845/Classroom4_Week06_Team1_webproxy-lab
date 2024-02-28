#include "csapp.h"
#include "csapp.h"

int main(int args, char **argv){        //args 명렬중 인자 수 argv 인자의 값
    int clientfd;                       // 서버에 대한 연결을 나타내는 파일 디스크립터
    char *host, *port, buf[MAXLINE];    // 연결할 서버의 호스트 이름과 포트번호를 저장
                                        // buf 네트워크를 통해 전송되는 데이터를 임시 저장하는 버퍼
    rio_t rio;                          // Robust I/O (Rio) 구조체의 인스턴스로, 
                                        // 버퍼링된 I/O 작업을 쉽게 처리할 수 있도록 합니다.

    if (args != 3)                      // 인자가 3개가 아니면 메세지 출력후 종료
    {                                   // ./echoclient ${주소} ${포트번호} 형식으로 입력해야함
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);  //argv[0] 실행파일 이름 ./echoclient
        exit(0);
    }
    host = argv[1];                     // 가운데 인자는 주소
    port = argv[2];                     // 마지막 인자는 포트번호

    clientfd = Open_clientfd(host, port);   // Open_clientfd 함수를 사용하여 지정된 
                                            // 호스트와 포트에 대한 TCP 연결을 설정한다.
                                            // 연결 성공시 연결에 대한 파일 디스크립터를 clientfd에 저장한다.
    Rio_readinitb(&rio, clientfd);          // Rio_readinitb 함수를 사용하여 rio_t 구조체를 초기화하고, 
                                            // 이를 통해 버퍼링된 I/O 작업을 수행할 수 있다.

    while (Fgets(buf, MAXLINE, stdin) != NULL)  //Fgets 함수를 사용하여 표준 입력(stdin)에서 한 줄을 읽고, buf에 저장한다.
    {
        Rio_writen(clientfd, buf, strlen(buf)); // 버퍼에 저장된 값을 서버에 전송한다.
        Rio_readlineb(&rio, buf, MAXLINE);      // 서버로부터 응답을 받아 buf에 저장
        Fputs(buf, stdout);                     // buf에 저장된 값을 표준 출력에 출력한다.
    }
    Close(clientfd);                            // 연결을 닫고 
    exit(0);                                    // 프로그램 정상종료
    
}