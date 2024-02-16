#if !defined(WORKER_H)
#define WORKER_H

#include <pthread.h>
#include <conc_queue.h>

/**
 * \file worker.h
 * \brief Interfaccia per il Worker. 
 *          Oltre alla struttura dati rappresentante i parametri del thread,
 *          essa avrà come interfaccia solamente il metodo main del Worker;
 *          Questo per dividere le parti del programma in unità distinte
 */

typedef struct threadArgs // tipo di dato usato per passare gli argomenti al thread
{
    BQueue_t *q;
    int max_path_len;
    const char* sockname;
} threadArgs_t;

/**
 * \brief Funzione che rappresenta il ciclo di vita del Worker
 *
 * \param arg argomento del Worker void* (in questo caso viene passato threadArgs_t)
 */
void *main_worker(void *arg);

#endif // WORKER_H