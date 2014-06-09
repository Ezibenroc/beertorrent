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

/* Renvoie vrai ssi plus de requête à envoyer. */
int end_job() {
    u_int i ;
    int flag=1 ;
    for(i = 0 ; i < nb_files && flag ; i++) {
        pthread_mutex_lock(&torrent_list[i]->torrent->request_search_lock) ;
        pthread_mutex_lock(&torrent_list[i]->torrent->request_lock) ;
        flag = isfull(torrent_list[i]->torrent->request) ;
        pthread_mutex_unlock(&torrent_list[i]->torrent->request_lock) ;
        pthread_mutex_unlock(&torrent_list[i]->torrent->request_search_lock) ;
    }
    return flag ;
}

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
    assert(quit_program || q->first != q->last);
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


/* Choisis un pair et une pièce pour la prochaîne requête, pour le fichier donné. */
/* Renvois 1 en cas de succés, 0 en cas d'échec (piece_id et peer ne sont alors pas modifiés). */
/* Pré-condition : fichier non complet. */
int choose_piece_peer_for_file(u_int *piece_id, struct proto_peer **peer, struct torrent_info *bt)  {
    u_int piece = (u_int)(bt->torrent->last_downloaded_piece+1) ;
    int flag, npeer, ipeer ;
    int ntry, itry ;
    pthread_mutex_lock(&bt->torrent->request_search_lock) ;
    /* Recherche de la pièce (déterministe). */
    while(true) {
        if(piece >= bt->torrent->have->totalpiece) {/* pas de pièce à demander */
            pthread_mutex_unlock(&bt->torrent->request_search_lock) ;            
            return 0 ;
        }
        pthread_mutex_lock(&bt->torrent->request_lock) ;
        flag = isinbitfield(bt->torrent->request,piece) ;
        pthread_mutex_unlock(&bt->torrent->request_lock) ;
        pthread_mutex_lock(&bt->torrent->have_lock) ;
        flag = flag||isinbitfield(bt->torrent->have,piece) ;
        pthread_mutex_unlock(&bt->torrent->have_lock) ;
        if(!flag) /* on a trouvé la prochaine pièce */
            break ;
        piece++ ;
    }
    /* Recherche du pair (non déterministe). */
    pthread_mutex_lock(&bt->peerlist->lock) ;
    npeer = bt->peerlist->nbPeers ;
    pthread_mutex_unlock(&bt->peerlist->lock) ;
    if(npeer == 0) {
        pthread_mutex_unlock(&bt->torrent->request_search_lock) ;
        return 0 ;
    }
    ntry = npeer ; /* nombre d'essais random */
    flag = 0 ;
    for(itry = 0 ; itry < ntry ; itry++) {
        ipeer = rand()%npeer ;
        pthread_mutex_lock(&bt->peerlist->pentry[ipeer].lock) ;
        if(isinbitfield(bt->peerlist->pentry[ipeer].pieces,piece)) /* trouvé un pair */
            flag = 1 ;
        pthread_mutex_unlock(&bt->peerlist->pentry[ipeer].lock) ;
        if(flag)
            break ;
    }
    if(flag) { /* trouvé un pair */
        *piece_id = piece ;
        *peer = &bt->peerlist->pentry[ipeer] ;
        setbitinfield(bt->torrent->request,piece) ;
        pthread_mutex_unlock(&bt->torrent->request_search_lock) ;
        return 1 ;
    }
    /* Recherche du pair (non déterministe) */
    flag = 0 ;
    for(ipeer = 0 ; ipeer < npeer ; ipeer++) {
        pthread_mutex_lock(&bt->peerlist->pentry[ipeer].lock) ;
        if(isinbitfield(bt->peerlist->pentry[ipeer].pieces,piece)) /* trouvé un pair */
            flag = 1 ;
        pthread_mutex_unlock(&bt->peerlist->pentry[ipeer].lock) ;
        if(flag)
            break ;    
    }
    if(flag) { /* trouvé un pair */
        *piece_id = piece ;
        *peer = &bt->peerlist->pentry[ipeer] ;
        setbitinfield(bt->torrent->request,piece) ;
        pthread_mutex_unlock(&bt->torrent->request_search_lock) ;
        return 1 ;
    }
    else {/* pas de pair */
        pthread_mutex_unlock(&bt->torrent->request_search_lock) ;
        return 0 ;
    }
}

