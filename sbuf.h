#ifndef __SBUF_H__
#define __SUBF_H__

#include <semaphore.h>

// 원형 버퍼
typedef struct
{
    int *buf;       // 버퍼 배열(원형 큐)
    int n;          // 최대 슬롯수
    int front;      // buf[(front+1)%n]을 통해 첫번째 슬롯에 있는 녀석 가리킴
    int rear;       // buf[rear%n]을 통해 마지막에 있는 녀석 가리킴
    sem_t mutex; // 버퍼에 접근하려는 것을 제한
    sem_t slots; // 사용 가능한 슬롯
    sem_t items; // 사용 가능한 아이템
} sbuf_t;

// proxy.c 에서 가져다 쓸 것이기에
// .h에 선언하는 것이 더 알맞아 보인다

// 원형 큐 슬롯 초기화
void sbuf_init(sbuf_t * sp, int n);

// 버퍼 비우기
void sbuf_deinit(sbuf_t * sp);

// 버퍼의 뒤에 삽입
void sbuf_insert(sbuf_t * sp, int item);

// 버퍼 앞에서 제거 및 반환
int sbuf_remove(sbuf_t * sp);

#endif