/* Renommage des ID des clients. */
/* Implémentation d'une hashmap simplifiée. */

#ifndef RENAME_H
#define RENAME_H

#include <pthread.h>
#include <sys/types.h>

#define ID_MAX 1024



struct map_entry {
    u_int ID ;                  /* ID du client */
    u_int name ;          /* nouvel entier */
    struct map_entry * next;
};

struct map {
    u_int map_size  ; 
    pthread_mutex_t size_lock ;
    struct map_entry *name_from_id[ID_MAX+1] ;
    u_int id_from_name[ID_MAX+1] ;
    pthread_mutex_t map_lock[ID_MAX+1]; /* protections individuelles des cases */    
};

/* Initialise la map. */
struct map *init_map() ;

/* Libère la map. */
void destroy_map(struct map *m) ;

#define reinit_map(m) destroy_map(m) ; m=init_map() ;

/* Retourne le nouveau nom. */
/* S'il n'existe pas dans la map, l'ajoute. */
u_int get_name(struct map *m, u_int id) ;

/* Retourne le nom original. */
u_int get_id(struct map *m, u_int name);

/* Retourne le nombre de clés stockés. */
u_int get_map_size(struct map *m) ;

#endif
