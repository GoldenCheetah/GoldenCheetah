
/*** EDGELIST.C ***/

#include "vdefs.h"

int ELhashsize ;
Site * bottomsite ;
Freelist hfl ;
Halfedge * ELleftend, * ELrightend, **ELhash ;

int ntry, totalsearch ;

void
ELinitialize(void)
    {
    int i ;

    freeinit(&hfl, sizeof(Halfedge)) ;
    ELhashsize = 2 * sqrt_nsites ;
    ELhash = (Halfedge **)myalloc( sizeof(*ELhash) * ELhashsize) ;
    for (i = 0  ; i < ELhashsize  ; i++)
        {
        ELhash[i] = (Halfedge *)NULL ;
        }
    ELleftend = HEcreate((Edge *)NULL, 0) ;
    ELrightend = HEcreate((Edge *)NULL, 0) ;
    ELleftend->ELleft = (Halfedge *)NULL ;
    ELleftend->ELright = ELrightend ;
    ELrightend->ELleft = ELleftend ;
    ELrightend->ELright = (Halfedge *)NULL ;
    ELhash[0] = ELleftend ;
    ELhash[ELhashsize-1] = ELrightend ;
    }

Halfedge *
HEcreate(Edge * e, int pm)
    {
    Halfedge * answer ;

    answer = (Halfedge *)getfree(&hfl) ;
    answer->ELedge = e ;
    answer->ELpm = pm ;
    answer->PQnext = (Halfedge *)NULL ;
    answer->vertex = (Site *)NULL ;
    answer->ELrefcnt = 0 ;
    return (answer) ;
    }

void
ELinsert(Halfedge * lb, Halfedge * new)
    {
    new->ELleft = lb ;
    new->ELright = lb->ELright ;
    (lb->ELright)->ELleft = new ;
    lb->ELright = new ;
    }

/* Get entry from hash table, pruning any deleted nodes */

Halfedge *
ELgethash(int b)
    {
    Halfedge * he ;

    if ((b < 0) || (b >= ELhashsize))
        {
        return ((Halfedge *)NULL) ;
        }
    he = ELhash[b] ;
    if ((he == (Halfedge *)NULL) || (he->ELedge != (Edge *)DELETED))
        {
        return (he) ;
        }
    /* Hash table points to deleted half edge.  Patch as necessary. */
    ELhash[b] = (Halfedge *)NULL ;
    if ((--(he->ELrefcnt)) == 0)
        {
        makefree((Freenode *)he, (Freelist *)&hfl) ;
        }
    return ((Halfedge *)NULL) ;
    }

Halfedge *
ELleftbnd(Point * p)
    {
    int i, bucket ;
    Halfedge * he ;

    /* Use hash table to get close to desired halfedge */
    bucket = (p->x - xmin) / deltax * ELhashsize ;
    if (bucket < 0)
        {
        bucket = 0 ;
        }
    if (bucket >= ELhashsize)
        {
        bucket = ELhashsize - 1 ;
        }
    he = ELgethash(bucket) ;
    if  (he == (Halfedge *)NULL)
        {
        for (i = 1 ; 1 ; i++)
            {
            if ((he = ELgethash(bucket-i)) != (Halfedge *)NULL)
                {
                break ;
                }
            if ((he = ELgethash(bucket+i)) != (Halfedge *)NULL)
                {
                break ;
                }
            }
        totalsearch += i ;
        }
    ntry++ ;
    /* Now search linear list of halfedges for the corect one */
    if (he == ELleftend || (he != ELrightend && right_of(he,p)))
        {
        do  {
            he = he->ELright ;
            } while (he != ELrightend && right_of(he,p)) ;
        he = he->ELleft ;
        }
    else
        {
        do  {
            he = he->ELleft ;
            } while (he != ELleftend && !right_of(he,p)) ;
        }
    /*** Update hash table and reference counts ***/
    if ((bucket > 0) && (bucket < ELhashsize-1))
        {
        if (ELhash[bucket] != (Halfedge *)NULL)
            {
            (ELhash[bucket]->ELrefcnt)-- ;
            }
        ELhash[bucket] = he ;
        (ELhash[bucket]->ELrefcnt)++ ;
        }
    return (he) ;
    }

/*** This delete routine can't reclaim node, since pointers from hash
 : table may be present.
 ***/

void
ELdelete(Halfedge * he)
    {
    (he->ELleft)->ELright = he->ELright ;
    (he->ELright)->ELleft = he->ELleft ;
    he->ELedge = (Edge *)DELETED ;
    }

Halfedge *
ELright(Halfedge * he)
    {
    return (he->ELright) ;
    }

Halfedge *
ELleft(Halfedge * he)
    {
    return (he->ELleft) ;
    }

Site *
leftreg(Halfedge * he)
    {
    if (he->ELedge == (Edge *)NULL)
        {
        return(bottomsite) ;
        }
    return (he->ELpm == le ? he->ELedge->reg[le] :
        he->ELedge->reg[re]) ;
    }

Site *
rightreg(Halfedge * he)
    {
    if (he->ELedge == (Edge *)NULL)
        {
        return(bottomsite) ;
        }
    return (he->ELpm == le ? he->ELedge->reg[re] :
        he->ELedge->reg[le]) ;
    }

