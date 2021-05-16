/////////////////////////////////////////////////////////////////////////////////
// 
//  Solution of linear systems involved in the Levenberg - Marquardt
//  minimization algorithm
//  Copyright (C) 2004  Manolis Lourakis (lourakis at ics forth gr)
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


/* Solvers for the linear systems Ax=b. Solvers should NOT modify their A & B arguments! */


#ifndef LM_REAL // not included by Axb.c
#error This file should not be compiled directly!
#endif


#ifdef LINSOLVERS_RETAIN_MEMORY
#define __STATIC__ static
#else
#define __STATIC__ // empty
#endif /* LINSOLVERS_RETAIN_MEMORY */

#ifdef HAVE_LAPACK

/* prototypes of LAPACK routines */

#define GEQRF LM_MK_LAPACK_NAME(geqrf)
#define ORGQR LM_MK_LAPACK_NAME(orgqr)
#define TRTRS LM_MK_LAPACK_NAME(trtrs)
#define POTF2 LM_MK_LAPACK_NAME(potf2)
#define POTRF LM_MK_LAPACK_NAME(potrf)
#define POTRS LM_MK_LAPACK_NAME(potrs)
#define GETRF LM_MK_LAPACK_NAME(getrf)
#define GETRS LM_MK_LAPACK_NAME(getrs)
#define GESVD LM_MK_LAPACK_NAME(gesvd)
#define GESDD LM_MK_LAPACK_NAME(gesdd)
#define SYTRF LM_MK_LAPACK_NAME(sytrf)
#define SYTRS LM_MK_LAPACK_NAME(sytrs)
#define PLASMA_POSV LM_CAT_(PLASMA_, LM_ADD_PREFIX(posv))

#ifdef __cplusplus
extern "C" {
#endif
/* QR decomposition */
extern int GEQRF(int *m, int *n, LM_REAL *a, int *lda, LM_REAL *tau, LM_REAL *work, int *lwork, int *info);
extern int ORGQR(int *m, int *n, int *k, LM_REAL *a, int *lda, LM_REAL *tau, LM_REAL *work, int *lwork, int *info);

/* solution of triangular systems */
extern int TRTRS(char *uplo, char *trans, char *diag, int *n, int *nrhs, LM_REAL *a, int *lda, LM_REAL *b, int *ldb, int *info);

/* Cholesky decomposition and systems solution */
extern int POTF2(char *uplo, int *n, LM_REAL *a, int *lda, int *info);
extern int POTRF(char *uplo, int *n, LM_REAL *a, int *lda, int *info); /* block version of dpotf2 */
extern int POTRS(char *uplo, int *n, int *nrhs, LM_REAL *a, int *lda, LM_REAL *b, int *ldb, int *info);

/* LU decomposition and systems solution */
extern int GETRF(int *m, int *n, LM_REAL *a, int *lda, int *ipiv, int *info);
extern int GETRS(char *trans, int *n, int *nrhs, LM_REAL *a, int *lda, int *ipiv, LM_REAL *b, int *ldb, int *info);

/* Singular Value Decomposition (SVD) */
extern int GESVD(char *jobu, char *jobvt, int *m, int *n, LM_REAL *a, int *lda, LM_REAL *s, LM_REAL *u, int *ldu,
                   LM_REAL *vt, int *ldvt, LM_REAL *work, int *lwork, int *info);

/* lapack 3.0 new SVD routine, faster than xgesvd().
 * In case that your version of LAPACK does not include them, use the above two older routines
 */
extern int GESDD(char *jobz, int *m, int *n, LM_REAL *a, int *lda, LM_REAL *s, LM_REAL *u, int *ldu, LM_REAL *vt, int *ldvt,
                   LM_REAL *work, int *lwork, int *iwork, int *info);

/* LDLt/UDUt factorization and systems solution */
extern int SYTRF(char *uplo, int *n, LM_REAL *a, int *lda, int *ipiv, LM_REAL *work, int *lwork, int *info);
extern int SYTRS(char *uplo, int *n, int *nrhs, LM_REAL *a, int *lda, int *ipiv, LM_REAL *b, int *ldb, int *info);
#ifdef __cplusplus
}
#endif

/* precision-specific definitions */
#define AX_EQ_B_QR LM_ADD_PREFIX(Ax_eq_b_QR)
#define AX_EQ_B_QRLS LM_ADD_PREFIX(Ax_eq_b_QRLS)
#define AX_EQ_B_CHOL LM_ADD_PREFIX(Ax_eq_b_Chol)
#define AX_EQ_B_LU LM_ADD_PREFIX(Ax_eq_b_LU)
#define AX_EQ_B_SVD LM_ADD_PREFIX(Ax_eq_b_SVD)
#define AX_EQ_B_BK LM_ADD_PREFIX(Ax_eq_b_BK)
#define AX_EQ_B_PLASMA_CHOL LM_ADD_PREFIX(Ax_eq_b_PLASMA_Chol)

/*
 * This function returns the solution of Ax = b
 *
 * The function is based on QR decomposition with explicit computation of Q:
 * If A=Q R with Q orthogonal and R upper triangular, the linear system becomes
 * Q R x = b or R x = Q^T b.
 * The last equation can be solved directly.
 *
 * A is mxm, b is mx1
 *
 * The function returns 0 in case of error, 1 if successful
 *
 * This function is often called repetitively to solve problems of identical
 * dimensions. To avoid repetitive malloc's and free's, allocated memory is
 * retained between calls and free'd-malloc'ed when not of the appropriate size.
 * A call with NULL as the first argument forces this memory to be released.
 */
