#define _GNU_SOURCE             /* pour pthread_yield */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>
#include "../src/rename.h"

#define VERBOSE 0

struct map *m;

void *thread1(void *ptr)  {
    u_short a,b,c,d,e ;
    a=get_name(m,23) ;
    b=get_name(m,39104) ;
    c=get_name(m,2) ;
    d=get_name(m,781) ;
    pthread_yield() ;
    e=get_name(m,31) ;
    pthread_yield() ;
    assert(a==get_name(m,23)) ;
    assert(b==get_name(m,39104)) ;
    assert(c==get_name(m,2)) ;
    assert(d==get_name(m,781)) ;
    assert(e==get_name(m,31)) ;
    pthread_yield() ;
    assert(a==get_name(m,23)) ;
    assert(23==get_id(m,a)) ;
    assert(b==get_name(m,39104)) ;
    assert(39104==get_id(m,b)) ;
    assert(c==get_name(m,2)) ;
    assert(2==get_id(m,c)) ;
    assert(d==get_name(m,781)) ;
    assert(781==get_id(m,d)) ;
    assert(e==get_name(m,31)) ;
    assert(31==get_id(m,e)) ;
    
    if(VERBOSE) printf("Thread 1 :");
    if(VERBOSE) printf("\ta=%u",a);
    if(VERBOSE) printf("\tb=%u",b);
    if(VERBOSE) printf("\tc=%u",c);
    if(VERBOSE) printf("\td=%u",d);
    if(VERBOSE) printf("\te=%u",e);
    if(VERBOSE) printf("\n");
    
    pthread_exit(NULL) ;
    ptr=ptr;
}

void *thread2(void *ptr)  {
    u_short a,b,c,d,e ;
    a=get_name(m,21) ;
    pthread_yield() ;
    b=get_name(m,39101) ;
    c=get_name(m,1) ;
    d=get_name(m,781) ;
    e=get_name(m,31) ;
    pthread_yield() ;
    assert(a==get_name(m,21)) ;
    assert(21==get_id(m,a)) ;
    assert(b==get_name(m,39101)) ;
    assert(39101==get_id(m,b)) ;
    assert(c==get_name(m,1)) ;
    assert(1==get_id(m,c)) ;
    assert(d==get_name(m,781)) ;
    assert(781==get_id(m,d)) ;
    assert(e==get_name(m,31)) ;
    assert(31==get_id(m,e)) ;
    pthread_yield() ;
    assert(a==get_name(m,21)) ;
    assert(b==get_name(m,39101)) ;
    assert(c==get_name(m,1)) ;
    assert(d==get_name(m,781)) ;
    assert(e==get_name(m,31)) ;
    
    if(VERBOSE) printf("Thread 2 :");
    if(VERBOSE) printf("\ta=%u",a);
    if(VERBOSE) printf("\tb=%u",b);
    if(VERBOSE) printf("\tc=%u",c);
    if(VERBOSE) printf("\td=%u",d);
    if(VERBOSE) printf("\te=%u",e);
    if(VERBOSE) printf("\n");
    
    pthread_exit(NULL) ;
    ptr=ptr;
}

void *thread3(void *ptr) {
    u_short a,b,c,d,e ;
    a=get_name(m,28) ;
    b=get_name(m,39108) ;
    pthread_yield() ;
    c=get_name(m,8) ;
    d=get_name(m,781) ;
    e=get_name(m,31) ;
    pthread_yield() ;
    assert(a==get_name(m,28)) ;
    assert(b==get_name(m,39108)) ;
    assert(c==get_name(m,8)) ;
    assert(d==get_name(m,781)) ;
    assert(e==get_name(m,31)) ;
    pthread_yield() ;
    assert(a==get_name(m,28)) ;
    assert(b==get_name(m,39108)) ;
    assert(c==get_name(m,8)) ;
    assert(d==get_name(m,781)) ;
    assert(e==get_name(m,31)) ;
    assert(28==get_id(m,a)) ;
    assert(39108==get_id(m,b)) ;
    assert(8==get_id(m,c)) ;
    assert(781==get_id(m,d)) ;
    assert(31==get_id(m,e)) ;
    
    if(VERBOSE) printf("Thread 3 :");
    if(VERBOSE) printf("\ta=%u",a);
    if(VERBOSE) printf("\tb=%u",b);
    if(VERBOSE) printf("\tc=%u",c);
    if(VERBOSE) printf("\td=%u",d);
    if(VERBOSE) printf("\te=%u",e);
    if(VERBOSE) printf("\n");
    
    pthread_exit(NULL) ;
    ptr=ptr;
}

int main() {
    int i ;
    u_int tmp2 ;
    u_short j,tmp1 ;
    pthread_t t1,t2,t3 ;
    srand((u_int)time(NULL)) ;
    
    /* Premier jeu de tests. */
    m=init_map() ;
    assert(get_name(m,234)==0) ;
    assert(get_name(m,284783)==1) ;
    assert(get_name(m,0)==2) ;
    assert(get_name(m,284783)==1) ;
    assert(get_name(m,234+ID_MAX)==3) ; /* même case dans la map */
    assert(get_name(m,0+ID_MAX)==4) ;
    assert(get_name(m,234)==0) ;
    for(i = 0 ; i < ID_MAX-5 ; i++)
        get_name(m,(u_int)rand()) ;
    assert(get_name(m,234)==0) ;
    assert(get_name(m,284783)==1) ;
    assert(get_name(m,0)==2) ;
    assert(get_name(m,284783)==1) ;
    assert(get_name(m,234+ID_MAX)==3) ; /* même case dans la map */
    assert(get_name(m,0+ID_MAX)==4) ;
    assert(get_name(m,234)==0) ;
    assert(get_id(m,0)==234) ;
    assert(get_id(m,1)==284783) ;
    assert(get_id(m,2)==0) ;
    assert(get_id(m,3)==234+ID_MAX) ;
    assert(get_id(m,4)==0+ID_MAX) ;
    
    /* Second jeu de tests. */
    reinit_map(m) ;
    for(i = 0 ; i < 10000 ; i++) {
        tmp1=get_name(m,tmp2=(u_int)rand()%ID_MAX) ;
        assert(tmp2==get_id(m,tmp1));
    }
        
        
    /* Troisième jeu de tests. */
    reinit_map(m) ;
    for(j = 0 ; j < ID_MAX ; j++) {
        assert(j==get_name(m,j)) ;
        assert(j==get_id(m,j)) ;
    }
    for(j = 0 ; j < ID_MAX ; j++) {
        assert(j==get_name(m,j)) ;
        assert(j==get_id(m,j)) ;
    }
        
    /* Quatrième jeu de tests. */
    reinit_map(m) ;
    for(j = 0 ; j < ID_MAX ; j++) {
        assert(j==get_name(m,(u_int)j*ID_MAX)) ;
        assert(j*ID_MAX==get_id(m,j));
    }
    for(j = 0 ; j < ID_MAX ; j++) {
        assert(j==get_name(m,(u_int)j*ID_MAX)) ;
        assert(j*ID_MAX==get_id(m,j));
    }
        
    reinit_map(m) ;
    
    
    (pthread_create(&t1,NULL,thread1,NULL)) ;
    (pthread_create(&t2,NULL,thread2,NULL)) ;
    (pthread_create(&t3,NULL,thread3,NULL)) ;
    
    (pthread_join(t1,NULL)) ;
    (pthread_join(t2,NULL)) ;
    (pthread_join(t3,NULL)) ;
    
    assert(get_map_size(m) == 11) ;
    destroy_map(m);
    printf("ok\n");
    return 0 ;
}
