#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <getopt.h> //non incluso con -std=C99
#include <dirent.h>

#include <master.h>
#include <worker.h>
#include <util.h>
#include <conn.h>

volatile sig_atomic_t print = 0;
volatile sig_atomic_t end = 0;

/**
 * \file master.c
 * \brief parte Master del processo MasterWorker. Implementazione dell'interfaccia master.h
 */

/**
 * \brief funzione chiamata dai segnali SIGHUP/SIGINT/SIGQUIT/SIGTERM/SIGUSR1(comportamento spiegato nella relazione)
 *
 * \param signum numero segnale
 */
static void sighandler(int signum) {
    switch (signum){
        case SIGHUP:
        case SIGINT:
        case SIGQUIT:
        case SIGTERM:
            end = 1;
            break;
        case SIGUSR1: 
            print = 1;
            break;
    }
}

/**
 * \brief Funzione di gestione segnali del Master (comportamento spiegato nella relazione)
 *
 */
void handle_master_signals(){

    sigset_t set;

    //maschero tutti i segnali fino a quando non installo gestore
    CHECK_EQ_EXIT("sigfillset", sigfillset(&set), -1, "sigfillset failed\n");
    CHECK_EQ_EXIT("pthread_sigmask", pthread_sigmask(SIG_SETMASK, &set, NULL), -1, "pthread_sigmask failed\n");

    struct sigaction s;
    CHECK_NEQ_EXIT("memset", memset(&s, 0, sizeof(s)), &s, "memset failed\n");

    s.sa_flags = ERESTART;
    s.sa_handler = &sighandler;

    CHECK_EQ_EXIT("sigaction", sigaction(SIGINT, &s, NULL), -1, "sigaction failed\n");
    CHECK_EQ_EXIT("sigaction", sigaction(SIGQUIT, &s, NULL), -1, "sigaction failed\n");
    CHECK_EQ_EXIT("sigaction", sigaction(SIGTERM, &s, NULL), -1, "sigaction failed\n");
    CHECK_EQ_EXIT("sigaction", sigaction(SIGHUP, &s, NULL), -1, "sigaction failed\n");
    CHECK_EQ_EXIT("sigaction", sigaction(SIGUSR1, &s, NULL), -1, "sigaction failed\n");

    s.sa_handler = SIG_IGN;
    CHECK_EQ_EXIT("sigaction", sigaction(SIGPIPE, &s, NULL), -1, "sigaction failed\n");
    
    //ripristino tutti i segnali
    CHECK_EQ_EXIT("sigemptyset", sigemptyset(&set), -1, "sigemptyset failed\n");
    CHECK_EQ_EXIT("pthread_sigmask", pthread_sigmask(SIG_SETMASK, &set, NULL), -1, "pthread_sigmask failed\n");
}

/**
 * \brief Funzione di invio messaggio msg verso sockfd
 *
 * \param msg messaggio da inviare
 * \param sockfd file descriptor del socket
 * \param max_mcomms_len lunghezza massima del messaggio da parte del master
 */
void send_to_sockfd(const int sockfd, const char* msg, const int max_mcomms_len){

    char buff[max_mcomms_len];
    CHECK_NEQ_RETURN("strncpy", strncpy(buff, msg, max_mcomms_len), buff, , "strcpy of %s failed\n", msg);
    if((writen(sockfd, buff, max_mcomms_len)) < 0) {
        print_error("writen failed while trying to write: %s. \n", buff);
        close(sockfd);
        exit(M_FAILURE);
    }
}

/**
 * \brief Funzione di inizializzazione threads
 *
 * \param th array di threads
 * \param threadpool_size dimensione threadpool
 * \param thARGS argomento dei workers
 * 
 * \retval M_SUCCESS in caso di successo
 * \retval M_FAILURE in caso di errore
 */
