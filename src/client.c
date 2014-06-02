#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "client.h"
#include "peerfunc.h"
#include "common.h"


int main(int argc, char *argv[]) {
    int i,j ;
    struct beerTorrent **torrent ;
    struct proto_tracker_peerlistentry *peerlist ;
    
    /* Génération de l'ID et du port */
    srand((u_int) time(NULL)) ;
    my_id = (u_int) rand() ;
    my_port = (u_short) (rand()%(65536-1024)+1024)  ;
    
    init_cancel() ;
    
    if(argc < 2) {
        fprintf(stderr,"SYNTAX: %s [filenames]\n",argv[0]) ;
        exit(EXIT_FAILURE) ;
    }
    torrent = (struct beerTorrent**) malloc(sizeof(struct beerTorrent*)*(argc-1)) ;
    for(i = 1 ; i < argc ; i++) {
        torrent[i-1] = addtorrent(argv[i]);
        printf("Bitfield size : %d\n",torrent[i-1]->have->totalpiece) ;
        assert(torrent[i-1]);   

    }
    peerlist = gettrackerinfos(torrent[0], my_id, my_port);
    for(i = 1 ; i < argc ; i++) {
        deletetorrent(torrent[i-1]) ; 
    }
    free(peerlist->pentry);
    free(peerlist);
    return 0 ;
}
