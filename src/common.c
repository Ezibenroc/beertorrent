#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "common.h"

#define size_id 22
void print_id() {
    char s[size_id] ;
    int pos =0;
    u_int tmp ;
    memset(s,'#',(size_id-1)*sizeof(char)) ;
    s[size_id-1]='\0' ;
    printf("%s\n",s);
    
    memset(s,' ',(size_id-1)*sizeof(char)) ;
    s[0] = '#' ; s[size_id-2] = '#' ;
    s[2] = 'I' ; s[3] = 'D' ;  s[7] = '=' ;
    pos = size_id-4 ;
    tmp = my_id ;
    if(tmp==0) s[pos] = '0' ;
    while(tmp != 0) {
        s[pos--] = (char) (tmp%10+'0') ;
        tmp=tmp/10 ;
    }
    printf("%s\n",s);
    
    memset(s,' ',(size_id-1)*sizeof(char)) ;
    s[0] = '#' ; s[size_id-2] = '#' ;
    s[2] = 'P' ; s[3] = 'O' ; s[4] = 'R' ; s[5] = 'T' ; s[7] = '=' ;
    pos = size_id-4 ;
    tmp = my_port ;
    if(tmp==0) s[pos] = '0' ;
    while(tmp != 0) {
        s[pos--] = (char) (tmp%10+'0') ;
        tmp=tmp/10 ;
    }
    printf("%s\n",s);
    
    memset(s,35,(size_id-1)*sizeof(char)) ;
    printf("%s\n",s);
}

u_int hash(u_char *str)
{
    u_int hash_ = 0;
    int c;

    while ((c = (int)(*str++)))
        hash_ += (u_int)c;

    return hash_;
}

u_int file2hash(int fd)
{

    int bytes;
    u_char data[1024];
    u_int h = 0;

    while ((bytes = (int)read(fd, data, 1024)) != 0)
        h += hash(data);

    return h;
}

char *get_filename_ext(char *filename)
{
    char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return NULL;
    return dot + 1;
}


void init_cancel() {
    int i ;
    for(i = 0 ; i < N_SOCK ; i++) {
        memset(cancel[i].msg,0,N_CANCEL*2*sizeof(u_int)) ;
        cancel[i].ind = 0 ;
        pthread_mutex_init(&(cancel[i].lock), NULL);
    }
}

struct proto_client_handshake* construct_handshake(struct beerTorrent *torrent) {
    struct proto_client_handshake* hs ;
    hs = (struct proto_client_handshake*) malloc(sizeof(struct proto_client_handshake)) ;
    assert(hs);
    hs->version = 2 ;
    hs->filehash = torrent->filehash ;
    hs->peerId = my_id ;
    return hs ;
}

void send_handshake(const struct proto_peer *peer, const struct proto_client_handshake *hs) {
    write(peer->sockfd,&hs->version,sizeof(hs->version)) ;
    write(peer->sockfd,&hs->filehash,sizeof(hs->filehash)) ;
    write(peer->sockfd,&hs->peerId,sizeof(hs->peerId)) ;

}

void receive_handshake(const struct proto_peer *peer, const struct proto_client_handshake *hs) {
    u_char c ; u_int i ;
    read(peer->sockfd, &c, sizeof(u_char));
    assert(c==hs->version) ;
    read(peer->sockfd, &i, sizeof(u_int));
    assert(i==hs->filehash) ;
    read(peer->sockfd, &i, sizeof(u_int));
    assert(i==peer->peerId) ;
}

int init_peer_connection(struct proto_peer *peer, const struct proto_client_handshake *hs) {
    int fd ;
    struct hostent *sp;
    struct sockaddr_in sins;
    
    sp = gethostbyaddr(&(peer->ipaddr), sizeof(peer->ipaddr), AF_INET);
    if (sp == NULL) {
        perror("gethostbyaddr");
        exit(errno);
    }
    
    fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror ("socket");
        exit(errno);
    }    
    
    sins.sin_family = AF_INET;
    memcpy (&sins.sin_addr, sp->h_addr_list[0], (size_t)sp->h_length);
    sins.sin_port = htons(peer->port);
    
    if (connect (fd, (struct sockaddr *)&sins, sizeof(sins)) == -1) {
        perror ("connect");
        exit (errno);
    }
    
    peer->sockfd = fd ;
    send_handshake(peer,hs) ;
    receive_handshake(peer,hs) ; 
    free(hs);   
    
}

void init_peers_connections(struct torrent_info *ti) {
    /* Construction du handshake, identique pour tous. */
    struct proto_client_handshake* hs = construct_handshake(ti->torrent) ;
    
    int i ;
    /* Parcours des pairs */
    for(i = 0 ; i < ti->peerlist->nbPeers ; i++) {
        init_peer_connection(&ti->peerlist->pentry[i],hs) ;
    }
}
