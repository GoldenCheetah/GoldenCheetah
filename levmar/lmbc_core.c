/////////////////////////////////////////////////////////////////////////////////
// 
//  Levenberg - Marquardt non-linear minimization algorithm
//  Copyright (C) 2004-05  Manolis Lourakis (lourakis at ics forth gr)
//  Institute of Computer Science, Foundation for Research & Technology - Hellas
//  Heraklion, Crete, Greece.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
/////////////////////////////////////////////////////////////////////////////////

#ifndef LM_REAL // not included by lmbc.c
#error This file should not be compiled directly!
#endif


/* precision-specific definitions */
#define FUNC_STATE LM_ADD_PREFIX(func_state)
#define LNSRCH LM_ADD_PREFIX(lnsrch)
#define BOXPROJECT LM_ADD_PREFIX(boxProject)
#define BOXSCALE LM_ADD_PREFIX(boxScale)
#define LEVMAR_BOX_CHECK LM_ADD_PREFIX(levmar_box_check)
#define VECNORM LM_ADD_PREFIX(vecnorm)
#define LEVMAR_BC_DER LM_ADD_PREFIX(levmar_bc_der)
#define LEVMAR_BC_DIF LM_ADD_PREFIX(levmar_bc_dif)
#define LEVMAR_FDIF_FORW_JAC_APPROX LM_ADD_PREFIX(levmar_fdif_forw_jac_approx)
#define LEVMAR_FDIF_CENT_JAC_APPROX LM_ADD_PREFIX(levmar_fdif_cent_jac_approx)
#define LEVMAR_TRANS_MAT_MAT_MULT LM_ADD_PREFIX(levmar_trans_mat_mat_mult)
#define LEVMAR_L2NRMXMY LM_ADD_PREFIX(levmar_L2nrmxmy)
#define LEVMAR_COVAR LM_ADD_PREFIX(levmar_covar)
#define LMBC_DIF_DATA LM_ADD_PREFIX(lmbc_dif_data)
#define LMBC_DIF_FUNC LM_ADD_PREFIX(lmbc_dif_func)
#define LMBC_DIF_JACF LM_ADD_PREFIX(lmbc_dif_jacf)

#ifdef HAVE_LAPACK
#define AX_EQ_B_LU LM_ADD_PREFIX(Ax_eq_b_LU)
#define AX_EQ_B_CHOL LM_ADD_PREFIX(Ax_eq_b_Chol)
#define AX_EQ_B_QR LM_ADD_PREFIX(Ax_eq_b_QR)
#define AX_EQ_B_QRLS LM_ADD_PREFIX(Ax_eq_b_QRLS)
#define AX_EQ_B_SVD LM_ADD_PREFIX(Ax_eq_b_SVD)
#define AX_EQ_B_BK LM_ADD_PREFIX(Ax_eq_b_BK)
#else
#define AX_EQ_B_LU LM_ADD_PREFIX(Ax_eq_b_LU_noLapack)
#endif /* HAVE_LAPACK */

#ifdef HAVE_PLASMA
#define AX_EQ_B_PLASMA_CHOL LM_ADD_PREFIX(Ax_eq_b_PLASMA_Chol)
#endif

/* find the median of 3 numbers */
#define __MEDIAN3(a, b, c) ( ((a) >= (b))?\
        ( ((c) >= (a))? (a) : ( ((c) <= (b))? (b) : (c) ) ) : \
        ( ((c) >= (b))? (b) : ( ((c) <= (a))? (a) : (c) ) ) )

/* Projections to feasible set \Omega: P_{\Omega}(y) := arg min { ||x - y|| : x \in \Omega},  y \in R^m */

/* project vector p to a box shaped feasible set. p is a mx1 vector.
 * Either lb, ub can be NULL. If not NULL, they are mx1 vectors
 */
static void BOXPROJECT(LM_REAL *p, LM_REAL *lb, LM_REAL *ub, int m)
{
register int i;

  if(!lb){ /* no lower bounds */
    if(!ub) /* no upper bounds */
      return;
    else{ /* upper bounds only */
      for(i=m; i-->0; )
        if(p[i]>ub[i]) p[i]=ub[i];
    }
  }
  else
    if(!ub){ /* lower bounds only */
      for(i=m; i-->0; )
        if(p[i]<lb[i]) p[i]=lb[i];
    }
    else /* box bounds */
      for(i=m; i-->0; )
        p[i]=__MEDIAN3(lb[i], p[i], ub[i]);
}
#undef __MEDIAN3

/* pointwise scaling of bounds with the mx1 vector scl. If div=1 scaling is by 1./scl.
 * Either lb, ub can be NULL. If not NULL, they are mx1 vectors
 */
static void BOXSCALE(LM_REAL *lb, LM_REAL *ub, LM_REAL *scl, int m, int div)
{
register int i;

  if(!lb){ /* no lower bounds */
    if(!ub) /* no upper bounds */
      return;
    else{ /* upper bounds only */
      if(div){
        for(i=m; i-->0; )
          if(ub[i]!=LM_REAL_MAX)
            ub[i]=ub[i]/scl[i];
      }else{
        for(i=m; i-->0; )
          if(ub[i]!=LM_REAL_MAX)
            ub[i]=ub[i]*scl[i];
      }
    }
  }
  else
    if(!ub){ /* lower bounds only */
      if(div){
        for(i=m; i-->0; )
          if(lb[i]!=LM_REAL_MIN)
            lb[i]=lb[i]/scl[i];
      }else{
        for(i=m; i-->0; )
          if(lb[i]!=LM_REAL_MIN)
            lb[i]=lb[i]*scl[i];
      }
    }
    else{ /* box bounds */
      if(div){
        for(i=m; i-->0; ){
          if(ub[i]!=LM_REAL_MAX)
            ub[i]=ub[i]/scl[i];
          if(lb[i]!=LM_REAL_MIN)
            lb[i]=lb[i]/scl[i];
        }
      }else{
        for(i=m; i-->0; ){
          if(ub[i]!=LM_REAL_MAX)
            ub[i]=ub[i]*scl[i];
          if(lb[i]!=LM_REAL_MIN)
            lb[i]=lb[i]*scl[i];
        }
      }
    }
}

/* compute the norm of a vector in a manner that avoids overflows
 */
