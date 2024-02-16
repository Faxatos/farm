#if !defined(UTIL_H)
#define UTIL_H

#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>

#if !defined(EXTRA_LEN_PRINT_ERROR)
#define EXTRA_LEN_PRINT_ERROR 512
#endif

//macro per controllo 

#define SYSCALL_EXIT(name, r, sc, str, ...) \
  if ((r = sc) == -1)                       \
  {                                         \
    perror(#name);                          \
    int errno_copy = errno;                 \
    print_error(str, ##__VA_ARGS__);        \
    exit(errno_copy);                       \
  }

#define SYSCALL_RETURN(name, r, sc, ret, str, ...) \
  if ((r = sc) == -1)                              \
  {                                                \
    perror(#name);                                 \
    int errno_copy = errno;                        \
    print_error(str, ##__VA_ARGS__);               \
    errno = errno_copy;                            \
    return ret;                                    \
  }

#define CHECK_EQ_EXIT(name, X, val, str, ...) \
  if ((X) == val)                             \
  {                                           \
    perror(#name);                            \
    int errno_copy = errno;                   \
    print_error(str, ##__VA_ARGS__);          \
    exit(errno_copy);                         \
  }

#define CHECK_NEQ_EXIT(name, X, val, str, ...) \
  if ((X) != val)                              \
  {                                            \
    perror(#name);                             \
    int errno_copy = errno;                    \
    print_error(str, ##__VA_ARGS__);           \
    exit(errno_copy);                          \
  }

#define CHECK_EQ_RETURN(name, X, val, ret, str, ...) \
  if ((X) == val)                                    \
  {     					                                   \
    perror(#name);                                   \
    print_error(str, ##__VA_ARGS__);                 \
    return ret;                                      \
  }

#define CHECK_NEQ_RETURN(name, X, val, ret, str, ...) \
  if ((X) != val)                                     \
  {               					                          \
    perror(#name);                                    \
    print_error(str, ##__VA_ARGS__);                  \
    return ret;                                       \
  }
 
#define CHECK_NEQ_CONTINUE(name, X, val, str, ...)    \
  if ((X) != val)                                     \
  {                                                   \
    print_error(str, ##__VA_ARGS__);                  \
    continue;                                         \
  }

//nanosleep prototype
int nanosleep(const struct timespec *req, struct timespec *rem);

/**
 * \brief Procedura di utilita' per la stampa degli errori
 * 
 * \param str stringa da stampare, [argomenti della stringa]
 *
 */
static inline void print_error(const char *str, ...){
  const char err[] = "ERROR: ";
  va_list argp;
  char *p = (char *)malloc(strlen(str) + strlen(err) + EXTRA_LEN_PRINT_ERROR);
  if (!p)
  {
    perror("malloc");
    fprintf(stderr, "FATAL ERROR on function 'print_error'\n");
    return;
  }
  strcpy(p, err);
  strcpy(p + strlen(err), str);
  va_start(argp, str);
  vfprintf(stderr, p, argp);
  va_end(argp);
  free(p);
}

/**
 * \brief Controlla se la stringa passata come primo argomento e' un numero.
 * 
 * \param s stringa da convertire
 * \param n long in cui verrà inserito il numero convertito
 * 
 * \retval  0 se n è un numero
 * \retval 1 se n non è un numbero
 * \retval -1 se overflow/underflow
 */
static inline int isNumber(const char *s, long *n){
  if (s == NULL)
    return 1;
  if (strlen(s) == 0)
    return 1;
  char *e = NULL;
  errno = 0;
  long val = strtol(s, &e, 10);
  if (errno == ERANGE)
    return -1; // overflow/underflow
  if (e != NULL && *e == (char)0)
  {
    *n = val;
    return 0; // successo
  }
  return 1; // non è un numero
}

/**
 * \brief Controlla se la stringa passata è un file regolare.
 * 
 * \param file string passata
 * 
 * \return  0 è un file regolare
 * \return  1 non è un file regolare
 * \return  -1 stat error
 */
static inline int isRegular(const char *file){
  struct stat statbuf;
  if(stat(file, &statbuf) == -1)
    return -1;
  if (S_ISREG(statbuf.st_mode))
    return 0;
  return 1;
}

/**
 * \brief Controlla se la stringa passata è un file .ext
 * 
 * \param filename stringa passata
 * \param ext estensione da controllare
 * 
 * \return  0 è un file .ext
 * \return  1 non è un file con estensione .ext
 */
static inline int isExt(const char *filename, const char *ext){
  const char* dot = strrchr(filename, '.'); //prelevo estensione
  if(!dot || strcmp(dot, filename) == 0)
    return 1;
  if(strcmp(dot+1, ext) == 0)
    return 0;
  return 1;
}

/**
 * \brief Controlla se la stringa passata è una directory.
 * 
 * \param file string passata
 * 
 * \return  0 è una directory
 * \return  1 non è una directory
 * \return  -1 stat error
 */
static inline int isDir(const char *file){
  struct stat statbuf;
  if(stat(file, &statbuf) == -1)
    return -1;
  if (S_ISDIR(statbuf.st_mode))
    return 0;
  return 1;
}

/**
 * \brief Controlla se la somma di due long restituisce overflow
 * 
 * \param a primo addendo
 * \param b primo secondo
 * 
 * \return  a + b se tutto ok
 * \return  -1 se overflow
 */
static inline long safeAdd(long a, long b){
  if(a > 0 && b > __LONG_MAX__ - a)
    return -1;
  return a+b;
}


#define LOCK(l)                              \
  if (pthread_mutex_lock(l) != 0)            \
  {                                          \
    fprintf(stderr, "ERRORE FATALE lock\n"); \
    pthread_exit((void *)EXIT_FAILURE);      \
  }
#define UNLOCK(l)                              \
  if (pthread_mutex_unlock(l) != 0)            \
  {                                            \
    fprintf(stderr, "ERRORE FATALE unlock\n"); \
    pthread_exit((void *)EXIT_FAILURE);        \
  }
#define WAIT(c, l)                           \
  if (pthread_cond_wait(c, l) != 0)          \
  {                                          \
    fprintf(stderr, "ERRORE FATALE wait\n"); \
    pthread_exit((void *)EXIT_FAILURE);      \
  }
#define TWAIT(c,l,t) {							                            \
  int r=0;								                                      \
  if ((r=pthread_cond_timedwait(c,l,t))!=0 && r!=ETIMEDOUT) {		\
    fprintf(stderr, "ERRORE FATALE timed wait\n");			        \
    pthread_exit((void *)EXIT_FAILURE); 			                  \
  }						                                                  \
  if(r == ETIMEDOUT){                                           \
    fprintf(stderr, "Producer timeout!\n");                     \
    return -2;	                                                \
  }                                                             \
  return 0;	                                                    \
  }                                                               
#define SIGNAL(c)                              \
  if (pthread_cond_signal(c) != 0)             \
  {                                            \
    fprintf(stderr, "ERRORE FATALE signal\n"); \
    pthread_exit((void *)EXIT_FAILURE);        \
  }
#define BCAST(c)                                  \
  if (pthread_cond_broadcast(c) != 0)             \
  {                                               \
    fprintf(stderr, "ERRORE FATALE broadcast\n"); \
    pthread_exit((void *)EXIT_FAILURE);           \
  }

#endif /* UTIL_H */
