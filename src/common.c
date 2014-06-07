#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <stdlib.h>
#include <poll.h>

#include "common.h"
#include "peerfunc.h"
#include "rename.h"

#define KEEP_ALIVE 0
#define HAVE 1
#define BIT_FIELD 2
#define REQUEST 3
#define PIECE 4
#define CANCEL 5

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
    q->first = 0 ;
    q->last = 0 ;
    pthread_mutex_init(&(q->lock), NULL);
    sem_init(&(q->full),0,0) ; /* initialement aucun élément */
    return q ;
}
struct non_waiting_queue *init_non_waiting_queue() {
    struct non_waiting_queue *q ;
    q = (struct non_waiting_queue*) malloc(sizeof(struct non_waiting_queue)) ;
    q->first = 0 ;
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
    pthread_mutex_lock(&peer->lock) ;
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
    pthread_mutex_unlock(&peer->lock) ;
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
            socket_to_peer[id_sock] = ti->peerlist->pentry[i].peerId ;
            peers[id_sock] = ti->peerlist->pentry ;
            pthread_mutex_lock(&ti->peerlist->pentry[i].lock) ;
            ti->peerlist->pentry[i].pieces = createbitfield(ti->torrent->filelength,ti->torrent->piecelength) ; /* bitfield initialisé à 00...0 */
            pthread_mutex_unlock(&ti->peerlist->pentry[i].lock) ;
            if(ti->torrent->have->nbpiece >0) /* pas la peine d'envoyer le bitfield si on n'a pas de pieces */
                send_bitfield(&ti->peerlist->pentry[i],ti->torrent) ;
            i++ ;
        }
    }
    free(hs);
}



/* Surveille toutes les sockets référencées. */
/* Fonction exécutée par un thread. */
void *watch_sockets(void *useless) {
    u_int i,size, i_fd ;
    int nb_readable = 0 ;
    struct pollfd *socket_set;
    int timeout = 1000;   /* timeout (1s) pour poll, afin de régulièrement remettre les sockets dans l'ensemble */
    useless=useless;
    /* Initialisation. */
    socket_set = (struct pollfd*) malloc(sizeof(struct pollfd)*N_SOCK) ;
    size = get_map_size(socket_map) ;
    for(i = 0 ; i < N_SOCK ; i++) {
        socket_set[i].events = POLLIN ;
        socket_set[i].revents = 0 ;
    }
    for(i = 0 ; i < size ; i++)
        socket_set[i].fd = (int) get_id(socket_map,i) ;
    for(i = size ; i < N_SOCK ; i++)
        socket_set[i].fd = -1 ; /* pas de socket pour l'instant */
    /* Boucle d'écoute */
    while(1) {
        if((nb_readable=poll(socket_set,size,timeout))<0) {
            perror("poll");
            exit(errno) ;
        }
        /* On réactive les sockets de handled_request */
        pthread_mutex_lock(&handled_request->lock) ;

        while(handled_request->first != handled_request->last) {
            i_fd = get_name(socket_map,(u_int)handled_request->queue[handled_request->first]) ;
            size = get_map_size(socket_map) ; /* peut être de nouvelles sockets */
            assert(socket_set[i_fd].fd < 0) ;
            socket_set[i_fd].fd = handled_request->queue[handled_request->first] ;
            handled_request->first ++ ;
        }
        pthread_mutex_unlock(&handled_request->lock) ;
        /* On place dans request toutes les sockets avec un message en attente. */
        if(nb_readable == 0) continue ;
        pthread_mutex_lock(&print_lock) ;
        green();
        printf("[Watching thread]\t");
        blue();
        printf("There is %d additional readable socket(s).\n",nb_readable) ;
        normal();
        pthread_mutex_unlock(&print_lock) ;
        pthread_mutex_lock(&request->lock) ;
        for(i = 0 ; i < size ; i++) {
            if(socket_set[i].revents != 0) { /* un événement ! */
                assert(socket_set[i].revents == POLLIN) ;
                assert(socket_set[i].fd > 0) ;
                request->queue[request->last] = socket_set[i].fd ;
                request->last = (request->last+1)%N_SOCK ;
                if(sem_post(&request->full) < 0) { /* signal sur la semaphore */
                    perror("sem_post");
                    exit(errno) ;
                }
                socket_set[i].fd = -1 ; /* desactive la surveillance sur cette socket */
            }
        }
        pthread_mutex_unlock(&request->lock) ;
    }
    return useless;
}

/* Boucle : défile une socket à traiter, lis son message et exécute les actions appropriées. */
/* Fonction exécutée par un thread. */
void *treat_sockets(void* ptr) {
    int thread_id = *(int*)ptr;
    int sockfd, message_length, file_hash, peer_id ;
    char message_id ;
    u_int s_name, file_name ;
    
    while(1) {
        sockfd = pop(request) ;
        s_name = get_name(socket_map,(u_int)sockfd) ;
        file_hash = (int)socket_to_file[s_name] ;
        peer_id = (int)socket_to_peer[s_name] ;
        file_name = get_name(file_map,(u_int)file_hash) ;
        assert_read_socket(sockfd,&message_length,sizeof(int)) ;
        assert_read_socket(sockfd,&message_id,sizeof(char)) ;
        pthread_mutex_lock(&print_lock) ;
        green();
        printf("[#%d thread]\t",thread_id);
        normal();
        switch(message_id) {
            case KEEP_ALIVE :
                blue();
                printf("Received KEEP_ALIVE from peer %d (file %s)\n",peer_id,torrent_list[file_name]->torrent->filename);
                normal() ;
                pthread_mutex_unlock(&print_lock) ;
                printf("\t\t\t\t\t\tNOT YET IMPLEMENTED.\n");
            break ;
            case HAVE:
                pthread_mutex_unlock(&print_lock) ;
                read_have(peers[s_name],torrent_list[file_name]->torrent);
            break ;
            case BIT_FIELD:
                pthread_mutex_unlock(&print_lock) ;
                read_bitfield(peers[s_name],torrent_list[file_name]->torrent,message_length);
            break ;
            case REQUEST:
                blue();
                printf("Received REQUEST from peer %d (file %s)\n",peer_id,torrent_list[file_name]->torrent->filename);
                normal() ;
                pthread_mutex_unlock(&print_lock) ;
                printf("\t\t\t\t\t\tNOT YET IMPLEMENTED.\n");
            break ;
            case PIECE:
                blue();
                printf("Received PIECE from peer %d (file %s)\n",peer_id,torrent_list[file_name]->torrent->filename);
                normal() ;
                pthread_mutex_unlock(&print_lock) ;
                printf("\t\t\t\t\t\tNOT YET IMPLEMENTED.\n");
            break ;
            case CANCEL:
                blue();
                printf("Received CANCEL from peer %d (file %s)\n",peer_id,torrent_list[file_name]->torrent->filename);
                normal() ;
                pthread_mutex_unlock(&print_lock) ;
                printf("\t\t\t\t\t\tNOT YET IMPLEMENTED.\n");
            break ;
            default :
                blue();
                printf("Error, unknown message (id %d).\n",(int)message_id) ;
                normal() ;
                printf("\n");
                exit(EXIT_FAILURE) ;
            break;
        }
    }
    
    return ptr;
}
