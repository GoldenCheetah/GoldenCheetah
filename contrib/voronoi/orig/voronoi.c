
/*** VORONOI.C ***/

#include "vdefs.h"

extern Site * bottomsite ;
extern Halfedge * ELleftend, * ELrightend ;

/*** implicit parameters: nsites, sqrt_nsites, xmin, xmax, ymin, ymax,
 : deltax, deltay (can all be estimates).
 : Performance suffers if they are wrong; better to make nsites,
 : deltax, and deltay too big than too small.  (?)
 ***/

void
voronoi(Site *(*nextsite)(void))
    {
    Site * newsite, * bot, * top, * temp, * p, * v ;
    Point newintstar ;
    int pm ;
    Halfedge * lbnd, * rbnd, * llbnd, * rrbnd, * bisector ;
    Edge * e ;

    PQinitialize() ;
    bottomsite = (*nextsite)() ;
    out_site(bottomsite) ;
    ELinitialize() ;
    newsite = (*nextsite)() ;
    while (1)
        {
        if(!PQempty())
            {
            newintstar = PQ_min() ;
            }
        if (newsite != (Site *)NULL && (PQempty()
            || newsite -> coord.y < newintstar.y
            || (newsite->coord.y == newintstar.y
            && newsite->coord.x < newintstar.x))) {/* new site is
smallest */
            {
            out_site(newsite) ;
            }
        lbnd = ELleftbnd(&(newsite->coord)) ;
        rbnd = ELright(lbnd) ;
        bot = rightreg(lbnd) ;
        e = bisect(bot, newsite) ;
        bisector = HEcreate(e, le) ;
        ELinsert(lbnd, bisector) ;
        p = intersect(lbnd, bisector) ;
        if (p != (Site *)NULL)
            {
            PQdelete(lbnd) ;
            PQinsert(lbnd, p, dist(p,newsite)) ;
            }
        lbnd = bisector ;
        bisector = HEcreate(e, re) ;
        ELinsert(lbnd, bisector) ;
        p = intersect(bisector, rbnd) ;
        if (p != (Site *)NULL)
            {
            PQinsert(bisector, p, dist(p,newsite)) ;
            }
        newsite = (*nextsite)() ;
        }
    else if (!PQempty())   /* intersection is smallest */
            {
            lbnd = PQextractmin() ;
            llbnd = ELleft(lbnd) ;
            rbnd = ELright(lbnd) ;
            rrbnd = ELright(rbnd) ;
            bot = leftreg(lbnd) ;
            top = rightreg(rbnd) ;
            out_triple(bot, top, rightreg(lbnd)) ;
            v = lbnd->vertex ;
            makevertex(v) ;
            endpoint(lbnd->ELedge, lbnd->ELpm, v);
            endpoint(rbnd->ELedge, rbnd->ELpm, v) ;
            ELdelete(lbnd) ;
            PQdelete(rbnd) ;
            ELdelete(rbnd) ;
            pm = le ;
            if (bot->coord.y > top->coord.y)
                {
                temp = bot ;
                bot = top ;
                top = temp ;
                pm = re ;
                }
            e = bisect(bot, top) ;
            bisector = HEcreate(e, pm) ;
            ELinsert(llbnd, bisector) ;
            endpoint(e, re-pm, v) ;
            deref(v) ;
            p = intersect(llbnd, bisector) ;
            if (p  != (Site *) NULL)
                {
                PQdelete(llbnd) ;
                PQinsert(llbnd, p, dist(p,bot)) ;
                }
            p = intersect(bisector, rrbnd) ;
            if (p != (Site *) NULL)
                {
                PQinsert(bisector, p, dist(p,bot)) ;
                }
            }
        else
            {
            break ;
            }
        }

    for( lbnd = ELright(ELleftend) ;
         lbnd != ELrightend ;
         lbnd = ELright(lbnd))
        {
        e = lbnd->ELedge ;
        out_ep(e) ;
        }
    }

