#if !defined(COLLECTOR_H)
#define COLLECTOR_H

#define C_SUCCESS 0
#define C_FAILURE -1

#define _MIN_MESS_LEN 1
#define _MAX_MESS_LEN 2048

#include <sor_list.h>

/**
 * \file collector.h
 * \brief Interfaccia per il Collector. 
 */

/**
 * \brief funzione che raccoglie i risultati dai Workers e li salva in SList 'l'
 *
 * \param l lista in cui verranno salvati i risultati
 * \param max_path_len massima lunghezza dei path ricevuti dai Workers
 * \param max_comms_len massima lunghezza delle comunicazioni ricevute dal Master
 * \param sockname nome del socket a cui collegarsi
 * 
 * \return C_SUCCESS se tutto va bene, C_FAILURE in caso di errore
 */

int receive_results(SList *l, int max_path_len, int max_comms_len, const char* sockname);

/**
 * \brief funzione di gestione segnali del collector
 *
 */

void handle_collector_signals();

#endif // COLLECTOR_H