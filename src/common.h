#ifndef COMMON_H
#define COMMON_H

#include "peerfunc.h"

#define max(x, y) (x > y ? x : y)
#define min(x, y) (x < y ? x : y)

/* ID du pair. */
u_int my_id ;
/* Port du pair. */
u_short my_port ;

/* Fonction d'affichage (ID et port). */
void print_id() ;

/* Hachage d'une chaîne de caractère. */
u_int hash(u_char *str) ;

/* Hachage d'un fichier (donné par son descripteur). */
u_int file2hash(int fd) ;

/* Retourne un pointeur vers le début de l'extension du fichier. */
char *get_filename_ext(char *filename) ;


#define N_SOCK 1024
#define N_CANCEL 8
struct cancel_entry {
    u_int msg[N_CANCEL][2] ; /* tableau circulaire représentant les message "cancel" reçus */
    u_int ind ;
    pthread_mutex_t lock ;
} cancel[N_SOCK] ;    

/* Initialisation du tableau cancel. */
void init_cancel() ;


/* Construit un handshake correspondant au torrent donné. */
struct proto_client_handshake* construct_handshake(struct beerTorrent *torrent);

/* Envoie le handshake au pair donné. */
void send_handshake(const struct proto_peer *peer, const struct proto_client_handshake *hs);

/* Initialise la connection de tous les pairs (création des sockets, réalisation des handshakes). */
void init_peers_connections(struct torrent_info *ti);
/*
void send_bitfield(struct torrent_state *torrent_state, struct peer *peer);*/

#endif