int AX_EQ_B_QR(LM_REAL *A, LM_REAL *B, LM_REAL *x, int m)
{
__STATIC__ LM_REAL *buf=NULL;
__STATIC__ int buf_sz=0;

static int nb=0; /* no __STATIC__ decl. here! */

LM_REAL *a, *tau, *r, *work;
int a_sz, tau_sz, r_sz, tot_sz;
register int i, j;
int info, worksz, nrhs=1;
register LM_REAL sum;

    if(!A)
#ifdef LINSOLVERS_RETAIN_MEMORY
    {
      if(buf) free(buf);
      buf=NULL;
      buf_sz=0;

      return 1;
    }
#else
      return 1; /* NOP */
#endif /* LINSOLVERS_RETAIN_MEMORY */
   
    /* calculate required memory size */
    a_sz=m*m;
    tau_sz=m;
    r_sz=m*m; /* only the upper triangular part really needed */
    if(!nb){
      LM_REAL tmp;

      worksz=-1; // workspace query; optimal size is returned in tmp
      GEQRF((int *)&m, (int *)&m, NULL, (int *)&m, NULL, (LM_REAL *)&tmp, (int *)&worksz, (int *)&info);
      nb=((int)tmp)/m; // optimal worksize is m*nb
    }
    worksz=nb*m;
    tot_sz=a_sz + tau_sz + r_sz + worksz;

#ifdef LINSOLVERS_RETAIN_MEMORY
    if(tot_sz>buf_sz){ /* insufficient memory, allocate a "big" memory chunk at once */
      if(buf) free(buf); /* free previously allocated memory */

      buf_sz=tot_sz;
      buf=(LM_REAL *)malloc(buf_sz*sizeof(LM_REAL));
      if(!buf){
        fprintf(stderr, RCAT("memory allocation in ", AX_EQ_B_QR) "() failed!\n");
        exit(1);
      }
    }
#else
      buf_sz=tot_sz;
      buf=(LM_REAL *)malloc(buf_sz*sizeof(LM_REAL));
      if(!buf){
        fprintf(stderr, RCAT("memory allocation in ", AX_EQ_B_QR) "() failed!\n");
        exit(1);
      }
#endif /* LINSOLVERS_RETAIN_MEMORY */

    a=buf;
    tau=a+a_sz;
    r=tau+tau_sz;
    work=r+r_sz;

  /* store A (column major!) into a */
	for(i=0; i<m; i++)
		for(j=0; j<m; j++)
			a[i+j*m]=A[i*m+j];

  /* QR decomposition of A */
  GEQRF((int *)&m, (int *)&m, a, (int *)&m, tau, work, (int *)&worksz, (int *)&info);
  /* error treatment */
  if(info!=0){
    if(info<0){
      fprintf(stderr, RCAT(RCAT("LAPACK error: illegal value for argument %d of ", GEQRF) " in ", AX_EQ_B_QR) "()\n", -info);
      exit(1);
    }
    else{
      fprintf(stderr, RCAT(RCAT("Unknown LAPACK error %d for ", GEQRF) " in ", AX_EQ_B_QR) "()\n", info);
#ifndef LINSOLVERS_RETAIN_MEMORY
      free(buf);
#endif

      return 0;
    }
  }

  /* R is stored in the upper triangular part of a; copy it in r so that ORGQR() below won't destroy it */ 
  memcpy(r, a, r_sz*sizeof(LM_REAL));

  /* compute Q using the elementary reflectors computed by the above decomposition */
  ORGQR((int *)&m, (int *)&m, (int *)&m, a, (int *)&m, tau, work, (int *)&worksz, (int *)&info);
  if(info!=0){
    if(info<0){
      fprintf(stderr, RCAT(RCAT("LAPACK error: illegal value for argument %d of ", ORGQR) " in ", AX_EQ_B_QR) "()\n", -info);
      exit(1);
    }
    else{
      fprintf(stderr, RCAT("Unknown LAPACK error (%d) in ", AX_EQ_B_QR) "()\n", info);
#ifndef LINSOLVERS_RETAIN_MEMORY
      free(buf);
#endif

      return 0;
    }
  }

  /* Q is now in a; compute Q^T b in x */
  for(i=0; i<m; i++){
    for(j=0, sum=0.0; j<m; j++)
      sum+=a[i*m+j]*B[j];
    x[i]=sum;
  }

  /* solve the linear system R x = Q^t b */
  TRTRS("U", "N", "N", (int *)&m, (int *)&nrhs, r, (int *)&m, x, (int *)&m, &info);
  /* error treatment */
  if(info!=0){
    if(info<0){
      fprintf(stderr, RCAT(RCAT("LAPACK error: illegal value for argument %d of ", TRTRS) " in ", AX_EQ_B_QR) "()\n", -info);
      exit(1);
    }
    else{
      fprintf(stderr, RCAT("LAPACK error: the %d-th diagonal element of A is zero (singular matrix) in ", AX_EQ_B_QR) "()\n", info);
#ifndef LINSOLVERS_RETAIN_MEMORY
      free(buf);
#endif

      return 0;
    }
  }

#ifndef LINSOLVERS_RETAIN_MEMORY
  free(buf);
#endif

	return 1;
}

/*
 * This function returns the solution of min_x ||Ax - b||
 *
 * || . || is the second order (i.e. L2) norm. This is a least squares technique that
 * is based on QR decomposition:
 * If A=Q R with Q orthogonal and R upper triangular, the normal equations become
 * (A^T A) x = A^T b  or (R^T Q^T Q R) x = A^T b or (R^T R) x = A^T b.
 * This amounts to solving R^T y = A^T b for y and then R x = y for x
 * Note that Q does not need to be explicitly computed
 *
 * A is mxn, b is mx1
 *
 * The function returns 0 in case of error, 1 if successful
 *
 * This function is often called repetitively to solve problems of identical
 * dimensions. To avoid repetitive malloc's and free's, allocated memory is
 * retained between calls and free'd-malloc'ed when not of the appropriate size.
 * A call with NULL as the first argument forces this memory to be released.
 */
