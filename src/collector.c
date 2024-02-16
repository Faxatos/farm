#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <signal.h>

#include <collector.h>
#include <util.h>
#include <conn.h>

/**
 * @brief funzione di gestione segnali (comportamento spiegato nella relazione)
 *
 */

void handle_collector_signals(){

    sigset_t set;

    //maschero tutti i segnali fino a quando non installo gestore
    CHECK_EQ_EXIT("sigfillset", sigfillset(&set), -1, "sigfillset failed\n");
    CHECK_EQ_EXIT("pthread_sigmask", pthread_sigmask(SIG_SETMASK, &set, NULL), -1, "pthread_sigmask failed\n");

    struct sigaction s;
    CHECK_NEQ_EXIT("memset", memset(&s, 0, sizeof(s)), &s, "memset failed\n");

    s.sa_flags = ERESTART;
    s.sa_handler = SIG_IGN;

    CHECK_EQ_EXIT("sigaction", sigaction(SIGINT, &s, NULL), -1, "sigaction failed\n");
    CHECK_EQ_EXIT("sigaction", sigaction(SIGQUIT, &s, NULL), -1, "sigaction failed\n");
    CHECK_EQ_EXIT("sigaction", sigaction(SIGTERM, &s, NULL), -1, "sigaction failed\n");
    CHECK_EQ_EXIT("sigaction", sigaction(SIGHUP, &s, NULL), -1, "sigaction failed\n");
    CHECK_EQ_EXIT("sigaction", sigaction(SIGUSR1, &s, NULL), -1, "sigaction failed\n");
    
    //ripristino tutti i segnali
    CHECK_EQ_EXIT("sigemptyset", sigemptyset(&set), -1, "sigemptyset failed\n");
    CHECK_EQ_EXIT("pthread_sigmask", pthread_sigmask(SIG_SETMASK, &set, NULL), -1, "pthread_sigmask failed\n");
}

/**
 * @brief funzione che interpreta comunicazioni da parte del Master
 *
 * @param l lista in cui vengono caricati i risultati
 * @param end indica la fine della raccolta dati da parte del collector
 * @param msg messaggio del Master
 */

static void master_comms(SList *l, int *end, char* msg){
    if(strcmp(msg, "quit") == 0)
        *end = 1;
    else if(strcmp(msg, "usr1") == 0){
        printSList(l);
    }
    /*
    else if(strcmp(msg, "usr2") == 0){
        //other behaviors
    }
    */
    else
        print_error("'%s' command not recognized", msg);
    return;
}

/**
 * @brief funzione che raccoglie i risultati dai Workers
 *
 * @param l lista in cui verranno salvati i risultati
 */

int receive_results(SList *l, int max_path_len, int max_comms_len, const char* sockname) {

    
    if(max_path_len < _MIN_MESS_LEN || max_path_len > _MAX_MESS_LEN)
        return C_FAILURE;
    const size_t MAX_WORKER_MESS_LEN = max_path_len + 1 + 10; //+1 term char + 10 long size
    if(max_comms_len < _MIN_MESS_LEN || max_comms_len > _MAX_MESS_LEN)
        return C_FAILURE;
    const size_t MAX_MASTER_MESS_LEN = max_comms_len;

    int listenfd, fdmax = 0; 

    char msg[MAX_WORKER_MESS_LEN];
    int end = 0;

    struct sockaddr_un serv_addr;
    CHECK_NEQ_EXIT("memset", memset(&serv_addr, 0, sizeof(serv_addr)), &serv_addr, "memset failed\n");
    serv_addr.sun_family = AF_UNIX;
    CHECK_NEQ_EXIT("strncpy", strncpy(serv_addr.sun_path, sockname, UNIX_PATH_MAX-1), serv_addr.sun_path, "strncpy of %s failed\n", sockname);

    //preparo socket in ascolto, dove listenfd = socket file descriptor
    SYSCALL_EXIT("socket", listenfd, socket(AF_UNIX, SOCK_STREAM, 0), "socket failed\n");
    int unused;
    SYSCALL_EXIT("bind", unused, bind(listenfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)), "bind failed\n");
    SYSCALL_EXIT("listen", unused, listen(listenfd, MAXBACKLOG), "listen failed\n");

    fd_set set, tmpset;

    FD_ZERO(&set);
    FD_ZERO(&tmpset);

    FD_SET(listenfd, &set);
    fdmax = listenfd;

    //accept() chiamata bloccante che attende la connessione con il Master
    int masterfd;
    SYSCALL_EXIT("accept", masterfd, accept(listenfd, (struct sockaddr *)NULL, NULL), "accept master failed\n");

    FD_SET(masterfd, &set);

    if(masterfd > fdmax)
        fdmax = masterfd;

    int nfd;
    
    while (!end) { //fino a quando non si chiude fd del master
        tmpset = set;
        if((nfd = select(fdmax + 1, &tmpset, NULL, NULL, NULL)) == -1) {
            if(errno == EINTR) {
                continue;
            }  
            perror("select");
            unlink(sockname);
            return C_FAILURE;
        }
        else {
            for (int fd = 0; fd < fdmax + 1; fd++) {
            
                if(FD_ISSET(fd, &tmpset)) {
                    long connfd;
                    if(fd == listenfd) {  //nuova richiesta di connessione
                        SYSCALL_RETURN("accept", connfd, accept(listenfd, (struct sockaddr *)NULL, NULL), C_FAILURE, "accept failed");
                        FD_SET(connfd, &set); // aggiungo il descrittore al master set
                        if (connfd > fdmax)
                            fdmax = connfd; // ricalcolo il massimo
                        continue;
                    }
                    else if(fd == masterfd) {  //comunicazione da parte del Master
                        if(readn(fd, msg, MAX_MASTER_MESS_LEN) <=0) {
                            FD_CLR(fd, &set);
                            close(fd);
                            end = 1;
                            continue;
                        }

                        master_comms(l, &end, msg);
                    
                        CHECK_NEQ_RETURN("memset", memset(msg, 0, MAX_MASTER_MESS_LEN), msg, C_FAILURE, "memset failed\n");
                    } 
                    else { //lettura
                        if(readn(fd, msg, MAX_WORKER_MESS_LEN)<=0) {
                            FD_CLR(fd, &set);
                            close(fd);
                            continue;
                        }
                        
                        char * file_path;
                        long result;

                        result = strtol(msg, &file_path, 10); //divido risultato da file_path
                        
                        CHECK_EQ_EXIT("addNode", addNode(l, file_path, result), -1,"addNode failed (alloc error)");

                        CHECK_NEQ_RETURN("memset", memset(msg, 0, MAX_WORKER_MESS_LEN), msg, C_FAILURE, "memset failed\n");
                    }
                }
            }
        } 
    }

    unlink(sockname);
    return C_SUCCESS;
}