static int init_threads(pthread_t *th, size_t threadpool_size, threadArgs_t *thARGS){

    sigset_t mask, oldmask;
    CHECK_EQ_EXIT("sigemptyset", sigemptyset(&mask), -1, "sigemptyset failed\n");
    CHECK_EQ_EXIT("sigaddset", sigaddset(&mask, SIGINT), -1, "sigaddset failed\n");
    CHECK_EQ_EXIT("sigaddset", sigaddset(&mask, SIGQUIT), -1, "sigaddset failed\n");
    CHECK_EQ_EXIT("sigaddset", sigaddset(&mask, SIGTERM), -1, "sigaddset failed\n");
    CHECK_EQ_EXIT("sigaddset", sigaddset(&mask, SIGHUP), -1, "sigaddset failed\n");
    CHECK_EQ_EXIT("sigaddset", sigaddset(&mask, SIGPIPE), -1, "sigaddset failed\n");
    CHECK_EQ_EXIT("sigaddset", sigaddset(&mask, SIGUSR1), -1, "sigaddset failed\n");

    //maschero i segnali che non devono essere visibili ai workers
    CHECK_EQ_EXIT("pthread_sigmask", pthread_sigmask(SIG_BLOCK, &mask, &oldmask), -1, "pthread_sigmask failed\n");

    for (int i = 0; i < threadpool_size; ++i) // avvio workers
        CHECK_NEQ_EXIT("pthread_create", pthread_create(&th[i], NULL, main_worker, thARGS), 0, "pthread_create failed (Worker)");

    //ripristino la vecchia maschera
    CHECK_EQ_EXIT("pthread_sigmask", pthread_sigmask(SIG_SETMASK, &oldmask, NULL), -1, "pthread_sigmask failed\n");
        
    return M_SUCCESS;
}

/**
 * \brief Funzione di join threads
 *
 * \param th array di threads
 * \param threadpool_size dimensione threadpool
 */
static void join_threads(pthread_t *th, size_t threadpool_size){
    for (int i = 0; i < threadpool_size; ++i){
        CHECK_NEQ_EXIT("pthread_join", pthread_join(th[i], NULL), 0, "pthread_join failed (Worker)");
    }

    free(th);
}

/**
 * \brief nanosleep() per i richiesti msec ms
 * 
 * \param msec ms da attendere
 * \param sockfd file descriptor del socket
 * \param max_mcomms_len lunghezza massima del messaggio da parte del master in caso di eventuale comunicazine
 * 
 */
static inline void ms_sleep(size_t msec, const int sockfd, const int max_mcomms_len)
{
    struct timespec ts;
    int res;

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    do {
        res = nanosleep(&ts, &ts);
        if(print){ //se interruzione setta flag print = 1
            send_to_sockfd(sockfd, "usr1", max_mcomms_len); //mando comando di print
            print = 0;
        }
        else if(res != 0 && end == 0){ //non sono stato fermato da end o print
            perror("nanosleep");
            print_error("nanosleep failed with errno=%d", errno);
        }
    } while (res && !end); //esco solo se finisce tempo delay o interruzione chiama fine programma
}

/**
 * \brief Funzione di push stringa in coda concorrente
 *
 * \param to_push stringa da inserire
 * \param mARGS argomenti del master (controllare definizione di masterArgs)
 * 
 */
static void push_into_queue(char *to_push, masterArgs mARGS){

    if(isRegular(to_push) == 0 && isExt(to_push, mARGS.ext) == 0){ //se il file ha le proprietà corrette
        int ret = push(mARGS.q, to_push);
        if(ret == 0) //operazione andata a buon fine
            ms_sleep(mARGS.delay, mARGS.collectorfd, mARGS.max_mcomms_len);
        else if(ret == -1) //push error
            end = 1; //termino coda
        else if(ret == -2) //timeout su coda concorrente
            end = 2; //termino coda
    }
    else{ //file non rispetta le proprietà desiderate
        print_error("%s is not a regular file or is not a file.%s\n", to_push, mARGS.ext);
    }
}

/**
 * \brief Funzione ricorsiva che cerca tutti i files e li inserisce nella coda concorrente
 *
 * \param basepath nome della directory passato come argomento
 * \param mARGS argomenti del master (controllare definizione di masterArgs)
 */
