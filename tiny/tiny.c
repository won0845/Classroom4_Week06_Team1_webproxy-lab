/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

int main(int argc, char **argv)
{
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);
  while (1)
  {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen); // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);  // line:netp:tiny:doit
    Close(connfd); // line:netp:tiny:close
  }
}
void doit(int fd)
{
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  /* Read request line and headers */
  Rio_readinitb(&rio, fd);
  Rio_readlineb(&rio, buf, MAXLINE);
  printf("Request headers:\n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version);
  if (strcasecmp(method, "GET"))
  {
    clienterror(fd, method, "501", "Not implemented",
                "Tiny does not implement this method");
    return;
  }
  read_requesthdrs(&rio);

  /* Parse URI from GET request */
  is_static = parse_uri(uri, filename, cgiargs);
  if (stat(filename, &sbuf) < 0)
  {
    clienterror(fd, filename, "404", "Not found",
                "Tiny couldn’t find this file");
    return;
  }

  if (is_static)
  { /* Serve static content */
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))
    {
      clienterror(fd, filename, "403", "Forbidden",
                  "Tiny couldn’t read the file");
      return;
    }
    serve_static(fd, filename, sbuf.st_size);
  }
  else
  { /* Serve dynamic content */
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode))
    {
      clienterror(fd, filename, "403", "Forbidden",
                  "Tiny couldn’t run the CGI program");
      return;
    }
    serve_dynamic(fd, filename, cgiargs);
  }
}

void clienterror(int fd, char *cause, char *errnum,
                 char *shortmsg, char *longmsg)
{
  // clienterror 함수는 클라이언트에게 에러 상황을 알리고,
  // 사용자가 이해할 수 있는 방식으로 에러 정보를 제공
  // 요청 메소드를 제대로 입력하지 않은 경우 사용되는 함수
  // 대소문자 구분 하지 않음
  char buf[MAXLINE], body[MAXBUF];

  /* Build the HTTP response body */
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor="
                "ffffff"
                ">\r\n",
          body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  /* Print the HTTP response */
  // 응답 헤더 코드 errnum -> 상태 코드, shortmsg -> 상태 메세지
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  // body 문자열 길이 출력
  Rio_writen(fd, body, strlen(body));
}

void read_requesthdrs(rio_t *rp)
{ // HTTP 요청 헤더를 읽고, 터미널에 출력하는 함수
  char buf[MAXLINE];
  do {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  } while (strcmp(buf, "\r\n"));
  return;
}

int parse_uri(char *uri, char *filename, char *cgiargs)
{ 
  // char *uri: 분석할 URI
  // char *filename: 분석된 결과, 요청된 파일의 경로와 이름을 저장할 변수의 주소
  // char *cgiargs: 동적 콘텐츠를 요청할 때 사용될 CGI 인자를 저장할 변수의 주소
  char *ptr;

  if (!strstr(uri, "cgi-bin"))
  { /* Static content */
    // cgi-bin가 uri에 없는 경우
    // 정적 콘텐츠 처리의 경우
    
    strcpy(cgiargs, "");              // cgiargs 는 빈 문자열로 설정,
    strcpy(filename, ".");            // filename은 현재 디렉토리로 시작 URI가 추가
    strcat(filename, uri);            // 
    if (uri[strlen(uri) - 1] == '/')  // URI가 슬래시("/")로 끝난다면, 
      strcat(filename, "home.html");  // 기본 파일 이름으로 "home.html"이 filename에 추가
    return 1;                         // 정적 콘텐츠를 나타내는 1을 반환
  }
  else
  { /* Dynamic content */
    // 동적 콘텐츠의 경우
    // 
    ptr = index(uri, '?');        // URI 내에서 '?' 문자를 검색하여, CGI 인자의 시작을 찾는다.
    if (ptr)                      // ?가 있다면 
    {
      strcpy(cgiargs, ptr + 1);   // ? 이후의 모든 문자열은 CGI 인자(cgiargs)로 복사되고,
      *ptr = '\0';                // ?를 문자열 종료 문자로 변결하여  URI를 CGI 스크립트 경로와 인자로 분리한다.
    }
    else
      strcpy(cgiargs, "");        // ? 가 없으면 cgiargs는 빈문자로 설정된다.
      
    strcpy(filename, ".");        // filename은 현재 디렉토리(.)로 시작한뒤
    strcat(filename, uri);        // URI 추가
    return 0;                     // 동적 콘텐츠를 나타내는 0을 반환한다.
  }
}

