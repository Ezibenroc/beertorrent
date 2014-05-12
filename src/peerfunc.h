#ifndef PEERFUNC_H
#define PEERFUNC_H

#include <assert.h>
#include <stdbool.h>
#include <sys/types.h>
#include <netinet/in.h> /* struct sockaddr_in */
#include <stdio.h>

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>

#define PROTO_VERSION 2
#define PORTTRACKER 3955
#define MAXNAMELENGTH 128

/* Contient toutes les informations liées à un torrent. */
struct torrent_state {
    struct beerTorrent *torrent;
    struct peerlist *peerlist;
    u_int *requests;
};

/* Tracker communication structures */
struct proto_tracker_clientrequest
{
    u_int fileHash;
    u_int peerId;
    u_short port;
};

struct proto_tracker_trackeranswer
{
    u_char status;
    u_char nbPeers;
};

struct proto_tracker_peerentry
{
    u_int peerId;
    struct in_addr ipaddr;
    u_short port;
};

struct proto_tracker_peerlistentry
{
    u_char nbPeers;
    struct proto_tracker_peerentry * pentry;
};

/* Inter-client communication structures */
struct proto_client_handshake
{
    u_char version;
    u_int filehash;
    u_int peerId;
};

struct proto_peer {
    struct proto_tracker_peerentry *peer ;
    int sockfd ; /* socket attachée à ce client */
};

#define HANDSHAKE_SIZE 9 /* Ne pas faire confiance à sizeof pour les structures */

struct proto_client_messageheader
{
    u_int length;
    u_char id;
};

struct proto_client_requestpayload
{
    u_int index;
    u_int offset;
    u_int length;
};

/* Beertorrent struct */
struct bitfield
{
    u_char * array;
    u_int arraysize;
    u_int nbpiece;
};

struct beerTorrent
{
    u_int filelength;
    u_int filehash;
    char filename[MAXNAMELENGTH];
    u_int piecelength;
    struct in_addr trackerip;
    FILE * fp;
    pthread_mutex_t file_lock;
    struct bitfield * bf;
    bool download_ended;
};

struct bitfield * createbitfield(u_int filelength, u_int piecelength);

void deletebitfield(struct bitfield * bf);

void setbitfield(struct bitfield * dst, struct bitfield * src);

void setbitinfield(struct bitfield * bf, u_int id);

int isinbitfield(struct bitfield * bf, u_int id) __attribute__((pure));

struct beerTorrent * addtorrent(char * filename);

struct proto_tracker_peerlistentry * gettrackerinfos(struct beerTorrent * bt, u_int myId, u_short myPort);

int write_socket(int fd,const char *buf,int len);
#define assert_write_socket(fd, var, len) assert(-1 != write_socket(fd, (const char*) var, len))

int readblock(int fd, char* buffer, int len);
#define assert_read_socket(fd, var, len) assert(-1 != readblock(fd, (char*) var, len));

#endif
