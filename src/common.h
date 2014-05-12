#ifndef COMMON_H
#define COMMON_H

#include "peerfunc.h"

#define max(x, y) (x > y ? x : y)
#define min(x, y) (x < y ? x : y)

u_int my_id ;

/* Construction d'un handshake. */
struct proto_client_handshake* construct_handshake(struct beerTorrent *torrent);

/* Envoie d'un handshake. */
void send_handshake(struct proto_peer *peer, const struct proto_client_handshake *hs);

/*
void initialize_peer_connection(int socket_fd, struct torrent_state *torrent_state);
void send_bitfield(struct torrent_state *torrent_state, struct peer *peer);*/

#endif
