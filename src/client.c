#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "client.h"
#include "peerfunc.h"
#include "common.h"


int main(int argc, char *argv[]) {
    struct beerTorrent *torrent ;
    struct proto_tracker_peerlistentry *peerlist ;
    
    /* Génération de l'ID */
    srand((u_int) time(NULL)) ;
    my_id = (u_int) rand() ;
    
    if(argc != 2) {
        fprintf(stderr,"SYNTAX: %s [filename]\n",argv[0]) ;
        exit(EXIT_FAILURE) ;
    }
    torrent = addtorrent(argv[1]);
    assert(torrent);
    
    peerlist = gettrackerinfos(torrent, my_id, my_port);
    
    peerlist++;
    
    return 0 ;
}
