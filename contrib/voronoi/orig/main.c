/*** MAIN.C ***/

#include <stdio.h>
#include <stdlib.h>  /* realloc(), qsort() */

#include "vdefs.h"

Site * readone(void), * nextone(void) ;
void readsites(void) ;

int sorted, triangulate, plot, debug, nsites, siteidx ;
float xmin, xmax, ymin, ymax ;
Site * sites ;
Freelist sfl ;

int
main(int argc, char *argv[])
    {
    int c ;
    Site *(*next)() ;

    sorted = triangulate = plot = debug = 0 ;
    while ((c = getopt(argc, argv, "dpst")) != EOF)
        {
        switch(c)
            {
            case 'd':
                debug = 1 ;
                break ;
            case 's':
                sorted = 1 ;
                break ;
            case 't':
                triangulate = 1 ;
                break ;
            case 'p':
                plot = 1 ;
                break ;
            }
        }

    freeinit(&sfl, sizeof(Site)) ;
    if (sorted)
        {
        scanf("%d %f %f %f %f", &nsites, &xmin, &xmax, &ymin, &ymax) ;
        next = readone ;
        }
    else
        {
        readsites() ;
        next = nextone ;
        }
    siteidx = 0 ;
    geominit() ;
    if (plot)
        {
        plotinit() ;
        }
    voronoi(next) ;
    return (0) ;
    }

/*** sort sites on y, then x, coord ***/

int
scomp(const void * vs1, const void * vs2)
    {
    Point * s1 = (Point *)vs1 ;
    Point * s2 = (Point *)vs2 ;

    if (s1->y < s2->y)
        {
        return (-1) ;
        }
    if (s1->y > s2->y)
        {
        return (1) ;
        }
    if (s1->x < s2->x)
        {
        return (-1) ;
        }
    if (s1->x > s2->x)
        {
        return (1) ;
        }
    return (0) ;
    }

/*** return a single in-storage site ***/

Site *
nextone(void)
    {
    Site * s ;

    if (siteidx < nsites)
        {
        s = &sites[siteidx++];
        return (s) ;
        }
    else
        {
        return ((Site *)NULL) ;
        }
    }

/*** read all sites, sort, and compute xmin, xmax, ymin, ymax ***/

void
readsites(void)
    {
    int i ;

    nsites = 0 ;
    sites = (Site *) myalloc(4000 * sizeof(Site));
    while (scanf("%f %f", &sites[nsites].coord.x,
&sites[nsites].coord.y) !=EOF)
        {
        sites[nsites].sitenbr = nsites ;
        sites[nsites++].refcnt = 0 ;
        if (nsites % 4000 == 0)
            sites = (Site *)
realloc(sites,(nsites+4000)*sizeof(Site));
        }

    qsort((void *)sites, nsites, sizeof(Site), scomp) ;
    xmin = sites[0].coord.x ;
    xmax = sites[0].coord.x ;
    for (i = 1 ; i < nsites ; ++i)
        {
        if(sites[i].coord.x < xmin)
            {
            xmin = sites[i].coord.x ;
            }
        if (sites[i].coord.x > xmax)
            {
            xmax = sites[i].coord.x ;
            }
        }
    ymin = sites[0].coord.y ;
    ymax = sites[nsites-1].coord.y ;
    }

/*** read one site ***/

Site *
readone(void)
    {
    Site * s ;

    s = (Site *)getfree(&sfl) ;
    s->refcnt = 0 ;
    s->sitenbr = siteidx++ ;
    if (scanf("%f %f", &(s->coord.x), &(s->coord.y)) == EOF)
        {
        return ((Site *)NULL ) ;
        }
    return (s) ;
    }

