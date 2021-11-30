/////////////////////////////////////////////////////////////////////////////////
// 
//  Levenberg - Marquardt non-linear minimization algorithm
//  Copyright (C) 2009  Manolis Lourakis (lourakis at ics forth gr)
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

#ifndef LM_REAL // not included by lmbleic.c
#error This file should not be compiled directly!
#endif


/* precision-specific definitions */
#define LMBLEIC_DATA LM_ADD_PREFIX(lmbleic_data)
#define LMBLEIC_ELIM LM_ADD_PREFIX(lmbleic_elim)
#define LMBLEIC_FUNC LM_ADD_PREFIX(lmbleic_func)
#define LMBLEIC_JACF LM_ADD_PREFIX(lmbleic_jacf)
#define LEVMAR_BLEIC_DER LM_ADD_PREFIX(levmar_bleic_der)
#define LEVMAR_BLEIC_DIF LM_ADD_PREFIX(levmar_bleic_dif)
#define LEVMAR_BLIC_DER LM_ADD_PREFIX(levmar_blic_der)
#define LEVMAR_BLIC_DIF LM_ADD_PREFIX(levmar_blic_dif)
#define LEVMAR_LEIC_DER LM_ADD_PREFIX(levmar_leic_der)
#define LEVMAR_LEIC_DIF LM_ADD_PREFIX(levmar_leic_dif)
#define LEVMAR_LIC_DER LM_ADD_PREFIX(levmar_lic_der)
#define LEVMAR_LIC_DIF LM_ADD_PREFIX(levmar_lic_dif)
#define LEVMAR_BLEC_DER LM_ADD_PREFIX(levmar_blec_der)
#define LEVMAR_BLEC_DIF LM_ADD_PREFIX(levmar_blec_dif)
#define LEVMAR_TRANS_MAT_MAT_MULT LM_ADD_PREFIX(levmar_trans_mat_mat_mult)
#define LEVMAR_COVAR LM_ADD_PREFIX(levmar_covar)
#define LEVMAR_FDIF_FORW_JAC_APPROX LM_ADD_PREFIX(levmar_fdif_forw_jac_approx)

struct LMBLEIC_DATA{
  LM_REAL *jac;
  int nineqcnstr; // #inequality constraints
  void (*func)(LM_REAL *p, LM_REAL *hx, int m, int n, void *adata);
  void (*jacf)(LM_REAL *p, LM_REAL *jac, int m, int n, void *adata);
  void *adata;
};


/* wrapper ensuring that the user-supplied function is called with the right number of variables (i.e. m) */
static void LMBLEIC_FUNC(LM_REAL *pext, LM_REAL *hx, int mm, int n, void *adata)
{
struct LMBLEIC_DATA *data=(struct LMBLEIC_DATA *)adata;
int m;

  m=mm-data->nineqcnstr;
  (*(data->func))(pext, hx, m, n, data->adata);
}


/* wrapper for computing the Jacobian at pext. The Jacobian is nxmm */
static void LMBLEIC_JACF(LM_REAL *pext, LM_REAL *jacext, int mm, int n, void *adata)
{
struct LMBLEIC_DATA *data=(struct LMBLEIC_DATA *)adata;
int m;
register int i, j;
LM_REAL *jac, *jacim, *jacextimm;

  m=mm-data->nineqcnstr;
  jac=data->jac;

  (*(data->jacf))(pext, jac, m, n, data->adata);

  for(i=0; i<n; ++i){
    jacextimm=jacext+i*mm;
    jacim=jac+i*m;
    for(j=0; j<m; ++j)
      jacextimm[j]=jacim[j]; //jacext[i*mm+j]=jac[i*m+j];

    for(j=m; j<mm; ++j)
      jacextimm[j]=0.0; //jacext[i*mm+j]=0.0;
  }
}


/* 
 * This function is similar to LEVMAR_DER except that the minimization is
 * performed subject to the box constraints lb[i]<=p[i]<=ub[i], the linear
 * equation constraints A*p=b, A being k1xm, b k1x1, and the linear inequality
 * constraints C*p>=d, C being k2xm, d k2x1. 
 *
 * The inequalities are converted to equations by introducing surplus variables,
 * i.e. c^T*p >= d becomes c^T*p - y = d, with y>=0. To transform all inequalities
 * to equations, a total of k2 surplus variables are introduced; a problem with only
 * box and linear constraints results then and is solved with LEVMAR_BLEC_DER()
 * Note that opposite direction inequalities should be converted to the desired
 * direction by negating, i.e. c^T*p <= d becomes -c^T*p >= -d
 *
 * This function requires an analytic Jacobian. In case the latter is unavailable,
 * use LEVMAR_BLEIC_DIF() bellow
 *
 */
