/* refactor of Steven Future's algorithm from original C to C++ class */

#include <QList>
#include <QRectF>
#include <QPointF>
#include <QLineF>

#ifndef __VDEFS_H
#define __VDEFS_H 1

#ifndef NULL
#define NULL 0
#endif

#define DELETED -2

typedef struct tagFreenode
    {
    struct tagFreenode * nextfree;
    } Freenode ;


typedef struct tagFreelist
    {
    Freenode * head;
    int nodesize;
    } Freelist ;

typedef struct tagPoint
    {
    float x ;
    float y ;
    } Point ;

/* structure used both for sites and for vertices */

typedef struct tagSite
    {
    Point coord ;
    int sitenbr ;
    int refcnt ;
    } Site ;


typedef struct tagEdge
    {
    float a, b, c ;
    Site * ep[2] ;
    Site * reg[2] ;
    int edgenbr ;
    } Edge ;

// renamed from original re and le as they clash
#define voronoi_le 0
#define voronoi_re 1

typedef struct tagHalfedge
    {
    struct tagHalfedge * ELleft ;
    struct tagHalfedge * ELright ;
    Edge * ELedge ;
    int ELrefcnt ;
    char ELpm ;
    Site * vertex ;
    float ystar ;
    struct tagHalfedge * PQnext ;
    } Halfedge ;


class Voronoi {

    public:

        Voronoi();
        ~Voronoi();

        void addSite(QPointF point);
        void run(QRectF boundingRect);

        // the output is a vector of lines to draw
        QList<QLineF> &lines() { return output; }

    private:

        // original global variables
        int triangulate, plot, debug, nsites, siteidx ;
        float xmin, xmax, ymin, ymax ;

        int ELhashsize ;
        Site * bottomsite ;
        Freelist hfl ;
        Halfedge * ELleftend, * ELrightend, **ELhash ;
        int ntry, totalsearch ;
        float deltax, deltay ;
        int nedges, sqrt_nsites, nvertices ;
        Freelist sfl ;
        Freelist efl ;
        int PQmin, PQcount, PQhashsize ;
        Halfedge * PQhash ;
        int total_alloc ;
        float pxmin, pxmax, pymin, pymax, cradius;

        // refactoring to Qt containers
        QList<void *> malloclist; // keep tabs on all the malloc'ed memory
        QList<Site*> sites;
        QList<QLineF> output;

        /*** implicit parameters: nsites, sqrt_nsites, xmin, xmax, ymin, ymax,
         : deltax, deltay (can all be estimates).
         : Performance suffers if they are wrong; better to make nsites,
         : deltax, and deltay too big than too small.  (?)
         ***/

        // DCEL routines
        void ELinitialize(void);
        Halfedge * HEcreate(Edge * e, int pm);
        void ELinsert(Halfedge * lb, Halfedge * newone);
        Halfedge * ELgethash(int b);
        Halfedge * ELleftbnd(Point * p);
        void ELdelete(Halfedge * he);
        Halfedge * ELright(Halfedge * he);
        Halfedge * ELleft(Halfedge * he);
        Site * leftreg(Halfedge * he);
        Site * rightreg(Halfedge * he);

        // geometry
        void geominit(void);
        Edge * bisect(Site * s1, Site * s2);
        Site * intersect(Halfedge * el1, Halfedge * el2);
        int right_of(Halfedge * el, Point * p);
        void endpoint(Edge * e, int lr, Site * s);
        float dist(Site * s, Site * t);
        void makevertex(Site * v);
        void deref(Site * v);
        void ref(Site * v);
        void PQinsert(Halfedge * he, Site * v, float offset);
        void PQdelete(Halfedge * he);
        int PQbucket(Halfedge * he);
        int PQempty(void);
        Point PQ_min(void);
        Halfedge * PQextractmin(void);
        void PQinitialize(void);

        // c memory management functions
        void freeinit(Freelist * fl, int size);
        char * getfree(Freelist * fl);
        void makefree(Freenode * curr, Freelist * fl);
        char * myalloc(unsigned n);

        // output functions (not used in GC)
        void openpl(void);
        void line(float ax, float ay, float bx, float by);
        void circle(float ax, float ay, float radius);
        void range(float pxmin, float pxmax, float pymin, float pymax);
        void out_bisector(Edge * e);
        void out_ep(Edge * e);
        void out_vertex(Site * v);
        void out_site(Site * s);
        void out_triple(Site * s1, Site * s2, Site * s3);
        void plotinit(void);
        void clip_line(Edge * e);

};
#endif