int AX_EQ_B_QRLS(LM_REAL *A, LM_REAL *B, LM_REAL *x, int m, int n)
{
__STATIC__ LM_REAL *buf=NULL;
__STATIC__ int buf_sz=0;

static int nb=0; /* no __STATIC__ decl. here! */

LM_REAL *a, *tau, *r, *work;
int a_sz, tau_sz, r_sz, tot_sz;
register int i, j;
int info, worksz, nrhs=1;
register LM_REAL sum;
   
    if(!A)
#ifdef LINSOLVERS_RETAIN_MEMORY
    {
      if(buf) free(buf);
      buf=NULL;
      buf_sz=0;

      return 1;
    }
#else
      return 1; /* NOP */
#endif /* LINSOLVERS_RETAIN_MEMORY */
   
    if(m<n){
		  fprintf(stderr, RCAT("Normal equations require that the number of rows is greater than number of columns in ", AX_EQ_B_QRLS) "() [%d x %d]! -- try transposing\n", m, n);
		  exit(1);
	  }
      
    /* calculate required memory size */
    a_sz=m*n;
    tau_sz=n;
    r_sz=n*n;
    if(!nb){
      LM_REAL tmp;

      worksz=-1; // workspace query; optimal size is returned in tmp
      GEQRF((int *)&m, (int *)&m, NULL, (int *)&m, NULL, (LM_REAL *)&tmp, (int *)&worksz, (int *)&info);
      nb=((int)tmp)/m; // optimal worksize is m*nb
    }
    worksz=nb*m;
    tot_sz=a_sz + tau_sz + r_sz + worksz;

#ifdef LINSOLVERS_RETAIN_MEMORY
    if(tot_sz>buf_sz){ /* insufficient memory, allocate a "big" memory chunk at once */
      if(buf) free(buf); /* free previously allocated memory */

      buf_sz=tot_sz;
      buf=(LM_REAL *)malloc(buf_sz*sizeof(LM_REAL));
      if(!buf){
        fprintf(stderr, RCAT("memory allocation in ", AX_EQ_B_QRLS) "() failed!\n");
        exit(1);
      }
    }
#else
      buf_sz=tot_sz;
      buf=(LM_REAL *)malloc(buf_sz*sizeof(LM_REAL));
      if(!buf){
        fprintf(stderr, RCAT("memory allocation in ", AX_EQ_B_QRLS) "() failed!\n");
        exit(1);
      }
#endif /* LINSOLVERS_RETAIN_MEMORY */

    a=buf;
    tau=a+a_sz;
    r=tau+tau_sz;
    work=r+r_sz;

  /* store A (column major!) into a */
	for(i=0; i<m; i++)
		for(j=0; j<n; j++)
			a[i+j*m]=A[i*n+j];

  /* compute A^T b in x */
  for(i=0; i<n; i++){
    for(j=0, sum=0.0; j<m; j++)
      sum+=A[j*n+i]*B[j];
    x[i]=sum;
  }

  /* QR decomposition of A */
  GEQRF((int *)&m, (int *)&n, a, (int *)&m, tau, work, (int *)&worksz, (int *)&info);
  /* error treatment */
  if(info!=0){
    if(info<0){
      fprintf(stderr, RCAT(RCAT("LAPACK error: illegal value for argument %d of ", GEQRF) " in ", AX_EQ_B_QRLS) "()\n", -info);
      exit(1);
    }
    else{
      fprintf(stderr, RCAT(RCAT("Unknown LAPACK error %d for ", GEQRF) " in ", AX_EQ_B_QRLS) "()\n", info);
#ifndef LINSOLVERS_RETAIN_MEMORY
      free(buf);
#endif

      return 0;
    }
  }

  /* R is stored in the upper triangular part of a. Note that a is mxn while r nxn */
  for(j=0; j<n; j++){
    for(i=0; i<=j; i++)
      r[i+j*n]=a[i+j*m];

    /* lower part is zero */
    for(i=j+1; i<n; i++)
      r[i+j*n]=0.0;
  }

  /* solve the linear system R^T y = A^t b */
  TRTRS("U", "T", "N", (int *)&n, (int *)&nrhs, r, (int *)&n, x, (int *)&n, &info);
  /* error treatment */
  if(info!=0){
    if(info<0){
      fprintf(stderr, RCAT(RCAT("LAPACK error: illegal value for argument %d of ", TRTRS) " in ", AX_EQ_B_QRLS) "()\n", -info);
      exit(1);
    }
    else{
      fprintf(stderr, RCAT("LAPACK error: the %d-th diagonal element of A is zero (singular matrix) in ", AX_EQ_B_QRLS) "()\n", info);
#ifndef LINSOLVERS_RETAIN_MEMORY
      free(buf);
#endif

      return 0;
    }
  }

  /* solve the linear system R x = y */
  TRTRS("U", "N", "N", (int *)&n, (int *)&nrhs, r, (int *)&n, x, (int *)&n, &info);
  /* error treatment */
  if(info!=0){
    if(info<0){
      fprintf(stderr, RCAT(RCAT("LAPACK error: illegal value for argument %d of ", TRTRS) " in ", AX_EQ_B_QRLS) "()\n", -info);
      exit(1);
    }
    else{
      fprintf(stderr, RCAT("LAPACK error: the %d-th diagonal element of A is zero (singular matrix) in ", AX_EQ_B_QRLS) "()\n", info);
#ifndef LINSOLVERS_RETAIN_MEMORY
      free(buf);
#endif

      return 0;
    }
  }

#ifndef LINSOLVERS_RETAIN_MEMORY
  free(buf);
#endif

	return 1;
}

/*
 * This function returns the solution of Ax=b
 *
 * The function assumes that A is symmetric & postive definite and employs
 * the Cholesky decomposition:
 * If A=L L^T with L lower triangular, the system to be solved becomes
 * (L L^T) x = b
 * This amounts to solving L y = b for y and then L^T x = y for x
 *
 * A is mxm, b is mx1
 *
 * The function returns 0 in case of error, 1 if successful
 *
 * This function is often called repetitively to solve problems of identical
 * dimensions. To avoid repetitive malloc's and free's, allocated memory is
 * retained between calls and free'd-malloc'ed when not of the appropriate size.
 * A call with NULL as the first argument forces this memory to be released.
 */
