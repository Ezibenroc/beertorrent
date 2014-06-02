#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>


#include "common.h"

#define DEFAULT_PIECE_LENGTH 16384
#define DEFAULT_IP "127.0.0.1"

char *beertorrent_extension(char *s) {
    char *bt ;
    char tmp ;
    char *dot = get_filename_ext(s) ;
    size_t size = strlen(s) ;
    if(dot == NULL)
        dot = s+size ;
    else
        dot-- ;
    bt = (char*) malloc(sizeof(char)*(size+13)) ;
    tmp = *dot ;
    *dot = '\0' ;
    strcpy(bt,s) ;
    *dot = tmp ;
    bt = strcat(bt,".beertorrent");
    return bt ;
}

/* From http://stackoverflow.com/questions/8236/how-do-you-determine-the-size-of-a-file-in-c */
off_t fsize(const char *filename) {
    struct stat st;

    if (stat(filename, &st) == 0)
        return st.st_size;

    perror("stat") ;
    exit(errno) ;

}

int main(int argc, char *argv[]) {
    int fd ;
    FILE *f_bt ;
    char *name_bt ;
    if(argc != 2) {
        fprintf(stderr,"Syntax : %s [file]\n",argv[0]) ;
        exit(EXIT_FAILURE) ;
    }
    fd = open(argv[1],O_RDONLY) ;
    if(fd==-1) {
        perror("open");
        exit(errno);
    }
    
    name_bt = beertorrent_extension(argv[1]) ;
    f_bt = fopen(name_bt,"w") ;
    if(f_bt==NULL) {
        perror("fopen");
        exit(errno);
    }
    fprintf(f_bt,"%lu\n",fsize(argv[1])) ;
    fprintf(f_bt,"%u\n",file2hash(fd)) ;
    fprintf(f_bt,"%s\n",argv[1]) ;
    fprintf(f_bt,"%d\n",DEFAULT_PIECE_LENGTH);
    fprintf(f_bt,"%s\n",DEFAULT_IP);
    
    free(name_bt);
    close(fd);
    fclose(f_bt);
    return 0 ;
}
