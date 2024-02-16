#include <worker.h>
#include <conc_queue.h>
#include <conn.h>
#include <string.h>
#include <util.h>
#include <math.h>

#include <pthread.h>

#define OVERFLOW -2
#define FILE_ERROR -1

/** 
 * \brief Task eseguito dal Worker
 *
 * \param file_to_calculate nome del file dal calcolare (task)
 * 
 * \retval result se il risultato è stato calcolato senza problemi
 * \retval OVERFLOW se è stato rilevato un overflow
 * \retval FILE_ERROR se è stato rilevato un errore durante la gestione del file
 */
static long compute_result(char* file_to_calculate){
    long ret = 0;
    FILE *file;
    CHECK_EQ_RETURN("fopen", file = fopen(file_to_calculate, "rb"), NULL, FILE_ERROR, "fopen error of %s\n", file_to_calculate);

    CHECK_NEQ_RETURN("fseek", fseek(file, 0, SEEK_END), 0, FILE_ERROR, "fseek error of file %s\n", file_to_calculate);
    long file_size;
    //recupero lunghezza file
    CHECK_EQ_RETURN("ftell", file_size = ftell(file), -1, FILE_ERROR, "ftell error of file %s\n", file_to_calculate);
    //per ottenere numero di elementi (assumendo che i dati vengano interpretati come 'long')
    int num_elements = file_size / sizeof(long);

    long *arr;
    CHECK_EQ_RETURN("malloc", arr = (long int *)malloc(num_elements * sizeof(long int)), NULL, FILE_ERROR, "malloc error");

    //mi riposiziono in cima
    CHECK_NEQ_RETURN("fseek", fseek(file, 0, SEEK_SET), 0, FILE_ERROR, "fseek error of file %s\n", file_to_calculate);
    //e leggo da file
    CHECK_NEQ_RETURN("fread", fread(arr, sizeof(long int), num_elements, file), num_elements, FILE_ERROR, "fread error of file %s\n", file_to_calculate);

    for (int i = 0; i < num_elements; i++){ //effettuo calcolo
        ret = safeAdd(ret, arr[i] * i);
        if(ret < 0)
            return OVERFLOW;
    }

    free(arr);

    fclose(file);

    return ret;
}

/**
 * \brief Funzione che rappresenta il ciclo di vita del Worker
 *
 * \param arg argomento del Worker void* (in questo caso viene passato threadArgs_t)
 */
void *main_worker(void *arg){
    BQueue_t *q = ((threadArgs_t*)arg)->q;
    int max_path_len = ((threadArgs_t *)arg)->max_path_len;
    const char* sockname = ((threadArgs_t *)arg)->sockname;

    const size_t MAX_WORKER_MESS_LEN = max_path_len + 1 + 10; //+1 term char + 10 long size

    int sockfd;
    SYSCALL_RETURN("socket", sockfd, socket(AF_UNIX, SOCK_STREAM, 0), NULL, "socket error\n");

    struct sockaddr_un server_addr;
    server_addr.sun_family = AF_UNIX;
    CHECK_NEQ_RETURN("strcpy", strcpy(server_addr.sun_path, sockname), server_addr.sun_path, NULL, "strcpy of %s failed\n", sockname);

    int notused;
    //mi connetto a Collector
    SYSCALL_RETURN("connect", notused, connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)), NULL, "connect error\n");

    char buff[MAX_WORKER_MESS_LEN];

    while(1){
        char* file_to_calculate = pop(q); //estraggo file
        if(!file_to_calculate) //q parametro non valido or calloc error
            return NULL;
            
        if(file_to_calculate == EOS) //se si tratta di EOS termino vita Worker
            break;

        CHECK_NEQ_RETURN("memset", memset(buff, 0, MAX_WORKER_MESS_LEN), buff, NULL, "memset failed\n");

        long result = compute_result(file_to_calculate);
        if(result < 0){ //error
            free(file_to_calculate);
            break; 
        }

        int res_len = (int) log10(result) + 1;
        CHECK_NEQ_RETURN("sprintf", sprintf(buff, "%ld%s", result, file_to_calculate), res_len + strlen(file_to_calculate), NULL, "sprintf error\n");
        free(file_to_calculate);
        if((writen(sockfd, buff, MAX_WORKER_MESS_LEN)) == -1) {
            print_error("no readers in the channel\n");
            close(sockfd);
            return NULL;
        }

        #ifdef RETURN_AFTER_ONE_TASK //test purposes (vedi relazione test 7)
            return NULL;
        #endif
    }

    close(sockfd);
    return NULL;
}