static LM_REAL VECNORM(LM_REAL *x, int n)
{
#ifdef HAVE_LAPACK
#define NRM2 LM_MK_BLAS_NAME(nrm2)
extern LM_REAL NRM2(int *n, LM_REAL *dx, int *incx);
int one=1;

  return NRM2(&n, x, &one);
#undef NRM2
#else // no LAPACK, use the simple method described by Blue in TOMS78
register int i;
LM_REAL max, sum, tmp;

  for(i=n, max=0.0; i-->0; )
    if(x[i]>max) max=x[i];
    else if(x[i]<-max) max=-x[i];

  for(i=n, sum=0.0; i-->0; ){
    tmp=x[i]/max;
    sum+=tmp*tmp;
  }

  return max*(LM_REAL)sqrt(sum);
#endif /* HAVE_LAPACK */
}

struct FUNC_STATE{
  int n, *nfev;
  LM_REAL *hx, *x;
  LM_REAL *lb, *ub;
  void *adata;
};

static void
LNSRCH(int m, LM_REAL *x, LM_REAL f, LM_REAL *g, LM_REAL *p, LM_REAL alpha, LM_REAL *xpls,
       LM_REAL *ffpls, void (*func)(LM_REAL *p, LM_REAL *hx, int m, int n, void *adata), struct FUNC_STATE *state,
       int *mxtake, int *iretcd, LM_REAL stepmx, LM_REAL steptl, LM_REAL *sx)
{
/* Find a next newton iterate by backtracking line search.
 * Specifically, finds a \lambda such that for a fixed alpha<0.5 (usually 1e-4),
 * f(x + \lambda*p) <= f(x) + alpha * \lambda * g^T*p
 *
 * Translated (with a few changes) from Schnabel, Koontz & Weiss uncmin.f,  v1.3
 * Main changes include the addition of box projection and modification of the scaling 
 * logic since uncmin.f operates in the original (unscaled) variable space.

 * PARAMETERS :

 *	m       --> dimension of problem (i.e. number of variables)
 *	x(m)    --> old iterate:	x[k-1]
 *	f       --> function value at old iterate, f(x)
 *	g(m)    --> gradient at old iterate, g(x), or approximate
 *	p(m)    --> non-zero newton step
 *	alpha   --> fixed constant < 0.5 for line search (see above)
 *	xpls(m) <--	 new iterate x[k]
 *	ffpls   <--	 function value at new iterate, f(xpls)
 *	func    --> name of subroutine to evaluate function
 *	state   <--> information other than x and m that func requires.
 *			    state is not modified in xlnsrch (but can be modified by func).
 *	iretcd  <--	 return code
 *	mxtake  <--	 boolean flag indicating step of maximum length used
 *	stepmx  --> maximum allowable step size
 *	steptl  --> relative step size at which successive iterates
 *			    considered close enough to terminate algorithm
 *	sx(m)	  --> diagonal scaling matrix for x, can be NULL

 *	internal variables

 *	sln		 newton length
 *	rln		 relative length of newton step
*/

    register int i, j;
    int firstback = 1;
    LM_REAL disc;
    LM_REAL a3, b;
    LM_REAL t1, t2, t3, lambda, tlmbda, rmnlmb;
    LM_REAL scl, rln, sln, slp;
    LM_REAL tmp1, tmp2;
    LM_REAL fpls, pfpls = 0., plmbda = 0.; /* -Wall */

    f*=LM_CNST(0.5);
    *mxtake = 0;
    *iretcd = 2;
    tmp1 = 0.;
    for (i = m; i-- > 0;  )
      tmp1 += p[i] * p[i];
    sln = (LM_REAL)sqrt(tmp1);
    if (sln > stepmx) {
	  /*	newton step longer than maximum allowed */
	    scl = stepmx / sln;
      for (i = m; i-- > 0;  ) /* p * scl */
        p[i]*=scl;
	    sln = stepmx;
    }
    for (i = m, slp = rln = 0.; i-- > 0;  ){
      slp+=g[i]*p[i]; /* g^T * p */

      tmp1 = (FABS(x[i])>=LM_CNST(1.))? FABS(x[i]) : LM_CNST(1.);
      tmp2 = FABS(p[i])/tmp1;
      if(rln < tmp2) rln = tmp2;
    }
    rmnlmb = steptl / rln;
    lambda = LM_CNST(1.0);

    /*	check if new iterate satisfactory.  generate new lambda if necessary. */

    for(j = _LSITMAX_; j-- > 0;  ) {
      for (i = m; i-- > 0;  )
        xpls[i] = x[i] + lambda * p[i];
      BOXPROJECT(xpls, state->lb, state->ub, m); /* project to feasible set */

      /* evaluate function at new point */
      if(!sx){
        (*func)(xpls, state->hx, m, state->n, state->adata); ++(*(state->nfev));
      }
      else{
        for (i = m; i-- > 0;  ) xpls[i] *= sx[i];
        (*func)(xpls, state->hx, m, state->n, state->adata); ++(*(state->nfev));
        for (i = m; i-- > 0;  ) xpls[i] /= sx[i];
      }
      /* ### state->hx=state->x-state->hx, tmp1=||state->hx|| */
#if 1
       tmp1=LEVMAR_L2NRMXMY(state->hx, state->x, state->hx, state->n);
#else
      for(i=0, tmp1=0.0; i<state->n; ++i){
        state->hx[i]=tmp2=state->x[i]-state->hx[i];
        tmp1+=tmp2*tmp2;
      }
#endif
      fpls=LM_CNST(0.5)*tmp1; *ffpls=tmp1;

	    if (fpls <= f + slp * alpha * lambda) { /* solution found */
	      *iretcd = 0;
	      if (lambda == LM_CNST(1.) && sln > stepmx * LM_CNST(.99)) *mxtake = 1;
	      return;
	    }

	    /* else : solution not (yet) found */

      /* First find a point with a finite value */

	    if (lambda < rmnlmb) {
	      /* no satisfactory xpls found sufficiently distinct from x */

	      *iretcd = 1;
	      return;
	    }
	    else { /*	calculate new lambda */

	      /* modifications to cover non-finite values */
	      if (!LM_FINITE(fpls)) {
		      lambda *= LM_CNST(0.1);
		      firstback = 1;
	      }
	      else {
		      if (firstback) { /*	first backtrack: quadratic fit */
		        tlmbda = -lambda * slp / ((fpls - f - slp) * LM_CNST(2.));
		        firstback = 0;
		      }
		      else { /*	all subsequent backtracks: cubic fit */
		        t1 = fpls - f - lambda * slp;
		        t2 = pfpls - f - plmbda * slp;
		        t3 = LM_CNST(1.) / (lambda - plmbda);
		        a3 = LM_CNST(3.) * t3 * (t1 / (lambda * lambda)
				      - t2 / (plmbda * plmbda));
		        b = t3 * (t2 * lambda / (plmbda * plmbda)
			          - t1 * plmbda / (lambda * lambda));
		        disc = b * b - a3 * slp;
		        if (disc > b * b)
			      /* only one positive critical point, must be minimum */
			        tlmbda = (-b + ((a3 < 0)? -(LM_REAL)sqrt(disc): (LM_REAL)sqrt(disc))) /a3;
		        else
			      /* both critical points positive, first is minimum */
			        tlmbda = (-b + ((a3 < 0)? (LM_REAL)sqrt(disc): -(LM_REAL)sqrt(disc))) /a3;

		        if (tlmbda > lambda * LM_CNST(.5))
			        tlmbda = lambda * LM_CNST(.5);
		      }
		      plmbda = lambda;
		      pfpls = fpls;
		      if (tlmbda < lambda * LM_CNST(.1))
		        lambda *= LM_CNST(.1);
		      else
		        lambda = tlmbda;
        }
	    }
    }
    /* this point is reached when the iterations limit is exceeded */
	  *iretcd = 1; /* failed */
	  return;
} /* LNSRCH */

