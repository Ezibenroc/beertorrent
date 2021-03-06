#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>

#include "peerfunc.h"
#include "common.h"
#include "rename.h"

/* socket d'écoute (variable globale, afin de pouvoir stoper proprement le thread d'écoute à la fin du programme) */
int ss; 

/* Fonction appelée lorsque l'on reçoit un handshake d'un nouveau pair. */
void handleNewConnection (int fd, struct sockaddr_in from, int len) {
    struct proto_peer *peer;
    struct proto_client_handshake *hs ;
    u_int h,peer_id,file_id,tmp,sock_id ;
    u_char version ;
    len=len;
    read(fd, &version, sizeof(u_char));
    assert(version==PROTO_VERSION) ;
    read(fd, &h, sizeof(u_int));
    tmp = get_map_size(file_map) ;
    file_id = get_name(file_map,h) ;
    if(file_id == tmp) {
        fprintf(stderr,"Error, get contacted for a non-referenced file.\n");
        exit(EXIT_FAILURE) ;
    }
    read(fd, &peer_id, sizeof(u_int));
    
    /* Ajout de ce pair à la liste des pairs du fichier */
    peer = (struct proto_peer*) malloc(sizeof(struct proto_peer)) ;
    peer->peerId = peer_id ;
    peer->ipaddr = from.sin_addr ; /* remarque : pas besoin */
    peer->port = 0 ; /* on ne connais pas le port, mais on n'en a pas besoin */
    peer->sockfd = fd ;
    assert(pthread_mutex_init(&peer->lock, NULL)==0);
    peer->pieces = createbitfield(torrent_list[file_id]->torrent->filelength,torrent_list[file_id]->torrent->piecelength) ; /* bitfield initialisé à 00...0 */
    pthread_mutex_lock(&torrent_list[file_id]->peerlist->lock) ;
    torrent_list[file_id]->peerlist->pentry[torrent_list[file_id]->peerlist->nbPeers] = *peer ;
    assert(++torrent_list[file_id]->peerlist->nbPeers != 0) ;
    pthread_mutex_unlock(&torrent_list[file_id]->peerlist->lock) ;
    
    sock_id = get_name(socket_map,(u_int)fd) ;
    socket_to_file[sock_id] = torrent_list[file_id]->torrent->filehash ;
    socket_to_peer[sock_id] = peer->peerId ;
    peers[sock_id] = peer ;
    
    pthread_mutex_lock(&print_lock) ;
    green();
    printf("[Listening thread]\t");    
    blue();
    printf("Receive handshake from %d\n",peer->peerId) ;
    normal() ;
    pthread_mutex_unlock(&print_lock) ;

    /* Ajoute la socket dans la queue handled_request, afin que l'on commence à écouter par la suite. */
    pthread_mutex_lock(&(handled_request->lock)) ;
    handled_request->queue[handled_request->last] = peer->sockfd ;
    handled_request->last = (handled_request->last+1)%N_SOCK ;
    pthread_mutex_unlock(&(handled_request->lock)) ;
    
    /* Réponse au handshake */
    hs = construct_handshake(torrent_list[file_id]->torrent) ;
    send_handshake(peer, hs);
    free(hs);
    pthread_mutex_lock(&print_lock) ;
    printf("Peer %u added successfully (file %s).\n",peer_id,torrent_list[file_id]->torrent->filename) ;
    pthread_mutex_unlock(&print_lock) ;
    if(torrent_list[file_id]->torrent->have->nbpiece >0) /* pas la peine d'envoyer le bitfield si on n'a pas de pieces */
        send_bitfield(peer,torrent_list[file_id]->torrent,-1) ;
}

/* Boucle surveillant les connections entrantes, afin d'ajouter d'éventuels nouveaux pairs */
void *start_client(void *useless) {
    struct sockaddr_in soc_in;
    int val;
    struct sockaddr_in from;
    int len;
    int f,tmp;
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,&tmp) ;
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,&tmp) ;
    /* socket Internet, de type stream (fiable, bi-directionnel) */
    ss = socket (PF_INET, SOCK_STREAM, 0);

    /* Force la réutilisation de l'adresse si non allouée */
    val = 1;
    setsockopt (ss, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    /* Nomme le socket: socket inet, port PORT, adresse IP quelconque */
    soc_in.sin_family = AF_INET;
    /*~ soc_in.sin_addr.s_addr = htonl (INADDR_ANY);*/
    soc_in.sin_addr.s_addr = INADDR_ANY;
    soc_in.sin_port = htons(my_port);
    bind (ss, (struct sockaddr*)&soc_in, sizeof(soc_in));

    pthread_mutex_lock(&print_lock) ;
    printf("Client start accepting incomming connections ...\n");
    pthread_mutex_unlock(&print_lock) ;
    /* Listen */
    listen (ss, 5);

    while (!quit_program) {

        /* Accept & Serve */
        len = sizeof (from);
        f = accept (ss, (struct sockaddr *)&from, (u_int*)&len);
        if(f<0 && quit_program)
            break ;
        else if(f<0) {
            perror("accept");
            exit(errno);
        }

        handleNewConnection(f, from, len);
    }
    pthread_mutex_lock(&print_lock) ;
    green();
    printf("[Listening thread]\t");
    normal();
    printf("will quit.\n") ;
    pthread_mutex_unlock(&print_lock) ;
    pthread_exit(useless);
}