int AX_EQ_B_CHOL(LM_REAL *A, LM_REAL *B, LM_REAL *x, int m)
{
__STATIC__ LM_REAL *buf=NULL;
__STATIC__ int buf_sz=0;

LM_REAL *a;
int a_sz, tot_sz;
int info, nrhs=1;
   
    if(!A)
#ifdef LINSOLVERS_RETAIN_MEMORY
    {
      if(buf) free(buf);
      buf=NULL;
      buf_sz=0;

      return 1;
    }
#else
      return 1; /* NOP */
#endif /* LINSOLVERS_RETAIN_MEMORY */
   
    /* calculate required memory size */
    a_sz=m*m;
    tot_sz=a_sz;

#ifdef LINSOLVERS_RETAIN_MEMORY
    if(tot_sz>buf_sz){ /* insufficient memory, allocate a "big" memory chunk at once */
      if(buf) free(buf); /* free previously allocated memory */

      buf_sz=tot_sz;
      buf=(LM_REAL *)malloc(buf_sz*sizeof(LM_REAL));
      if(!buf){
        fprintf(stderr, RCAT("memory allocation in ", AX_EQ_B_CHOL) "() failed!\n");
        exit(1);
      }
    }
#else
      buf_sz=tot_sz;
      buf=(LM_REAL *)malloc(buf_sz*sizeof(LM_REAL));
      if(!buf){
        fprintf(stderr, RCAT("memory allocation in ", AX_EQ_B_CHOL) "() failed!\n");
        exit(1);
      }
#endif /* LINSOLVERS_RETAIN_MEMORY */

  a=buf;

  /* store A into a and B into x. A is assumed symmetric,
   * hence no transposition is needed
   */
  memcpy(a, A, a_sz*sizeof(LM_REAL));
  memcpy(x, B, m*sizeof(LM_REAL));

  /* Cholesky decomposition of A */
  //POTF2("L", (int *)&m, a, (int *)&m, (int *)&info);
  POTRF("L", (int *)&m, a, (int *)&m, (int *)&info);
  /* error treatment */
  if(info!=0){
    if(info<0){
      fprintf(stderr, RCAT(RCAT(RCAT("LAPACK error: illegal value for argument %d of ", POTF2) "/", POTRF) " in ",
                      AX_EQ_B_CHOL) "()\n", -info);
      exit(1);
    }
    else{
      fprintf(stderr, RCAT(RCAT(RCAT("LAPACK error: the leading minor of order %d is not positive definite,\nthe factorization could not be completed for ", POTF2) "/", POTRF) " in ", AX_EQ_B_CHOL) "()\n", info);
#ifndef LINSOLVERS_RETAIN_MEMORY
      free(buf);
#endif

      return 0;
    }
  }

  /* solve using the computed Cholesky in one lapack call */
  POTRS("L", (int *)&m, (int *)&nrhs, a, (int *)&m, x, (int *)&m, &info);
  if(info<0){
    fprintf(stderr, RCAT(RCAT("LAPACK error: illegal value for argument %d of ", POTRS) " in ", AX_EQ_B_CHOL) "()\n", -info);
    exit(1);
  }

#if 0
  /* alternative: solve the linear system L y = b ... */
  TRTRS("L", "N", "N", (int *)&m, (int *)&nrhs, a, (int *)&m, x, (int *)&m, &info);
  /* error treatment */
  if(info!=0){
    if(info<0){
      fprintf(stderr, RCAT(RCAT("LAPACK error: illegal value for argument %d of ", TRTRS) " in ", AX_EQ_B_CHOL) "()\n", -info);
      exit(1);
    }
    else{
      fprintf(stderr, RCAT("LAPACK error: the %d-th diagonal element of A is zero (singular matrix) in ", AX_EQ_B_CHOL) "()\n", info);
#ifndef LINSOLVERS_RETAIN_MEMORY
      free(buf);
#endif

      return 0;
    }
  }

  /* ... solve the linear system L^T x = y */
  TRTRS("L", "T", "N", (int *)&m, (int *)&nrhs, a, (int *)&m, x, (int *)&m, &info);
  /* error treatment */
  if(info!=0){
    if(info<0){
      fprintf(stderr, RCAT(RCAT("LAPACK error: illegal value for argument %d of ", TRTRS) "in ", AX_EQ_B_CHOL) "()\n", -info);
      exit(1);
    }
    else{
      fprintf(stderr, RCAT("LAPACK error: the %d-th diagonal element of A is zero (singular matrix) in ", AX_EQ_B_CHOL) "()\n", info);
#ifndef LINSOLVERS_RETAIN_MEMORY
      free(buf);
#endif

      return 0;
    }
  }
#endif /* 0 */

#ifndef LINSOLVERS_RETAIN_MEMORY
  free(buf);
#endif

	return 1;
}

#ifdef HAVE_PLASMA

/* Linear algebra using PLASMA parallel library for multicore CPUs.
 * http://icl.cs.utk.edu/plasma/
 *
 * WARNING: BLAS multithreading should be disabled, e.g. setenv MKL_NUM_THREADS 1
 */

#ifndef _LM_PLASMA_MISC_
/* avoid multiple inclusion of helper code */
#define _LM_PLASMA_MISC_

#include <plasma.h>
#include <cblas.h>
#include <lapacke.h>
#include <plasma_tmg.h>
#include <core_blas.h>

/* programmatically determine the number of cores on the current machine */
#ifdef _WIN32
#include <windows.h>
#elif __linux
#include <unistd.h>
#endif
static int getnbcores()
{
#ifdef _WIN32
  SYSTEM_INFO sysinfo;
  GetSystemInfo(&sysinfo);
  return sysinfo.dwNumberOfProcessors;
#elif __linux
  return sysconf(_SC_NPROCESSORS_ONLN);
#else // unknown system
  return 2<<1; // will be halved by right shift below
#endif
}

static int PLASMA_ncores=-(getnbcores()>>1); // >0 if PLASMA initialized, <0 otherwise

/* user-specified number of cores */
void levmar_PLASMA_setnbcores(int cores)
{
  PLASMA_ncores=(cores>0)? -cores : ((cores)? cores : -2);
}
#endif /* _LM_PLASMA_MISC_ */

