/* Renommage des ID des clients. */
/* Implémentation d'une hashmap simplifiée. */

#ifndef RENAME_H
#define RENAME_H

#include <pthread.h>
#include <sys/types.h>

#define ID_MAX 254

/* Taille de la map, et nom du prochain nouvel ID. */
u_short map_size  ;
pthread_mutex_t size_lock ;



struct map_entry {
    u_int ID ;                  /* ID du client */
    u_short name ;          /* nouvel entier */
    struct map_entry * next;
};

struct map_entry *map[ID_MAX+1] ;
u_int id_from_name[ID_MAX+1] ;
pthread_mutex_t map_lock[ID_MAX+1]; /* protections individuelles des cases */

/* Initialise la map. */
void init_map() ;

/* Libère la map. */
void destroy_map() ;

#define reinit_map() destroy_map() ; init_map() ;

/* Retourne le nouveau nom du client id */
/* S'il n'existe pas dans la map, l'ajoute. */
u_short get_name(u_int id) ;

/* Retourne l'ID du client */
u_int get_id(u_short name);

/* Retourne le nombre de clients stockés. */
u_int get_map_size() ;

#endif
