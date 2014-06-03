#include <assert.h>

#include "tracker.h"

/* Communication structures */
struct proto_tracker_clientrequest {
    u_int fileHash;
    u_int peerId;
    u_short port;
};

struct proto_tracker_trackeranswer {
    u_char status;
    u_char nbPeers;
};

struct proto_tracker_peerentry {
    u_int peerId;
    struct in_addr ipaddr;
    u_short port;
};

/* Internal structures */

struct proto_tracker_peerlistentry {
    struct proto_tracker_peerentry pentry;
    struct proto_tracker_peerlistentry * next;
};

struct fileListEntry {
    u_int fileHash;
    u_char nbPeers;
    struct proto_tracker_peerlistentry * entries;
};

struct fileList {
    int nbfiles;
    struct fileListEntry * list[MAXFILES];
};

/* Global Vars*/

struct fileList filesList;

/* Functions */

u_int hash(u_char *str) {
    u_int hash_ = 0;
    int c;

    while ((c = (int)(*str++)))
        hash_ += (u_int)c;

    return hash_;
}

u_int file2hash(int fd) {

    int bytes;
    u_char data[1024];
    u_int h = 0;

    while ((bytes = (int)read(fd, data, 1024)) != 0)
        h += hash(data);

    return h;
}


struct fileListEntry * searchHash(u_int hashToSearch) {

    int i = 0;
    struct fileListEntry * ptr;

    while(i < filesList.nbfiles) {

        ptr = filesList.list[i];

        if(ptr->fileHash == hashToSearch)
        {
            /* Hash found ! */
            return ptr;
        }

        /* Not found*/
        i++;
    }

    return NULL;
}

const char *get_filename_ext(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}

void addTorrent(char * filename) {

    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    struct fileListEntry * fileEntry;
    u_int hash_;

    fp = fopen(filename, "r");
    if (fp == NULL)
        return;

    /* Format :
     * length (Taille du fichier en octet)
     * hashsum (Unsigned long correspondant à la signature du fichier)
     * name (Nom du fichier)
     * piece length (Taille de chacun des morceaux à télécharger)
     * announce (IP du Tracker)
     * */
    getline(&line, &len, fp);
    hash_ = (u_int) atol(line); /*skip*/

    getline(&line, &len, fp);
    hash_ = (u_int) atol(line);

    free(line);

    fileEntry = malloc(sizeof(struct fileListEntry));
    assert(fileEntry);
    fileEntry->fileHash = hash_;
    fileEntry->nbPeers = 0;
    fileEntry->entries = NULL;

    filesList.list[filesList.nbfiles] = fileEntry;
    filesList.nbfiles++;

    printf("%s added\n", filename);
}


void serveRequest (int fd, struct sockaddr_in from, int len) {

    struct proto_tracker_clientrequest req;
    struct proto_tracker_trackeranswer resp;
    struct fileListEntry * fEntry;
    struct proto_tracker_peerlistentry * e;
    len=len;
    printf("Serving request ...\n");

    /* Parse Hash|peerId|Port + get IP */
    read(fd, &req.fileHash, sizeof(req.fileHash));
    read(fd, &req.peerId, sizeof(req.peerId));
    read(fd, &req.port, sizeof(req.port));

    printf("* hash: %u\n", req.fileHash);
    printf("* peerId: %u\n", req.peerId);
    printf("* port: %hu\n", req.port);

    /* If hash exists */
    if((fEntry = searchHash(req.fileHash)) != NULL) {
        /* Insert peerId|IP|port in hashmap */

        struct proto_tracker_peerlistentry * newe = malloc(sizeof(struct proto_tracker_peerlistentry));
        assert(newe);
        newe->pentry.peerId = req.peerId;
        newe->pentry.port = req.port;
        newe->pentry.ipaddr = from.sin_addr;

        newe->next = fEntry->entries;
        fEntry->nbPeers++;
        fEntry->entries = newe;

        /* Return success + peerlist */
        resp.status = 0;
        resp.nbPeers = fEntry->nbPeers;
        write(fd, &resp.status, sizeof(resp.status));
        write(fd, &resp.nbPeers, sizeof(resp.nbPeers));

        /* For all entry, write infos */
        e = fEntry->entries;
        while(e != NULL) {

            write(fd, &(e->pentry.peerId), sizeof(e->pentry.peerId));
            write(fd, &(e->pentry.ipaddr), sizeof(e->pentry.ipaddr));
            write(fd, &(e->pentry.port), sizeof(e->pentry.port));

            e = e->next;
        }

        printf("* SUCCESS: peer added\n");
    }
    else {
        /* Else return error + empty list */
        printf("* ERROR: hash not found\n");

        resp.status = 1;
        resp.nbPeers = 0;
        write(fd, &resp.status, sizeof(resp.status));
        write(fd, &resp.nbPeers, sizeof(resp.nbPeers));
    }

    /* close */
    close(fd);
}

void startTracker() {

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
    soc_in.sin_port = htons (PORT);
    bind (ss, (struct sockaddr*)&soc_in, sizeof(soc_in));

    printf("Tracker started ...\n");
    /* Listen */
    listen (ss, 5);

    while (1) {
        struct sockaddr_in from;
        int len;
        int f;

        /* Accept & Serve */
        len = sizeof (from);
        f = accept (ss, (struct sockaddr *)&from, (u_int*)&len);

        serveRequest(f, from, len);
    }
}

void fillTracker(const char* dir) {
    char cwd[1024];
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;

    /* search all beertorrent in dir */
    printf("Filling tracker ...\n");

    if((dp = opendir(dir)) == NULL)
    {
        perror("opendir");
        return;
    }
    getcwd(cwd, sizeof(cwd));

    chdir(dir);
    while((entry = readdir(dp)) != NULL)
    {
        lstat(entry->d_name,&statbuf);
        if(S_ISREG(statbuf.st_mode))
        {
            /* Found a regular file, if ext = .beertorrent, read it */
            if(strcmp(get_filename_ext(entry->d_name), "beertorrent") == 0)
            {
                addTorrent(entry->d_name);
            }
        }
    }
    closedir(dp);

    chdir(cwd);
}

int main(int argc, char** argv) {
    argc=argc;
    argv[0]=argv[0];

    /* Init struct */
    filesList.nbfiles = 0;

    /* Fill tracker */
    fillTracker("./beertorrent");

    /* Start routine */
    startTracker();

    return 0;
}