static void file_seeker(char *basepath, masterArgs mARGS){

    char path[mARGS.max_path_len];
    struct dirent *dp;
    DIR *dir;
    CHECK_EQ_RETURN("opendir", dir = opendir(basepath), NULL, ,"opendir of %s failed\n", basepath);

    if(end)
        return;

    while ((errno = 0, dp = readdir(dir)) != NULL){

        if(end)
            return;
        //aggiungo path sottodirectory al basepath se basepath len + sottodirectory len < MAX_PATH_LEN
        if(strlen(basepath) + strlen(dp->d_name) < mARGS.max_path_len - 6){ //-6 perch`e name_len minima di file .dat `e 5 (x.dat) + char '/' da inserire nel path
            CHECK_NEQ_CONTINUE("strncpy", strncpy(path, basepath, mARGS.max_path_len), path, "strcpy of %s failed\n", basepath);
            CHECK_NEQ_CONTINUE("strcat", strcat(path, "/"), path, "strcat of %s failed\n", "/");
            CHECK_NEQ_CONTINUE("strncat", strncat(path, dp->d_name, mARGS.max_path_len), path, "strncat of %s failed\n", dp->d_name);
        }
        else{
            print_error("file or sub-directory %s/%s is too long\n", basepath, dp->d_name);
            continue;
        }

        //recupero stat di i-esimo file
        struct stat statbuf;
        if (stat(path, &statbuf) < 0){
            perror("stat");
            print_error("stat of %s failed with errno=%d\n", path, errno);
            continue;
        }

        if (S_ISDIR(statbuf.st_mode)){ // se è sotto directory
            if (strncmp(dp->d_name, ".", mARGS.max_path_len) != 0 && strncmp(dp->d_name, "..", mARGS.max_path_len) != 0){ // e se non è punto o doppio punto
                file_seeker(path, mARGS); //chiamata ricorsiva
            }
        }
        else // se non è sotto directory
            push_into_queue(path, mARGS); //provo ad aggiungere il file in coda
    }

    if (errno != 0) // controllo errori
      perror("readdir");
    closedir(dir);

}

/**
 * \brief Funzione di inserimento files da argv in coda concorrente
 *
 * \param argc int che indica dimensione di argv
 * \param argv array contenente gli argomenti inseriti sulla command line durante l'avvio del programma
 * \param opt_index indice a cui getopt e' arrivato
 * \param th array di threads
 * \param mARGS argomenti del master (controllare definizione di masterArgs)
 * \param dirs array dinamico contenenti nomi delle directories trovati nei primi argvs
 * 
 * \retval M_SUCCESS in caso di successo
 * \retval M_FAILURE in caso di errore
 */