/* 
 * This function seeks the parameter vector p that best describes the measurements
 * vector x under box constraints.
 * More precisely, given a vector function  func : R^m --> R^n with n>=m,
 * it finds p s.t. func(p) ~= x, i.e. the squared second order (i.e. L2) norm of
 * e=x-func(p) is minimized under the constraints lb[i]<=p[i]<=ub[i].
 * If no lower bound constraint applies for p[i], use -DBL_MAX/-FLT_MAX for lb[i];
 * If no upper bound constraint applies for p[i], use DBL_MAX/FLT_MAX for ub[i].
 *
 * This function requires an analytic Jacobian. In case the latter is unavailable,
 * use LEVMAR_BC_DIF() bellow
 *
 * Returns the number of iterations (>=0) if successful, LM_ERROR if failed
 *
 * For details, see C. Kanzow, N. Yamashita and M. Fukushima: "Levenberg-Marquardt
 * methods for constrained nonlinear equations with strong local convergence properties",
 * Journal of Computational and Applied Mathematics 172, 2004, pp. 375-397.
 * Also, see K. Madsen, H.B. Nielsen and O. Tingleff's lecture notes on 
 * unconstrained Levenberg-Marquardt at http://www.imm.dtu.dk/pubdb/views/edoc_download.php/3215/pdf/imm3215.pdf
 *
 * The algorithm implemented by this function employs projected gradient steps. Since steepest descent
 * is very sensitive to poor scaling, diagonal scaling has been implemented through the dscl argument:
 * Instead of minimizing f(p) for p, f(D*q) is minimized for q=D^-1*p, D being a diagonal scaling
 * matrix whose diagonal equals dscl (see Nocedal-Wright p.27). dscl should contain "typical" magnitudes 
 * for the parameters p. A NULL value for dscl implies no scaling. i.e. D=I.
 * To account for scaling, the code divides the starting point and box bounds pointwise by dscl. Moreover,
 * before calling func and jacf the scaling has to be undone (by multiplying), as should be done with
 * the final point. Note also that jac_q=jac_p*D, where jac_q, jac_p are the jacobians w.r.t. q & p, resp.
 */