/*
 * This function returns the solution of Ax=b
 *
 * The function assumes that A is symmetric & positive definite and employs the
 * Cholesky decomposition implemented by PLASMA for homogeneous multicore processors.
 *
 * A is mxm, b is mx1
 *
 * The function returns 0 in case of error, 1 if successfull
 *
 * This function is often called repetitively to solve problems of identical
 * dimensions. To avoid repetitive malloc's and free's, allocated memory is
 * retained between calls and free'd-malloc'ed when not of the appropriate size.
 * A call with NULL as the first argument forces this memory to be released.
 */
int AX_EQ_B_PLASMA_CHOL(LM_REAL *A, LM_REAL *B, LM_REAL *x, int m)
{
__STATIC__ LM_REAL *buf=NULL;
__STATIC__ int buf_sz=0;

LM_REAL *a;
int a_sz, tot_sz;
int info, nrhs=1;

    if(A==NULL){
#ifdef LINSOLVERS_RETAIN_MEMORY
      if(buf) free(buf);
      buf=NULL;
      buf_sz=0;
#endif /* LINSOLVERS_RETAIN_MEMORY */

      PLASMA_Finalize();
      PLASMA_ncores=-PLASMA_ncores;

      return 1;
    }

    /* calculate required memory size */
    a_sz=m*m;
    tot_sz=a_sz;

#ifdef LINSOLVERS_RETAIN_MEMORY
    if(tot_sz>buf_sz){ /* insufficient memory, allocate a "big" memory chunk at once */
      if(buf) free(buf); /* free previously allocated memory */

      buf_sz=tot_sz;
      buf=(LM_REAL *)malloc(buf_sz*sizeof(LM_REAL));
      if(!buf){
        fprintf(stderr, RCAT("memory allocation in ", AX_EQ_B_PLASMA_CHOL) "() failed!\n");
        exit(1);
      }
    }
#else
    buf_sz=tot_sz;
    buf=(LM_REAL *)malloc(buf_sz*sizeof(LM_REAL));
    if(!buf){
      fprintf(stderr, RCAT("memory allocation in ", AX_EQ_B_PLASMA_CHOL) "() failed!\n");
      exit(1);
    }
#endif /* LINSOLVERS_RETAIN_MEMORY */

    a=buf;

    /* store A into a and B into x; A is assumed to be symmetric,
     * hence no transposition is needed
     */
    memcpy(a, A, a_sz*sizeof(LM_REAL));
    memcpy(x, B, m*sizeof(LM_REAL));

  /* initialize PLASMA */
  if(PLASMA_ncores<0){
    PLASMA_ncores=-PLASMA_ncores;
    PLASMA_Init(PLASMA_ncores);
    fprintf(stderr, RCAT("\n", AX_EQ_B_PLASMA_CHOL) "(): PLASMA is running on %d cores.\n\n", PLASMA_ncores);
  }
  
  /* Solve the linear system */
  info=PLASMA_POSV(PlasmaLower, m, 1, a, m, x, m);
  /* error treatment */
  if(info!=0){
    if(info<0){
      fprintf(stderr, RCAT(RCAT("LAPACK error: illegal value for argument %d of ", PLASMA_POSV) " in ",
                      AX_EQ_B_PLASMA_CHOL) "()\n", -info);
      exit(1);
    }
    else{
      fprintf(stderr, RCAT(RCAT("LAPACK error: the leading minor of order %d is not positive definite,\n"
                                "the factorization could not be completed for ", PLASMA_POSV) " in ", AX_EQ_B_CHOL) "()\n", info);
#ifndef LINSOLVERS_RETAIN_MEMORY
      free(buf);
#endif
      return 0;
    }
  }

#ifndef LINSOLVERS_RETAIN_MEMORY
  free(buf);
#endif

	return 1;
}
#endif /* HAVE_PLASMA */

/*
 * This function returns the solution of Ax = b
 *
 * The function employs LU decomposition:
 * If A=L U with L lower and U upper triangular, then the original system
 * amounts to solving
 * L y = b, U x = y
 *
 * A is mxm, b is mx1
 *
 * The function returns 0 in case of error, 1 if successful
 *
 * This function is often called repetitively to solve problems of identical
 * dimensions. To avoid repetitive malloc's and free's, allocated memory is
 * retained between calls and free'd-malloc'ed when not of the appropriate size.
 * A call with NULL as the first argument forces this memory to be released.
 */
int AX_EQ_B_LU(LM_REAL *A, LM_REAL *B, LM_REAL *x, int m)
{
__STATIC__ LM_REAL *buf=NULL;
__STATIC__ int buf_sz=0;

int a_sz, ipiv_sz, tot_sz;
register int i, j;
int info, *ipiv, nrhs=1;
LM_REAL *a;
   
    if(!A)
#ifdef LINSOLVERS_RETAIN_MEMORY
    {
      if(buf) free(buf);
      buf=NULL;
      buf_sz=0;

      return 1;
    }
#else
      return 1; /* NOP */
#endif /* LINSOLVERS_RETAIN_MEMORY */
   
    /* calculate required memory size */
    ipiv_sz=m;
    a_sz=m*m;
    tot_sz=a_sz*sizeof(LM_REAL) + ipiv_sz*sizeof(int); /* should be arranged in that order for proper doubles alignment */

#ifdef LINSOLVERS_RETAIN_MEMORY
    if(tot_sz>buf_sz){ /* insufficient memory, allocate a "big" memory chunk at once */
      if(buf) free(buf); /* free previously allocated memory */

      buf_sz=tot_sz;
      buf=(LM_REAL *)malloc(buf_sz);
      if(!buf){
        fprintf(stderr, RCAT("memory allocation in ", AX_EQ_B_LU) "() failed!\n");
        exit(1);
      }
    }
#else
      buf_sz=tot_sz;
      buf=(LM_REAL *)malloc(buf_sz);
      if(!buf){
        fprintf(stderr, RCAT("memory allocation in ", AX_EQ_B_LU) "() failed!\n");
        exit(1);
      }
#endif /* LINSOLVERS_RETAIN_MEMORY */

    a=buf;
    ipiv=(int *)(a+a_sz);

    /* store A (column major!) into a and B into x */
	  for(i=0; i<m; i++){
		  for(j=0; j<m; j++)
        a[i+j*m]=A[i*m+j];

      x[i]=B[i];
    }

  /* LU decomposition for A */
	GETRF((int *)&m, (int *)&m, a, (int *)&m, ipiv, (int *)&info);  
	if(info!=0){
		if(info<0){
      fprintf(stderr, RCAT(RCAT("argument %d of ", GETRF) " illegal in ", AX_EQ_B_LU) "()\n", -info);
			exit(1);
		}
		else{
      fprintf(stderr, RCAT(RCAT("singular matrix A for ", GETRF) " in ", AX_EQ_B_LU) "()\n");
#ifndef LINSOLVERS_RETAIN_MEMORY
      free(buf);
#endif

			return 0;
		}
	}

  /* solve the system with the computed LU */
  GETRS("N", (int *)&m, (int *)&nrhs, a, (int *)&m, ipiv, x, (int *)&m, (int *)&info);
	if(info!=0){
		if(info<0){
			fprintf(stderr, RCAT(RCAT("argument %d of ", GETRS) " illegal in ", AX_EQ_B_LU) "()\n", -info);
			exit(1);
		}
		else{
			fprintf(stderr, RCAT(RCAT("unknown error for ", GETRS) " in ", AX_EQ_B_LU) "()\n");
#ifndef LINSOLVERS_RETAIN_MEMORY
      free(buf);
#endif

			return 0;
		}
	}

#ifndef LINSOLVERS_RETAIN_MEMORY
  free(buf);
#endif

	return 1;
}

