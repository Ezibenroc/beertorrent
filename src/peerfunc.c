#include "peerfunc.h"
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>  /* prototypes pour les fonctions dans inet(3N) */
#include <netdb.h>      /* struct hostent */
#include <sys/socket.h> /* constantes, familles... */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/stat.h>
#include <errno.h>

#include "common.h"

/* Renvoie la liste de pairs associés à un torrent (requête au tracker). */
struct proto_tracker_peerlist * gettrackerinfos(struct beerTorrent * bt, u_int myId, u_short myPort)
{
    struct proto_tracker_trackeranswer ans;
    int trackersock;
    struct hostent *sp;
    struct sockaddr_in sins;
    /*
    struct in_addr **addr_list;
    int i;
    */
    struct proto_tracker_clientrequest req;
    struct proto_tracker_peerlist * peerList;
    u_char j, peer_ind;


    printf("Getting tracker infos for %s\n", bt->filename);

    /*~ inet_pton(AF_INET, "127.0.0.1", &ipv4addr);*/
    sp = gethostbyaddr(&(bt->trackerip), sizeof(bt->trackerip), AF_INET);

    if (sp == NULL) {
        perror("gethostbyaddr");
        exit(errno);
    }

    trackersock = socket (PF_INET, SOCK_STREAM, 0);
    if (trackersock == -1) {
        perror ("socket");
        exit(errno);
    }

    sins.sin_family = AF_INET;
    memcpy (&sins.sin_addr, sp->h_addr_list[0], (size_t)sp->h_length);
    sins.sin_port = htons (PORTTRACKER);

    /* print information about this host:

    printf("Official name is: %s\n", sp->h_name);
    printf("    IP addresses: ");
    addr_list = (struct in_addr **)sp->h_addr_list;
    for(i = 0; addr_list[i] != NULL; i++) {
        printf("%s ", inet_ntoa(*addr_list[i]));
    }

    printf("\n");*/

    if (connect (trackersock, (struct sockaddr *)&sins, sizeof(sins)) == -1) {
        perror ("connect");
        exit(errno);
    }

    /* Envoit de la requête au tracker. */
    req.fileHash = bt->filehash;
    req.peerId = myId;
    req.port = myPort;
    write(trackersock, &req.fileHash, sizeof(req.fileHash));
    write(trackersock, &req.peerId, sizeof(req.peerId));
    write(trackersock, &req.port, sizeof(req.port));

    /* Obtention de la réponse du tracker. */
    read(trackersock, &ans.status, sizeof(ans.status));
    read(trackersock, &ans.nbPeers, sizeof(ans.nbPeers));
    if(ans.status == 1) {
        printf("Error when retrieving peers info\n");
        return NULL;
    }

    peerList = malloc(sizeof(struct proto_tracker_peerlist));
    assert(peerList);
    peerList->nbPeers = ans.nbPeers;
    pthread_mutex_init(&peerList->lock, NULL);

    /* De nouveaux pairs peuvent être ajoutés après l'obtention des pairs, on alloue donc pour le nombre max. */
    peerList->pentry = malloc(sizeof(struct proto_peer) * MAX_PEERS);
    assert(peerList->pentry);

    /* Obtention des pairs. */
    peer_ind = 0 ;
    for(j=0; j<peerList->nbPeers; j++) {

        read(trackersock, &(peerList->pentry[peer_ind].peerId), sizeof(peerList->pentry[j].peerId));
        read(trackersock, &(peerList->pentry[peer_ind].ipaddr), sizeof(peerList->pentry[j].ipaddr));
        read(trackersock, &(peerList->pentry[peer_ind].port), sizeof(peerList->pentry[j].port));
        peerList->pentry[peer_ind].sockfd = -1 ;
        
        if(peerList->pentry[peer_ind].peerId == my_id) /* c'est moi ! pas la peine de m'ajouter à la liste des pairs */
            continue ;
        
        peer_ind ++ ;  
    }
    
    peerList->nbPeers = peer_ind ;
    printf("NbPeers: %hu\n", peerList->nbPeers);  
    for(j=0; j<peerList->nbPeers; j++) {
        printf("* peerId: %u\n", (peerList->pentry[j]).peerId);
        printf("* IPaddr: %s\n", inet_ntoa((peerList->pentry[j]).ipaddr));
        printf("* port: %hu\n", (peerList->pentry[j]).port);
    }
    
    close(trackersock);
    return peerList;
}

/* Supprime le pair. */
void delete_peer(struct proto_peer *p) {
    close(p->sockfd) ;
    free(p);
}


/* Initialisation d'un champ de bits. */
struct bitfield * createbitfield(u_int filelength, u_int piecelength) {
    struct bitfield * bf = malloc(sizeof(struct bitfield));
    assert(bf);

    bf->totalpiece = (filelength-1) / piecelength + 1;
    bf->nbpiece = 0;
    bf->arraysize = (bf->totalpiece-1) / 8 + 1;
    bf->array = malloc(sizeof(u_char) * bf->arraysize);
    assert(bf->array);
    memset(bf->array, 0, bf->arraysize);

