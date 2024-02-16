#if !defined(DYN_ARRAY_H)
#define DYN_ARRAY_H

#include <stdlib.h>

/** 
 * \brief Array dinamico (con politica LIFO) di stringhe lunghe strings_size
 *
 */
typedef struct d_array{
    char **data;
    size_t aused; // dimensione attuale
    size_t asize; // dimensione massima
    size_t strings_size; // lunghezza massima stringhe
} DArray;

/** 
 * \brief Alloca ed inizializza un array dinamico di stringhe
 *
 * \retval NULL se si sono verificati problemi nell'allocazione (errno settato)
 * \retval d puntatore all'array dinamico
 */
DArray *initDArray(size_t init_size, size_t strings_size);

/** 
 * \brief Cancella un array dinamico allocato con con initDArray.
 *
 * \param d puntatore all'array dinamico da cancellare
 */
void deleteDArray(DArray *d);

/** 
 * \brief Restituisce il numero di elementi
 * \param d puntatore all'array dinamico
 *
 * \retval aused se numero elementi >= 0
 * \retval -1 se errore (errno settato opportunamente)
 */
int getDUsed(DArray *d);

/** 
 * \brief Inserisce un nodo nell'array dinamico in coda (se necessario viene raddoppiata la dimensione dell'array).
 * \param d puntatore all'array dinamico
 * \param data puntatore alla stringa da inserire
 *
 * \retval 0 se successo
 * \retval -2 se i dati non sono stati passati correttamente (errno settato opportunamente)
 * \retval -1 se errore (errno settato opportunamente)
 */
int addDData(DArray *d, char *data);

/** 
 * \brief Restituisce la stringa in coda all'array.
 * \param d puntatore all'array dinamico
 *
 * \retval stringa puntatore alla stringa restituita.
 */
char *getDData(DArray *d);

#endif /* DYN_ARRAY_H */