void *interact (void *useless) {
    char c ;
    quit_program = 0 ;
    while(!quit_program) {
        switch(c=(char)getchar()) {
            case 'q' :
                pthread_mutex_lock(&print_lock) ;
                green() ;
                printf("WILL QUIT.\n");
                quit_program = 1 ;
                close(ss);
                normal() ;
                pthread_mutex_unlock(&print_lock) ;
            break ;
            default :
                if(c!='\n') {
                    pthread_mutex_lock(&print_lock) ;
                    green() ;
                    printf("UNKNOWN COMMAND : %c.\n",c);
                    normal() ;
                    pthread_mutex_unlock(&print_lock) ;
                }
            break ;
        }        
    }
    pthread_mutex_lock(&print_lock) ;
    green();
    printf("[Interacting thread]\t");
    normal();
    printf("will quit.\n") ;
    pthread_mutex_unlock(&print_lock) ;
    pthread_exit(useless);
}

int main(int argc, char *argv[]) {
    unsigned int tmp,i,id_arg ;
    void *ptr ;
    pthread_t watcher, user, sender[N_THREAD], listener ;
    int thread_id[N_THREAD];
    file_map = init_map() ; 
    socket_map = init_map() ;
    
    assert(pthread_mutex_init(&print_lock, NULL)==0);
    assert(sizeof(int)==sizeof(u_int)) ; 
    
    nb_files = 0 ;
    nb_files_to_download = 0 ;
    max_piecelength = 0 ;
    
    /* Génération de l'ID et du port */
    srand((u_int) time(NULL)) ;
    my_id = (u_int) rand() ;
    my_port = (u_short) (rand()%(65536-1024)+1024)  ;
    
    print_id();    
    
    if(argc < 2) {
        fprintf(stderr,"SYNTAX: %s [filenames]\n",argv[0]) ;
        exit(EXIT_FAILURE) ;
    }
    nb_files = (unsigned int) argc-1 ;
    torrent_list = (struct torrent_info**) malloc(sizeof(struct torrent_info*)*nb_files) ;
    assert(torrent_list) ;
    i=0; id_arg=1 ;
    while(id_arg <= nb_files) {
        torrent_list[i] = (struct torrent_info *) malloc(sizeof(struct torrent_info)) ;
        assert(torrent_list[i]) ;
        torrent_list[i]->torrent = addtorrent(argv[id_arg]);
        assert(torrent_list[i]->torrent);   
        tmp = get_name(file_map,torrent_list[i]->torrent->filehash) ;
        if(tmp==i) { /* fichier pas encore référencé... sinon on a un doublon */
            torrent_list[i]->peerlist = gettrackerinfos(torrent_list[i]->torrent, my_id, my_port);
            assert(torrent_list[i]->peerlist) ;
            if(!torrent_list[i]->torrent->download_ended)
                nb_files_to_download++ ;
            i++ ;
        }
        else {
            fprintf(stderr,"File %s already referenced. Removed from list.\n",torrent_list[i]->torrent->filename);
            deletetorrent(torrent_list[i]->torrent) ;
            free(torrent_list[i]) ;
        }
        assert(tmp == i-1);
        id_arg++ ;
    }
    nb_files -= id_arg-i-1 ;
    printf("Total files referenced : %d\n",nb_files);
    
    /* Initialisation des deux files de requêtes. */
    init_queues() ;
    
    /* Initialisation du tableau répertoriant les requêtes "cancel". */
    init_cancel() ;
    
    
    /* Initialisation des connections (création des sockets, handshake) */
    for(i=0 ; i < nb_files ; i++)
        init_peers_connections(torrent_list[i]) ;
    
    /* Création des threads. */
    if((pthread_create(&user,NULL,interact,NULL))!=0) {
        perror("pthread_create");
        exit(errno);
    }
    if((pthread_create(&watcher,NULL,watch_sockets,NULL))!=0) {
        perror("pthread_create");
        exit(errno);
    }
    for(i = 0 ; i < N_THREAD ; i++) {
        thread_id[i] = (int)i ;
        if((pthread_create(&sender[i],NULL,treat_sockets,(void*)&thread_id[i]))!=0) {
            perror("pthread_create");
            exit(errno);
        }    
    }
    if((pthread_create(&listener,NULL,start_client,NULL))!=0) {
        perror("pthread_create");
        exit(errno);
    }
    pthread_join(user,&ptr);
    pthread_join(watcher,&ptr);
    for(i = 0 ; i < N_THREAD ; i++) {
        pthread_join(sender[i],&ptr);
    }
    /* Arrêt du thread d'écoute (assez sale, pas réussi à le faire autrement) */
    pthread_cancel(listener);
    pthread_mutex_lock(&print_lock) ;
    green();
    printf("[Listening thread]\t");
    normal();
    printf("will quit.\n") ;
    pthread_mutex_unlock(&print_lock) ;
    pthread_join(listener,&ptr);
    
    /* Nettoyage */
    for(i = 0 ; i < nb_files ; i++)
        deletetorrentinfo(torrent_list[i]) ;
    free(torrent_list);
    return 0 ;
}
