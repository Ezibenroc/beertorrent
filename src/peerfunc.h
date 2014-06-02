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
#include <stdlib.h>
/*#define malloc(x) malloc(x) ; fprintf(stderr,"\t\t\t\t\t\t\t\t\t\tMalloc : %lu\n",x) */

#define MAX_PEERS 256

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
    u_int totalpiece;
    u_int nbpiece;
};

struct beerTorrent
{
    u_int filelength;               /* longueur du fichier */
    u_int filehash;                 /* hash du fichier */   
    char filename[MAXNAMELENGTH];   /* nom du fichier */
    u_int piecelength;              /* longueur des pièces du fichier */
    struct in_addr trackerip;       /* adresse IP du tracker associé */
    FILE * fp;                      /* fichier associé */
    pthread_mutex_t file_lock;      /* mutex pour R/W sur le fichier */
    struct bitfield * have;         /* champ de bits représentant les pièces possédées */
    pthread_mutex_t have_lock;      /* mutex pour R/W sur ce champ */
    struct bitfield * request;      /* champ de bits représentant les pièces pour lesquels on a émis une requête */
    pthread_mutex_t request_lock;   /* mutex pour R/W sur ce champ */
    bool download_ended;            /* téléchargement terminé ou non */
};

struct bitfield * createbitfield(u_int filelength, u_int piecelength);

void deletebitfield(struct bitfield * bf);

void setbitfield(struct bitfield * dst, struct bitfield * src);

void setbitinfield(struct bitfield * bf, u_int id);

int isinbitfield(struct bitfield * bf, u_int id) __attribute__((pure));

struct beerTorrent * addtorrent(char * filename);

void deletetorrent(struct beerTorrent *t) ;

struct proto_tracker_peerlistentry * gettrackerinfos(struct beerTorrent * bt, u_int myId, u_short myPort);

int write_socket(int fd,const char *buf,int len);
#define assert_write_socket(fd, var, len) assert(-1 != write_socket(fd, (const char*) var, len))

int readblock(int fd, char* buffer, int len);
#define assert_read_socket(fd, var, len) assert(-1 != readblock(fd, (char*) var, len));

#endif
