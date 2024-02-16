#if !defined(CONQ_QUEUE_H)
#define CONQ_QUEUE_H

#include <pthread.h>

// End-Of-Stream (EOS): valore speciale per la terminazione
#define EOS (void*)0x1   
//tempo di attesa massimo da parte del Producer in caso di coda piena
#define WAIT_TIME_SECONDS 3

/** Coda concorrente lunga qlen di stringhe lunghe str_len
 *  Coda implementata come una coda circolare
 */
typedef struct bqueue_t
{
    char **queue;
    size_t head;  // indice di testa
    size_t tail;  // indice di coda
    size_t qsize; // dimensione attuale coda
    size_t qlen;  // dimensione massima coda
    size_t str_len;  // dimensione stringhe
    pthread_mutex_t m;
    pthread_cond_t cfull;
    pthread_cond_t cempty;
} BQueue_t;

/** Alloca ed inizializza una coda lunga n
 *
 *   \param n lunghezza della coda
 * 
 *   \retval NULL se si sono verificati problemi nell'allocazione (errno settato)
 *   \retval q puntatore alla coda allocata
 */
BQueue_t *initBQueue(size_t n, size_t str_len);

/** Cancella una coda allocata con initBQueue.
 *
 *   \param q puntatore alla coda da cancellare
 */
void deleteBQueue(BQueue_t *q);

/** Inserisce una stringa nella coda.
 *   \param data puntatore alla stringa da inserire
 *
 *   \retval 0 se successo
 *   \retval -1 se errore (errno settato opportunamente)
 *   \retval -2 se timeout (dove tempo di attesa massimo producer: WAIT_TIME_SECONDS)
 */
int push(BQueue_t *q, char *data);

/** Restituisce la stringa in testa alla coda.
 *  Viene anche estratto se non si tratta della stringa speciale terminatrice EOS
 *
 *  \retval stringa puntatore alla stringa restituita.
 *  \retval null in caso di errore (errno settato opportunamente)
 */
char *pop(BQueue_t *q);

#endif /* CONQ_QUEUE_H */
