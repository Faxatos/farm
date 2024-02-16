#if !defined(SOR_LIST_H)
#define SOR_LIST

#include <stdlib.h>

/** Struttura dati singolo nodo
 *
 */
typedef struct list_node
{
    char* string;
    long index;
    struct list_node *next;
} SNode;

/** Lista ordinata (per index) di nodi contenenti un index value e una stringa lunga str_len
 *
 */
typedef struct sorted_list
{
    SNode *head;
    size_t lsize; // dimensione attuale lista
    size_t str_len;
} SList;

/** Alloca ed inizializza una lista ordinata vuota di stringhe lunghe max_path_len
 * 
 *   \param max_path_len lunghezza massima delle stringhe
 *
 *   \retval NULL se si sono verificati problemi nell'allocazione (errno settato)
 *   \retval l puntatore alla lista allocata
 */
SList *initSList(size_t max_str_len);

/** Cancella una lista allocata con initSList.
 *
 *   \param l puntatore alla lista da cancellare
 */
void deleteSList(SList *l);

/** Inserisce un nodo nella lista ordinata in modo ordinato.
 *   \param l puntatore alla lista
 *   \param string puntatore alla stringa da inserire
 *   \param index index della stringa 
 *
 *   \retval 0 se successo
 *   \retval -1 se errore (errno settato opportunamente)
 */
int addNode(SList *l, char *path, long result);

/**  Stampa tutta la lista con il formato "'index' 'stringa'"
 *
 *  \param l puntatore alla lista
 */
void printSList(SList *l);

#endif // SOR_LIST