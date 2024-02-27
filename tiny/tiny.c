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

  // 명령행 인수가 2개인지 확인 2개가 아니면 프로그램의 사용법을 알려주고 프로그램을 종료한다.
  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  // 연결 요청을 받을 준비가 된 듣기 식별자를 생성한다.
  listenfd = Open_listenfd(argv[1]);
  // 무한 서버 루프
  while (1)
  {
    clientlen = sizeof(clientaddr);
    // 연결요청 접수
    connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen); // line:netp:tiny:accept
    // 소켓주소를 이용해서 호스트 이름, 서비스이름, 서비스 정보 플래그를 얻는다. 얻은 정보를 저장
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    // 트랜잭션 수행
    doit(connfd);  // line:netp:tiny:doit
    // 자신의 연결 끝을 닫는다.
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

  // 요청 라인 분석
  Rio_readinitb(&rio, fd); // open한 식별자마다 한 번 호출, 식별자 fd를 주소 rio에 위치한 rio_t 타입의 읽기 버퍼와 연결한다.
  Rio_readlineb(&rio, buf, MAXLINE); // 다음 텍스트 줄의 파일 rio에서 읽고, 메모리 buf로 복사하고 텍스트 라인을 널 문자로 종료시킨다.
                                     // 최대 MAXLINE개의 바이트를 읽으며, 종료용 널 문자를 위한 공간을 남겨둔다.
  printf("Request headers : \n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version);

  // Tiny는 GET메소드만 지원한다. 다른 메소드(POST같은)를 요청하면, 에러 메시지를 보내고 main 루틴으로 돌아오고
  // 그 후 연결을 닫고 연결을 기다린다.
  if (strcasecmp(method, "GET")) // 두개의 문자가 같으면 0을 반환 다르면 양의 정수를 반환
  {
    clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
    return;
  }
  // Tiny는 요청헤더내의 어떤 정보도 사용하지않는다. 
  // 요청한 헤더 파일을 요청하지만 읽고 무시한다.
  read_requesthdrs(&rio);

  // URI를 파일 이름과 비어있는 CGI인자 스트링으로 분석
  // 정적 도는 동적컨텐츠를 위한 것인지 나타내는 플래그를 설정한다.
  is_static = parse_uri(uri, filename, cgiargs);
  // 파일이 디스크 상에 없다면 바로 에러메세지 출력
  if (stat(filename, &sbuf) < 0)
  {
    clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
    return;
  }
  // 동적이나 정적이냐
  // Serve static content
  if (is_static)
  {
    // S_ISREG 파일 타입이 일반 파일인지 여기서는 일반파일이 아니면 참, 일반 파일이란 텍스트, 이미지, 음악 등 사용자 데이터를 저장하는 파일
    // S_IRUSR은 사용자 읽기 권한 비트 상수.여기서는 읽기 권한이 없는 경우 참
    // POSIX 표준에서 정의된 파일 권한 관련 매크로/상수입니다.
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))
    {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
      return;
    }
    // 정적 컨텐츠를 서버에게 제공
    serve_static(fd, filename, sbuf.st_size);
  }
  // Serve dynamic content
  else
  {
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode))
    {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
      return;
    }
    serve_dynamic(fd, filename, cgiargs);
  }
}


void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  // build the http response body
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor="
                "ffffff"
                ">\r\n",
          body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  // print the http response
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}

void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);
  while (strcmp(buf, "\r\n"))
  {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}

int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;
  if (!strstr(uri, "cgi-bin"))
  { /* Static content */
    strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri);
    if (uri[strlen(uri) - 1] == '/')
      strcat(filename, "home.html");
    return 1;
  }
  else
  { /* Dynamic content */
    ptr = index(uri, '?');
    if (ptr)
    {
      strcpy(cgiargs, ptr + 1);
      *ptr = '\0';
    }
    else
      strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri);
    return 0;
  }
}

void serve_static(int fd, char *filename, int filesize)
{
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];
  /* Send response headers to client */
  get_filetype(filename, filetype);
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  Rio_writen(fd, buf, strlen(buf));
  printf("Response headers:\n");
  printf("%s", buf);

  /* Send response body to client */
  // srcfd = Open(filename, O_RDONLY, 0);
  // srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
  // Close(srcfd);
  // Rio_writen(fd, srcp, filesize);
  // Munmap(srcp, filesize);

  // HW 11.9
  
  srcfd = Open(filename, O_RDONLY, 0);
  srcp = malloc(filesize);
  Rio_readn(srcfd,srcp,filesize);
  Close(srcfd);
  Rio_writen(fd, srcp, filesize);
  free(srcp);
}

/*
 * get_filetype - Derive file type from filename
 */
void get_filetype(char *filename, char *filetype)
{
  if (strstr(filename, ".html"))
    strcpy(filetype, "text/html");
  else if (strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
  else if (strstr(filename, ".png"))
    strcpy(filetype, "image/png");
  else if (strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg");
  else
    strcpy(filetype, "text/plain");
}

void serve_dynamic(int fd, char *filename, char *cgiargs)
{
  char buf[MAXLINE], *emptylist[] = {NULL};

  /* Return first part of HTTP response */
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  if (Fork() == 0)
  { /* Child */
    /* Real server would set all CGI vars here */
    setenv("QUERY_STRING", cgiargs, 1);
    Dup2(fd, STDOUT_FILENO);              /* Redirect stdout to client */
    Execve(filename, emptylist, environ); /* Run CGI program */
  }
  Wait(NULL); /* Parent waits for and reaps child */
}