/* Choisis un fichier, puis une pièce et un pair, pour la prochaine requête. */
/* Endore le thread si rien n'est trouvé. */
/* Pré-condition : il existe un fichier incomplet. */
int choose_piece_peer(u_int *piece_id, struct proto_peer **peer, struct beerTorrent **torrent, int thread_id, int blocking) {
    int itry, ntry = (int)nb_files ;
    u_int file_id ;
    u_int time_sleep = 1 ;
    while(1) {
        for(itry = 0 ; itry < ntry ; itry++) {
            file_id = (u_int)rand()%nb_files ;
            if(!torrent_list[file_id]->torrent->download_ended && choose_piece_peer_for_file(piece_id,peer,torrent_list[file_id])) {
                *torrent = torrent_list[file_id]->torrent ;
                break ;
            }
        }
        if(itry < ntry) /* trouvé ! */
            return 1 ;
        else { /* Non trouvé, on fait une recherche exhaustive */
            for(file_id = 0 ; file_id < nb_files ; file_id++) {
                if(!torrent_list[file_id]->torrent->download_ended && choose_piece_peer_for_file(piece_id,peer,torrent_list[file_id])) {
                    *torrent = torrent_list[file_id]->torrent ;
                    break ;
                }            
            }
            if(file_id < nb_files) /* trouvé */
                return 1 ;
            else if(!blocking)
                return 0 ;
            else {
                if(time_sleep >= 8)
                    return 0 ;
                pthread_mutex_lock(&print_lock) ;
                green();
                printf("[#%d thread]\t",thread_id);
                normal() ;
                printf("Did not find a piece to request. Sleep for %u second(s).\n",time_sleep);
                pthread_mutex_unlock(&print_lock) ;            
                sleep(time_sleep) ;
                time_sleep = min(60,time_sleep*2) ; /* sleep exponentiel jusqu'à la borne */
            }
        }
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
            peers[id_sock] = &ti->peerlist->pentry[i] ;
            pthread_mutex_lock(&ti->peerlist->pentry[i].lock) ;
            ti->peerlist->pentry[i].pieces = createbitfield(ti->torrent->filelength,ti->torrent->piecelength) ; /* bitfield initialisé à 00...0 */
            pthread_mutex_unlock(&ti->peerlist->pentry[i].lock) ;
            if(ti->torrent->have->nbpiece >0) /* pas la peine d'envoyer le bitfield si on n'a pas de pieces */
                send_bitfield(&ti->peerlist->pentry[i],ti->torrent,-2) ;
            i++ ;
        }
    }
    free(hs);
}
/*
void send_first_requests() {
    u_int new_piece ;
    struct proto_peer *new_peer ;
    struct beerTorrent *new_torrent ; 
    u_int max_request,i ;  
   //  Calcul du nombre de requêtes. 
    max_request = 0 ;
    for(i = 0 ; i < nb_files ; i++) {
        if(!torrent_list[i]->torrent->download_ended)
            max_request += torrent_list[i]->torrent->filelength/torrent_list[i]->torrent->piecelength + (torrent_list[i]->torrent->filelength%torrent_list[i]->torrent->piecelength!=0) ;
        if(max_request >= N_REQUESTS) {
            max_request = N_REQUESTS ;
            break ;
        }
    }
    for(i = 0 ; i < max_request ; i++) {
        choose_piece_peer(&new_piece, &new_peer, &new_torrent, -1) ;
        if(new_piece <= new_torrent->filelength/new_torrent->piecelength)//  pièce entière 
            send_request(new_peer, new_torrent, new_piece, 0, new_torrent->piecelength, -1) ; 
        else if(new_torrent->filelength%new_torrent->piecelength!=0)
            send_request(new_peer, new_torrent, new_piece, 0, new_torrent->filelength%new_torrent->piecelength, -1) ;
    }
}*/


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
    while(!quit_program) {
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
    pthread_mutex_lock(&print_lock) ;
    green();
    printf("[Watching thread]\t");
    normal();
    printf("will quit.\n") ;
    pthread_mutex_unlock(&print_lock) ;
    for(i = 0 ; i < N_THREAD ; i++) {
        if(sem_post(&request->full) < 0) { /* signal sur la semaphore */
            perror("sem_post");
            exit(errno) ;
        }
    }
    free(socket_set);
    pthread_exit(useless);
}