int LEVMAR_BLEIC_DER(
  void (*func)(LM_REAL *p, LM_REAL *hx, int m, int n, void *adata), /* functional relation describing measurements. A p \in R^m yields a \hat{x} \in  R^n */
  void (*jacf)(LM_REAL *p, LM_REAL *j, int m, int n, void *adata),  /* function to evaluate the Jacobian \part x / \part p */ 
  LM_REAL *p,         /* I/O: initial parameter estimates. On output has the estimated solution */
  LM_REAL *x,         /* I: measurement vector. NULL implies a zero vector */
  int m,              /* I: parameter vector dimension (i.e. #unknowns) */
  int n,              /* I: measurement vector dimension */
  LM_REAL *lb,        /* I: vector of lower bounds. If NULL, no lower bounds apply */
  LM_REAL *ub,        /* I: vector of upper bounds. If NULL, no upper bounds apply */
  LM_REAL *A,         /* I: equality constraints matrix, k1xm. If NULL, no linear equation constraints apply */
  LM_REAL *b,         /* I: right hand constraints vector, k1x1 */
  int k1,             /* I: number of constraints (i.e. A's #rows) */
  LM_REAL *C,         /* I: inequality constraints matrix, k2xm */
  LM_REAL *d,         /* I: right hand constraints vector, k2x1 */
  int k2,             /* I: number of inequality constraints (i.e. C's #rows) */
  int itmax,          /* I: maximum number of iterations */
  LM_REAL opts[4],    /* I: minim. options [\mu, \epsilon1, \epsilon2, \epsilon3]. Respectively the scale factor for initial \mu,
                       * stopping thresholds for ||J^T e||_inf, ||Dp||_2 and ||e||_2. Set to NULL for defaults to be used
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
  LM_REAL *work,     /* working memory at least LM_BLEIC_DER_WORKSZ() reals large, allocated if NULL */
  LM_REAL *covar,    /* O: Covariance matrix corresponding to LS solution; mxm. Set to NULL if not needed. */
  void *adata)       /* pointer to possibly additional data, passed uninterpreted to func & jacf.
                      * Set to NULL if not needed
                      */
{
  struct LMBLEIC_DATA data;
  LM_REAL *ptr, *pext, *Aext, *bext, *covext; /* corresponding to p, A, b, covar for the full set of variables;
                                                 pext=[p, surplus], pext is mm, Aext is (k1+k2)xmm, bext (k1+k2), covext is mmxmm
                                               */
  LM_REAL *lbext, *ubext; // corresponding to lb, ub for the full set of variables
  int mm, ret, k12;
  register int i, j, ii;
  register LM_REAL tmp;
  LM_REAL locinfo[LM_INFO_SZ];

  if(!jacf){
    fprintf(stderr, RCAT("No function specified for computing the Jacobian in ", LEVMAR_BLEIC_DER)
      RCAT("().\nIf no such function is available, use ", LEVMAR_BLEIC_DIF) RCAT("() rather than ", LEVMAR_BLEIC_DER) "()\n");
    return LM_ERROR;
  }

  if(!C || !d){
    fprintf(stderr, RCAT(LCAT(LEVMAR_BLEIC_DER, "(): missing inequality constraints, use "), LEVMAR_BLEC_DER) "() in this case!\n");
    return LM_ERROR;
  }

  if(!A || !b) k1=0; // sanity check

  mm=m+k2;

  if(n<m-k1){
    fprintf(stderr, LCAT(LEVMAR_BLEIC_DER, "(): cannot solve a problem with fewer measurements + equality constraints [%d + %d] than unknowns [%d]\n"), n, k1, m);
    return LM_ERROR;
  }

  k12=k1+k2;
  ptr=(LM_REAL *)malloc((3*mm + k12*mm + k12 + n*m + (covar? mm*mm : 0))*sizeof(LM_REAL));
  if(!ptr){
    fprintf(stderr, LCAT(LEVMAR_BLEIC_DER, "(): memory allocation request failed\n"));
    return LM_ERROR;
  }
  pext=ptr;
  lbext=pext+mm;
  ubext=lbext+mm;
  Aext=ubext+mm;
  bext=Aext+k12*mm;
  data.jac=bext+k12;
  covext=covar? data.jac+n*m : NULL;
  data.nineqcnstr=k2;
  data.func=func;
  data.jacf=jacf;
  data.adata=adata;

  /* compute y s.t. C*p - y=d, i.e. y=C*p-d.
   * y is stored in the last k2 elements of pext
   */
  for(i=0; i<k2; ++i){
    for(j=0, tmp=0.0; j<m; ++j)
      tmp+=C[i*m+j]*p[j];
    pext[j=i+m]=tmp-d[i];

    /* surplus variables must be >=0 */
    lbext[j]=0.0;
    ubext[j]=LM_REAL_MAX;
  }
  /* set the first m elements of pext equal to p */
  for(i=0; i<m; ++i){
    pext[i]=p[i];
    lbext[i]=lb? lb[i] : LM_REAL_MIN;
    ubext[i]=ub? ub[i] : LM_REAL_MAX;
  }

  /* setup the constraints matrix */
  /* original linear equation constraints */
  for(i=0; i<k1; ++i){
    for(j=0; j<m; ++j)
      Aext[i*mm+j]=A[i*m+j];

    for(j=m; j<mm; ++j)
      Aext[i*mm+j]=0.0;

    bext[i]=b[i];
  }
  /* linear equation constraints resulting from surplus variables */
  for(i=0, ii=k1; i<k2; ++i, ++ii){
    for(j=0; j<m; ++j)
      Aext[ii*mm+j]=C[i*m+j];

    for(j=m; j<mm; ++j)
      Aext[ii*mm+j]=0.0;

    Aext[ii*mm+m+i]=-1.0;

    bext[ii]=d[i];
  }

  if(!info) info=locinfo; /* make sure that LEVMAR_BLEC_DER() is called with non-null info */
  /* note that the default weights for the penalty terms are being used below */
  ret=LEVMAR_BLEC_DER(LMBLEIC_FUNC, LMBLEIC_JACF, pext, x, mm, n, lbext, ubext, Aext, bext, k12, NULL, itmax, opts, info, work, covext, (void *)&data);

  /* copy back the minimizer */
  for(i=0; i<m; ++i)
    p[i]=pext[i];

#if 0
printf("Surplus variables for the minimizer:\n");
for(i=m; i<mm; ++i)
  printf("%g ", pext[i]);
printf("\n\n");
#endif

  if(covar){
    for(i=0; i<m; ++i){
      for(j=0; j<m; ++j)
        covar[i*m+j]=covext[i*mm+j];
    }
  }

  free(ptr);

  return ret;
}

