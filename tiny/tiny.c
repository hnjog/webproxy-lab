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

  printf("try make listen fd");
  listenfd = Open_listenfd(argv[1]);
  while (1)
  {
    printf("in while...");
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); // 연결 요청 접수
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);  // 트랜잭션 수행
    Close(connfd); // 연결 닫기
  }
}

void doit(int fd)
{
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  // 요청 라인과 헤더 읽기
  Rio_readinitb(&rio, fd);
  Rio_readlineb(&rio, buf, MAXLINE);
  printf("Request headers:\n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version); // buf 문자열에서 method, uri, version 읽기

  // 예제 코드는 "GET"만 구현하였기에
  // 차후 다른 명령어 구현하려 한다면 일단 이부분 수정을 고려하기
  if (strcmp(method, "GET"))
  {
    clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
    return;
  }

  read_requesthdrs(&rio);

  // URI와 GET 요청 파싱
  is_static = parse_uri(uri, filename, cgiargs);
  if (stat(filename, &sbuf) < 0)
  {
    clienterror(fd, method, "404", "Not found", "Tiny couldn't find this file");
    return;
  }

  if (is_static)
  {
    // 정적 요청
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))
    {
      clienterror(fd, method, "403", "Forbidden", "Tiny couldn't read the file");
      return;
    }

    serve_static(fd, filename, sbuf.st_size);
  }
  else
  {
    // 동적 요청
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode))
    {
      clienterror(fd, method, "403", "Forbidden", "Tiny couldn't run the CGI program");
      return;
    }

    serve_dynamic(fd, filename, cgiargs);
  }
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  // HTTP response body
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor = "
                "ffffff"
                ">\r\n",
          body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em> The Tiny Web server</em>\r\n", body);

  // HTTP response header
  sprintf(buf, "HTTP/1.0 %s %s \r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));

  // 헤더 다 썼으면 body도 써준다
  Rio_writen(fd, body, strlen(buf));

  // write()를 사용하면 해당하는 fd(file descriptor)(즉, 소켓)에
  // 데이터를 써준다
  // 다만 이후 소켓을 전송하는 것은 OS 내부의 프로토콜 처리 방식으로 처리
  // OS 내부에서 프로토콜의 처리방식이 존재하기에
  // TCP/IP가 실제로 사용된다는 표현은
  // 각 프로토콜이 구현되어 있다는 점을 뜻한다
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
}

int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;

  // 정적 컨텐츠
  // strstr : substring 검색
  // 내부에 존재하지 않는 경우 NULL 반환
  if (!strstr(uri, "cgi-bin"))
  {
    strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri);
    if (uri[strlen(uri) - 1] == '/')
    {
      // strcat : 1의 문자열에 2를 연결
      strcat(filename, "home.html");
    }

    return 1;
  }
  else // 동적 컨텐츠
  {
    ptr = index(uri, '?');
    if (ptr)
    {
      strcpy(cgiargs, ptr + 1);
      *ptr = '\0';
    }
    else
    {
      strcpy(cgiargs, "");
    }
    strcpy(filename, ".");
    strcat(filename, uri);

    return 0;
  }
}

void serve_static(int fd, char *filename, int filesize)
{
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  // 클라에 응답 헤더 보내기
  get_filetype(filename, filetype);
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type : %s\r\n\r\n", buf, filetype);
  Rio_writen(fd, buf, strlen(buf));
  printf("Response headers:\n");
  printf("%s", buf);

  // 클라에 응답 바디 보내기

  // 파일을 열기 위한 시스템 콜 호출 (filename을 읽기 전용으로 열고, 파일 디스크립터 반환)
  // OS는 파일 시스템 or 네트워크를 통해 접근할 수 있는 '자원'을 '파일 디스크립터'로 표현
  // 그렇기에 filename이란 파일을 열고 새로 할당된 고유한 파일 디스크립터를 반환한다
  srcfd = Open(filename, O_RDONLY, 0);

  // srcfd가 가리키는 파일을 가상 메모리에 매핑한다
  // 따라서 filesize 만큼,
  // 현재 메인 메모리(어쩌면 디스크)에 있는 srcfd가 가리키는 열린 파일을
  // srcp 의 가상 메모리 주소 공간에 매핑한다 (읽기 전용)
  // 또한 MAP_PRIVATE인 경우, 데이터의 사본을 생성하여 원본을 변경하지 않도록 한다 (참고로 다른 옵션들도 or로 같이 지정 가능)
  // 별도의 공간이 할당된다는 점도 알고 있자
  // 이런식으로 가상 메모리 주소 공간에 할당하면
  // 현재 프로세스가 파일의 위치를 지속적으로 알 수 있으므로
  // 매번 Read 같은 함수를 호출할 필요가 없음
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);

  // Open한 파일 디스크립터를 닫아준다
  Close(srcfd);

  // 소켓에 불러온 파일 데이터를 써준다
  Rio_writen(fd, srcp, filesize);

  // 파일 데이터를 unmap 함으로서 가상 메모리 매핑을 해제한다
  Munmap(srcp, filesize);

  // Close와 Munmap을 모두 호출해야
  // 디스크에서 메인 메모리로 가져온 데이터 들이 '해제'된다
  // close는 파일 디스크립터를 닫는 것이며, 호출하지 않으면 파일이 계속 '열려있는 것'으로 취급된다
  // Mmap은 메인 메모리에 올라온 데이터를 포인터로 받아 프로세스의 포인터에 매핑해준다
  // 다만 이때, 'heap' 영역 같은 곳에 할당되는 개념은 아니다
  // Unmap은 해당하는 매핑을 해제함으로서, 운영체제가 해당 메모리 공간이
  // '참조'되지 않는다는 점을 알게 하여, 차후 메모리 할당 공간 후보가 될 수 있도록 한다
}

void get_filetype(char *filename, char *filetype)
{
  if (strstr(filename, ".html"))
  {
    strcpy(filetype, "text/html");
  }
  else if (strstr(filename, ".gif"))
  {
    strcpy(filetype, "image/gif");
  }
  else if (strstr(filename, ".png"))
  {
    strcpy(filetype, "image/png");
  }
  else if (strstr(filename, ".jpg"))
  {
    strcpy(filetype, "image/jpeg");
  }
  else
  {
    strcpy(filetype, "text/plain");
  }
}

void serve_dynamic(int fd, char *filename, char *cgiargs)
{
  char buf[MAXLINE], *emptylist[] = {NULL,};
  
  // HTTP 응답의 첫 부분 반환
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  Rio_writen(fd, buf, strlen(buf));

  if(Fork() == 0)
  {
    // 자식들
    setenv("QUERY_STRING",cgiargs,1);
    Dup2(fd,STDOUT_FILENO);
    execve(filename,emptylist,environ);
  }
  Wait(NULL);
}