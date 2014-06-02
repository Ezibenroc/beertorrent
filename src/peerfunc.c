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


/* Tracker functions */
struct proto_tracker_peerlistentry * gettrackerinfos(struct beerTorrent * bt, u_int myId, u_short myPort)
{
    struct proto_tracker_trackeranswer ans;
    int trackersock;
    struct hostent *sp;
    struct sockaddr_in sins;
    struct in_addr **addr_list;
    int i;
    struct proto_tracker_clientrequest req;
    struct proto_tracker_peerlistentry * peerList;
    u_char j;


    printf("Getting tracker infos for %s\n", bt->filename);

    /*~ inet_pton(AF_INET, "127.0.0.1", &ipv4addr);*/
    sp = gethostbyaddr(&(bt->trackerip), sizeof(bt->trackerip), AF_INET);

    if (sp == NULL)
    {
        fprintf (stderr, "gethostbyaddr not found\n");
        exit (1);
    }

    trackersock = socket (PF_INET, SOCK_STREAM, 0);
    if (trackersock == -1)
    {
        perror ("socket failed");
        exit (1);
    }

    sins.sin_family = AF_INET;
    memcpy (&sins.sin_addr, sp->h_addr_list[0], (size_t)sp->h_length);
    sins.sin_port = htons (PORTTRACKER);

    /* print information about this host:*/

    printf("Official name is: %s\n", sp->h_name);
    printf("    IP addresses: ");
    addr_list = (struct in_addr **)sp->h_addr_list;
    for(i = 0; addr_list[i] != NULL; i++)
    {
        printf("%s ", inet_ntoa(*addr_list[i]));
    }

    printf("\n");

    if (connect (trackersock, (struct sockaddr *)&sins, sizeof(sins)) == -1)
    {
        perror ("connect failed");
        exit (1);
    }

    /* Send request to tracker */
    req.fileHash = bt->filehash;
    req.peerId = myId;
    req.port = myPort;
    write(trackersock, &req.fileHash, sizeof(req.fileHash));
    write(trackersock, &req.peerId, sizeof(req.peerId));
    write(trackersock, &req.port, sizeof(req.port));

    /* Get answer from tracker */
    read(trackersock, &ans.status, sizeof(ans.status));
    read(trackersock, &ans.nbPeers, sizeof(ans.nbPeers));
    if(ans.status == 1)
    {
        printf("Error when retrieving peers info\n");
        return NULL;
    }

    printf("NbPeers: %hu\n", ans.nbPeers);

    peerList = malloc(sizeof(struct proto_tracker_peerlistentry));
    assert(peerList);
    peerList->nbPeers = ans.nbPeers;

    /* De nouveaux pairs peuvent être ajoutés après l'obtention des pairs, on alloue donc pour le nombre max. */
    peerList->pentry = malloc(sizeof(struct proto_tracker_peerentry) * MAX_PEERS);
    assert(peerList->pentry);

    /* Get n peers */
    for(j=0; j<peerList->nbPeers; j++)
    {

        read(trackersock, &(peerList->pentry[j].peerId), sizeof(peerList->pentry[j].peerId));
        read(trackersock, &(peerList->pentry[j].ipaddr), sizeof(peerList->pentry[j].ipaddr));
        read(trackersock, &(peerList->pentry[j].port), sizeof(peerList->pentry[j].port));

        printf("* peerId: %u\n", (peerList->pentry[j]).peerId);
        printf("* IPaddr: %s\n", inet_ntoa((peerList->pentry[j]).ipaddr));
        printf("* port: %hu\n", (peerList->pentry[j]).port);
    }

    close(trackersock);
    return peerList;
}

/* Beertorrent functions */

struct bitfield * createbitfield(u_int filelength, u_int piecelength)
{
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

void deletebitfield(struct bitfield * bf)
{
    free(bf->array);
    free(bf);
}

void setbitfield(struct bitfield * dst, struct bitfield * src)
{
    u_int i = 0 ;
    memcpy(dst->array, src->array, dst->arraysize);
    dst->nbpiece = 0 ;
    for(i = 0 ; i < src->totalpiece ; i++) {
        dst->nbpiece += (u_int)isinbitfield(src,i) ;
    }
    assert(dst->nbpiece == src->nbpiece);
}

int isinbitfield(struct bitfield * bf, u_int id)
{
    return !!(bf->array[id/8] & (0x1 << (id%8)));
}

void setbitinfield(struct bitfield * bf, u_int id)
{
    bf->nbpiece += (u_int)!isinbitfield(bf,id) ;
    bf->array[id/8] |= (u_char)(0x1 << (id%8));
}

struct beerTorrent * addtorrent(char * filename)
{

    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    size_t ln;
    
    struct beerTorrent * bt = malloc(sizeof(struct beerTorrent));

    assert(bt);

    fp = fopen(filename, "r");
    if (fp == NULL)
    {
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

    if( access( bt->filename, F_OK ) != -1 )
    {
        /*file exists - supposely complete*/

        if(!(bt->fp = fopen(bt->filename, "rb")))
        {

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
        if(!(bt->fp = fopen(bt->filename, "w+b")))
        {

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

void deletetorrent(struct beerTorrent *t) {
    fclose(t->fp);
    assert(pthread_mutex_destroy(&t->file_lock)==0);
    deletebitfield(t->have);         
    assert(pthread_mutex_destroy(&t->have_lock)==0);
    deletebitfield(t->request);         
    assert(pthread_mutex_destroy(&t->request_lock)==0);
    
    free(t) ;
}

int write_socket(int fd,const char *buf,int len)
{
    int currentsize=0;
    while(currentsize<len)
    {
        int count=(int)write(fd,buf+currentsize,(size_t)(len-currentsize));
        if(count<0) return -1;
        currentsize+=count;
    }
    return currentsize;
}

int readblock(int fd, char* buffer, int len)
{
    int ret  = 0;
    int count = 0;
    while (count < len)
    {
        ret = (int)read(fd, buffer + count, (size_t)(len - count));
        if (ret <= 0)
        {
            return (-1);
        }
        count += ret;
    }
    return count;
}