/* Similar to the LEVMAR_BLEIC_DER() function above, except that the Jacobian is approximated
 * with the aid of finite differences (forward or central, see the comment for the opts argument)
 */
int LEVMAR_BLEIC_DIF(
  void (*func)(LM_REAL *p, LM_REAL *hx, int m, int n, void *adata), /* functional relation describing measurements. A p \in R^m yields a \hat{x} \in  R^n */
  LM_REAL *p,         /* I/O: initial parameter estimates. On output has the estimated solution */
  LM_REAL *x,         /* I: measurement vector. NULL implies a zero vector */
  int m,              /* I: parameter vector dimension (i.e. #unknowns) */
  int n,              /* I: measurement vector dimension */
  LM_REAL *lb,        /* I: vector of lower bounds. If NULL, no lower bounds apply */
  LM_REAL *ub,        /* I: vector of upper bounds. If NULL, no upper bounds apply */
  LM_REAL *A,         /* I: equality constraints matrix, k1xm. If NULL, no linear equation constraints apply */
  LM_REAL *b,         /* I: right hand constraints vector, k1x1 */
  int k1,             /* I: number of constraints (i.e. A's #rows) */
  LM_REAL *C,         /* I: inequality constraints matrix, k2xm */
  LM_REAL *d,         /* I: right hand constraints vector, k2x1 */
  int k2,             /* I: number of inequality constraints (i.e. C's #rows) */
  int itmax,          /* I: maximum number of iterations */
  LM_REAL opts[5],    /* I: opts[0-3] = minim. options [\mu, \epsilon1, \epsilon2, \epsilon3, \delta]. Respectively the
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
  LM_REAL *work,     /* working memory at least LM_BLEIC_DIF_WORKSZ() reals large, allocated if NULL */
  LM_REAL *covar,    /* O: Covariance matrix corresponding to LS solution; mxm. Set to NULL if not needed. */
  void *adata)       /* pointer to possibly additional data, passed uninterpreted to func.
                      * Set to NULL if not needed
                      */
{
  struct LMBLEIC_DATA data;
  LM_REAL *ptr, *pext, *Aext, *bext, *covext; /* corresponding to p, A, b, covar for the full set of variables;
                                                 pext=[p, surplus], pext is mm, Aext is (k1+k2)xmm, bext (k1+k2), covext is mmxmm
                                               */
  LM_REAL *lbext, *ubext; // corresponding to lb, ub for the full set of variables
  int mm, ret, k12;
  register int i, j, ii;
  register LM_REAL tmp;
  LM_REAL locinfo[LM_INFO_SZ];

  if(!C || !d){
    fprintf(stderr, RCAT(LCAT(LEVMAR_BLEIC_DIF, "(): missing inequality constraints, use "), LEVMAR_BLEC_DIF) "() in this case!\n");
    return LM_ERROR;
  }
  if(!A || !b) k1=0; // sanity check

  mm=m+k2;

  if(n<m-k1){
    fprintf(stderr, LCAT(LEVMAR_BLEIC_DIF, "(): cannot solve a problem with fewer measurements + equality constraints [%d + %d] than unknowns [%d]\n"), n, k1, m);
    return LM_ERROR;
  }

  k12=k1+k2;
  ptr=(LM_REAL *)malloc((3*mm + k12*mm + k12 + (covar? mm*mm : 0))*sizeof(LM_REAL));
  if(!ptr){
    fprintf(stderr, LCAT(LEVMAR_BLEIC_DIF, "(): memory allocation request failed\n"));
    return LM_ERROR;
  }
  pext=ptr;
  lbext=pext+mm;
  ubext=lbext+mm;
  Aext=ubext+mm;
  bext=Aext+k12*mm;
  data.jac=NULL;
  covext=covar? bext+k12 : NULL;
  data.nineqcnstr=k2;
  data.func=func;
  data.jacf=NULL;
  data.adata=adata;

  /* compute y s.t. C*p - y=d, i.e. y=C*p-d.
   * y is stored in the last k2 elements of pext
   */
  for(i=0; i<k2; ++i){
    for(j=0, tmp=0.0; j<m; ++j)
      tmp+=C[i*m+j]*p[j];
    pext[j=i+m]=tmp-d[i];

    /* surplus variables must be >=0 */
    lbext[j]=0.0;
    ubext[j]=LM_REAL_MAX;
  }
  /* set the first m elements of pext equal to p */
  for(i=0; i<m; ++i){
    pext[i]=p[i];
    lbext[i]=lb? lb[i] : LM_REAL_MIN; 
    ubext[i]=ub? ub[i] : LM_REAL_MAX;
  }

  /* setup the constraints matrix */
  /* original linear equation constraints */
  for(i=0; i<k1; ++i){
    for(j=0; j<m; ++j)
      Aext[i*mm+j]=A[i*m+j];

    for(j=m; j<mm; ++j)
      Aext[i*mm+j]=0.0;

    bext[i]=b[i];
  }
  /* linear equation constraints resulting from surplus variables */
  for(i=0, ii=k1; i<k2; ++i, ++ii){
    for(j=0; j<m; ++j)
      Aext[ii*mm+j]=C[i*m+j];

    for(j=m; j<mm; ++j)
      Aext[ii*mm+j]=0.0;

    Aext[ii*mm+m+i]=-1.0;

    bext[ii]=d[i];
  }

  if(!info) info=locinfo; /* make sure that LEVMAR_BLEC_DIF() is called with non-null info */
  /* note that the default weights for the penalty terms are being used below */
  ret=LEVMAR_BLEC_DIF(LMBLEIC_FUNC, pext, x, mm, n, lbext, ubext, Aext, bext, k12, NULL, itmax, opts, info, work, covext, (void *)&data);

  /* copy back the minimizer */
  for(i=0; i<m; ++i)
    p[i]=pext[i];

#if 0
printf("Surplus variables for the minimizer:\n");
for(i=m; i<mm; ++i)
  printf("%g ", pext[i]);
printf("\n\n");
#endif

  if(covar){
    for(i=0; i<m; ++i){
      for(j=0; j<m; ++j)
        covar[i*m+j]=covext[i*mm+j];
    }
  }

  free(ptr);

  return ret;
}


