
/*** GEOMETRY.C ***/

#include <math.h>
#include "vdefs.h"

float deltax, deltay ;
int nedges, sqrt_nsites, nvertices ;
Freelist efl ;

void
geominit(void)
    {
    freeinit(&efl, sizeof(Edge)) ;
    nvertices = nedges = 0 ;
    sqrt_nsites = sqrt(nsites+4) ;
    deltay = ymax - ymin ;
    deltax = xmax - xmin ;
    }

Edge *
bisect(Site * s1, Site * s2)
    {
    float dx, dy, adx, ady ;
    Edge * newedge ;

    newedge = (Edge *)getfree(&efl) ;
    newedge->reg[0] = s1 ;
    newedge->reg[1] = s2 ;
    ref(s1) ;
    ref(s2) ;
    newedge->ep[0] = newedge->ep[1] = (Site *)NULL ;
    dx = s2->coord.x - s1->coord.x ;
    dy = s2->coord.y - s1->coord.y ;
    adx = dx>0 ? dx : -dx ;
    ady = dy>0 ? dy : -dy ;
    newedge->c = s1->coord.x * dx + s1->coord.y * dy + (dx*dx +
dy*dy) * 0.5 ;
    if (adx > ady)
        {
        newedge->a = 1.0 ;
        newedge->b = dy/dx ;
        newedge->c /= dx ;
        }
    else
        {
        newedge->b = 1.0 ;
        newedge->a = dx/dy ;
        newedge->c /= dy ;
        }
    newedge->edgenbr = nedges ;
    out_bisector(newedge) ;
    nedges++ ;
    return (newedge) ;
    }

Site *
intersect(Halfedge * el1, Halfedge * el2)
    {
    Edge * e1, * e2, * e ;
    Halfedge * el ;
    float d, xint, yint ;
    int right_of_site ;
    Site * v ;

    e1 = el1->ELedge ;
    e2 = el2->ELedge ;
    if ((e1 == (Edge*)NULL) || (e2 == (Edge*)NULL))
        {
        return ((Site *)NULL) ;
        }
    if (e1->reg[1] == e2->reg[1])
        {
        return ((Site *)NULL) ;
        }
    d = (e1->a * e2->b) - (e1->b * e2->a) ;
    if ((-1.0e-10 < d) && (d < 1.0e-10))
        {
        return ((Site *)NULL) ;
        }
    xint = (e1->c * e2->b - e2->c * e1->b) / d ;
    yint = (e2->c * e1->a - e1->c * e2->a) / d ;
    if ((e1->reg[1]->coord.y < e2->reg[1]->coord.y) ||
        (e1->reg[1]->coord.y == e2->reg[1]->coord.y &&
        e1->reg[1]->coord.x < e2->reg[1]->coord.x))
        {
        el = el1 ;
        e = e1 ;
        }
    else
        {
        el = el2 ;
        e = e2 ;
        }
    right_of_site = (xint >= e->reg[1]->coord.x) ;
    if ((right_of_site && (el->ELpm == le)) ||
       (!right_of_site && (el->ELpm == re)))
        {
        return ((Site *)NULL) ;
        }
    v = (Site *)getfree(&sfl) ;
    v->refcnt = 0 ;
    v->coord.x = xint ;
    v->coord.y = yint ;
    return (v) ;
    }

/*** returns 1 if p is to right of halfedge e ***/

int
right_of(Halfedge * el, Point * p)
    {
    Edge * e ;
    Site * topsite ;
    int right_of_site, above, fast ;
    float dxp, dyp, dxs, t1, t2, t3, yl ;

    e = el->ELedge ;
    topsite = e->reg[1] ;
    right_of_site = (p->x > topsite->coord.x) ;
    if (right_of_site && (el->ELpm == le))
        {
        return (1) ;
        }
    if(!right_of_site && (el->ELpm == re))
        {
        return (0) ;
        }
    if (e->a == 1.0)
        {
        dyp = p->y - topsite->coord.y ;
        dxp = p->x - topsite->coord.x ;
        fast = 0 ;
        if ((!right_of_site & (e->b < 0.0)) ||
         (right_of_site & (e->b >= 0.0)))
            {
            fast = above = (dyp >= e->b*dxp) ;
            }
        else
            {
            above = ((p->x + p->y * e->b) > (e->c)) ;
            if (e->b < 0.0)
                {
                above = !above ;
                }
            if (!above)
                {
                fast = 1 ;
                }
            }
        if (!fast)
            {
            dxs = topsite->coord.x - (e->reg[0])->coord.x ;
            above = (e->b * (dxp*dxp - dyp*dyp))
                    <
                    (dxs * dyp * (1.0 + 2.0 * dxp /
                    dxs + e->b * e->b)) ;
            if (e->b < 0.0)
                {
                above = !above ;
                }
            }
        }
    else  /*** e->b == 1.0 ***/
        {
        yl = e->c - e->a * p->x ;
        t1 = p->y - yl ;
        t2 = p->x - topsite->coord.x ;
        t3 = yl - topsite->coord.y ;
        above = ((t1*t1) > ((t2 * t2) + (t3 * t3))) ;
        }
    return (el->ELpm == le ? above : !above) ;
    }

void
endpoint(Edge * e, int lr, Site * s)
    {
    e->ep[lr] = s ;
    ref(s) ;
    if (e->ep[re-lr] == (Site *)NULL)
        {
        return ;
        }
    out_ep(e) ;
    deref(e->reg[le]) ;
    deref(e->reg[re]) ;
    makefree((Freenode *)e, (Freelist *) &efl) ;
    }

float
dist(Site * s, Site * t)
    {
    float dx,dy ;

    dx = s->coord.x - t->coord.x ;
    dy = s->coord.y - t->coord.y ;
    return (sqrt(dx*dx + dy*dy)) ;
    }

void
makevertex(Site * v)
    {
    v->sitenbr = nvertices++ ;
    out_vertex(v) ;
    }

void
deref(Site * v)
    {
    if (--(v->refcnt) == 0 )
        {
        makefree((Freenode *)v, (Freelist *)&sfl) ;
        }
    }

void
ref(Site * v)
    {
    ++(v->refcnt) ;
    }

