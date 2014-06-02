#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "client.h"
#include "peerfunc.h"
#include "common.h"
#include "rename.h"


int main(int argc, char *argv[]) {
    unsigned int nb_files = 0, tmp,i,id_arg ;
    /* Tableau des fichiers du client */
    struct beerTorrent **torrent ;
    /* Tableau des listes de pairs */
    /* peers[i] est la liste de pairs de torrent[i] */
    struct proto_tracker_peerlistentry **peers ;
    
    /* Renommage des fichiers et des sockets, pour pouvoir s'en servir d'indices dans des tableaux */
    struct map *file_map = init_map(), *socket_map = init_map() ;

    
    /* Génération de l'ID et du port */
    srand((u_int) time(NULL)) ;
    my_id = (u_int) rand() ;
    my_port = (u_short) (rand()%(65536-1024)+1024)  ;
    
    init_cancel() ;
    
    if(argc < 2) {
        fprintf(stderr,"SYNTAX: %s [filenames]\n",argv[0]) ;
        exit(EXIT_FAILURE) ;
    }
    nb_files = (unsigned int) argc-1 ;
    torrent = (struct beerTorrent**) malloc(sizeof(struct beerTorrent*)*nb_files) ;
    peers = (struct proto_tracker_peerlistentry**) malloc(sizeof(struct proto_tracker_peerlistentry*)*nb_files) ;
    i=0; id_arg=1 ;
    while(id_arg <= nb_files) {
        torrent[i] = addtorrent(argv[id_arg]);
        assert(torrent[i]);   
        tmp = get_name(file_map,torrent[i]->filehash) ;
        if(tmp==i) { /* fichier pas encore référencé... sinon on a un doublon */
            peers[i] = gettrackerinfos(torrent[i], my_id, my_port);
            assert(peers[i]) ;
            i++ ;
        }
        else {
            fprintf(stderr,"File %s already referenced. Removed from list.\n",torrent[i]->filename);
            deletetorrent(torrent[i]) ;
        }
        assert(tmp == i-1);
        id_arg++ ;
    }
    nb_files -= id_arg-i-1 ;
    printf("Total files referenced : %d\n",nb_files);
    return 0 ;
}
