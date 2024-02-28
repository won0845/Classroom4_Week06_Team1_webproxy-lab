#include "csapp.h"

void echo(int connfd)
{
    size_t n;                                               // 읽은 바이트 수를 저장하기 위한 변수
    char buf[MAXLINE];                                      // 클라이언트로부터 받은 데이터를 저장할 버퍼
    rio_t rio;                                              // 선언은 Robust I/O (Rio) 라이브러리의 I/O 버퍼 구조체. 
                                                            // 이 구조체는 버퍼링된 입출력 작업을 용이하게 한다.

    Rio_readinitb(&rio, connfd);                            // rio 구조체를 초기화하고, 
                                                            // 클라이언트와의 연결을 나타내는 파일 디스크립터 connfd와 연결
    while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0)    // 클라이언트로부터 데이터를 한줄 씩 읽는다
    {                                                       // \n이나 파일의 끝을 만날 때까지 데이터를 읽고, 읽은 바이트 수를 n에 저장
                                                            // 데이터가 더 이상 없을 때 (즉, 클라이언트가 연결을 닫았을 때), 루프는 종료
        printf("server received %d bytes\n", (int)n);       // 서버가 받은 바이트 수를 콘솔에 출력
                                                            // 서버가 얼마나 많은 데이터를 받았는지 로깅하는 데 유용
        Rio_writen(connfd, buf, n);                         // 버퍼 buf에 저장된 데이터를 클라이언트로 다시 보낸다.
}