int LEVMAR_BC_DER(
  void (*func)(LM_REAL *p, LM_REAL *hx, int m, int n, void *adata), /* functional relation describing measurements. A p \in R^m yields a \hat{x} \in  R^n */
  void (*jacf)(LM_REAL *p, LM_REAL *j, int m, int n, void *adata),  /* function to evaluate the Jacobian \part x / \part p */ 
  LM_REAL *p,         /* I/O: initial parameter estimates. On output has the estimated solution */
  LM_REAL *x,         /* I: measurement vector. NULL implies a zero vector */
  int m,              /* I: parameter vector dimension (i.e. #unknowns) */
  int n,              /* I: measurement vector dimension */
  LM_REAL *lb,        /* I: vector of lower bounds. If NULL, no lower bounds apply */
  LM_REAL *ub,        /* I: vector of upper bounds. If NULL, no upper bounds apply */
  LM_REAL *dscl,      /* I: diagonal scaling constants. NULL implies no scaling */
  int itmax,          /* I: maximum number of iterations */
  LM_REAL opts[4],    /* I: minim. options [\mu, \epsilon1, \epsilon2, \epsilon3]. Respectively the scale factor for initial \mu,
                       * stopping thresholds for ||J^T e||_inf, ||Dp||_2 and ||e||_2. Set to NULL for defaults to be used.
                       * Note that ||J^T e||_inf is computed on free (not equal to lb[i] or ub[i]) variables only.
                       */
  LM_REAL info[LM_INFO_SZ],
					           /* O: information regarding the minimization. Set to NULL if don't care
                      * info[0]= ||e||_2 at initial p.
                      * info[1-4]=[ ||e||_2, ||J^T e||_inf,  ||Dp||_2, mu/max[J^T J]_ii ], all computed at estimated p.
                      * info[5]= # iterations,
                      * info[6]=reason for terminating: 1 - stopped by small gradient J^T e
                      *                                 2 - stopped by small Dp
                      *                                 3 - stopped by itmax
                      *                                 4 - singular matrix. Restart from current p with increased mu 
                      *                                 5 - no further error reduction is possible. Restart with increased mu
                      *                                 6 - stopped by small ||e||_2
                      *                                 7 - stopped by invalid (i.e. NaN or Inf) "func" values. This is a user error
                      * info[7]= # function evaluations
                      * info[8]= # Jacobian evaluations
                      * info[9]= # linear systems solved, i.e. # attempts for reducing error
                      */
  LM_REAL *work,     /* working memory at least LM_BC_DER_WORKSZ() reals large, allocated if NULL */
  LM_REAL *covar,    /* O: Covariance matrix corresponding to LS solution; mxm. Set to NULL if not needed. */
  void *adata)       /* pointer to possibly additional data, passed uninterpreted to func & jacf.
                      * Set to NULL if not needed
                      */
{
register int i, j, k, l;
int worksz, freework=0, issolved;
/* temp work arrays */
LM_REAL *e,          /* nx1 */
       *hx,         /* \hat{x}_i, nx1 */
       *jacTe,      /* J^T e_i mx1 */
       *jac,        /* nxm */
       *jacTjac,    /* mxm */
       *Dp,         /* mx1 */
   *diag_jacTjac,   /* diagonal of J^T J, mx1 */
       *pDp,        /* p + Dp, mx1 */
   *sp_pDp=NULL;    /* dscl*p or dscl*pDp, mx1 */

register LM_REAL mu,  /* damping constant */
                tmp; /* mainly used in matrix & vector multiplications */
LM_REAL p_eL2, jacTe_inf, pDp_eL2; /* ||e(p)||_2, ||J^T e||_inf, ||e(p+Dp)||_2 */
LM_REAL p_L2, Dp_L2=LM_REAL_MAX, dF, dL;
LM_REAL tau, eps1, eps2, eps2_sq, eps3;
LM_REAL init_p_eL2;
int nu=2, nu2, stop=0, nfev, njev=0, nlss=0;
const int nm=n*m;

/* variables for constrained LM */
struct FUNC_STATE fstate;
LM_REAL alpha=LM_CNST(1e-4), beta=LM_CNST(0.9), gamma=LM_CNST(0.99995), rho=LM_CNST(1e-8);
LM_REAL t, t0, jacTeDp;
LM_REAL tmin=LM_CNST(1e-12), tming=LM_CNST(1e-18); /* minimum step length for LS and PG steps */
const LM_REAL tini=LM_CNST(1.0); /* initial step length for LS and PG steps */
int nLMsteps=0, nLSsteps=0, nPGsteps=0, gprevtaken=0;
int numactive;
int (*linsolver)(LM_REAL *A, LM_REAL *B, LM_REAL *x, int m)=NULL;

  mu=jacTe_inf=t=0.0;  tmin=tmin; /* -Wall */

  if(n<m){
    fprintf(stderr, LCAT(LEVMAR_BC_DER, "(): cannot solve a problem with fewer measurements [%d] than unknowns [%d]\n"), n, m);
    return LM_ERROR;
  }

  if(!jacf){
    fprintf(stderr, RCAT("No function specified for computing the Jacobian in ", LEVMAR_BC_DER)
        RCAT("().\nIf no such function is available, use ", LEVMAR_BC_DIF) RCAT("() rather than ", LEVMAR_BC_DER) "()\n");
    return LM_ERROR;
  }

  if(!LEVMAR_BOX_CHECK(lb, ub, m)){
    fprintf(stderr, LCAT(LEVMAR_BC_DER, "(): at least one lower bound exceeds the upper one\n"));
    return LM_ERROR;
  }

  if(dscl){ /* check that scaling consts are valid */
    for(i=m; i-->0; )
      if(dscl[i]<=0.0){
        fprintf(stderr, LCAT(LEVMAR_BC_DER, "(): scaling constants should be positive (scale %d: %g <= 0)\n"), i, dscl[i]);
        return LM_ERROR;
      }

    sp_pDp=(LM_REAL *)malloc(m*sizeof(LM_REAL));
    if(!sp_pDp){
      fprintf(stderr, LCAT(LEVMAR_BC_DER, "(): memory allocation request failed\n"));
      return LM_ERROR;
    }
  }

  if(opts){
	  tau=opts[0];
	  eps1=opts[1];
	  eps2=opts[2];
	  eps2_sq=opts[2]*opts[2];
	  eps3=opts[3];
  }
  else{ // use default values
	  tau=LM_CNST(LM_INIT_MU);
	  eps1=LM_CNST(LM_STOP_THRESH);
	  eps2=LM_CNST(LM_STOP_THRESH);
	  eps2_sq=LM_CNST(LM_STOP_THRESH)*LM_CNST(LM_STOP_THRESH);
	  eps3=LM_CNST(LM_STOP_THRESH);
  }

  if(!work){
    worksz=LM_BC_DER_WORKSZ(m, n); //2*n+4*m + n*m + m*m;
    work=(LM_REAL *)malloc(worksz*sizeof(LM_REAL)); /* allocate a big chunk in one step */
    if(!work){
      fprintf(stderr, LCAT(LEVMAR_BC_DER, "(): memory allocation request failed\n"));
      return LM_ERROR;
    }
    freework=1;
  }

  /* set up work arrays */
  e=work;
  hx=e + n;
  jacTe=hx + n;
  jac=jacTe + m;
  jacTjac=jac + nm;
  Dp=jacTjac + m*m;
  diag_jacTjac=Dp + m;
  pDp=diag_jacTjac + m;

  fstate.n=n;
  fstate.hx=hx;
  fstate.x=x;
  fstate.lb=lb;
  fstate.ub=ub;
  fstate.adata=adata;
  fstate.nfev=&nfev;
  
  /* see if starting point is within the feasible set */
  for(i=0; i<m; ++i)
    pDp[i]=p[i];
  BOXPROJECT(p, lb, ub, m); /* project to feasible set */
  for(i=0; i<m; ++i)
    if(pDp[i]!=p[i])
      fprintf(stderr, RCAT("Warning: component %d of starting point not feasible in ", LEVMAR_BC_DER) "()! [%g projected to %g]\n",
                      i, pDp[i], p[i]);

  /* compute e=x - f(p) and its L2 norm */
  (*func)(p, hx, m, n, adata); nfev=1;
  /* ### e=x-hx, p_eL2=||e|| */
#if 1
  p_eL2=LEVMAR_L2NRMXMY(e, x, hx, n);
#else
  for(i=0, p_eL2=0.0; i<n; ++i){
    e[i]=tmp=x[i]-hx[i];
    p_eL2+=tmp*tmp;
  }
#endif
  init_p_eL2=p_eL2;
  if(!LM_FINITE(p_eL2)) stop=7;

  if(dscl){
    /* scale starting point and constraints */
    for(i=m; i-->0; ) p[i]/=dscl[i];
    BOXSCALE(lb, ub, dscl, m, 1);
  }

  for(k=0; k<itmax && !stop; ++k){
    /* Note that p and e have been updated at a previous iteration */

    if(p_eL2<=eps3){ /* error is small */
      stop=6;
      break;
    }

    /* Compute the Jacobian J at p,  J^T J,  J^T e,  ||J^T e||_inf and ||p||^2.
     * Since J^T J is symmetric, its computation can be sped up by computing
     * only its upper triangular part and copying it to the lower part
     */

    if(!dscl){
      (*jacf)(p, jac, m, n, adata); ++njev;
    }
    else{
      for(i=m; i-->0; ) sp_pDp[i]=p[i]*dscl[i];
      (*jacf)(sp_pDp, jac, m, n, adata); ++njev;

      /* compute jac*D */
      for(i=n; i-->0; ){
        register LM_REAL *jacim;

        jacim=jac+i*m;
        for(j=m; j-->0; )
          jacim[j]*=dscl[j]; // jac[i*m+j]*=dscl[j];
      }
    }

    /* J^T J, J^T e */
    if(nm<__BLOCKSZ__SQ){ // this is a small problem
      /* J^T*J_ij = \sum_l J^T_il * J_lj = \sum_l J_li * J_lj.
       * Thus, the product J^T J can be computed using an outer loop for
       * l that adds J_li*J_lj to each element ij of the result. Note that
       * with this scheme, the accesses to J and JtJ are always along rows,
       * therefore induces less cache misses compared to the straightforward
       * algorithm for computing the product (i.e., l loop is innermost one).
       * A similar scheme applies to the computation of J^T e.
       * However, for large minimization problems (i.e., involving a large number
       * of unknowns and measurements) for which J/J^T J rows are too large to
       * fit in the L1 cache, even this scheme incures many cache misses. In
       * such cases, a cache-efficient blocking scheme is preferable.
       *
       * Thanks to John Nitao of Lawrence Livermore Lab for pointing out this
       * performance problem.
       *
       * Note that the non-blocking algorithm is faster on small
       * problems since in this case it avoids the overheads of blocking. 
       */
      register LM_REAL alpha, *jaclm, *jacTjacim;

      /* looping downwards saves a few computations */
      for(i=m*m; i-->0; )
        jacTjac[i]=0.0;
      for(i=m; i-->0; )
        jacTe[i]=0.0;

      for(l=n; l-->0; ){
        jaclm=jac+l*m;
        for(i=m; i-->0; ){
          jacTjacim=jacTjac+i*m;
          alpha=jaclm[i]; //jac[l*m+i];
          for(j=i+1; j-->0; ) /* j<=i computes lower triangular part only */
            jacTjacim[j]+=jaclm[j]*alpha; //jacTjac[i*m+j]+=jac[l*m+j]*alpha

          /* J^T e */
          jacTe[i]+=alpha*e[l];
        }
      }

      for(i=m; i-->0; ) /* copy to upper part */
        for(j=i+1; j<m; ++j)
          jacTjac[i*m+j]=jacTjac[j*m+i];
    }
    else{ // this is a large problem
      /* Cache efficient computation of J^T J based on blocking
       */
      LEVMAR_TRANS_MAT_MAT_MULT(jac, jacTjac, n, m);

      /* cache efficient computation of J^T e */
      for(i=0; i<m; ++i)
        jacTe[i]=0.0;

      for(i=0; i<n; ++i){
        register LM_REAL *jacrow;

        for(l=0, jacrow=jac+i*m, tmp=e[i]; l<m; ++l)
          jacTe[l]+=jacrow[l]*tmp;
      }
    }

	  /* Compute ||J^T e||_inf and ||p||^2. Note that ||J^T e||_inf
     * is computed for free (i.e. inactive) variables only. 
     * At a local minimum, if p[i]==ub[i] then g[i]>0;
     * if p[i]==lb[i] g[i]<0; otherwise g[i]=0 
     */
    for(i=j=numactive=0, p_L2=jacTe_inf=0.0; i<m; ++i){
      if(ub && p[i]==ub[i]){ ++numactive; if(jacTe[i]>0.0) ++j; }
      else if(lb && p[i]==lb[i]){ ++numactive; if(jacTe[i]<0.0) ++j; }
      else if(jacTe_inf < (tmp=FABS(jacTe[i]))) jacTe_inf=tmp;

      diag_jacTjac[i]=jacTjac[i*m+i]; /* save diagonal entries so that augmentation can be later canceled */
      p_L2+=p[i]*p[i];
    }
    //p_L2=sqrt(p_L2);

#if 0
if(!(k%100)){
  printf("Current estimate: ");
  for(i=0; i<m; ++i)
    printf("%.9g ", p[i]);
  printf("-- errors %.9g %0.9g, #active %d [%d]\n", jacTe_inf, p_eL2, numactive, j);
}
#endif

    /* check for convergence */
    if(j==numactive && (jacTe_inf <= eps1)){
      Dp_L2=0.0; /* no increment for p in this case */
      stop=1;
      break;
    }

   /* compute initial damping factor */
    if(k==0){
      if(!lb && !ub){ /* no bounds */
        for(i=0, tmp=LM_REAL_MIN; i<m; ++i)
          if(diag_jacTjac[i]>tmp) tmp=diag_jacTjac[i]; /* find max diagonal element */
        mu=tau*tmp;
      }
      else 
        mu=LM_CNST(0.5)*tau*p_eL2; /* use Kanzow's starting mu */
    }

    /* determine increment using a combination of adaptive damping, line search and projected gradient search */
    while(1){
      /* augment normal equations */
      for(i=0; i<m; ++i)
        jacTjac[i*m+i]+=mu;

      /* solve augmented equations */
#ifdef HAVE_LAPACK
      /* 7 alternatives are available: LU, Cholesky + Cholesky with PLASMA, LDLt, 2 variants of QR decomposition and SVD.
       * For matrices with dimensions of at least a few hundreds, the PLASMA implementation of Cholesky is the fastest.
       * From the serial solvers, Cholesky is the fastest but might occasionally be inapplicable due to numerical round-off;
       * QR is slower but more robust; SVD is the slowest but most robust; LU is quite robust but
       * slower than LDLt; LDLt offers a good tradeoff between robustness and speed
       */

      issolved=AX_EQ_B_BK(jacTjac, jacTe, Dp, m); ++nlss; linsolver=AX_EQ_B_BK;
      //issolved=AX_EQ_B_LU(jacTjac, jacTe, Dp, m); ++nlss; linsolver=AX_EQ_B_LU;
      //issolved=AX_EQ_B_CHOL(jacTjac, jacTe, Dp, m); ++nlss; linsolver=AX_EQ_B_CHOL;
#ifdef HAVE_PLASMA
      //issolved=AX_EQ_B_PLASMA_CHOL(jacTjac, jacTe, Dp, m); ++nlss; linsolver=AX_EQ_B_PLASMA_CHOL;
#endif
      //issolved=AX_EQ_B_QR(jacTjac, jacTe, Dp, m); ++nlss; linsolver=AX_EQ_B_QR;
      //issolved=AX_EQ_B_QRLS(jacTjac, jacTe, Dp, m, m); ++nlss; linsolver=(int (*)(LM_REAL *A, LM_REAL *B, LM_REAL *x, int m))AX_EQ_B_QRLS;
      //issolved=AX_EQ_B_SVD(jacTjac, jacTe, Dp, m); ++nlss; linsolver=AX_EQ_B_SVD;

#else
      /* use the LU included with levmar */
      issolved=AX_EQ_B_LU(jacTjac, jacTe, Dp, m); ++nlss; linsolver=AX_EQ_B_LU;
#endif /* HAVE_LAPACK */

      if(issolved){
        for(i=0; i<m; ++i)
          pDp[i]=p[i] + Dp[i];

        /* compute p's new estimate and ||Dp||^2 */
        BOXPROJECT(pDp, lb, ub, m); /* project to feasible set */
        for(i=0, Dp_L2=0.0; i<m; ++i){
          Dp[i]=tmp=pDp[i]-p[i];
          Dp_L2+=tmp*tmp;
        }
        //Dp_L2=sqrt(Dp_L2);

        if(Dp_L2<=eps2_sq*p_L2){ /* relative change in p is small, stop */
          stop=2;
          break;
        }

        if(Dp_L2>=(p_L2+eps2)/(LM_CNST(EPSILON)*LM_CNST(EPSILON))){ /* almost singular */
          stop=4;
          break;
        }

        if(!dscl){
          (*func)(pDp, hx, m, n, adata); ++nfev; /* evaluate function at p + Dp */
        }
        else{
          for(i=m; i-->0; ) sp_pDp[i]=pDp[i]*dscl[i];
          (*func)(sp_pDp, hx, m, n, adata); ++nfev; /* evaluate function at p + Dp */
        }

        /* ### hx=x-hx, pDp_eL2=||hx|| */
#if 1
        pDp_eL2=LEVMAR_L2NRMXMY(hx, x, hx, n);
#else
        for(i=0, pDp_eL2=0.0; i<n; ++i){ /* compute ||e(pDp)||_2 */
          hx[i]=tmp=x[i]-hx[i];
          pDp_eL2+=tmp*tmp;
        }
#endif
        /* the following test ensures that the computation of pDp_eL2 has not overflowed.
         * Such an overflow does no harm here, thus it is not signalled as an error
         */
        if(!LM_FINITE(pDp_eL2) && !LM_FINITE(VECNORM(hx, n))){
          stop=7;
          break;
        }

        if(pDp_eL2<=gamma*p_eL2){
          for(i=0, dL=0.0; i<m; ++i)
            dL+=Dp[i]*(mu*Dp[i]+jacTe[i]);

#if 1
          if(dL>0.0){
            dF=p_eL2-pDp_eL2;
            tmp=(LM_CNST(2.0)*dF/dL-LM_CNST(1.0));
            tmp=LM_CNST(1.0)-tmp*tmp*tmp;
            mu=mu*( (tmp>=LM_CNST(ONE_THIRD))? tmp : LM_CNST(ONE_THIRD) );
          }
          else{
            tmp=LM_CNST(0.1)*pDp_eL2; /* pDp_eL2 is the new p_eL2 */
            mu=(mu>=tmp)? tmp : mu;
          }
#else

          tmp=LM_CNST(0.1)*pDp_eL2; /* pDp_eL2 is the new p_eL2 */
          mu=(mu>=tmp)? tmp : mu;
#endif

          nu=2;

          for(i=0 ; i<m; ++i) /* update p's estimate */
            p[i]=pDp[i];

          for(i=0; i<n; ++i) /* update e and ||e||_2 */
            e[i]=hx[i];
          p_eL2=pDp_eL2;
          ++nLMsteps;
          gprevtaken=0;
          break;
        }
        /* note that if the LM step is not taken, code falls through to the LM line search below */
      }
      else{

      /* the augmented linear system could not be solved, increase mu */

        mu*=nu;
        nu2=nu<<1; // 2*nu;
        if(nu2<=nu){ /* nu has wrapped around (overflown). Thanks to Frank Jordan for spotting this case */
          stop=5;
          break;
        }
        nu=nu2;

        for(i=0; i<m; ++i) /* restore diagonal J^T J entries */
          jacTjac[i*m+i]=diag_jacTjac[i];

        continue; /* solve again with increased nu */
      }

      /* if this point is reached, the LM step did not reduce the error;
       * see if it is a descent direction
       */

      /* negate jacTe (i.e. g) & compute g^T * Dp */
      for(i=0, jacTeDp=0.0; i<m; ++i){
        jacTe[i]=-jacTe[i];
        jacTeDp+=jacTe[i]*Dp[i];
      }

      if(jacTeDp<=-rho*pow(Dp_L2, LM_CNST(_POW_)/LM_CNST(2.0))){
        /* Dp is a descent direction; do a line search along it */
#if 1
        /* use Schnabel's backtracking line search; it requires fewer "func" evaluations */
        {
        int mxtake, iretcd;
        LM_REAL stepmx, steptl=LM_CNST(1e3)*(LM_REAL)sqrt(LM_REAL_EPSILON);

        tmp=(LM_REAL)sqrt(p_L2); stepmx=LM_CNST(1e3)*( (tmp>=LM_CNST(1.0))? tmp : LM_CNST(1.0) );

        LNSRCH(m, p, p_eL2, jacTe, Dp, alpha, pDp, &pDp_eL2, func, &fstate,
               &mxtake, &iretcd, stepmx, steptl, dscl); /* NOTE: LNSRCH() updates hx */
        if(iretcd!=0 || !LM_FINITE(pDp_eL2)) goto gradproj; /* rather inelegant but effective way to handle LNSRCH() failures... */
        }
#else
        /* use the simpler (but slower!) line search described by Kanzow et al */
        for(t=tini; t>tmin; t*=beta){
          for(i=0; i<m; ++i)
            pDp[i]=p[i] + t*Dp[i];
          BOXPROJECT(pDp, lb, ub, m); /* project to feasible set */

          if(!dscl){
            (*func)(pDp, hx, m, n, adata); ++nfev; /* evaluate function at p + t*Dp */
          }
          else{
            for(i=m; i-->0; ) sp_pDp[i]=pDp[i]*dscl[i];
            (*func)(sp_pDp, hx, m, n, adata); ++nfev; /* evaluate function at p + t*Dp */
          }

          /* compute ||e(pDp)||_2 */
          /* ### hx=x-hx, pDp_eL2=||hx|| */
#if 1
          pDp_eL2=LEVMAR_L2NRMXMY(hx, x, hx, n);
#else
          for(i=0, pDp_eL2=0.0; i<n; ++i){
            hx[i]=tmp=x[i]-hx[i];
            pDp_eL2+=tmp*tmp;
          }
#endif /* ||e(pDp)||_2 */
          if(!LM_FINITE(pDp_eL2)) goto gradproj; /* treat as line search failure */

          //if(LM_CNST(0.5)*pDp_eL2<=LM_CNST(0.5)*p_eL2 + t*alpha*jacTeDp) break;
          if(pDp_eL2<=p_eL2 + LM_CNST(2.0)*t*alpha*jacTeDp) break;
        }
#endif /* line search alternatives */

        ++nLSsteps;
        gprevtaken=0;

        /* NOTE: new estimate for p is in pDp, associated error in hx and its norm in pDp_eL2.
         * These values are used below to update their corresponding variables 
         */
      }
      else{
        /* Note that this point can also be reached via a goto when LNSRCH() fails. */
gradproj:

        /* jacTe has been negated above. Being a descent direction, it is next used
         * to make a projected gradient step
         */

        /* compute ||g|| */
        for(i=0, tmp=0.0; i<m; ++i)
          tmp+=jacTe[i]*jacTe[i];
        tmp=(LM_REAL)sqrt(tmp);
        tmp=LM_CNST(100.0)/(LM_CNST(1.0)+tmp);
        t0=(tmp<=tini)? tmp : tini; /* guard against poor scaling & large steps; see (3.50) in C.T. Kelley's book */

        /* if the previous step was along the gradient descent, try to use the t employed in that step */
        for(t=(gprevtaken)? t : t0; t>tming; t*=beta){
          for(i=0; i<m; ++i)
            pDp[i]=p[i] - t*jacTe[i];
          BOXPROJECT(pDp, lb, ub, m); /* project to feasible set */
          for(i=0, Dp_L2=0.0; i<m; ++i){
            Dp[i]=tmp=pDp[i]-p[i];
            Dp_L2+=tmp*tmp;
          }

          if(!dscl){
            (*func)(pDp, hx, m, n, adata); ++nfev; /* evaluate function at p - t*g */
          }
          else{
            for(i=m; i-->0; ) sp_pDp[i]=pDp[i]*dscl[i];
            (*func)(sp_pDp, hx, m, n, adata); ++nfev; /* evaluate function at p - t*g */
          }

          /* compute ||e(pDp)||_2 */
          /* ### hx=x-hx, pDp_eL2=||hx|| */
#if 1
          pDp_eL2=LEVMAR_L2NRMXMY(hx, x, hx, n);
#else
          for(i=0, pDp_eL2=0.0; i<n; ++i){
            hx[i]=tmp=x[i]-hx[i];
            pDp_eL2+=tmp*tmp;
          }
#endif
          /* the following test ensures that the computation of pDp_eL2 has not overflowed.
           * Such an overflow does no harm here, thus it is not signalled as an error
           */
          if(!LM_FINITE(pDp_eL2) && !LM_FINITE(VECNORM(hx, n))){
            stop=7;
            goto breaknested;
          }

          /* compute ||g^T * Dp||. Note that if pDp has not been altered by projection
           * (i.e. BOXPROJECT), jacTeDp=-t*||g||^2
           */
          for(i=0, jacTeDp=0.0; i<m; ++i)
            jacTeDp+=jacTe[i]*Dp[i];

          if(gprevtaken && pDp_eL2<=p_eL2 + LM_CNST(2.0)*LM_CNST(0.99999)*jacTeDp){ /* starting t too small */
            t=t0;
            gprevtaken=0;
            continue;
          }
          //if(LM_CNST(0.5)*pDp_eL2<=LM_CNST(0.5)*p_eL2 + alpha*jacTeDp) terminatePGLS;
          if(pDp_eL2<=p_eL2 + LM_CNST(2.0)*alpha*jacTeDp) goto terminatePGLS;

          //if(pDp_eL2<=p_eL2 - LM_CNST(2.0)*alpha/t*Dp_L2) goto terminatePGLS; // sufficient decrease condition proposed by Kelley in (5.13)
        }
        
        /* if this point is reached then the gradient line search has failed */
        gprevtaken=0;
        break;

terminatePGLS:

        ++nPGsteps;
        gprevtaken=1;
        /* NOTE: new estimate for p is in pDp, associated error in hx and its norm in pDp_eL2 */
      }

      /* update using computed values */

      for(i=0, Dp_L2=0.0; i<m; ++i){
        tmp=pDp[i]-p[i];
        Dp_L2+=tmp*tmp;
      }
      //Dp_L2=sqrt(Dp_L2);

      if(Dp_L2<=eps2_sq*p_L2){ /* relative change in p is small, stop */
        stop=2;
        break;
      }

      for(i=0 ; i<m; ++i) /* update p's estimate */
        p[i]=pDp[i];

      for(i=0; i<n; ++i) /* update e and ||e||_2 */
        e[i]=hx[i];
      p_eL2=pDp_eL2;
      break;
    } /* inner loop */
  }

breaknested: /* NOTE: this point is also reached via an explicit goto! */

  if(k>=itmax) stop=3;

  for(i=0; i<m; ++i) /* restore diagonal J^T J entries */
    jacTjac[i*m+i]=diag_jacTjac[i];

  if(info){
    info[0]=init_p_eL2;
    info[1]=p_eL2;
    info[2]=jacTe_inf;
    info[3]=Dp_L2;
    for(i=0, tmp=LM_REAL_MIN; i<m; ++i)
      if(tmp<jacTjac[i*m+i]) tmp=jacTjac[i*m+i];
    info[4]=mu/tmp;
    info[5]=(LM_REAL)k;
    info[6]=(LM_REAL)stop;
    info[7]=(LM_REAL)nfev;
    info[8]=(LM_REAL)njev;
    info[9]=(LM_REAL)nlss;
  }

  /* covariance matrix */
  if(covar){
    LEVMAR_COVAR(jacTjac, covar, p_eL2, m, n);

    if(dscl){ /* correct for the scaling */
      for(i=m; i-->0; )
        for(j=m; j-->0; )
          covar[i*m+j]*=(dscl[i]*dscl[j]);
    }
  }
                                                               
  if(freework) free(work);

#ifdef LINSOLVERS_RETAIN_MEMORY
    if(linsolver) (*linsolver)(NULL, NULL, NULL, 0);
#endif

#if 0
printf("%d LM steps, %d line search, %d projected gradient\n", nLMsteps, nLSsteps, nPGsteps);
#endif

  if(dscl){
    /* scale final point and constraints */
    for(i=0; i<m; ++i) p[i]*=dscl[i];
    BOXSCALE(lb, ub, dscl, m, 0);
    free(sp_pDp);
  }

  return (stop!=4 && stop!=7)?  k : LM_ERROR;
}

