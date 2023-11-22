#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

void doit(int fd);
void parse_uri(char *uri, char *request_ip, char *port, char *filename);
void serve_Proxy(int servefd,int clientfd, char *filename, int filesize, char *method);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

void make_header(char *method, char *request_ip, char *user_agent_hdr, char *version, int requested_fd, char *filename);

// 일단 프록시 서버는 클라에서 요청하는 정적인 컨텐츠를 처내는 용도임을 잊지 말자
// 일단 tiny에서 사용한 녀석들 중
// static 녀석들만 가져와서 작동시켜 보자
// 그리고 tiny로 보내는 방식을 취하는듯 함

// 요청을 서버로 보낸다(tiny)

void doit(int fd)
{
  int requested_fd;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char request_ip[MAXLINE], filename[MAXLINE], port[MAXLINE];
  rio_t rio;

  // 요청 라인과 헤더 읽기
  Rio_readinitb(&rio, fd);
  Rio_readlineb(&rio, buf, MAXLINE);
  printf("Request headers:\n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version); // buf 문자열에서 method, uri, version 읽기

  if (strcmp(method, "GET") && strcmp(method, "HEAD"))
  {
    clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
    return;
  }

  // uri를 나누어
  // 요청 ip , port, filename으로 나누다
  // 내부에서 return
  parse_uri(uri, request_ip, port, filename);

  printf("request IP : %s\r\n",request_ip);
  printf("Port : %s\r\n",port);
  printf("filename : %s\r\n",filename);
  requested_fd = Open_clientfd(request_ip, port);

  // 서버로 전송함
  make_header(method, request_ip, user_agent_hdr, version, requested_fd, filename);

  // 어차피 서버에서
  serve_Proxy(requested_fd,fd,filename,MAXLINE,method);
  Close(requested_fd);
}

// uri 분할 역할의 함수
void parse_uri(char *uri, char *request_ip, char *port, char *filename)
{
  // http:// 가 앞에 붙어서 오네??
  char tempUri[MAXBUF] = {0,};
  char *ptr = strchr(uri, '/');
  size_t templen = strlen(ptr + 2);
  strncpy(tempUri, ptr + 2,templen);
  ptr = strchr(tempUri,':');

  // port 존재 (:이 있다)
  if (ptr != NULL)
  {
    *ptr = '\0';             // 기준점에 null 문자 넣어주고
    strcpy(request_ip, tempUri); // 전반부는 요청 IP에
    strcpy(port, ptr + 1);   // 이후는 port번호로 넣어준다

    ptr = strchr(port, '/');
    // port 번호 이후 요청하는 파일 이름이 있다면
    if (ptr != NULL)
    {
      // filename에 넣어주고
      strcpy(filename, ptr);
      // null문자로 바꿔준다
      *ptr = '\0';
      // port가 배열이기에
      // null문자가 있는 부분까지만 유지하고
      // 나머지는 버리기 위함
      strcpy(port, port);
    }
  }
  else // 포트번호가 없는 요청이다
  {
    // uri 를 요청 IP 취급한다
    strcpy(request_ip, tempUri);

    ptr = strchr(request_ip, '/');
    if (ptr != NULL)
    {
      strcpy(filename, ptr + 1);
    }
    port = "80";
  }
}

//
void make_header(char *method, char *request_ip, char *user_agent_hdr, char *version, int requested_fd, char *filename)
{
  char buf[MAXLINE];

  sprintf(buf, "%s %s %s\r\n", method, filename, "HTTP/1.0");
  sprintf(buf, "%sHost: %s\r\n", buf, request_ip); // 목적지 IP입력
  sprintf(buf, "%s%s", buf, user_agent_hdr);
  sprintf(buf, "%sConnection: %s\r\n", buf, "close");
  sprintf(buf, "%sProxy-Connection: %s\r\n\r\n", buf, "close");

  Rio_writen(requested_fd, buf, strlen(buf));
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
  sprintf(body, "%s<hr><em> The Tiny Web server</em><html>\r\n", body);

  // HTTP response header
  sprintf(buf, "HTTP/1.0 %s %s \r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));

  Rio_writen(fd, body, strlen(buf));
}

void serve_Proxy(int servefd,int clientfd, char *filename, int filesize, char *method)
{
  char *srcp;

  // 백 서버가 보낸 내용 읽기
  // 어차피 같은 fd로 보내주기에
  // 믿고 써보자
  srcp = (char*)Malloc(filesize);

  // 정 안되면 여기서
  Rio_readn(servefd,srcp,filesize);

  Rio_writen(clientfd,srcp,filesize);

  Free(srcp);
}

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
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); // 연결 요청 접수
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);  // 트랜잭션 수행
    Close(connfd); // 연결 닫기
  }
}
