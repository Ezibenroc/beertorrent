#include <stdlib.h>
#include <assert.h>
#include <pthread.h>

#include "rename.h"

void init_map() {
    int i ;
    for(i = 0 ; i <= ID_MAX ; i++) {
        map[i] = NULL ;
        assert(pthread_mutex_init(&map_lock[i], NULL)==0);
    }
    assert(pthread_mutex_init(&size_lock, NULL)==0);
    map_size = 0 ;
}

void destroy_map() {
    int i ;
    struct map_entry *p, *old ;
    for(i = 0 ; i <= ID_MAX ; i++) {
        p = map[i] ;
        while(p != NULL) {
           old = p ;
           p = p->next ;
           free(old) ; 
        }
        assert(pthread_mutex_destroy(&map_lock[i])==0);        
    }
    assert(pthread_mutex_destroy(&size_lock)==0);
}

u_short get_name(u_int id) {
    struct map_entry *p, *new ;
    u_int i = id%(ID_MAX+1) ;
    
    pthread_mutex_lock(&map_lock[i]) ;      
    
    /* Recherche de id à la case i */
    p=map[i] ;
    while(p != NULL && p->ID != id) {
        p=p->next ;
    }
    if(p != NULL) { /* Déjà référencé, on renvoie sa valeur. */
        pthread_mutex_unlock(&map_lock[i]) ;
        return p->name ;    
    }
    else { /* Non référencé, on l'ajoute en tête */
        pthread_mutex_lock(&size_lock) ;
        assert(map_size != ID_MAX) ;
        new = malloc(sizeof(struct map_entry)) ;
        new->ID = id ;
        new->name = map_size++ ;   
        new->next = map[i] ;
        map[i] = new ;
        id_from_name[new->name] = new->ID ;
        pthread_mutex_unlock(&size_lock) ;
        pthread_mutex_unlock(&map_lock[i]) ;
        return new->name ;
    }
}

u_int get_id(u_short name) {
    assert(name < map_size) ; /* on ne met pas de mutex ici, pour ne pas allourdir le code */
    return id_from_name[name] ;
}

u_int get_map_size() {
    u_int s ;
    pthread_mutex_lock(&size_lock) ;
    s = map_size ;
    pthread_mutex_unlock(&size_lock) ;
    return s ; 
}