/* following struct & LMBC_DIF_XXX functions won't be necessary if a true secant
 * version of LEVMAR_BC_DIF() is implemented...
 */
struct LMBC_DIF_DATA{
  int ffdif; // nonzero if forward differencing is used
  void (*func)(LM_REAL *p, LM_REAL *hx, int m, int n, void *adata);
  LM_REAL *hx, *hxx;
  void *adata;
  LM_REAL delta;
};

static void LMBC_DIF_FUNC(LM_REAL *p, LM_REAL *hx, int m, int n, void *data)
{
struct LMBC_DIF_DATA *dta=(struct LMBC_DIF_DATA *)data;

  /* call user-supplied function passing it the user-supplied data */
  (*(dta->func))(p, hx, m, n, dta->adata);
}

static void LMBC_DIF_JACF(LM_REAL *p, LM_REAL *jac, int m, int n, void *data)
{
struct LMBC_DIF_DATA *dta=(struct LMBC_DIF_DATA *)data;

  if(dta->ffdif){
    /* evaluate user-supplied function at p */
    (*(dta->func))(p, dta->hx, m, n, dta->adata);
    LEVMAR_FDIF_FORW_JAC_APPROX(dta->func, p, dta->hx, dta->hxx, dta->delta, jac, m, n, dta->adata);
  }
  else
    LEVMAR_FDIF_CENT_JAC_APPROX(dta->func, p, dta->hx, dta->hxx, dta->delta, jac, m, n, dta->adata);
}


