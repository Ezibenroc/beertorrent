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
    
    /* Tableau des torrent_list (et infos associées) */
    struct torrent_info **torrent_list ;
    
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
    return 0 ;
}
