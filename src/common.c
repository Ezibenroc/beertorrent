#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "common.h"
#include "peerfunc.h"
#include "rename.h"

#define size_id 22
/* Fonction d'affichage (ID et port). */
void print_id() {
    char s[size_id] ;
    int pos =0;
    u_int tmp ;
    green();
    memset(s,'#',(size_id-1)*sizeof(char)) ;
    s[size_id-1]='\0' ;
    printf("%s\n",s);
    
    memset(s,' ',(size_id-1)*sizeof(char)) ;
    s[0] = '#' ; s[size_id-2] = '#' ;
    s[2] = 'I' ; s[3] = 'D' ;  s[7] = '=' ;
    pos = size_id-4 ;
    tmp = my_id ;
    if(tmp==0) s[pos] = '0' ;
    while(tmp != 0) {
        s[pos--] = (char) (tmp%10+'0') ;
        tmp=tmp/10 ;
    }
    printf("%s\n",s);
    
    memset(s,' ',(size_id-1)*sizeof(char)) ;
    s[0] = '#' ; s[size_id-2] = '#' ;
    s[2] = 'P' ; s[3] = 'O' ; s[4] = 'R' ; s[5] = 'T' ; s[7] = '=' ;
    pos = size_id-4 ;
    tmp = my_port ;
    if(tmp==0) s[pos] = '0' ;
    while(tmp != 0) {
        s[pos--] = (char) (tmp%10+'0') ;
        tmp=tmp/10 ;
    }
    printf("%s\n",s);
    
    memset(s,35,(size_id-1)*sizeof(char)) ;
    printf("%s",s);
    normal();
    printf("\n");
}

/* Hachage d'une chaîne de caractère. */
u_int hash(u_char *str)
{
    u_int hash_ = 0;
    int c;

    while ((c = (int)(*str++)))
        hash_ += (u_int)c;

    return hash_;
}

/* Hachage d'un fichier (donné par son descripteur). */
u_int file2hash(int fd)
{

    int bytes;
    u_char data[1024];
    u_int h = 0;

    while ((bytes = (int)read(fd, data, 1024)) != 0)
        h += hash(data);

    return h;
}

/* Retourne un pointeur vers le début de l'extension du fichier. */
char *get_filename_ext(char *filename)
{
    char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return NULL;
    return dot + 1;
}

struct waiting_queue *init_waiting_queue() {
    struct waiting_queue *q ;
    q = (struct waiting_queue*) malloc(sizeof(struct waiting_queue)) ;
    q->first = N_SOCK-1 ;
    q->last = 0 ;
    pthread_mutex_init(&(q->lock), NULL);
    sem_init(&(q->full),0,0) ; /* initialement aucun élément */
    return q ;
}
struct non_waiting_queue *init_non_waiting_queue() {
    struct non_waiting_queue *q ;
    q = (struct non_waiting_queue*) malloc(sizeof(struct non_waiting_queue)) ;
    q->first = N_SOCK-1 ;
    q->last = 0 ;
    pthread_mutex_init(&(q->lock), NULL);
    return q ;
}

/* Retourne la tête de la file (appel bloquant si file vide). */
int pop(struct waiting_queue *q) {
    int tmp ;
    sem_wait(&(q->full)) ;
    pthread_mutex_lock(&q->lock) ;
    tmp = q->queue[q->first] ;
    q->first = (q->first+1)%N_SOCK ;
    pthread_mutex_unlock(&q->lock) ;  
    return tmp ;  
}

/* Initialisation du tableau cancel. */
void init_cancel() {
    int i ;
    for(i = 0 ; i < N_SOCK ; i++) {
        memset(cancel[i].msg,0,N_CANCEL*2*sizeof(u_int)) ;
        cancel[i].ind = 0 ;
        pthread_mutex_init(&(cancel[i].lock), NULL);
    }
}

/* Construit un handshake correspondant au torrent donné. */
struct proto_client_handshake* construct_handshake(struct beerTorrent *torrent) {
    struct proto_client_handshake* hs ;
    hs = (struct proto_client_handshake*) malloc(sizeof(struct proto_client_handshake)) ;
    assert(hs);
    hs->version = 2 ;
    hs->filehash = torrent->filehash ;
    hs->peerId = my_id ;
    return hs ;
}

/* Envoie le handshake au pair donné. */
void send_handshake(const struct proto_peer *peer, const struct proto_client_handshake *hs) {
    cyan() ;
    printf("Send handshake to %d\n",peer->peerId) ;
    normal() ;
    write(peer->sockfd,&hs->version,sizeof(hs->version)) ;
    write(peer->sockfd,&hs->filehash,sizeof(hs->filehash)) ;
    write(peer->sockfd,&hs->peerId,sizeof(hs->peerId)) ;

}

