#define _POSIX_C_SOURCE 199309L
#include <conc_queue.h>

#include <errno.h>
#include <stdio.h>
#include <pthread.h>
#include <util.h>
#include <stdlib.h>
#include <time.h>

/**
 * \file conq_queue.c
 * \brief File di implementazione dell'interfaccia per la coda di dimensione finita
 */

/* ------------------- funzioni di utilita' -------------------- */

// assumo (per semplicita') che le mutex non siano mai di
// tipo 'robust mutex' (pthread_mutex_lock(3)) per cui possono
// di fatto ritornare solo EINVAL se la mutex non e' stata inizializzata.

static inline void LockQueue(BQueue_t *q) { LOCK(&q->m); }
static inline void UnlockQueue(BQueue_t *q) { UNLOCK(&q->m); }

/**
 * \brief Attesa con timeout su condition variable q->cfull
 * 
 * \param q coda concorrente
 * 
 * \retval -1 error
 * \retval -2 timeout
 *
 */
static inline int WaitToProduce(BQueue_t *q) { 
    struct timespec max_wait;
    const int gettime_rv = clock_gettime(CLOCK_REALTIME, &max_wait); // recupero tempo assoluto
    if(gettime_rv){ //error
        return -1;
    }
    max_wait.tv_sec += WAIT_TIME_SECONDS; // aggiungo tempo timeout
    TWAIT(&q->cfull, &q->m, &max_wait); //richiamo macro TWAIT (vedi util.h)
}
static inline void WaitToConsume(BQueue_t *q) { WAIT(&q->cempty, &q->m); }
static inline void SignalProducer(BQueue_t *q) { SIGNAL(&q->cfull); }
static inline void SignalConsumer(BQueue_t *q) { SIGNAL(&q->cempty); }
static inline void BroadcastConsumers(BQueue_t *q) { BCAST(&q->cempty); }

/**
 * \brief Procedura di utilita' per liberare la memoria in caso di errore
 *
 */

static void errorHandler(BQueue_t *q)
{
    int myerrno = errno;
    if (q->queue){
        for (int i = 0; i < q->qlen; i++){
            if(q->queue[i])
                free(q->queue[i]);
            else
                break;
        }
        free(q->queue);
    }
    if (&q->m)
        pthread_mutex_destroy(&q->m);
    if (&q->cfull)
        pthread_cond_destroy(&q->cfull);
    if (&q->cempty)
        pthread_cond_destroy(&q->cempty);
    free(q);
    errno = myerrno;
}

/* ------------------- interfaccia della coda ------------------ */

BQueue_t *initBQueue(size_t n, size_t str_len)
{
    BQueue_t *q = (BQueue_t *)calloc(1, sizeof(BQueue_t));
    if (!q)
    {
        perror("calloc");
        return NULL;
    }
    
    q->queue = (char **)calloc(n, sizeof(char *));
    if (!q->queue)
    {
        perror("calloc queue");
        errorHandler(q);
        return NULL;
    }

    for (int i = 0; i < n; i++)
    {
        q->queue[i] = (char *)calloc(str_len, sizeof(char));
        if (q->queue[i] == NULL)
        {
            perror("malloc queue");
            errorHandler(q);
            return NULL;
        }
    }

    if (pthread_mutex_init(&q->m, NULL) != 0)
    {
        perror("pthread_mutex_init");
        errorHandler(q);
        return NULL;
    }
    
    if (pthread_cond_init(&q->cfull, NULL) != 0)
    {
        perror("pthread_cond_init full");
        errorHandler(q);
        return NULL;
    }

    if (pthread_cond_init(&q->cempty, NULL) != 0)
    {
        perror("pthread_cond_init empty");
        errorHandler(q);
        return NULL;
    }

    q->head = q->tail = 0;
    q->qlen = 0;
    q->qsize = n;
    q->str_len = str_len;
    return q;
}

void deleteBQueue(BQueue_t *q)
{
    if (!q)
    {
        errno = EINVAL;
        return;
    }
    if (q->queue){
        for (int i = 0; i < q->qsize; i++)
            if(q->queue[i] != EOS)
                free(q->queue[i]);
        free(q->queue);
    }
    if (&q->m)
        pthread_mutex_destroy(&q->m);
    if (&q->cfull)
        pthread_cond_destroy(&q->cfull);
    if (&q->cempty)
        pthread_cond_destroy(&q->cempty);
    free(q);
}

int push(BQueue_t *q, char *data)
{
    if (!q || !data)
    {
        errno = EINVAL;
        return -1;
    }

    LockQueue(q); // lock su mutex

    while (q->qlen == q->qsize){ // condizione di attesa (coda piena)
        int ret = WaitToProduce(q); // attesa su variabile di condizione
        if (ret != 0) 
            return ret; // Producer error
    }     

    // inserimento stringa in coda
    if(data!=EOS){ //se non è EOS
        strncpy(q->queue[q->tail], data, q->str_len);
        SignalConsumer(q); // avviso un Consumer
    }
    else{ //se è EOS
        free(q->queue[q->tail]);
        q->queue[q->tail] = EOS; 
        BroadcastConsumers(q); // avviso tutti i Consumers
    }
    q->tail += (q->tail + 1 >= q->qsize) ? (1 - q->qsize) : 1;
    q->qlen += 1;

    UnlockQueue(q);    // unlock su mutex

    return 0;
}

char *pop(BQueue_t *q)
{
    if (!q)
    {
        errno = EINVAL;
        return NULL;
    }

    LockQueue(q); // lock su mutex

    while (q->qlen == 0)  // condizione di attesa (coda vuota)
        WaitToConsume(q); // attesa su variabile di condizione

    // estrazione stringa dalla coda

    char *data;
    if(q->queue[q->head] != EOS){
        data = (char *)calloc(q->str_len, sizeof(char));
        if(!data){
            perror("calloc");
            return NULL;
        }
        strncpy(data, q->queue[q->head], q->str_len);

        // vado a modificare puntatori di testa coda e ad eliminare la stringa se non è EOS
        memset(q->queue[q->head], 0, strlen(q->queue[q->head]) + 1);
        q->head += (q->head + 1 >= q->qsize) ? (1 - q->qsize) : 1;
        q->qlen -= 1;
    }
    else
        data = EOS;

    SignalProducer(q); // avviso il master
    UnlockQueue(q);    // unlock su mutex

    return data;
}