static int feed_files(int argc, char **argv, int opt_index, pthread_t *th, masterArgs mARGS, DArray *dirs){
    
    //definisco argomento dei threads
    threadArgs_t thARGS;
    CHECK_NEQ_RETURN("memset", memset(&thARGS, 0, sizeof(threadArgs_t)), &thARGS, M_FAILURE, "memset failed\n");
    thARGS.q = mARGS.q;
    thARGS.max_path_len = mARGS.max_path_len;
    thARGS.sockname = mARGS.sockname;

    //inizializzo threads
    CHECK_EQ_RETURN("init_threads", init_threads(th, mARGS.threadpool_size, &thARGS), M_FAILURE, M_FAILURE, "init_threads failed\n");

    char to_push[mARGS.max_path_len];

    while(getDUsed(dirs) != 0){ //itero prima per i nomi delle directories ricavati da parse_first_args()
        struct stat statbuf;
        char* dirname;
        CHECK_EQ_EXIT("getDData", dirname = getDData(dirs), NULL, "getDData error");
        CHECK_NEQ_RETURN("strncpy", strncpy(to_push, dirname, mARGS.max_path_len), to_push, M_FAILURE, "strncpy of %s failed\n", dirname);
        stat(to_push, &statbuf);
        if(stat(to_push, &statbuf) < 0){ //non esco dal programma in caso stat fallisca, ma semplicemente passo alla prossima dir
            print_error("stat of %s failed with errno=%d -> ", to_push, errno);
            perror("stat");
            free(dirname);
            continue;
        }
        if (S_ISDIR(statbuf.st_mode)) //se viene passata una directory
            file_seeker(to_push, mARGS); //itero ricorsivamente sui files
        else // altrimenti errore
            print_error("%s is not a directory\n", to_push);
        free(dirname);
    }

    deleteDArray(dirs);

    while(opt_index < argc && end == 0){ //itero per tutte le stringhe di argv rimaste
        fflush(stdout);
        if(strcmp(argv[opt_index], "-d") != 0){ //se non si tratta del flag -d
            CHECK_NEQ_RETURN("strncpy", strncpy(to_push, argv[opt_index], mARGS.max_path_len), to_push, M_FAILURE, "strncpy of %s failed\n", argv[opt_index]);
            to_push[mARGS.max_path_len-1] = '\0';

            push_into_queue(to_push, mARGS); //provo a inserire in queue argv[opt_index]
        }
        else{ //trovo un flag -d
            if (opt_index == argc - 1) // se non contiene argomento
            {
                print_error("last -d flag doesn't have an argument\n");
                break;
            }
            struct stat statbuf;
            opt_index++; //punto ad argomento di -d
            stat(argv[opt_index], &statbuf);
            if(stat(argv[opt_index], &statbuf) < 0){ //non esco dal programma in caso stat fallisca, ma semplicemente passo al prossimo argv
                print_error("stat of %s failed with errno=%d -> ", argv[opt_index], errno);
                perror("stat");
                continue;
            }
            if (S_ISDIR(statbuf.st_mode)) //se viene passata una directory
                file_seeker(argv[opt_index], mARGS); //itero ricorsivamente sui files
            else // altrimenti errore
                print_error("%s is not a directory\n", argv[opt_index]);
        }
        opt_index++;
    }

    if(end != 2) //se non esco per timeout sull'attesa di coda piena del Master
        push(mARGS.q, EOS); //inserisco EOS all'interno della coda

    join_threads(th, mARGS.threadpool_size); //e infine effettuo il join dei threads

    return M_SUCCESS;
}

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
int execute_master(masterArgs mARGS, int argc, char **argv, int argc_index, DArray *dirs){

    if(argc_index == argc && getDUsed(dirs) == 0){ //caso in cui non vengono inseriti files / directory
        printf("Master: no input files / directory\n");
        return M_SUCCESS;
    }

    pthread_t *th;        // dichiaro array di thread

    CHECK_EQ_EXIT("malloc", th = malloc(mARGS.threadpool_size * sizeof(pthread_t)), NULL, "malloc error");

    int ret = feed_files(argc, argv, argc_index, th, mARGS, dirs);

    return ret;

}

/**
 * \brief Funzione di connessione da parte del Master utilizzando sock_name (chiamata bloccante)
 *
 * \param sock_name nome del socket a cui connettersi
 * \param retry_timeout tempo (in ms) di delay tra tentativi di connessione
 * 
 * \retval sockfd file descriptor del socket a cui il master è stato connesso
 * \retval M_FAILURE in caso di errore
 */