    return bf;
}

/* Suppression d'un champ de bits. */
void deletebitfield(struct bitfield * bf) {
    free(bf->array);
    free(bf);
}

/* Copie d'un champ. */
void setbitfield(struct bitfield * dst, struct bitfield * src) {
    u_int i = 0 ;
    memcpy(dst->array, src->array, dst->arraysize);
    dst->nbpiece = 0 ;
    for(i = 0 ; i < src->totalpiece ; i++) {
        dst->nbpiece += (u_int)isinbitfield(src,i) ;
    }
    assert(dst->nbpiece == src->nbpiece);
}

/* Met le bit d'indice donné à 1 dans le champ. */
int isinbitfield(struct bitfield * bf, u_int id) {
    return !!(bf->array[id/8] & (0x1 << (id%8)));
}

/* Renvoie vrai ssi le bit d'indice donné est dans le champ. */
void setbitinfield(struct bitfield * bf, u_int id) {
    bf->nbpiece += (u_int)!isinbitfield(bf,id) ;
    bf->array[id/8] |= (u_char)(0x1 << (id%8));
}

/* Initialise un torrent à partir du fichier beertorrent donné. */
struct beerTorrent * addtorrent(char * filename) {

    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    size_t ln;
    
    struct beerTorrent * bt = malloc(sizeof(struct beerTorrent));

    assert(bt);

    fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("Cannot open %s\n", filename);
        return NULL;
    }
    pthread_mutex_init(&bt->file_lock, NULL);
    pthread_mutex_init(&bt->have_lock, NULL);
    pthread_mutex_init(&bt->request_lock, NULL);

    /* Format :
     * length (Taille du fichier en octet)
     * hashsum (Unsigned long correspondant à la signature du fichier)
     * name (Nom du fichier)
     * piece length (Taille de chacun des morceaux à télécharger)
     * announce (IP du Tracker)
     * */
    getline(&line, &len, fp);
    bt->filelength = (u_int) atol(line); /*skip*/

    getline(&line, &len, fp);
    bt->filehash = (u_int) atol(line);

    getline(&line, &len, fp);
    ln = strlen(line) - 1;
    if (line[ln] == '\n')
        line[ln] = '\0';
    strcpy(bt->filename, line);

    getline(&line, &len, fp);
    bt->piecelength = (u_int) atol(line);

    getline(&line, &len, fp);
    ln = strlen(line) - 1;
    if (line[ln] == '\n')
        line[ln] = '\0';
    inet_pton(AF_INET, line, &(bt->trackerip));

    free(line);

    printf("Opening %s\n", bt->filename);

    bt->have = createbitfield(bt->filelength, bt->piecelength);
    bt->request = createbitfield(bt->filelength, bt->piecelength);

    if( access( bt->filename, F_OK ) != -1 ) {
        /*file exists - supposely complete*/

        if(!(bt->fp = fopen(bt->filename, "rb"))) {

            perror("fopen file");
            deletebitfield(bt->have);
            deletebitfield(bt->request);
            free(bt);
            return NULL;
        }

        /* set full bitfield*/
        memset(bt->have->array, 255, bt->have->arraysize);

        printf("%s already downloaded.\n", bt->filename);
        bt->download_ended = true;

    }
    else
    {
        /* file doesn't exist*/
        if(!(bt->fp = fopen(bt->filename, "w+b"))) {

            perror("fopen file");
            deletebitfield(bt->have);
            deletebitfield(bt->request);
            free(bt);
            return NULL;
        }
        printf("%s has to be downloaded.\n", bt->filename);
        bt->download_ended = false;
    }

    printf("%s added\n", filename);

    return bt;
}

/* Suppression d'un torrent. */
void deletetorrent(struct beerTorrent *t) {
    fclose(t->fp);
    assert(pthread_mutex_destroy(&t->file_lock)==0);
    deletebitfield(t->have);         
    assert(pthread_mutex_destroy(&t->have_lock)==0);
    deletebitfield(t->request);         
    assert(pthread_mutex_destroy(&t->request_lock)==0);
    
    free(t) ;
}

/* Écriture simplifiée et vérifiée dans une socket. */
int write_socket(int fd,const char *buf,int len) {
    int currentsize=0;
    while(currentsize<len) {
        int count=(int)write(fd,buf+currentsize,(size_t)(len-currentsize));
        if(count<0) return -1;
        currentsize+=count;
    }
    return currentsize;
}

/* Lecture simplifiée et vérifiée dans une socket. */
int readblock(int fd, char* buffer, int len) {
    int ret  = 0;
    int count = 0;
    while (count < len) {
        ret = (int)read(fd, buffer + count, (size_t)(len - count));
        if (ret <= 0) {
            return (-1);
        }
        count += ret;
    }
    return count;
}
