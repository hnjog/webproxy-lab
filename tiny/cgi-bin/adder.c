/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "csapp.h"

int main(void)
{
  char *buf, *p, *method;
  char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
  int n1 = 0, n2 = 0;

  // getenv : 해당하는 환경변수의 값을 반환함
  // QUERY_STRING 는 CGI 스펙에서 사용하는 특정한 환경 변수중 하나임
  // 웹 서버에서 전송된 쿼리스트링에 대한 정보를 가진다
  // ex : http://www.example.com/search?query=apple
  // 이 중 ? 다음인 query=apple 이 바로 QUERY_STRING이 가지는 데이터가 되며
  // 이를 getenv("QUERY_STRING") 로 가져올 수 있게 된다
  if ((buf = getenv("QUERY_STRING")) != NULL)
  {
    // strchr : buf에서 &를 찾고 그 포인터를 리턴
    // (null 문자가 있는 녀석한테 쓰지 않으면 위험)
    p = strchr(buf, '&');
    *p = '\0';           // &로 연결된 인자를 나누는 용도로 널 문자로 대체한다
    strcpy(arg1, buf);   // 널 문자로 나뉜 전반부를 arg1에 넣는다
    strcpy(arg2, p + 1); // 이후 다음 부분을 arg2에 넣는다
    n1 = atoi(arg1);
    n2 = atoi(arg2);
  }

  method = getenv("METHOD");

  // 응답용 바디 만들기
  sprintf(content, "QUERY_STRING=%s", buf);
  sprintf(content, "Welcome to add.com");
  sprintf(content, "%sTHE Internet addition portal. \r\n<p>", content);
  sprintf(content, "%sThe answer is : %d + %d = %d \r\n<p>", content, n1, n2, n1 + n2);
  sprintf(content, "%sThanks for visiting!\r\n", content);

  // HTTP 응답 만들기
  printf("Connection : close\r\n");
  printf("Content-length: %d\r\n",(int)strlen(content));
  printf("Content-type : text/html\r\n\r\n");

  if(method == "GET")
  {
    printf("%s",content);
  }
  fflush(stdout); // 표준 출력 버퍼 비우기

  exit(0);
}
/* $end adder */
