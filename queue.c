#include <stdlib.h>
#include <threads.h>

// circular array
typedef struct _queue {
    int size;
    int used;
    int first;
    void **data;
    mtx_t *mtx;
    cnd_t *buffer_full;
    cnd_t *buffer_empty;
    int condition;
    int cnt;
} _queue;

#include "queue.h"

queue q_create(int size, int numthreads) {
    queue q = malloc(sizeof(_queue));
    q->condition=0;
    q->size  = size;
    q->used  = 0;
    q->first = 0;
    q->cnt=numthreads;
    q->data  = malloc(size * sizeof(void *));
    q->mtx= malloc(sizeof(mtx_t));
    q->buffer_empty= malloc(sizeof(cnd_t));
    q->buffer_full=malloc(sizeof(cnd_t));
    mtx_init(q->mtx, mtx_plain);
    cnd_init(q->buffer_empty);
    cnd_init(q->buffer_full);
    q->condition=0;

    return q;
}

int q_elements(queue q) {
    return q->used;
}

int q_insert(queue q, void *elem) {

    mtx_lock(q->mtx);
    while(q->size == q->used) {
        cnd_wait(q->buffer_full, q->mtx);
    }

    q->data[(q->first + q->used) % q->size] = elem;
    q->used++;

    if(q->used==1){
        cnd_broadcast(q->buffer_empty);
    }
    mtx_unlock(q->mtx);

    return 0;
}

void *q_remove(queue q) {
    void *res;
    mtx_lock(q->mtx);
    while(q->used == 0&&q->condition==0){
        cnd_wait(q->buffer_empty, q->mtx);
    }

    if(q->used==0&&q->condition){
        mtx_unlock(q->mtx);
        return NULL;
    }
    res = q->data[q->first];
    q->first = (q->first + 1) % q->size;
    q->used--;

    if(q->used==q->size-1){
        cnd_broadcast(q->buffer_full);
    }
    mtx_unlock(q->mtx);
    return res;
}

void q_destroy(queue q) {
    mtx_destroy(q->mtx);
    cnd_destroy(q->buffer_empty);
    cnd_destroy(q->buffer_full);
    free(q->buffer_empty);
    free(q->buffer_full);
    free(q->data);
    free(q->mtx);
    free(q);
}

void acabar(queue q){
    mtx_lock(q->mtx);
    q->cnt--;
    if(q->cnt==0){
        q->condition=1;
        mtx_unlock(q->mtx);
        cnd_broadcast(q->buffer_empty);
    }
    mtx_unlock(q->mtx);
}

void acabar2(queue q){
    mtx_lock(q->mtx);
    q->condition=1;
    cnd_broadcast(q->buffer_empty);
    mtx_unlock(q->mtx);
}