/*
 * This function returns the solution of Ax = b
 *
 * The function is based on SVD decomposition:
 * If A=U D V^T with U, V orthogonal and D diagonal, the linear system becomes
 * (U D V^T) x = b or x=V D^{-1} U^T b
 * Note that V D^{-1} U^T is the pseudoinverse A^+
 *
 * A is mxm, b is mx1.
 *
 * The function returns 0 in case of error, 1 if successful
 *
 * This function is often called repetitively to solve problems of identical
 * dimensions. To avoid repetitive malloc's and free's, allocated memory is
 * retained between calls and free'd-malloc'ed when not of the appropriate size.
 * A call with NULL as the first argument forces this memory to be released.
 */
int AX_EQ_B_SVD(LM_REAL *A, LM_REAL *B, LM_REAL *x, int m)
{
__STATIC__ LM_REAL *buf=NULL;
__STATIC__ int buf_sz=0;
static LM_REAL eps=LM_CNST(-1.0);

register int i, j;
LM_REAL *a, *u, *s, *vt, *work;
int a_sz, u_sz, s_sz, vt_sz, tot_sz;
LM_REAL thresh, one_over_denom;
register LM_REAL sum;
int info, rank, worksz, *iwork, iworksz;
   
    if(!A)
#ifdef LINSOLVERS_RETAIN_MEMORY
    {
      if(buf) free(buf);
      buf=NULL;
      buf_sz=0;

      return 1;
    }
#else
      return 1; /* NOP */
#endif /* LINSOLVERS_RETAIN_MEMORY */
   
  /* calculate required memory size */
#if 1 /* use optimal size */
  worksz=-1; // workspace query. Keep in mind that GESDD requires more memory than GESVD
  /* note that optimal work size is returned in thresh */
  GESVD("A", "A", (int *)&m, (int *)&m, NULL, (int *)&m, NULL, NULL, (int *)&m, NULL, (int *)&m, (LM_REAL *)&thresh, (int *)&worksz, &info);
  //GESDD("A", (int *)&m, (int *)&m, NULL, (int *)&m, NULL, NULL, (int *)&m, NULL, (int *)&m, (LM_REAL *)&thresh, (int *)&worksz, NULL, &info);
  worksz=(int)thresh;
#else /* use minimum size */
  worksz=5*m; // min worksize for GESVD
  //worksz=m*(7*m+4); // min worksize for GESDD
#endif
  iworksz=8*m;
  a_sz=m*m;
  u_sz=m*m; s_sz=m; vt_sz=m*m;

  tot_sz=(a_sz + u_sz + s_sz + vt_sz + worksz)*sizeof(LM_REAL) + iworksz*sizeof(int); /* should be arranged in that order for proper doubles alignment */

#ifdef LINSOLVERS_RETAIN_MEMORY
  if(tot_sz>buf_sz){ /* insufficient memory, allocate a "big" memory chunk at once */
    if(buf) free(buf); /* free previously allocated memory */

    buf_sz=tot_sz;
    buf=(LM_REAL *)malloc(buf_sz);
    if(!buf){
      fprintf(stderr, RCAT("memory allocation in ", AX_EQ_B_SVD) "() failed!\n");
      exit(1);
    }
  }
#else
    buf_sz=tot_sz;
    buf=(LM_REAL *)malloc(buf_sz);
    if(!buf){
      fprintf(stderr, RCAT("memory allocation in ", AX_EQ_B_SVD) "() failed!\n");
      exit(1);
    }
#endif /* LINSOLVERS_RETAIN_MEMORY */

  a=buf;
  u=a+a_sz;
  s=u+u_sz;
  vt=s+s_sz;
  work=vt+vt_sz;
  iwork=(int *)(work+worksz);

  /* store A (column major!) into a */
  for(i=0; i<m; i++)
    for(j=0; j<m; j++)
      a[i+j*m]=A[i*m+j];

  /* SVD decomposition of A */
  GESVD("A", "A", (int *)&m, (int *)&m, a, (int *)&m, s, u, (int *)&m, vt, (int *)&m, work, (int *)&worksz, &info);
  //GESDD("A", (int *)&m, (int *)&m, a, (int *)&m, s, u, (int *)&m, vt, (int *)&m, work, (int *)&worksz, iwork, &info);

  /* error treatment */
  if(info!=0){
    if(info<0){
      fprintf(stderr, RCAT(RCAT(RCAT("LAPACK error: illegal value for argument %d of ", GESVD), "/" GESDD) " in ", AX_EQ_B_SVD) "()\n", -info);
      exit(1);
    }
    else{
      fprintf(stderr, RCAT("LAPACK error: dgesdd (dbdsdc)/dgesvd (dbdsqr) failed to converge in ", AX_EQ_B_SVD) "() [info=%d]\n", info);
#ifndef LINSOLVERS_RETAIN_MEMORY
      free(buf);
#endif

      return 0;
    }
  }

  if(eps<0.0){
    LM_REAL aux;

    /* compute machine epsilon */
    for(eps=LM_CNST(1.0); aux=eps+LM_CNST(1.0), aux-LM_CNST(1.0)>0.0; eps*=LM_CNST(0.5))
                                          ;
    eps*=LM_CNST(2.0);
  }

  /* compute the pseudoinverse in a */
	for(i=0; i<a_sz; i++) a[i]=0.0; /* initialize to zero */
  for(rank=0, thresh=eps*s[0]; rank<m && s[rank]>thresh; rank++){
    one_over_denom=LM_CNST(1.0)/s[rank];

    for(j=0; j<m; j++)
      for(i=0; i<m; i++)
        a[i*m+j]+=vt[rank+i*m]*u[j+rank*m]*one_over_denom;
  }

	/* compute A^+ b in x */
	for(i=0; i<m; i++){
	  for(j=0, sum=0.0; j<m; j++)
      sum+=a[i*m+j]*B[j];
    x[i]=sum;
  }

#ifndef LINSOLVERS_RETAIN_MEMORY
  free(buf);
#endif

	return 1;
}

