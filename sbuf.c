#include"csapp.h"
#include"sbuf.h"

void sbuf_init(sbuf_t * sp, int n)
{
    sp->buf = Calloc(n,sizeof(int));
    sp->n = n;
    sp->front = sp->rear = 0;
    // sem_init (세마포어 변수, 공유여부(0 : 해당 프로세스 내에서만 공유, 그외 : 프로세스 간 공유), 초기값이며 동시에 접근 가능한 수)
    Sem_init(&sp->mutex,0,1);
    Sem_init(&sp->slots,0,n);
    Sem_init(&sp->items,0,0);
}

void sbuf_deinit(sbuf_t * sp)
{
    Free(sp->buf);
}

void sbuf_insert(sbuf_t * sp, int item)
{
    P(&sp->slots);
    P(&sp->mutex);
    int targetIndex = (sp->rear + 1) % (sp->n);
    sp->buf[targetIndex] = item;
    V(&sp->mutex);
    V(&sp->items);
}

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