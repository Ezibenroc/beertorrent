#ifndef COMMON_H
#define COMMON_H

#include "peerfunc.h"

#define max(x, y) (x > y ? x : y)
#define min(x, y) (x < y ? x : y)

u_int my_id ;
u_short my_port ;

u_int hash(u_char *str) ;
u_int file2hash(int fd) ;
char *get_filename_ext(char *filename) ;


#define N_SOCK 1024
#define N_CANCEL 8
struct cancel_entry {
    u_int msg[N_CANCEL][2] ; /* tableau circulaire représentant les message "cancel" reçus */
    u_int ind ;
    pthread_mutex_t lock ;
} cancel[N_SOCK] ;    

void init_cancel() ;


/* Construction d'un handshake. */
struct proto_client_handshake* construct_handshake(struct beerTorrent *torrent);

/* Envoie d'un handshake. */
void send_handshake(struct proto_peer *peer, const struct proto_client_handshake *hs);

/*
void init_peer_connection(int socket_fd, struct torrent_state *torrent_state);
void send_bitfield(struct torrent_state *torrent_state, struct peer *peer);*/

#endif