/* No Jacobian version of the LEVMAR_BC_DER() function above: the Jacobian is approximated with 
 * the aid of finite differences (forward or central, see the comment for the opts argument)
 * Ideally, this function should be implemented with a secant approach. Currently, it just calls
 * LEVMAR_BC_DER()
 */
int LEVMAR_BC_DIF(
  void (*func)(LM_REAL *p, LM_REAL *hx, int m, int n, void *adata), /* functional relation describing measurements. A p \in R^m yields a \hat{x} \in  R^n */
  LM_REAL *p,         /* I/O: initial parameter estimates. On output has the estimated solution */
  LM_REAL *x,         /* I: measurement vector. NULL implies a zero vector */
  int m,              /* I: parameter vector dimension (i.e. #unknowns) */
  int n,              /* I: measurement vector dimension */
  LM_REAL *lb,        /* I: vector of lower bounds. If NULL, no lower bounds apply */
  LM_REAL *ub,        /* I: vector of upper bounds. If NULL, no upper bounds apply */
  LM_REAL *dscl,      /* I: diagonal scaling constants. NULL implies no scaling */
  int itmax,          /* I: maximum number of iterations */
  LM_REAL opts[5],    /* I: opts[0-4] = minim. options [\mu, \epsilon1, \epsilon2, \epsilon3, \delta]. Respectively the
                       * scale factor for initial \mu, stopping thresholds for ||J^T e||_inf, ||Dp||_2 and ||e||_2 and
                       * the step used in difference approximation to the Jacobian. Set to NULL for defaults to be used.
                       * If \delta<0, the Jacobian is approximated with central differences which are more accurate
                       * (but slower!) compared to the forward differences employed by default. 
                       */
  LM_REAL info[LM_INFO_SZ],
					           /* O: information regarding the minimization. Set to NULL if don't care
                      * info[0]= ||e||_2 at initial p.
                      * info[1-4]=[ ||e||_2, ||J^T e||_inf,  ||Dp||_2, mu/max[J^T J]_ii ], all computed at estimated p.
                      * info[5]= # iterations,
                      * info[6]=reason for terminating: 1 - stopped by small gradient J^T e
                      *                                 2 - stopped by small Dp
                      *                                 3 - stopped by itmax
                      *                                 4 - singular matrix. Restart from current p with increased mu 
                      *                                 5 - no further error reduction is possible. Restart with increased mu
                      *                                 6 - stopped by small ||e||_2
                      *                                 7 - stopped by invalid (i.e. NaN or Inf) "func" values. This is a user error
                      * info[7]= # function evaluations
                      * info[8]= # Jacobian evaluations
                      * info[9]= # linear systems solved, i.e. # attempts for reducing error
                      */
  LM_REAL *work,     /* working memory at least LM_BC_DIF_WORKSZ() reals large, allocated if NULL */
  LM_REAL *covar,    /* O: Covariance matrix corresponding to LS solution; mxm. Set to NULL if not needed. */
  void *adata)       /* pointer to possibly additional data, passed uninterpreted to func.
                      * Set to NULL if not needed
                      */
{
struct LMBC_DIF_DATA data;
int ret;

  //fprintf(stderr, RCAT("\nWarning: current implementation of ", LEVMAR_BC_DIF) "() does not use a secant approach!\n\n");

  data.ffdif=!opts || opts[4]>=0.0;

  data.func=func;
  data.hx=(LM_REAL *)malloc(2*n*sizeof(LM_REAL)); /* allocate a big chunk in one step */
  if(!data.hx){
    fprintf(stderr, LCAT(LEVMAR_BC_DIF, "(): memory allocation request failed\n"));
    return LM_ERROR;
  }
  data.hxx=data.hx+n;
  data.adata=adata;
  data.delta=(opts)? FABS(opts[4]) : (LM_REAL)LM_DIFF_DELTA;

  ret=LEVMAR_BC_DER(LMBC_DIF_FUNC, LMBC_DIF_JACF, p, x, m, n, lb, ub, dscl, itmax, opts, info, work, covar, (void *)&data);

  if(info){ /* correct the number of function calls */
    if(data.ffdif)
      info[7]+=info[8]*(m+1); /* each Jacobian evaluation costs m+1 function calls */
    else
      info[7]+=info[8]*(2*m); /* each Jacobian evaluation costs 2*m function calls */
  }

  free(data.hx);

  return ret;
}

/* undefine everything. THIS MUST REMAIN AT THE END OF THE FILE */
#undef FUNC_STATE
#undef LNSRCH
#undef BOXPROJECT
#undef BOXSCALE
#undef LEVMAR_BOX_CHECK
#undef VECNORM
#undef LEVMAR_BC_DER
#undef LMBC_DIF_DATA
#undef LMBC_DIF_FUNC
#undef LMBC_DIF_JACF
#undef LEVMAR_BC_DIF
#undef LEVMAR_FDIF_FORW_JAC_APPROX
#undef LEVMAR_FDIF_CENT_JAC_APPROX
#undef LEVMAR_COVAR
#undef LEVMAR_TRANS_MAT_MAT_MULT
#undef LEVMAR_L2NRMXMY
#undef AX_EQ_B_LU
#undef AX_EQ_B_CHOL
#undef AX_EQ_B_PLASMA_CHOL
#undef AX_EQ_B_QR
#undef AX_EQ_B_QRLS
#undef AX_EQ_B_SVD
#undef AX_EQ_B_BK