/*
 * This function returns the solution of Ax = b for a real symmetric matrix A
 *
 * The function is based on LDLT factorization with the pivoting
 * strategy of Bunch and Kaufman:
 * A is factored as L*D*L^T where L is lower triangular and
 * D symmetric and block diagonal (aka spectral decomposition,
 * Banachiewicz factorization, modified Cholesky factorization)
 *
 * A is mxm, b is mx1.
 *
 * The function returns 0 in case of error, 1 if successfull
 *
 * This function is often called repetitively to solve problems of identical
 * dimensions. To avoid repetitive malloc's and free's, allocated memory is
 * retained between calls and free'd-malloc'ed when not of the appropriate size.
 * A call with NULL as the first argument forces this memory to be released.
 */
int AX_EQ_B_BK(LM_REAL *A, LM_REAL *B, LM_REAL *x, int m)
{
__STATIC__ LM_REAL *buf=NULL;
__STATIC__ int buf_sz=0, nb=0;

LM_REAL *a, *work;
int a_sz, ipiv_sz, work_sz, tot_sz;
int info, *ipiv, nrhs=1;
   
  if(!A)
#ifdef LINSOLVERS_RETAIN_MEMORY
  {
    if(buf) free(buf);
    buf=NULL;
    buf_sz=0;

    return 1;
  }
#else
  return 1; /* NOP */
#endif /* LINSOLVERS_RETAIN_MEMORY */

  /* calculate required memory size */
  ipiv_sz=m;
  a_sz=m*m;
  if(!nb){
    LM_REAL tmp;

    work_sz=-1; // workspace query; optimal size is returned in tmp
    SYTRF("L", (int *)&m, NULL, (int *)&m, NULL, (LM_REAL *)&tmp, (int *)&work_sz, (int *)&info);
    nb=((int)tmp)/m; // optimal worksize is m*nb
  }
  work_sz=(nb!=-1)? nb*m : 1;
  tot_sz=(a_sz + work_sz)*sizeof(LM_REAL) + ipiv_sz*sizeof(int); /* should be arranged in that order for proper doubles alignment */

#ifdef LINSOLVERS_RETAIN_MEMORY
  if(tot_sz>buf_sz){ /* insufficient memory, allocate a "big" memory chunk at once */
    if(buf) free(buf); /* free previously allocated memory */

    buf_sz=tot_sz;
    buf=(LM_REAL *)malloc(buf_sz);
    if(!buf){
      fprintf(stderr, RCAT("memory allocation in ", AX_EQ_B_BK) "() failed!\n");
      exit(1);
    }
  }
#else
  buf_sz=tot_sz;
  buf=(LM_REAL *)malloc(buf_sz);
  if(!buf){
    fprintf(stderr, RCAT("memory allocation in ", AX_EQ_B_BK) "() failed!\n");
    exit(1);
  }
#endif /* LINSOLVERS_RETAIN_MEMORY */

  a=buf;
  work=a+a_sz;
  ipiv=(int *)(work+work_sz);

  /* store A into a and B into x; A is assumed to be symmetric, hence
   * the column and row major order representations are the same
   */
  memcpy(a, A, a_sz*sizeof(LM_REAL));
  memcpy(x, B, m*sizeof(LM_REAL));

  /* LDLt factorization for A */
	SYTRF("L", (int *)&m, a, (int *)&m, ipiv, work, (int *)&work_sz, (int *)&info);
	if(info!=0){
		if(info<0){
      fprintf(stderr, RCAT(RCAT("LAPACK error: illegal value for argument %d of ", SYTRF) " in ", AX_EQ_B_BK) "()\n", -info);
			exit(1);
		}
		else{
      fprintf(stderr, RCAT(RCAT("LAPACK error: singular block diagonal matrix D for", SYTRF) " in ", AX_EQ_B_BK)"() [D(%d, %d) is zero]\n", info, info);
#ifndef LINSOLVERS_RETAIN_MEMORY
      free(buf);
#endif

			return 0;
		}
	}

  /* solve the system with the computed factorization */
  SYTRS("L", (int *)&m, (int *)&nrhs, a, (int *)&m, ipiv, x, (int *)&m, (int *)&info);
  if(info<0){
    fprintf(stderr, RCAT(RCAT("LAPACK error: illegal value for argument %d of ", SYTRS) " in ", AX_EQ_B_BK) "()\n", -info);
    exit(1);
	}

#ifndef LINSOLVERS_RETAIN_MEMORY
  free(buf);
#endif

	return 1;
}

