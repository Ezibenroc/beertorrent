#ifndef tracker_h
#define tracker_h

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h> /* constantes, familles... */
#include <netinet/in.h> /* struct sockaddr_in */
#include <arpa/inet.h>  /* prototypes pour les fonctions dans inet(3N) */
#include <netdb.h>      /* struct hostent */
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#define PORT 3955
#define MAXFILES 10
#define DEBUG_MALLOC 1

/* Hash d'une chaîne. */
u_int hash(u_char *str) __attribute__ ((pure));

/* Hash d'un fichier. */
u_int file2hash(int fd);

/* Remplis le tracker avec tous les fichiers .beertorrent présents dans le répertoire donné. */
void fillTracker(const char* dir);

/* Attend de nouvelles requêtes. */
void startTracker() __attribute__ ((noreturn));

/* Répond à une requête. */
void serveRequest (int fd, struct sockaddr_in from, int len);

/* Référence le torrent de nom donné. */
void addTorrent(char * filename);

/* Renvoie l'extension. */
const char *get_filename_ext(const char *filename) __attribute__ ((pure));

/* Renvoie le fichier correspondant au hash donné. */
struct fileListEntry * searchHash(u_int hashToSearch) __attribute__ ((pure));

#endif
