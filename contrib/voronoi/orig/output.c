
/*** OUTPUT.C ***/


#include <stdio.h>

#include "vdefs.h"

extern int triangulate, plot, debug ;
extern float ymax, ymin, xmax, xmin ;

float pxmin, pxmax, pymin, pymax, cradius;

void
openpl(void)
    {
    }

#pragma argsused
void
line(float ax, float ay, float bx, float by)
    {
    }

#pragma argsused
void
circle(float ax, float ay, float radius)
    {
    }

#pragma argsused
void
range(float pxmin, float pxmax, float pymin, float pymax)
    {
    }

void
out_bisector(Edge * e)
    {
    if (triangulate && plot && !debug)
        {
        line(e->reg[0]->coord.x, e->reg[0]->coord.y,
             e->reg[1]->coord.x, e->reg[1]->coord.y) ;
        }
    if (!triangulate && !plot && !debug)
        {
        printf("l %f %f %f\n", e->a, e->b, e->c) ;
        }
    if (debug)
        {
        printf("line(%d) %gx+%gy=%g, bisecting %d %d\n", e->edgenbr,
        e->a, e->b, e->c, e->reg[le]->sitenbr, e->reg[re]->sitenbr) ;
        }
    }

void
out_ep(Edge * e)
    {
    if (!triangulate && plot)
        {
        clip_line(e) ;
        }
    if (!triangulate && !plot)
        {
        printf("e %d", e->edgenbr);
        printf(" %d ", e->ep[le] != (Site *)NULL ? e->ep[le]->sitenbr : -1) ;
        printf("%d\n", e->ep[re] != (Site *)NULL ? e->ep[re]->sitenbr : -1) ;
        }
    }

void
out_vertex(Site * v)
    {
    if (!triangulate && !plot && !debug)
        {
        printf ("v %f %f\n", v->coord.x, v->coord.y) ;
        }
    if (debug)
        {
        printf("vertex(%d) at %f %f\n", v->sitenbr, v->coord.x, v->coord.y) ;
        }
    }

void
out_site(Site * s)
    {
    if (!triangulate && plot && !debug)
        {
        circle (s->coord.x, s->coord.y, cradius) ;
        }
    if (!triangulate && !plot && !debug)
        {
        printf("s %f %f\n", s->coord.x, s->coord.y) ;
        }
    if (debug)
        {
        printf("site (%d) at %f %f\n", s->sitenbr, s->coord.x, s->coord.y) ;
        }
    }

void
out_triple(Site * s1, Site * s2, Site * s3)
    {
    if (triangulate && !plot && !debug)
        {
        printf("%d %d %d\n", s1->sitenbr, s2->sitenbr, s3->sitenbr) ;
        }
    if (debug)
        {
        printf("circle through left=%d right=%d bottom=%d\n",
        s1->sitenbr, s2->sitenbr, s3->sitenbr) ;
        }
    }

void
plotinit(void)
    {
    float dx, dy, d ;

    dy = ymax - ymin ;
    dx = xmax - xmin ;
    d = ( dx > dy ? dx : dy) * 1.1 ;
    pxmin = xmin - (d-dx) / 2.0 ;
    pxmax = xmax + (d-dx) / 2.0 ;
    pymin = ymin - (d-dy) / 2.0 ;
    pymax = ymax + (d-dy) / 2.0 ;
    cradius = (pxmax - pxmin) / 350.0 ;
    openpl() ;
    range(pxmin, pymin, pxmax, pymax) ;
    }

void
clip_line(Edge * e)
    {
    Site * s1, * s2 ;
    float x1, x2, y1, y2 ;

    if (e->a == 1.0 && e->b >= 0.0)
        {
        s1 = e->ep[1] ;
        s2 = e->ep[0] ;
        }
    else
        {
        s1 = e->ep[0] ;
        s2 = e->ep[1] ;
        }
    if (e->a == 1.0)
        {
        y1 = pymin ;
        if (s1 != (Site *)NULL && s1->coord.y > pymin)
            {
             y1 = s1->coord.y ;
             }
        if (y1 > pymax)
            {
            return ;
            }
        x1 = e->c - e->b * y1 ;
        y2 = pymax ;
        if (s2 != (Site *)NULL && s2->coord.y < pymax)
            {
            y2 = s2->coord.y ;
            }
        if (y2 < pymin)
            {
            return ;
            }
        x2 = e->c - e->b * y2 ;
        if (((x1 > pxmax) && (x2 > pxmax)) || ((x1 < pxmin) && (x2 < pxmin)))
            {
            return ;
            }
        if (x1 > pxmax)
            {
            x1 = pxmax ;
            y1 = (e->c - x1) / e->b ;
            }
        if (x1 < pxmin)
            {
            x1 = pxmin ;
            y1 = (e->c - x1) / e->b ;
            }
        if (x2 > pxmax)
            {
            x2 = pxmax ;
            y2 = (e->c - x2) / e->b ;
            }
        if (x2 < pxmin)
            {
            x2 = pxmin ;
            y2 = (e->c - x2) / e->b ;
            }
        }
    else
        {
        x1 = pxmin ;
        if (s1 != (Site *)NULL && s1->coord.x > pxmin)
            {
            x1 = s1->coord.x ;
            }
        if (x1 > pxmax)
            {
            return ;
            }
        y1 = e->c - e->a * x1 ;
        x2 = pxmax ;
        if (s2 != (Site *)NULL && s2->coord.x < pxmax)
            {
            x2 = s2->coord.x ;
            }
        if (x2 < pxmin)
            {
            return ;
            }
        y2 = e->c - e->a * x2 ;
        if (((y1 > pymax) && (y2 > pymax)) || ((y1 < pymin) && (y2 <pymin)))
            {
            return ;
            }
        if (y1> pymax)
            {
            y1 = pymax ;
            x1 = (e->c - y1) / e->a ;
            }
        if (y1 < pymin)
            {
            y1 = pymin ;
            x1 = (e->c - y1) / e->a ;
            }
        if (y2 > pymax)
            {
            y2 = pymax ;
            x2 = (e->c - y2) / e->a ;
            }
        if (y2 < pymin)
            {
            y2 = pymin ;
            x2 = (e->c - y2) / e->a ;
            }
        }
    line(x1,y1,x2,y2);
    }