int connect_master(const char* sockname, const int retry_timeout){
    if(retry_timeout <= 0){
        return M_FAILURE;
    }
    int sockfd = -1;
    SYSCALL_RETURN("socket", sockfd, socket(AF_UNIX, SOCK_STREAM, 0), M_FAILURE, "socket error\n");

    struct sockaddr_un server_addr;
    server_addr.sun_family = AF_UNIX;
    CHECK_NEQ_RETURN("strcpy", strcpy(server_addr.sun_path, sockname), server_addr.sun_path, M_FAILURE, "strcpy of %s failed\n", sockname);

    struct timespec ts;
    ts.tv_sec = retry_timeout / 1000; 
    ts.tv_nsec = (retry_timeout % 1000) * 1000000;

    int res;
    //provo a connettermi con attesa di retry_timeout tra tentativi
    while(connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) { //in caso collector non sia pronto
        do {
            res = nanosleep(&ts, &ts);
        } while (res > 0); //in caso di interruzione finisco comunque attesa
        if(res == -1) //interrotto da interruzione non gestita
            break;
    }

    return sockfd;

}

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
int init_master_args(masterArgs *mARGS, BQueue_t *q, size_t nthread, int collectorfd, const char* sockname, const char* ext, size_t delay, int max_path_len, int max_mcomms_len){
    //parametri già controllati in parse_first_args
    mARGS->delay = delay;
    mARGS->threadpool_size = nthread;
    mARGS->collectorfd = collectorfd;

    mARGS->q = q;
    strncpy(mARGS->ext, ext, _MAX_EXT_LEN);

    char sockname_ext[4] = "sck";
    if((strlen(sockname) < _MIN_SOCKNAME_LEN || strlen(sockname) > _MAX_SOCKNAME_LEN) || isExt(sockname, sockname_ext) != 0)
        return M_FAILURE;
    strncpy(mARGS->sockname, sockname, _MAX_SOCKNAME_LEN);

    if(max_path_len < _MIN_PATH_LEN || max_path_len > _MAX_PATH_LEN)
        return M_FAILURE;
    mARGS->max_path_len = max_path_len;

    if(max_mcomms_len < _MIN_MCOMMS_LEN || max_path_len > _MAX_MCOMMS_LEN)
        return M_FAILURE;
    mARGS->max_mcomms_len = max_mcomms_len;

    return M_SUCCESS;
}

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
int parse_first_args(int argc, char **argv, size_t *nthread, size_t *qlen, size_t *delay, int *argc_index, DArray *dirs){
    
    char *programname = argv[0]; //program name

    int opt;

    while((opt = getopt(argc, argv, "n:q:d:t:")) != -1) {
        long tmp_par = 0;
        //gli argomenti n, q, t necessitano di un numero compreso tra i massimi valori disponibili per quel campo
        //in caso venga passato un valore non accettabile, essi manterranno il loro valore di default
        switch (opt) {
            case 'n': //nthread
                if(isNumber(optarg, &tmp_par) != 0){
                    print_error("option %c requires a number > %d and < %d (default value assigned: %d)\n", opt, _MIN_NTHREAD_VALUE, _MAX_NTHREAD_VALUE, _DEFAULT_NTHREAD_VALUE);
                    break;
                }
                if(tmp_par >= _MIN_NTHREAD_VALUE && tmp_par < _MAX_NTHREAD_VALUE)
                    *nthread = tmp_par;
                else
                    print_error("option %c requires a number > %d and < %d (default value assigned: %d)\n", opt, _MIN_NTHREAD_VALUE, _MAX_NTHREAD_VALUE, _DEFAULT_NTHREAD_VALUE);
                break;

            case 'q': //qlen
                if(isNumber(optarg, &tmp_par) != 0){
                    print_error("option %c requires a number > %d and < %d (default value assigned: %d)\n", opt, _MIN_QLEN_VALUE, _MAX_QLEN_VALUE, _DEFAULT_QLEN_VALUE);
                    break;
                }
                if(tmp_par >= _MIN_QLEN_VALUE && tmp_par < _MAX_QLEN_VALUE)
                    *qlen = tmp_par;
                else
                    print_error("option %c requires a number > %d and < %d (default value assigned: %d)\n", opt, _MIN_QLEN_VALUE, _MAX_QLEN_VALUE, _DEFAULT_QLEN_VALUE);
                break;

            case 'd': //caso in cui trovo -d, quindi salvo argomento in array dinamico
                CHECK_EQ_EXIT("addDData", addDData(dirs, optarg), -1, "addDData alloc error");
                break;

            case 't': //delay
                if(isNumber(optarg, &tmp_par) != 0){
                    print_error("option %c requires a number > %d and < %d (default value assigned: %d)\n", opt, _MIN_DELAY_VALUE, _MAX_DELAY_VALUE, _DEFAULT_DELAY_VALUE);
                    break;
                }
                if(tmp_par >= _MIN_DELAY_VALUE && tmp_par < _MAX_DELAY_VALUE)
                    *delay = tmp_par;
                else
                    print_error("option %c requires a number > %d and < %d (default value assigned: %d)\n", opt, _MIN_DELAY_VALUE, _MAX_DELAY_VALUE, _DEFAULT_DELAY_VALUE);
                break;
            case ':': //opt senza valore quando invece `e richiesto
                print_error("option %c requires a value (so it keeps default value)\n", optopt);
                break;
            case '?':  //opt non riconosciuta
                //print_error("%s: invalid option -- %c\n", programname, optopt);
                break;
            default:
                //usage print
                print_error("usage: %s [-n <nthread > 0>] [-q <qlen > 0>] {[-d <directory-name>] or [file-name-1, file-name-2, ..., file-name-m]} [-t <time delay>=0>]\n", programname);
                return M_FAILURE;
        }
    }

    *argc_index = optind;

    return M_SUCCESS;
}


