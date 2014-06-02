#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "client.h"
#include "peerfunc.h"
#include "common.h"


int main(int argc, char *argv[]) {
    int i;
    unsigned int nb_files = 0 ;
    struct beerTorrent **torrent ;
    struct proto_tracker_peerlistentry **peers ;
    
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
    for(i = 1 ; i < argc ; i++) {
        torrent[i-1] = addtorrent(argv[i]);
        assert(torrent[i-1]);   
        peers[i-1] = gettrackerinfos(torrent[i-1], my_id, my_port);
        assert(peers[i-1]) ;
    }
    return 0 ;
}
