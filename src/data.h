#ifndef DATA_H
#define DATA_H

/* Structure associée à chaque socket */
struct proto_sock {
    u_int file_hash ;   /* hash du nom du fichier associé */
    u_int peer_id ;     /* ID du pair associé */
};


#endif
