#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <master.h>
#include <collector.h>
#include <conc_queue.h>
#include <dyn_array.h>
#include <util.h>

#define F_SUCCESS 0
#define F_FAILURE -1

//consts
#define MAX_PATH_LEN 255
#define MAX_MCOMMS_LEN 5
#define SOCKNAME "./farm_sock.sck"
#define EXT "dat"
#define RETRY_TIME 50 //50 ms
#define DARRAY_INIT_SIZE 2

/**
 * @file farm.c
 * @brief programma farm
 */

int main(int argc, char **argv){

    //fork con avvio collector
    int collector_id;
    SYSCALL_RETURN("fork", collector_id, fork(), M_FAILURE, "fork failed");

	if (collector_id != 0){ // master branch:
        //gestione segnali
        handle_master_signals();

        //dichiaro e inizializzo (con valore di default) gli argomenti
        size_t nthread = _DEFAULT_NTHREAD_VALUE;
        size_t qlen = _DEFAULT_QLEN_VALUE;
        size_t delay = _DEFAULT_DELAY_VALUE; 

        int argc_index = 0; // conterrà optind

        DArray *dirs;
        //array dinamico contenente -d args fino a optind
        CHECK_EQ_EXIT("initDArray", dirs = initDArray(DARRAY_INIT_SIZE, MAX_PATH_LEN), NULL, "initDArray failed\n");

        //parsing argomenti
        if(parse_first_args(argc, argv, &nthread, &qlen, &delay, &argc_index, dirs) != M_SUCCESS)
            return F_FAILURE;

        //dichiaro e inizializzo coda concorrente
        BQueue_t *q;
        CHECK_EQ_EXIT("initBQueue", q = initBQueue(qlen, MAX_PATH_LEN), NULL, "initBQueue failed\n");

        //connetto il Master al Collector
        int collectorfd;
        CHECK_EQ_RETURN("connect_master", collectorfd = connect_master(SOCKNAME, RETRY_TIME), M_FAILURE, M_FAILURE, "connect_master failed\n");
    
        //inizializzo argomenti master
        masterArgs mARGS;
        CHECK_NEQ_RETURN("memset", memset(&mARGS, 0, sizeof(masterArgs)), &mARGS, M_FAILURE, "memset failed\n");
        CHECK_EQ_RETURN("init_master_args", init_master_args(&mARGS, q, nthread, collectorfd, SOCKNAME, EXT, delay, MAX_PATH_LEN, MAX_MCOMMS_LEN), M_FAILURE, M_FAILURE, 
            "error in consts defined in farm.c; check master interface to see possible values for cons\n");

        //richiamo la funzione di inserimento files in BQueue_t q
        CHECK_EQ_RETURN("init_master_args", execute_master(mARGS, argc, argv, argc_index, dirs), M_FAILURE, M_FAILURE, 
            "execute_master failed\n");

        //chiudo la connessione al Collector
        close(collectorfd);

        //cancello coda
        deleteBQueue(q);

        //attendo che Collector termini
        int status;
        CHECK_EQ_RETURN("waitpid", waitpid(collector_id, &status, 0), -1, M_FAILURE, "waitpid error\n");
        if (!WIFEXITED(status))
        {
            fprintf(stdout, "Collector terminato con FALLIMENTO (%d)\n", WEXITSTATUS(status));
            fflush(stdout);
        }

        return M_SUCCESS;
    }
    else{ // collector branch:
        //gestisco i segnali
        handle_collector_signals();

        //inizializzo lista
        SList *l;
        CHECK_EQ_EXIT("initSList", l = initSList(MAX_PATH_LEN), NULL, "initSList failed\n");

        //la carico con i risultati ricevuti dai Workers
        CHECK_EQ_RETURN("receive_results", receive_results(l, MAX_PATH_LEN, MAX_MCOMMS_LEN, SOCKNAME), C_FAILURE, C_SUCCESS, 
            "receive_results failed\n");

        //stampo la lista
        printSList(l); 

        //e infine la cancello
        deleteSList(l);

        return C_SUCCESS;
    }
}