void serve_static(int fd, char *filename, int filesize)
{
  // 정적 파일등을 클라이언트에게 서비스하는데 사용
  // fd 파일 디스크립터
  // filename 서비스할 파일 이름
  // filesize 파일크기를 입력으로 받는다.
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  /* Send response headers to client */
  get_filetype(filename, filetype);                         //요청된 파일의 MIME 타입을 filetype에 저장 클라에게 컨텐츠 종류를 알려줌
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  Rio_writen(fd, buf, strlen(buf));                         // buf에 저장된 응답 헤더를 Rio_writen 함수를 통해 클라이언트에게 전송한다.
                                                            // 이 단계에서 클라이언트는 응답의 메타 데이터를 수신한다.
  printf("Response headers:\n");                            // 서버 콘솔에 응답헤더 출력
  printf("%s", buf);                                        

  /* Send response body to client */
  srcfd = Open(filename, O_RDONLY, 0);                        // 파일을 읽기 위해 Open함수를 사용하여 filename을 연다.
                                                              // O_RDONLY 플래그는 파일을 읽기 전용 모드로 열겠다는 것을 의미한다.
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0); // Mmap 함수를 이용해서 파일을 메모리에 매핑한다.
                                                              // 파일의 내용을 메모리 주소공간에 매핑하여 파일I/O를 통해 
                                                              // 데이터를 읽는 대신 메모리 접근을 통해서 데이터를 빠르게 처리할 수 있게 해준다.
                                                              // PROT_READ는 매핑된 메모리 영역이 읽기 가능함을 나타낸다.
                                                              // MAP_PRIVATE는 매핑이 비공유임을 나타낸다.
  Close(srcfd);                                               // 파일 디스크럽터는 더 이상 필요하지 않으므로 Close를 호출하여 닫는다.
  Rio_writen(fd, srcp, filesize);                             // Rio_writen함수를 이용해서 매핑된 파일의 내용을 클라이언트에게 전송한다.
  Munmap(srcp, filesize);                                     // 메모리 매핑 해제
}

/*
 * get_filetype - Derive file type from filename
 */
void get_filetype(char *filename, char *filetype)
{
  // 주어진 파일 이름(filename)을 분석하여 파일의 MIME 타입을 결정하고, 이를 filetype 변수에 저장
  if (strstr(filename, ".html"))
    strcpy(filetype, "text/html");  
    // 파일 이름에 .html 확장자가 포함되어 있으면, filetype을 "text/html"로 설정
  else if (strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
    // 파일 이름에 .gif 확장자가 있으면, filetype을 "image/gif"로 설정
  else if (strstr(filename, ".png"))
    strcpy(filetype, "image/png");
    // 파일 이름에 .png 확장자가 포함되어 있으면, filetype을 "image/png"로 설정
  else if (strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg");
    // 파일 이름에 .jpg 확장자가 있으면, filetype을 "image/jpeg"로 설정
  else
    strcpy(filetype, "text/plain");
    // 위의 조건에 해당하지 않는 모든 파일에 대해서는 filetype을 "text/plain"으로 설정
}

void serve_dynamic(int fd, char *filename, char *cgiargs)
{
  // 동적 컨텐츠를 클라이언트에게 제공하는 데 사용
  // fd 클라이언트의 파일 디스크립터
  // filename 실행할 CGI프로그램의 파일 이름
  // cgiargs CGI 프로그램에 전달할 인자
  char buf[MAXLINE], *emptylist[] = {NULL};

  /* Return first part of HTTP response */
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));
  /* 
  클라이언트에게 HTTP 응답의 첫 부분을 전송한다. 
  200 OK와 같은상태 메시지와 서버 정보를 포함하는 기본 HTTP 헤더이다.
  이 단계는 클라이언트에게 요청이 성공적으로 수신되었음을 알린다.
  */ 

  if (Fork() == 0)
  { /* Child */
    /* Real server would set all CGI vars here */
    setenv("QUERY_STRING", cgiargs, 1);
    Dup2(fd, STDOUT_FILENO);              /* Redirect stdout to client */
    Execve(filename, emptylist, environ); /* Run CGI program */
  }
  /*
  Fork 시스템을 호출해서 새로운 자식 프로세스를 생성한다.
  자식 프로세스에서는 setenv 함수를 호출해서 QUERY_STRING 환경 변수를 설정한다.
  환경 변수에는 CGI 프로그램에 전달할 인자(cgiargs) 가 저장된다.
  CGI 프로그램이 클라이언트로 부터 받은 요청 정보를 처리하는데 사용된다

  Dup2(fd, STDOUT_FILENO)를 호출하여 자식 프로세스의 
  표준 출력을 클라이언트의 소켓 파일 디스크립터로 리다이렉션한다.
  CGI 프로그램의 출력이 직접 클라이언트로 전송된다.

  Execve 함수를 사용하여 CGI 프로그램을 실행한다.
  filename은 실행할 프로그램의 경로이며
  emptylist는 프로그램에 전달될 인자 배열(이 경우 인자가 없음)
  environ은 현재 환경 변수를 전달한다.
  */
  Wait(NULL); /* Parent waits for and reaps child */

  // Wait(NULL) 호출을 통해 부모 프로세스는 자식 프로세스의 실행이 
  // 완료될 때까지 대기하고, 자식 프로세스가 종료되면 이를 회수(리핑)한다.
  // 이는 자식 프로세스가 생성한 모든 리소스가 제대로 정리되고, 좀비 프로세스가 발생하지 않도록 한다.
}