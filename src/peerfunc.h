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

/* Taille maximale d'une pièce (tout fichiers confondus). */
size_t max_piecelength ;

/* Requête pour le tracker. */
struct proto_tracker_clientrequest {
    u_int fileHash;
    u_int peerId;
    u_short port;
};

/* Réponse du tracker. */
struct proto_tracker_trackeranswer {
    u_char status;
    u_char nbPeers;
};


/* Handshake. */
struct proto_client_handshake {
    u_char version;
    u_int filehash;
    u_int peerId;
};

/* Identité d'un pair. */
struct proto_peer {
    u_int peerId;
    struct in_addr ipaddr;
    u_short port;
    struct bitfield *pieces;
    pthread_mutex_t lock ;
    int sockfd ; /* socket attachée à ce client */
};

/* Supprime le pair. */
void deletepeer(struct proto_peer *p) ;


/* Tableau de pairs (protégé par mutex). */
struct proto_tracker_peerlist {
    u_char nbPeers;
    struct proto_peer * pentry;
    pthread_mutex_t lock;      
};

/* Header d'un message. */
struct proto_client_messageheader {
    u_int length;
    u_char id;
};

/* Payload pour une requête. */
struct proto_client_requestpayload {
    u_int index;
    u_int offset;
    u_int length;
};

/* Champ de bits.  */
struct bitfield {
    u_char * array;
    u_int arraysize;
    u_int totalpiece;
    u_int nbpiece;
};

/* Supprime la liste. */
void deletepeerlist(struct proto_tracker_peerlist *l) ;

/* Torrent. */
struct beerTorrent {
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
    pthread_mutex_t request_search_lock ; /* mutex pour la recherche de nouvelle requête à faire */
    bool download_ended;            /* téléchargement terminé ou non */
    int last_downloaded_piece ;     /* indice de la dernière pièce téléchargée (téléchargées dans l'ordre croissant) */
};

/* Torrent et ses pairs. */
struct torrent_info {
    int nb_files ;
    struct beerTorrent *torrent;
    struct proto_tracker_peerlist *peerlist;
};

/* Supprime torrent_info */
void deletetorrentinfo(struct torrent_info *bt);

/* Initialisation d'un champ de bits. */
struct bitfield * createbitfield(u_int filelength, u_int piecelength);

/* Suppression d'un champ de bits. */
void deletebitfield(struct bitfield * bf);

/* Copie d'un champ. */
void setbitfield(struct bitfield * dst, struct bitfield * src);

/* Met le bit d'indice donné à 1 dans le champ. */
void setbitinfield(struct bitfield * bf, u_int id);

/* Renvoie vrai ssi le bit d'indice donné est dans le champ. */
int isinbitfield(struct bitfield * bf, u_int id) __attribute__((pure));

/* Renvoie vrai ssi toutes les pièces sont à 1. */
int isfull(struct bitfield *bf) ;

/* Initialise un torrent à partir du fichier beertorrent donné. */
struct beerTorrent * addtorrent(char * filename);

/* Suppression d'un torrent. */
void deletetorrent(struct beerTorrent *t) ;

/* Renvoie la liste de pairs associés à un torrent (requête au tracker). */
struct proto_tracker_peerlist * gettrackerinfos(struct beerTorrent * bt, u_int myId, u_short myPort);

/* Écriture simplifiée et vérifiée dans une socket. */
int write_socket(int fd,const char *buf,int len);
#define assert_write_socket(fd, var, len) assert(-1 != write_socket(fd, (const char*) var, len))

/* Lecture simplifiée et vérifiée dans une socket. */
int read_socket(int fd, char* buffer, int len);
#define assert_read_socket(fd, var, len) assert(-1 != read_socket(fd, (char*) var, len));


/* Envoie du message bitfield au pair donné. */
void send_bitfield(struct proto_peer *peer, struct beerTorrent *torrent, int thread_id);

/* Reception du message bitfield du pair donné. */
void read_bitfield(struct proto_peer *peer, struct beerTorrent *torrent, int length, int thread_id);

/* Envoie du message have au pair donné. */
void send_have(struct proto_peer *peer, struct beerTorrent *torrent, int piece_id, int thread_id);

/* Reception du message have du pair donné. */
void read_have(struct proto_peer *peer, struct beerTorrent *torrent, int thread_id);

/* Envoie du message request au pair donné. */
void send_request(struct proto_peer *peer, struct beerTorrent *torrent, u_int piece_id, u_int block_offset, u_int block_length, int thread_id);

/* Reception du message request du pair donné. */
void read_request(struct proto_peer *peer, struct beerTorrent *torrent, char *buff, int thread_id);

/* Envoie du message piece au pair donné. */
void send_piece(struct proto_peer *peer, struct beerTorrent *torrent, u_int piece_id, u_int block_offset, u_int block_length, char *buff, int thread_id);

/* Reception du message piece du pair donné. */
void read_piece(struct proto_peer *peer, struct beerTorrent *torrent, struct proto_tracker_peerlist *peerlist, int length, char *buff, int thread_id);

#endif