/* Boucle : défile une socket à traiter, lis son message et exécute les actions appropriées. */
/* Fonction exécutée par un thread. */
void *treat_sockets(void* ptr) {
    int thread_id = *(int*)ptr;
    int sockfd, message_length, file_hash, peer_id ;
    int i,tmp;
    char message_id ;
    u_int s_name, file_name ;
    
    /* Buffer d'entrées sortie alloué une seule fois. */
    char *io_buff = (char*) malloc(max_piecelength*sizeof(char)) ;
    
    while(1) {
        sockfd = pop(request) ;
        if(quit_program)
            break ;
        s_name = get_name(socket_map,(u_int)sockfd) ;
        file_hash = (int)socket_to_file[s_name] ;
        peer_id = (int)socket_to_peer[s_name] ;
        file_name = get_name(file_map,(u_int)file_hash) ;

        if(-1==read_socket(sockfd,(char*)&message_length,sizeof(int))) { /* pair déconnecté, on le supprime */
            pthread_mutex_lock(&torrent_list[file_name]->peerlist->lock);
            for(i = 0 ; i < torrent_list[file_name]->peerlist->nbPeers ; i++)
                if(torrent_list[file_name]->peerlist->pentry[i].peerId == (u_int)peer_id)
                    break ;
            assert(i < torrent_list[file_name]->peerlist->nbPeers);
            tmp=i;
            deletepeer(&torrent_list[file_name]->peerlist->pentry[i]);
            printf("Peer %d does not respond anymore. Deleted from list.\n",peer_id);
            for(i=tmp;i< torrent_list[file_name]->peerlist->nbPeers-1 ; i++) {
                   torrent_list[file_name]->peerlist->pentry[i] = torrent_list[file_name]->peerlist->pentry[i+1] ;         
            }
            torrent_list[file_name]->peerlist->nbPeers--;
            pthread_mutex_unlock(&torrent_list[file_name]->peerlist->lock);
            continue ;
        }
        
        assert_read_socket(sockfd,&message_id,sizeof(char)) ;
        switch(message_id) {
            case KEEP_ALIVE :
                pthread_mutex_lock(&print_lock) ;
                blue();
                printf("Received KEEP_ALIVE from peer %d (file %s)\n",peer_id,torrent_list[file_name]->torrent->filename);
                normal() ;
                printf("\t\t\t\t\t\tNOT YET IMPLEMENTED.\n");
                pthread_mutex_unlock(&print_lock) ;
            break ;
            case HAVE:
                read_have(peers[s_name],torrent_list[file_name]->torrent,thread_id);
            break ;
            case BIT_FIELD:
                read_bitfield(peers[s_name],torrent_list[file_name]->torrent,message_length,thread_id);
            break ;
            case REQUEST:
                read_request(peers[s_name],torrent_list[file_name]->torrent,io_buff,thread_id);
            break ;
            case PIECE:
                read_piece(peers[s_name],torrent_list[file_name]->torrent,torrent_list[file_name]->peerlist,message_length,io_buff,thread_id) ;
            break ;
            case CANCEL:
                pthread_mutex_lock(&print_lock) ;
                blue();
                printf("Received CANCEL from peer %d (file %s)\n",peer_id,torrent_list[file_name]->torrent->filename);
                normal() ;
                printf("\t\t\t\t\t\tNOT YET IMPLEMENTED.\n");
                assert_read_socket(sockfd,&tmp,sizeof(u_int)) ;
                assert_read_socket(sockfd,&tmp,sizeof(u_int)) ;
                assert_read_socket(sockfd,&tmp,sizeof(u_int)) ;
                pthread_mutex_unlock(&print_lock) ;
            break ;
            default :
                blue();
                printf("Error, unknown message (id %d).\n",(int)message_id) ;
                normal() ;
                printf("\n");
                exit(EXIT_FAILURE) ;
            break;
        }
        pthread_mutex_lock(&handled_request->lock) ;
        handled_request->queue[handled_request->last] = sockfd ;
        handled_request->last = (handled_request->last+1)%N_SOCK ;
        pthread_mutex_unlock(&handled_request->lock) ;
    }
    free(io_buff);
    pthread_mutex_lock(&print_lock) ;
    green();
    printf("[#%d thread]\t\t",thread_id);
    normal();
    printf("will quit.\n") ;
    pthread_mutex_unlock(&print_lock) ;    
    pthread_exit(ptr);
}
