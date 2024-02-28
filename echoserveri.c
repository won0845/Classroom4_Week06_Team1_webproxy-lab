#include "csapp.h"

void echo(int connfd);

int main(int argc, char **argv)
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr; /* Enough space for any address */
    char client_hostname[MAXLINE], client_port[MAXLINE];

    if (argc != 2)                          //./echoserveri 를 포함하여 포트번호까지 2개의 인자
    {                                       // 2개의 인자가 아닌 경우 메세지 출력 후 프로그램 정상 종료
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    listenfd = Open_listenfd(argv[1]);      // 주어진 포트 번호로 소켓을 열고
                                            // 클라이언트의 연결을 기다린다.
    while (1)
    {
        clientlen = sizeof(struct sockaddr_storage);                        // 클라이언트 주소 정보를 저장할 변수의 크기를 설정한다.
                                                                            // struct sockaddr_storage는 모든 유형의 소켓 주소를 저장할 수 있을 만큼 충분히 크다.
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);           // Accept함수를 사용하여 클라이언트의 연결을 수락한다.
        // listenfd는 서버가 연결 요청을 듣고 있는 리스닝 소켓의 파일 디스크립터 // 연결된 클라이언트의 소켓에 대한 파일 디스크립터를 반환한다.
        // clientaddr는 연결된 클라이언트의 주소 정보를 저장한다.
        // clientlen은 주소 구조체의 크기다.
                                                                            // 연결식별자와 듣기식별자는 구분해야한다. 
        Getnameinfo((SA *)&clientaddr, clientlen, client_hostname, MAXLINE, // 연결된 클라이언트의 주소정보(clientaddr)를 사용하여 호스트 이름과 포트번호를 문자열로 추출한다.
                    client_port, MAXLINE, 0);                               // 디버깅이나 로깅 목적으로 유용할 수 있다.
        printf("Connected to (%s, %s)\n", client_hostname, client_port);    // 서버 콘솔에 연결된 클라이언트의 호스트 이름과 포트 번호를 출력
        echo(connfd);
        Close(connfd);                                  // 연결이 끝나면 클라이언트와 연결을 닫는다.
    }
    exit(0);
}