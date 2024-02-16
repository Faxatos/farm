#if !defined(MASTER_H)
#define MASTER_H

#define M_SUCCESS 0
#define M_FAILURE -1

//consts
#define _DEFAULT_NTHREAD_VALUE 4
#define _DEFAULT_QLEN_VALUE 8
#define _DEFAULT_DELAY_VALUE 0
#define _MIN_NTHREAD_VALUE 1
#define _MIN_QLEN_VALUE 1
#define _MIN_DELAY_VALUE 0
#define _MIN_SOCKNAME_LEN 5
#define _MIN_PATH_LEN 5
#define _MIN_MCOMMS_LEN 2
#define _MAX_NTHREAD_VALUE 256
#define _MAX_QLEN_VALUE 256
#define _MAX_DELAY_VALUE 8192
#define _MAX_SOCKNAME_LEN 256
#define _MAX_PATH_LEN 512
#define _MAX_MCOMMS_LEN 256
#define _MAX_EXT_LEN 5

#include <conc_queue.h>
#include <dyn_array.h>

/**
 * @file master.h
 * @brief Interfaccia per il Master. 
 *          Essa conterrà tutti i metodi necessari per inizializzare ed eseguire il Master.
 */

typedef struct mastArgs
{
    BQueue_t *q;
    size_t delay;
    size_t threadpool_size;
    int max_path_len;
    int max_mcomms_len;
    int collectorfd;
    char sockname[_MAX_SOCKNAME_LEN];
    char ext[_MAX_EXT_LEN];
} masterArgs;

/** 
 * \brief Funzione di gestione segnali del Master (comportamento spiegato nella relazione)
 *
 */
void handle_master_signals();

/**
 * \brief Funzione di invio messaggio msg verso socket identificato da sockfd
 *
 * \param msg messaggio da inviare
 * \param sockfd file descriptor del socket
 * \param max_mcomms_len lunghezza massima del messaggio da parte del master
 */
void send_to_sockfd(int sockfd, const char* msg, const int max_mcomms_len);

/**
 * \brief Funzione che avvia l'inserimento dei files da argv nella coda concorrente (dopo aver inserito quelli presenti in dirs)
 *
 * \param mArgs argomenti del master (controllare definizione di masterArgs)
 * \param argc int che indica dimensione di argv
 * \param argv contenente gli argomenti inseriti sulla command line durante l'avvio del programma
 * \param argv_index indice di argv a cui il processo `e arrivato dopo getopt()
 * \param dirs array dinamico contenenti nomi delle directories trovati nei primi argvs
 *
 * \retval M_SUCCESS in caso di successo
 * \retval M_FAILURE in caso di errore
 */
int execute_master(masterArgs mArgs, int argc, char **argv, int argc_index, DArray *dirs);

/**
 * \brief Funzione di connessione da parte del Master utilizzando sock_name (chiamata bloccante)
 *
 * \param sock_name nome del socket a cui connettersi
 * \param retry_timeout tempo (in ms) di delay tra tentativi di connessione
 * 
 * \retval sockfd file descriptor del socket a cui il master è stato connesso
 * \retval M_FAILURE in caso di errore
 */
int connect_master(const char* sock_name, const int retry_timeout);

/**
 * \brief Funzione di inizializzazione argomenti master
 * 
 * \param mARGS struttura in cui verranno salvati gli argomenti
 * \param q coda concorrente (bounded)
 * \param nthread specifica numero di thread Worker 
 * \param delay specifica delay tra richieste del Master (in ms)
 * \param collectorfd specifica il file descriptor del collector
 * \param sockname specifica il nome del socket a cui connettersi
 * \param sockname specifica il nome del socket a cui connettersi
 * \param ext specifica estensione dei files accettata
 * \param max_mcomms_len specifica lunghezza massima delle comunicazioni da parte del master
 * 
 * \retval M_SUCCESS se mARGS settato correttamente
 * \retval M_FAILURE altrimenti
 */
int init_master_args(masterArgs *mARGS, BQueue_t *q, size_t nthread, int collectorfd, const char* sockname, const char* ext, size_t delay, int max_path_len, int max_mcomms_len);

/**
 * \brief Funzione di parsing argomenti -n -q -t -d (andiamo a salvare gli argomenti di -d in dirs)
 *
 * \param argc intero che indica dimensione di argv
 * \param argv array contenente gli argomenti inseriti sulla command line durante l'avvio del programma
 * \param nthread specifica numero di thread Worker 
 * \param qlen specifica lunghezza coda concorrente (bounded)
 * \param delay specifica delay tra richieste del Master (in ms)
 * \param argc_index intero che a fine funzione conterr`a il valore di optind
 * \param dirs array dinamico in cui verranno salvati i nomi delle directories 
 * 
 * \retval M_SUCCESS se gli args sono stati parsati correttamente
 * \retval M_FAILURE altrimenti
 */
int parse_first_args(int argc, char **argv, size_t *nthread, size_t *qlen, size_t *delay, int *argc_index, DArray *dirs);


#endif // MASTER_H