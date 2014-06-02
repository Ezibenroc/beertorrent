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
u_int hash(u_char *str) __attribute__ ((pure));
void fillTracker(const char* dir);
void startTracker() __attribute__ ((noreturn));
void serveRequest (int fd, struct sockaddr_in from, int len);
void addTorrent(char * filename);
const char *get_filename_ext(const char *filename) __attribute__ ((pure));
struct fileListEntry * searchHash(u_int hashToSearch) __attribute__ ((pure));
u_int file2hash(int fd);

#endif
