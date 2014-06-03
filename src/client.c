#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "client.h"
#include "peerfunc.h"
#include "common.h"
#include "rename.h"

/* Renommage des fichiers et des sockets, pour pouvoir s'en servir d'indices dans des tableaux */
struct map *file_map, *socket_map ;

/* Tableau des torrent_list (et infos associées) */
struct torrent_info **torrent_list ;

void handleNewConnection (int fd, struct sockaddr_in from, int len) {
    struct proto_peer *peer;
    struct proto_client_handshake *hs ;
    u_int h,peer_id,file_id,tmp ;
    u_char version ;
    len=len;
    printf("New peer...\n");
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
    pthread_mutex_lock(&torrent_list[file_id]->peerlist->lock) ;
    torrent_list[file_id]->peerlist->pentry[torrent_list[file_id]->peerlist->nbPeers] = *peer ;
    assert(++torrent_list[file_id]->peerlist->nbPeers != 0) ;
    pthread_mutex_unlock(&torrent_list[file_id]->peerlist->lock) ;
    
    /* Réponse au handshake */
    hs = construct_handshake(torrent_list[file_id]->torrent) ;
    send_handshake(peer, hs);
    free(hs);
    printf("Peer %u added successfully.\n",peer_id) ;
}

void start_client() {
    struct sockaddr_in soc_in;
    int val;
    int ss;  /* socket d'écoute */

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

    printf("Client start accepting incomming connections ...\n");
    /* Listen */
    listen (ss, 5);

    while (1) {
        struct sockaddr_in from;
        int len;
        int f;

        /* Accept & Serve */
        len = sizeof (from);
        f = accept (ss, (struct sockaddr *)&from, (u_int*)&len);

        handleNewConnection(f, from, len);
    }
}


int main(int argc, char *argv[]) {
    unsigned int nb_files = 0, tmp,i,id_arg ;
   
    
    file_map = init_map() ; 
    socket_map = init_map() ;
    
    /* Génération de l'ID et du port */
    srand((u_int) time(NULL)) ;
    my_id = (u_int) rand() ;
    my_port = (u_short) (rand()%(65536-1024)+1024)  ;
    
    print_id();    
    
    init_cancel() ;
    
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
    
    /* Initialisation des connections (création des sockets, handshake) */
    for(i=0 ; i < nb_files ; i++)
        init_peers_connections(torrent_list[i]) ;
        
    start_client() ;
    return 0 ;
}
