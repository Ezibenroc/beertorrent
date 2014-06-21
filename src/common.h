#ifndef COMMON_H
#define COMMON_H

#include <semaphore.h>

#include "peerfunc.h"


#define blue() printf("\033[%sm","34")
#define cyan() printf("\033[%sm","36") 
#define green() printf("\033[%sm","32") 
#define normal() printf("\033[%sm","0") 

#define max(x, y) (x > y ? x : y)
#define min(x, y) (x < y ? x : y)

#define N_SOCK 1024
#define N_THREAD 4
#define N_REQUESTS 10

#define VERBOSE 1

#define pthread_mutex_lock(x) assert(!pthread_mutex_lock(x))
#define pthread_mutex_unlock(x) assert(!pthread_mutex_unlock(x))

int quit_program ;

/* ID du pair. */
u_int my_id ;
/* Port du pair. */
u_short my_port ;

pthread_mutex_t print_lock ;

/* Renommage des fichiers et des sockets, pour pouvoir s'en servir d'indices dans des tableaux */
struct map *file_map, *socket_map ;

/* socket_to_file[i] est le hash du fichier associé à la socket renommée en i */
u_int socket_to_file[N_SOCK] ;

/* socket_to_peer[i] est l'id du pair associé à la socket renommée en i */
u_int socket_to_peer[N_SOCK] ;

/* Listes des pairs (indicé par l'id de leur socket). */
struct proto_peer *peers[N_SOCK] ;

/* Tableau des torrent_list (et infos associées) */
struct torrent_info **torrent_list ;
/* Nombre de torrents */
unsigned int nb_files ;
unsigned int nb_files_to_download ;

/* Renvoie vrai ssi plus de requête à envoyer. */
int end_job();

/* FIFO de socket. */
/* Un thread repère les sockets ayant un message et les place dans la file request. */
/* Les autres treads défilent une socket, lisent son message et le traitent, et la placent dans la file handled_request. */
struct waiting_queue {
    int queue[N_SOCK] ;
    int first ; /* Indice du premier élément ajouté. */
    int last ;  /* Indice du dernier élément ajouté. */
    pthread_mutex_t lock ;
    sem_t full ;
};
struct waiting_queue *request ;

struct non_waiting_queue {
    int queue[N_SOCK] ;
    int first ; /* Indice du premier élément ajouté. */
    int last ;  /* Indice du dernier élément ajouté. */
    pthread_mutex_t lock ;
};
struct non_waiting_queue *handled_request ;

struct waiting_queue *init_waiting_queue() ;
struct non_waiting_queue *init_non_waiting_queue() ;
#define init_queues() request=init_waiting_queue() ; handled_request=init_non_waiting_queue()

/* Retourne la tête de la file (appel bloquant si file vide). */
int pop(struct waiting_queue *q) ;

/* Fonction d'affichage (ID et port). */
void print_id() ;

/* Hachage d'une chaîne de caractère. */
u_int hash(u_char *str) ;

/* Hachage d'un fichier (donné par son descripteur). */
u_int file2hash(int fd) ;

/* Retourne un pointeur vers le début de l'extension du fichier. */
char *get_filename_ext(char *filename) ;

#define N_CANCEL 8
struct cancel_entry {
    u_int msg[N_CANCEL][2] ; /* tableau circulaire représentant les message "cancel" reçus */
    u_int ind ;
    pthread_mutex_t lock ;
} cancel[N_SOCK] ;

/* Initialisation du tableau cancel. */
void init_cancel() ;

/* Choisis un pair et une pièce pour la prochaîne requête, pour le fichier donné. */
/* Renvois 1 en cas de succés, 0 en cas d'échec (piece_id et peer ne sont alors pas modifiés). */
/* Pré-condition : fichier non complet. */
int choose_piece_peer_for_file(u_int *piece_id, struct proto_peer **peer, struct torrent_info *bt) ;

/* Choisis un fichier, puis une pièce et un pair, pour la prochaine requête. */
/* Endore le thread si rien n'est trouvé et si blocking vaut 1. */
/* Renvoie 1 ssi un choix a pu être fait (si blocking vaut 1, renvoie toujours 1). */
/* Pré-condition : il existe un fichier incomplet. */
int choose_piece_peer(u_int *piece_id, struct proto_peer **peer, struct beerTorrent **torrent, int thread_id, int blocking) ;

/* Construit un handshake correspondant au torrent donné. */
struct proto_client_handshake* construct_handshake(struct beerTorrent *torrent);

/* Envoie le handshake au pair donné. */
void send_handshake(const struct proto_peer *peer, const struct proto_client_handshake *hs);

/* Initialise la connection de tous les pairs (création des sockets, réalisation des handshakes, envoi des bitfields). */
/* Remplis la table de sockets, et le tableau faisant la correspondance entre sockets et fichiers. */
void init_peers_connections(struct torrent_info *ti);

/* Envoie des premières requêtes. */
/* Pré-condition : nombre de fichiers à télécharger non nul. 
void send_first_requests() ;*/

/* Surveille toutes les sockets référencées. */
/* Fonction exécutée par un thread. */
void *watch_sockets(void*) ;

/* Boucle : défile une socket à traiter, lis son message et exécute les actions appropriées. */
/* Fonction exécutée par un thread. */
void *treat_sockets(void*) ;

#endif
