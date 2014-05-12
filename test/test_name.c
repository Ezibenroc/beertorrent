#define _GNU_SOURCE             /* pour pthread_yield */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>
#include "../src/rename.h"


void *thread1(void *ptr)  {
    u_int a,b,c,d,e ;
    a=name(23) ;
    b=name(39104) ;
    c=name(2) ;
    d=name(781) ;
    pthread_yield() ;
    e=name(31) ;
    pthread_yield() ;
    assert(a==name(23)) ;
    assert(b==name(39104)) ;
    assert(c==name(2)) ;
    assert(d==name(781)) ;
    assert(e==name(31)) ;
    pthread_yield() ;
    assert(a==name(23)) ;
    assert(b==name(39104)) ;
    assert(c==name(2)) ;
    assert(d==name(781)) ;
    assert(e==name(31)) ;
    
    printf("Thread 1 :");
    printf("\ta=%u",a);
    printf("\tb=%u",b);
    printf("\tc=%u",c);
    printf("\td=%u",d);
    printf("\te=%u",e);
    printf("\n");
    
    pthread_exit(NULL) ;
    ptr=ptr;
}

void *thread2(void *ptr)  {
    u_int a,b,c,d,e ;
    a=name(21) ;
    pthread_yield() ;
    b=name(39101) ;
    c=name(1) ;
    d=name(781) ;
    e=name(31) ;
    pthread_yield() ;
    assert(a==name(21)) ;
    assert(b==name(39101)) ;
    assert(c==name(1)) ;
    assert(d==name(781)) ;
    assert(e==name(31)) ;
    pthread_yield() ;
    assert(a==name(21)) ;
    assert(b==name(39101)) ;
    assert(c==name(1)) ;
    assert(d==name(781)) ;
    assert(e==name(31)) ;
    
    printf("Thread 2 :");
    printf("\ta=%u",a);
    printf("\tb=%u",b);
    printf("\tc=%u",c);
    printf("\td=%u",d);
    printf("\te=%u",e);
    printf("\n");
    
    pthread_exit(NULL) ;
    ptr=ptr;
}

void *thread3(void *ptr) {
    u_int a,b,c,d,e ;
    a=name(28) ;
    b=name(39108) ;
    pthread_yield() ;
    c=name(8) ;
    d=name(781) ;
    e=name(31) ;
    pthread_yield() ;
    assert(a==name(28)) ;
    assert(b==name(39108)) ;
    assert(c==name(8)) ;
    assert(d==name(781)) ;
    assert(e==name(31)) ;
    pthread_yield() ;
    assert(a==name(28)) ;
    assert(b==name(39108)) ;
    assert(c==name(8)) ;
    assert(d==name(781)) ;
    assert(e==name(31)) ;
    
    printf("Thread 3 :");
    printf("\ta=%u",a);
    printf("\tb=%u",b);
    printf("\tc=%u",c);
    printf("\td=%u",d);
    printf("\te=%u",e);
    printf("\n");
    
    pthread_exit(NULL) ;
    ptr=ptr;
}

int main() {
    int i ;
    u_int j ;
    pthread_t t1,t2,t3 ;
    srand((u_int)time(NULL)) ;
    
    /* Premier jeu de tests. */
    init_map() ;
    assert(name(234)==0) ;
    assert(name(284783)==1) ;
    assert(name(0)==2) ;
    assert(name(284783)==1) ;
    assert(name(234+ID_MAX)==3) ; /* même case dans la map */
    assert(name(0+ID_MAX)==4) ;
    assert(name(234)==0) ;
    for(i = 0 ; i < ID_MAX-5 ; i++)
        name((u_int)rand()) ;
    assert(name(234)==0) ;
    assert(name(284783)==1) ;
    assert(name(0)==2) ;
    assert(name(284783)==1) ;
    assert(name(234+ID_MAX)==3) ; /* même case dans la map */
    assert(name(0+ID_MAX)==4) ;
    assert(name(234)==0) ;
    
    /* Second jeu de tests. */
    reinit_map() ;
    for(i = 0 ; i < 10000 ; i++)
        name((u_int)rand()%ID_MAX) ;
        
    /* Troisième jeu de tests. */
    reinit_map() ;
    for(j = 0 ; j < ID_MAX ; j++)
        assert(j==name(j)) ;
    for(j = 0 ; j < ID_MAX ; j++)
        assert(j==name(j)) ;
        
    /* Quatrième jeu de tests. */
    reinit_map() ;
    for(j = 0 ; j < ID_MAX ; j++)
        assert(j==name(j*ID_MAX)) ;
    for(j = 0 ; j < ID_MAX ; j++)
        assert(j==name(j*ID_MAX)) ;
        
    reinit_map() ;
    
    
    (pthread_create(&t1,NULL,thread1,NULL)) ;
    (pthread_create(&t2,NULL,thread2,NULL)) ;
    (pthread_create(&t3,NULL,thread3,NULL)) ;
    
    (pthread_join(t1,NULL)) ;
    (pthread_join(t2,NULL)) ;
    (pthread_join(t3,NULL)) ;
    
    assert(get_map_size() == 11) ;
    destroy_map();
    return 0 ;
}