/* convenience wrappers to LEVMAR_BLEIC_DER/LEVMAR_BLEIC_DIF */

/* box & linear inequality constraints */
int LEVMAR_BLIC_DER(
  void (*func)(LM_REAL *p, LM_REAL *hx, int m, int n, void *adata),
  void (*jacf)(LM_REAL *p, LM_REAL *j, int m, int n, void *adata),
  LM_REAL *p, LM_REAL *x, int m, int n,
  LM_REAL *lb, LM_REAL *ub,
  LM_REAL *C, LM_REAL *d, int k2,
  int itmax, LM_REAL opts[4], LM_REAL info[LM_INFO_SZ], LM_REAL *work, LM_REAL *covar, void *adata)
{
  return LEVMAR_BLEIC_DER(func, jacf, p, x, m, n, lb, ub, NULL, NULL, 0, C, d, k2, itmax, opts, info, work, covar, adata);
}

int LEVMAR_BLIC_DIF(
  void (*func)(LM_REAL *p, LM_REAL *hx, int m, int n, void *adata),
  LM_REAL *p, LM_REAL *x, int m, int n,
  LM_REAL *lb, LM_REAL *ub,
  LM_REAL *C, LM_REAL *d, int k2,
  int itmax, LM_REAL opts[5], LM_REAL info[LM_INFO_SZ], LM_REAL *work, LM_REAL *covar, void *adata)
{
  return LEVMAR_BLEIC_DIF(func, p, x, m, n, lb, ub, NULL, NULL, 0, C, d, k2, itmax, opts, info, work, covar, adata);
}

