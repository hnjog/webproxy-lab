#include"csapp.h"
#include"sbuf.h"

void sbuf_init(sbuf_t * sp, int n)
{
    sp->buf = Calloc(n,sizeof(int));
    sp->n = n;
    sp->front = sp->rear = 0;
    // sem_init (세마포어 변수, 공유여부(0 : 해당 프로세스 내에서만 공유, 그외 : 프로세스 간 공유), 초기값이며 동시에 접근 가능한 수)
    Sem_init(&sp->mutex,0,1); // 처음에 1로 만들어서 아무 스레드나 진입 가능
    Sem_init(&sp->slots,0,n); // 처음에 n으로 선언해서 사용 가능 함 (빈 슬롯을 표현)
    Sem_init(&sp->items,0,0); // 처음에 0으로 선언해서 사용 불가함 (사용 가능한 아이템 없음)
}

void sbuf_deinit(sbuf_t * sp)
{
    Free(sp->buf);
}

// 혹시 slots 가 0이라면
// 누가 remove 해줄때까지 대기한다
void sbuf_insert(sbuf_t * sp, int item)
{
    P(&sp->slots); // 슬롯의 카운트 -1 (만약 0이 되는 경우는 현재 쓰레드 외에는 대기해야 함)
    P(&sp->mutex); // 현재 mutex가 0으로 만들고 임계영역(Critical Section)을 만든다
    // 여기서 부터 V(&sp->mutex) 까지 임계영역
    // 다른 스레드들도 V가 호출되기 전까지 대기 해야 함
    // 그러므로 그 두 사이의 영역은 V가 호출될 때까지 이 스레드만 사용하고
    // 데이터가 업데이트 됨(강제로 동기화가 된다)
    // Race condition(경쟁 상태)를 피하고, 코드의 무결성 유지
    // Race condition : 둘 이상의 프로세스나 스레드가 공유 자원에 동시에 접근하여 예측 불가능한 상황을 초래
    int targetIndex = (sp->rear + 1) % (sp->n);
    sp->buf[targetIndex] = item;
    V(&sp->mutex);
    V(&sp->items); // Item을 숫자를 늘려줌으로서 remove 요청에 들어간 스레드가 있다면 한 녀석은 통과된다
}

// 혹시 items 가 0이라면
// 누가 insert 해줄때까지 대기한다
int sbuf_remove(sbuf_t * sp)
{
    int item;
    P(&sp->items);
    P(&sp->mutex);
    int targetIndex = (sp->front + 1) % (sp->n);
    item = sp->buf[targetIndex];
    V(&sp->mutex);
    V(&sp->slots);
    return item;
}