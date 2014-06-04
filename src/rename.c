#include <stdlib.h>
#include <assert.h>
#include <pthread.h>

#include "rename.h"

/* Initialise la map. */
struct map *init_map() {
    int i ;
    struct map *m = malloc(sizeof(struct map)) ;
    for(i = 0 ; i <= ID_MAX ; i++) {
        m->name_from_id[i] = NULL ;
        assert(pthread_mutex_init(&m->map_lock[i], NULL)==0);
    }
    assert(pthread_mutex_init(&m->size_lock, NULL)==0);
    m->map_size = 0 ;
    return m ;
}

/* Libère la map. */
void destroy_map(struct map *m) {
    int i ;
    struct map_entry *p, *old ;
    for(i = 0 ; i <= ID_MAX ; i++) {
        p = m->name_from_id[i] ;
        while(p != NULL) {
           old = p ;
           p = p->next ;
           free(old) ; 
        }
        assert(pthread_mutex_destroy(&m->map_lock[i])==0);        
    }
    assert(pthread_mutex_destroy(&m->size_lock)==0);
    free(m);
}

/* Retourne le nouveau nom. */
/* S'il n'existe pas dans la map, l'ajoute. */
u_int get_name(struct map *m,u_int id) {
    struct map_entry *p, *new ;
    u_int i = id%(ID_MAX+1) ;
    
    pthread_mutex_lock(&m->map_lock[i]) ;      
    
    /* Recherche de id à la case i */
    p=m->name_from_id[i] ;
    while(p != NULL && p->ID != id) {
        p=p->next ;
    }
    if(p != NULL) { /* Déjà référencé, on renvoie sa valeur. */
        pthread_mutex_unlock(&m->map_lock[i]) ;
        return p->name ;    
    }
    else { /* Non référencé, on l'ajoute en tête */
        pthread_mutex_lock(&m->size_lock) ;
        assert(m->map_size != ID_MAX) ;
        new = malloc(sizeof(struct map_entry)) ;
        new->ID = id ;
        new->name = m->map_size++ ;   
        new->next = m->name_from_id[i] ;
        m->name_from_id[i] = new ;
        m->id_from_name[new->name] = new->ID ;
        pthread_mutex_unlock(&m->size_lock) ;
        pthread_mutex_unlock(&m->map_lock[i]) ;
        return new->name ;
    }
}

/* Retourne le nom original. */
u_int get_id(struct map *m, u_int name) {
    assert(name < m->map_size) ; /* on ne met pas de mutex ici, pour ne pas allourdir le code */
    return m->id_from_name[name] ;
}

/* Retourne le nombre de clés stockés. */
u_int get_map_size(struct map *m) {
    u_int s ;
    pthread_mutex_lock(&m->size_lock) ;
    s = m->map_size ;
    pthread_mutex_unlock(&m->size_lock) ;
    return s ; 
}
