
/*** MEMORY.C ***/

#include <stdio.h>
#include <stdlib.h>  /* malloc(), exit() */

#include "vdefs.h"

extern int sqrt_nsites, siteidx ;

void
freeinit(Freelist * fl, int size)
    {
    fl->head = (Freenode *)NULL ;
    fl->nodesize = size ;
    }

char *
getfree(Freelist * fl)
    {
    int i ;
    Freenode * t ;
    if (fl->head == (Freenode *)NULL)
        {
        t =  (Freenode *) myalloc(sqrt_nsites * fl->nodesize) ;
        for(i = 0 ; i < sqrt_nsites ; i++)
            {
            makefree((Freenode *)((char *)t+i*fl->nodesize), fl) ;
            }
        }
    t = fl->head ;
    fl->head = (fl->head)->nextfree ;
    return ((char *)t) ;
    }

void
makefree(Freenode * curr, Freelist * fl)
    {
    curr->nextfree = fl->head ;
    fl->head = curr ;
    }

int total_alloc ;

char *
myalloc(unsigned n)
    {
    char * t ;
    if ((t=malloc(n)) == (char *) 0)
        {
        fprintf(stderr,"Insufficient memory processing site %d (%d bytes in use)\n",
        siteidx, total_alloc) ;
        return(0) ; // was exit(0) in original source, we aint having that here !!!
        }
    total_alloc += n ;
    return (t) ;
    }