/* Reçois un handshake du pair donné, et vérifie qu'il est cohérent avec le handshake envoyé. */
void receive_handshake(const struct proto_peer *peer, const struct proto_client_handshake *hs) {
    u_char c ; u_int i ;
    blue();
    printf("Receive handshake from %d\n",peer->peerId) ;
    normal() ;
    read(peer->sockfd, &c, sizeof(u_char));
    assert(c==hs->version) ;
    read(peer->sockfd, &i, sizeof(u_int));
    assert(i==hs->filehash) ;
    read(peer->sockfd, &i, sizeof(u_int));
    assert(i==peer->peerId) ;
}

/* Initialise la connection pour un pair, avec le handshake donné. */
/* Créé une socket et réalise le handshake. */
/* Renvoie 1 en cas de succés, 0 en cas d'échec. */
int init_peer_connection(struct proto_peer *peer, const struct proto_client_handshake *hs) {
    int fd ;
    struct hostent *sp;
    struct sockaddr_in sins;
    
    sp = gethostbyaddr(&(peer->ipaddr), sizeof(peer->ipaddr), AF_INET);
    if (sp == NULL) {
        perror("gethostbyaddr");
        exit(errno);
    }
    
    fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror ("socket");
        exit(errno);
    }    
    
    sins.sin_family = AF_INET;
    memcpy (&sins.sin_addr, sp->h_addr_list[0], (size_t)sp->h_length);
    sins.sin_port = htons(peer->port);
    
    if (connect (fd, (struct sockaddr *)&sins, sizeof(sins)) == -1) {
        switch(errno) {
            case ECONNREFUSED : 
                printf("Connection to peer %d failed. Peer removed from list.\n",peer->peerId); return 0 ;
            break ;
            default : perror ("connect");
        }
        exit (errno);
    }
    peer->sockfd = fd ;
    send_handshake(peer,hs) ;
    receive_handshake(peer,hs) ; 
    return 1 ;
}

/* Initialise la connection de tous les pairs (création des sockets, réalisation des handshakes, envoi des bitfields). */
/* Remplis la table de sockets, et le tableau faisant la correspondance entre sockets et fichiers. */
void init_peers_connections(struct torrent_info *ti) {
    /* Construction du handshake, identique pour tous. */
    struct proto_client_handshake* hs = construct_handshake(ti->torrent) ;
    
    int i,j ;
    u_int id_sock ;
    /* Parcours des pairs */
    i=0 ;
    while(i < ti->peerlist->nbPeers) {
        if(!init_peer_connection(&ti->peerlist->pentry[i],hs)) { /* connection échouée, on supprime le pair */
            for(j = i ; j < ti->peerlist->nbPeers-1 ; j++) {
                ti->peerlist->pentry[j] = ti->peerlist->pentry[j+1] ;
            }
            ti->peerlist->nbPeers-- ;
        }
        else {
            id_sock = get_name(socket_map,(u_int)ti->peerlist->pentry[i].sockfd) ;
            socket_to_file[id_sock] = ti->torrent->filehash ;
            peer_bitfield[id_sock] = createbitfield(ti->torrent->filelength,ti->torrent->piecelength) ; /* bitfield initialisé à 00...0 */
            send_bitfield(ti->torrent,&ti->peerlist->pentry[i]) ;
            i++ ;
        }
    }
    free(hs);
}


/* Envoie le champ de bit au pair donné. */
void send_bitfield(struct beerTorrent *torrent, struct proto_peer *peer) {
    size_t size = (size_t) 1+torrent->have->arraysize+3*sizeof(u_int) ;
    char m_id = 2 ;
    cyan() ;
    printf("Send bitfield to %d\n",peer->peerId) ;
    normal() ;
    /* Length */
    assert_write_socket(peer->sockfd,&size,sizeof(int)) ;
    /* Message id */
    assert_write_socket(peer->sockfd,&m_id,sizeof(char)) ;
    /* Champs */
    assert_write_socket(peer->sockfd,torrent->have->array,(int)torrent->have->arraysize) ;
    assert_write_socket(peer->sockfd,&(torrent->have->arraysize),sizeof(u_int)) ;
    assert_write_socket(peer->sockfd,&(torrent->have->totalpiece),sizeof(u_int)) ;
    /* Pour se conformer à la specification donnée, on n'envoie pas le champ "nbpiece".
       Le récepteur aura à le recalculer. */
}