/* linear equation & inequality constraints */
int LEVMAR_LEIC_DER(
  void (*func)(LM_REAL *p, LM_REAL *hx, int m, int n, void *adata),
  void (*jacf)(LM_REAL *p, LM_REAL *j, int m, int n, void *adata),
  LM_REAL *p, LM_REAL *x, int m, int n,
  LM_REAL *A, LM_REAL *b, int k1,
  LM_REAL *C, LM_REAL *d, int k2,
  int itmax, LM_REAL opts[4], LM_REAL info[LM_INFO_SZ], LM_REAL *work, LM_REAL *covar, void *adata)
{
  return LEVMAR_BLEIC_DER(func, jacf, p, x, m, n, NULL, NULL, A, b, k1, C, d, k2, itmax, opts, info, work, covar, adata);
}

int LEVMAR_LEIC_DIF(
  void (*func)(LM_REAL *p, LM_REAL *hx, int m, int n, void *adata),
  LM_REAL *p, LM_REAL *x, int m, int n,
  LM_REAL *A, LM_REAL *b, int k1,
  LM_REAL *C, LM_REAL *d, int k2,
  int itmax, LM_REAL opts[5], LM_REAL info[LM_INFO_SZ], LM_REAL *work, LM_REAL *covar, void *adata)
{
  return LEVMAR_BLEIC_DIF(func, p, x, m, n, NULL, NULL, A, b, k1, C, d, k2, itmax, opts, info, work, covar, adata);
}

/* linear inequality constraints */
int LEVMAR_LIC_DER(
  void (*func)(LM_REAL *p, LM_REAL *hx, int m, int n, void *adata),
  void (*jacf)(LM_REAL *p, LM_REAL *j, int m, int n, void *adata),
  LM_REAL *p, LM_REAL *x, int m, int n,
  LM_REAL *C, LM_REAL *d, int k2,
  int itmax, LM_REAL opts[4], LM_REAL info[LM_INFO_SZ], LM_REAL *work, LM_REAL *covar, void *adata)
{
  return LEVMAR_BLEIC_DER(func, jacf, p, x, m, n, NULL, NULL, NULL, NULL, 0, C, d, k2, itmax, opts, info, work, covar, adata);
}

int LEVMAR_LIC_DIF(
  void (*func)(LM_REAL *p, LM_REAL *hx, int m, int n, void *adata),
  LM_REAL *p, LM_REAL *x, int m, int n,
  LM_REAL *C, LM_REAL *d, int k2,
  int itmax, LM_REAL opts[5], LM_REAL info[LM_INFO_SZ], LM_REAL *work, LM_REAL *covar, void *adata)
{
  return LEVMAR_BLEIC_DIF(func, p, x, m, n, NULL, NULL, NULL, NULL, 0, C, d, k2, itmax, opts, info, work, covar, adata);
}

/* undefine all. THIS MUST REMAIN AT THE END OF THE FILE */
#undef LMBLEIC_DATA
#undef LMBLEIC_ELIM
#undef LMBLEIC_FUNC
#undef LMBLEIC_JACF
#undef LEVMAR_FDIF_FORW_JAC_APPROX
#undef LEVMAR_COVAR
#undef LEVMAR_TRANS_MAT_MAT_MULT
#undef LEVMAR_BLEIC_DER
#undef LEVMAR_BLEIC_DIF
#undef LEVMAR_BLIC_DER
#undef LEVMAR_BLIC_DIF
#undef LEVMAR_LEIC_DER
#undef LEVMAR_LEIC_DIF
#undef LEVMAR_LIC_DER
#undef LEVMAR_LIC_DIF
#undef LEVMAR_BLEC_DER
#undef LEVMAR_BLEC_DIF