/* undefine all. IT MUST REMAIN IN THIS POSITION IN FILE */
#undef AX_EQ_B_QR
#undef AX_EQ_B_QRLS
#undef AX_EQ_B_CHOL
#undef AX_EQ_B_LU
#undef AX_EQ_B_SVD
#undef AX_EQ_B_BK
#undef AX_EQ_B_PLASMA_CHOL

#undef GEQRF
#undef ORGQR
#undef TRTRS
#undef POTF2
#undef POTRF
#undef POTRS
#undef GETRF
#undef GETRS
#undef GESVD
#undef GESDD
#undef SYTRF
#undef SYTRS
#undef PLASMA_POSV

#else // no LAPACK

/* precision-specific definitions */
#define AX_EQ_B_LU LM_ADD_PREFIX(Ax_eq_b_LU_noLapack)

/*
 * This function returns the solution of Ax = b
 *
 * The function employs LU decomposition followed by forward/back substitution (see 
 * also the LAPACK-based LU solver above)
 *
 * A is mxm, b is mx1
 *
 * The function returns 0 in case of error, 1 if successful
 *
 * This function is often called repetitively to solve problems of identical
 * dimensions. To avoid repetitive malloc's and free's, allocated memory is
 * retained between calls and free'd-malloc'ed when not of the appropriate size.
 * A call with NULL as the first argument forces this memory to be released.
 */
int AX_EQ_B_LU(LM_REAL *A, LM_REAL *B, LM_REAL *x, int m)
{
__STATIC__ void *buf=NULL;
__STATIC__ int buf_sz=0;

register int i, j, k;
int *idx, maxi=-1, idx_sz, a_sz, work_sz, tot_sz;
LM_REAL *a, *work, max, sum, tmp;

    if(!A)
#ifdef LINSOLVERS_RETAIN_MEMORY
    {
      if(buf) free(buf);
      buf=NULL;
      buf_sz=0;

      return 1;
    }
#else
    return 1; /* NOP */
#endif /* LINSOLVERS_RETAIN_MEMORY */
   
  /* calculate required memory size */
  idx_sz=m;
  a_sz=m*m;
  work_sz=m;
  tot_sz=(a_sz+work_sz)*sizeof(LM_REAL) + idx_sz*sizeof(int); /* should be arranged in that order for proper doubles alignment */

#ifdef LINSOLVERS_RETAIN_MEMORY
  if(tot_sz>buf_sz){ /* insufficient memory, allocate a "big" memory chunk at once */
    if(buf) free(buf); /* free previously allocated memory */

    buf_sz=tot_sz;
    buf=(void *)malloc(tot_sz);
    if(!buf){
      fprintf(stderr, RCAT("memory allocation in ", AX_EQ_B_LU) "() failed!\n");
      exit(1);
    }
  }
#else
    buf_sz=tot_sz;
    buf=(void *)malloc(tot_sz);
    if(!buf){
      fprintf(stderr, RCAT("memory allocation in ", AX_EQ_B_LU) "() failed!\n");
      exit(1);
    }
#endif /* LINSOLVERS_RETAIN_MEMORY */

  a=buf;
  work=a+a_sz;
  idx=(int *)(work+work_sz);

  /* avoid destroying A, B by copying them to a, x resp. */
  memcpy(a, A, a_sz*sizeof(LM_REAL));
  memcpy(x, B, m*sizeof(LM_REAL));

  /* compute the LU decomposition of a row permutation of matrix a; the permutation itself is saved in idx[] */
	for(i=0; i<m; ++i){
		max=0.0;
		for(j=0; j<m; ++j)
			if((tmp=FABS(a[i*m+j]))>max)
        max=tmp;
		  if(max==0.0){
        fprintf(stderr, RCAT("Singular matrix A in ", AX_EQ_B_LU) "()!\n");
#ifndef LINSOLVERS_RETAIN_MEMORY
        free(buf);
#endif

        return 0;
      }
		  work[i]=LM_CNST(1.0)/max;
	}

	for(j=0; j<m; ++j){
		for(i=0; i<j; ++i){
			sum=a[i*m+j];
			for(k=0; k<i; ++k)
        sum-=a[i*m+k]*a[k*m+j];
			a[i*m+j]=sum;
		}
		max=0.0;
		for(i=j; i<m; ++i){
			sum=a[i*m+j];
			for(k=0; k<j; ++k)
        sum-=a[i*m+k]*a[k*m+j];
			a[i*m+j]=sum;
			if((tmp=work[i]*FABS(sum))>=max){
				max=tmp;
				maxi=i;
			}
		}
		if(j!=maxi){
			for(k=0; k<m; ++k){
				tmp=a[maxi*m+k];
				a[maxi*m+k]=a[j*m+k];
				a[j*m+k]=tmp;
			}
			work[maxi]=work[j];
		}
		idx[j]=maxi;
		if(a[j*m+j]==0.0)
      a[j*m+j]=LM_REAL_EPSILON;
		if(j!=m-1){
			tmp=LM_CNST(1.0)/(a[j*m+j]);
			for(i=j+1; i<m; ++i)
        a[i*m+j]*=tmp;
		}
	}

  /* The decomposition has now replaced a. Solve the linear system using
   * forward and back substitution
   */
	for(i=k=0; i<m; ++i){
		j=idx[i];
		sum=x[j];
		x[j]=x[i];
		if(k!=0)
			for(j=k-1; j<i; ++j)
        sum-=a[i*m+j]*x[j];
		else
      if(sum!=0.0)
			  k=i+1;
		x[i]=sum;
	}

	for(i=m-1; i>=0; --i){
		sum=x[i];
		for(j=i+1; j<m; ++j)
      sum-=a[i*m+j]*x[j];
		x[i]=sum/a[i*m+i];
	}

#ifndef LINSOLVERS_RETAIN_MEMORY
  free(buf);
#endif

  return 1;
}

/* undefine all. IT MUST REMAIN IN THIS POSITION IN FILE */
#undef AX_EQ_B_LU

#endif /* HAVE_LAPACK */
