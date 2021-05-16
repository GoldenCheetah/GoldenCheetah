/*
 * Library:   lmfit (Levenberg-Marquardt least squares fitting)
 *
 * File:      lmmin.c
 *
 * Contents:  Levenberg-Marquardt minimization.
 *
 * Copyright: MINPACK authors, The University of Chikago (1980-1999)
 *            Joachim Wuttke, Forschungszentrum Juelich GmbH (2004-2013)
 *
 * License:   see ../COPYING (FreeBSD)
 *
 * Homepage:  apps.jcns.fz-juelich.de/lmfit
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include "lmmin.h"

#define MIN(a,b) (((a)<=(b)) ? (a) : (b))
#define MAX(a,b) (((a)>=(b)) ? (a) : (b))
#define SQR(x)   (x)*(x)

/* Declare functions that do the heavy numerics.
   Implementions are in this source file, below lmmin.
   Dependences: lmmin calls qrfac and lmpar; lmpar calls qrsolv. */
void lm_lmpar(
    const int n, double *const r, const int ldr, int *const ipvt,
    double *const diag, double *const qtb, double delta, double *const par,
    double *const x,
    double *const sdiag, double *const aux, double *const xdi);
void lm_qrfac(
    const int m, const int n, double *const a, int *const ipvt,
    double *const rdiag, double *const acnorm, double *const wa );
void lm_qrsolv(
    const int n, double *const r, const int ldr, int *const ipvt,
    double *const diag, double *const qtb, double *const x,
    double *const sdiag, double *const wa);


/*****************************************************************************/
/*  Numeric constants                                                        */
/*****************************************************************************/

/* machine-dependent constants from float.h */
#define LM_MACHEP     DBL_EPSILON   /* resolution of arithmetic */
#define LM_DWARF      DBL_MIN       /* smallest nonzero number */
#define LM_SQRT_DWARF sqrt(DBL_MIN) /* square should not underflow */
#define LM_SQRT_GIANT sqrt(DBL_MAX) /* square should not overflow */
#define LM_USERTOL    30*LM_MACHEP  /* users are recommended to require this */

/* If the above values do not work, the following seem good for an x86:
 LM_MACHEP     .555e-16
 LM_DWARF      9.9e-324
 LM_SQRT_DWARF 1.e-160
 LM_SQRT_GIANT 1.e150
 LM_USER_TOL   1.e-14
   The following values should work on any machine:
 LM_MACHEP     1.2e-16
 LM_DWARF      1.0e-38
 LM_SQRT_DWARF 3.834e-20
 LM_SQRT_GIANT 1.304e19
 LM_USER_TOL   1.e-14
*/

const lm_control_struct lm_control_double = {
    LM_USERTOL, LM_USERTOL, LM_USERTOL, LM_USERTOL, 100., 100, 1,
    NULL, 0, -1, -1 };
const lm_control_struct lm_control_float = {
    1.e-7,      1.e-7,      1.e-7,      1.e-7,      100., 100, 1,
    NULL, 0, -1, -1 };


/*****************************************************************************/
/*  Message texts (indexed by status.info)                                   */
/*****************************************************************************/

const char *lm_infmsg[] = {
    "found zero (sum of squares below underflow limit)",
    "converged  (the relative error in the sum of squares is at most tol)",
    "converged  (the relative error of the parameter vector is at most tol)",
    "converged  (both errors are at most tol)",
    "trapped    (by degeneracy; increasing epsilon might help)",
    "exhausted  (number of function calls exceeding preset patience)",
    "failed     (ftol<tol: cannot reduce sum of squares any further)",
    "failed     (xtol<tol: cannot improve approximate solution any further)",
    "failed     (gtol<tol: cannot improve approximate solution any further)",
    "crashed    (not enough memory)",
    "exploded   (fatal coding error: improper input parameters)",
    "stopped    (break requested within function evaluation)",
    "found nan  (function value is not-a-number or infinite)",
    "won't fit  (no free parameter)"
};

const char *lm_shortmsg[] = {
    "found zero",      //  0
    "converged (f)",   //  1
    "converged (p)",   //  2
    "converged (2)",   //  3
    "degenerate",      //  4
    "call limit",      //  5
    "failed (f)",      //  6
    "failed (p)",      //  7
    "failed (o)",      //  8
    "no memory",       //  9
    "invalid input",   // 10
    "user break",      // 11
    "found nan",       // 12
    "no free par"      // 13
};


/*****************************************************************************/
/*  Monitoring auxiliaries.                                                  */
/*****************************************************************************/

void lm_print_pars(const int nout, const double *par, FILE* fout)
{
    fprintf(fout, "  pars:");
    for (int i = 0; i < nout; ++i)
        fprintf(fout, " %23.16g", par[i]);
    fprintf(fout, "\n");
}


/*****************************************************************************/
/*  lmmin (main minimization routine)                                        */
/*****************************************************************************/

void lmmin(
    const int n, double *const x, const int m, const double* y,
    const void *const data,
    void (*const evaluate)(
        const double *const par, const int m_dat, const void *const data,
        double *const fvec, int *const userbreak),
    const lm_control_struct *const C, lm_status_struct *const S)
{
    int j, i;
    double actred, dirder, fnorm, fnorm1, gnorm, pnorm,
        prered, ratio, step, sum, temp, temp1, temp2, temp3;
    static double p1 = 0.1, p0001 = 1.0e-4;

    int maxfev = C->patience * (n+1);

    int    inner_success; /* flag for loop control */
    double lmpar = 0;     /* Levenberg-Marquardt parameter */
    double delta = 0;
    double xnorm = 0;
    double eps = sqrt(MAX(C->epsilon, LM_MACHEP)); /* for forward differences */

    int nout = C->n_maxpri==-1 ? n : MIN(C->n_maxpri, n);

    /* The workaround msgfile=NULL is needed for default initialization */
    FILE* msgfile = C->msgfile ? C->msgfile : stdout;

    /* Default status info; must be set ahead of first return statements */
    S->outcome = 0;      /* status code */
    S->userbreak = 0;
    S->nfev = 0;      /* function evaluation counter */

/***  Check input parameters for errors.  ***/

    if ( n < 0 ) {
        fprintf(stderr, "lmmin: invalid number of parameters %i\n", n);
        S->outcome = 10; /* invalid parameter */
        return;
    }
    if (m < n) {
        fprintf(stderr, "lmmin: number of data points (%i) "
                "smaller than number of parameters (%i)\n", m, n);
        S->outcome = 10;
        return;
    }
    if (C->ftol < 0 || C->xtol < 0 || C->gtol < 0) {
        fprintf(stderr,
                "lmmin: negative tolerance (at least one of %g %g %g)\n",
                C->ftol, C->xtol, C->gtol);
        S->outcome = 10;
        return;
    }
    if (maxfev <= 0) {
        fprintf(stderr, "lmmin: nonpositive function evaluations limit %i\n",
                maxfev);
        S->outcome = 10;
        return;
    }
    if (C->stepbound <= 0) {
        fprintf(stderr, "lmmin: nonpositive stepbound %g\n", C->stepbound);
        S->outcome = 10;
        return;
    }
    if (C->scale_diag != 0 && C->scale_diag != 1) {
        fprintf(stderr, "lmmin: logical variable scale_diag=%i, "
                "should be 0 or 1\n", C->scale_diag);
        S->outcome = 10;
        return;
    }

/***  Allocate work space.  ***/

    /* Allocate total workspace with just one system call */
    char *ws;
    if ( ( ws = malloc(
               (2*m+5*n+m*n)*sizeof(double) + n*sizeof(int) ) ) == NULL ) {
        S->outcome = 9;
        return;
    }

    /* Assign workspace segments. */
    char *pws = ws;
    double *fvec = (double*) pws; pws += m * sizeof(double)/sizeof(char);
    double *diag = (double*) pws; pws += n * sizeof(double)/sizeof(char);
    double *qtf  = (double*) pws; pws += n * sizeof(double)/sizeof(char);
    double *fjac = (double*) pws; pws += n*m*sizeof(double)/sizeof(char);
    double *wa1  = (double*) pws; pws += n * sizeof(double)/sizeof(char);
    double *wa2  = (double*) pws; pws += n * sizeof(double)/sizeof(char);
    double *wa3  = (double*) pws; pws += n * sizeof(double)/sizeof(char);
    double *wf   = (double*) pws; pws += m * sizeof(double)/sizeof(char);
    int    *ipvt = (int*)    pws; pws += n * sizeof(int)   /sizeof(char);

    /* Initialize diag */ // TODO: check whether this is still needed
    if (!C->scale_diag) {
        for (j = 0; j < n; j++)
            diag[j] = 1.;
    }

/***  Evaluate function at starting point and calculate norm.  ***/

    if( C->verbosity&1 )
        fprintf(msgfile, "lmmin start (ftol=%g gtol=%g xtol=%g)\n",
                C->ftol, C->gtol, C->xtol);
    if( C->verbosity&2 )
        lm_print_pars(nout, x, msgfile);
    (*evaluate)(x, m, data, fvec, &(S->userbreak));
    if( C->verbosity&8 ){
        if (y)
            for( i=0; i<m; ++i )
                fprintf(msgfile, "    i, f, y-f: %4i %18.8g %18.8g\n",
                        i, fvec[i], y[i]-fvec[i]);
        else
            for( i=0; i<m; ++i )
                fprintf(msgfile, "    i, f: %4i %18.8g\n", i, fvec[i]);
    }
    S->nfev = 1;
    if ( S->userbreak )
        goto terminate;
    if ( n == 0 ) {
        S->outcome = 13; /* won't fit */
        goto terminate;
    }
    fnorm = lm_fnorm(m, fvec, y);
    if( C->verbosity&2 )
        fprintf(msgfile, "  fnorm = %24.16g\n", fnorm);
    if( !isfinite(fnorm) ){
        if( C->verbosity )
            fprintf(msgfile, "nan case 1\n");
        S->outcome = 12; /* nan */
        goto terminate;
    } else if( fnorm <= LM_DWARF ){
        S->outcome = 0; /* sum of squares almost zero, nothing to do */
        goto terminate;
    }

/***  The outer loop: compute gradient, then descend.  ***/

    for( int outer=0; ; ++outer ) {

/***  [outer]  Calculate the Jacobian.  ***/

        for (j = 0; j < n; j++) {
            temp = x[j];
            step = MAX(eps*eps, eps * fabs(temp));
            x[j] += step; /* replace temporarily */
            (*evaluate)(x, m, data, wf, &(S->userbreak));
            ++(S->nfev);
            if ( S->userbreak )
                goto terminate;
            for (i = 0; i < m; i++)
                fjac[j*m+i] = (wf[i] - fvec[i]) / step;
            x[j] = temp; /* restore */
        }
        if ( C->verbosity&16 ) {
            /* print the entire matrix */
            printf("Jacobian\n");
            for (i = 0; i < m; i++) {
                printf("  ");
                for (j = 0; j < n; j++)
                    printf("%.5e ", fjac[j*m+i]);
                printf("\n");
            }
        }

/***  [outer]  Compute the QR factorization of the Jacobian.  ***/

/*      fjac is an m by n array. The upper n by n submatrix of fjac
 *        is made to contain an upper triangular matrix R with diagonal
 *        elements of nonincreasing magnitude such that
 *
 *              P^T*(J^T*J)*P = R^T*R
 *
 *              (NOTE: ^T stands for matrix transposition),
 *
 *        where P is a permutation matrix and J is the final calculated
 *        Jacobian. Column j of P is column ipvt(j) of the identity matrix.
 *        The lower trapezoidal part of fjac contains information generated
 *        during the computation of R.
 *
 *      ipvt is an integer array of length n. It defines a permutation
 *        matrix P such that jac*P = Q*R, where jac is the final calculated
 *        Jacobian, Q is orthogonal (not stored), and R is upper triangular
 *        with diagonal elements of nonincreasing magnitude. Column j of P
 *        is column ipvt(j) of the identity matrix.
 */

        lm_qrfac(m, n, fjac, ipvt, wa1, wa2, wa3);
        /* return values are ipvt, wa1=rdiag, wa2=acnorm */

/***  [outer]  Form Q^T * fvec, and store first n components in qtf.  ***/

        if (y)
            for (i = 0; i < m; i++)
                wf[i] = fvec[i] - y[i];
        else
            for (i = 0; i < m; i++)
                wf[i] = fvec[i];

        for (j = 0; j < n; j++) {
            temp3 = fjac[j*m+j];
            if (temp3 != 0) {
                sum = 0;
                for (i = j; i < m; i++)
                    sum += fjac[j*m+i] * wf[i];
                temp = -sum / temp3;
                for (i = j; i < m; i++)
                    wf[i] += fjac[j*m+i] * temp;
            }
            fjac[j*m+j] = wa1[j];
            qtf[j] = wf[j];
        }

/***  [outer]  Compute norm of scaled gradient and detect degeneracy.  ***/

        gnorm = 0;
        for (j = 0; j < n; j++) {
            if (wa2[ipvt[j]] == 0)
                continue;
            sum = 0;
            for (i = 0; i <= j; i++)
                sum += fjac[j*m+i] * qtf[i];
            gnorm = MAX(gnorm, fabs( sum / wa2[ipvt[j]] / fnorm ));
        }

        if (gnorm <= C->gtol) {
            S->outcome = 4;
            goto terminate;
        }

/***  [outer]  Initialize / update diag and delta. ***/

        if ( !outer ) {
            /* first iteration only */
            if (C->scale_diag) {
                /* diag := norms of the columns of the initial Jacobian */
                for (j = 0; j < n; j++)
                    diag[j] = wa2[j] ? wa2[j] : 1;
                /* xnorm := || D x || */
                for (j = 0; j < n; j++)
                    wa3[j] = diag[j] * x[j];
                xnorm = lm_enorm(n, wa3);
            } else {
                xnorm = lm_enorm(n, x);
            }
            if( !isfinite(xnorm) ){
                if( C->verbosity )
                    fprintf(msgfile, "nan case 2\n");
                S->outcome = 12; /* nan */
                goto terminate;
            }
            /* initialize the step bound delta. */
            if ( xnorm )
                delta = C->stepbound * xnorm;
            else
                delta = C->stepbound;
            /* only now print the header for the loop table */
            if( C->verbosity&2 ) {
                fprintf(msgfile, " #o #i     lmpar    prered  actred"
                        "        ratio    dirder      delta"
                        "      pnorm                 fnorm");
                for (i = 0; i < nout; ++i)
                    fprintf(msgfile, "               p%i", i);
                fprintf(msgfile, "\n");
            }
        } else {
            if (C->scale_diag) {
                for (j = 0; j < n; j++)
                    diag[j] = MAX( diag[j], wa2[j] );
            }
        }

/***  The inner loop. ***/
        int inner = 0;
        do {

/***  [inner]  Determine the Levenberg-Marquardt parameter.  ***/

            lm_lmpar(n, fjac, m, ipvt, diag, qtf, delta, &lmpar,
                     wa1, wa2, wf, wa3);
            /* used return values are fjac (partly), lmpar, wa1=x, wa3=diag*x */

            /* predict scaled reduction */
            pnorm = lm_enorm(n, wa3);
            if( !isfinite(pnorm) ){
                if( C->verbosity )
                    fprintf(msgfile, "nan case 3\n");
                S->outcome = 12; /* nan */
                goto terminate;
            }
            temp2 = lmpar * SQR( pnorm / fnorm );
            for (j = 0; j < n; j++) {
                wa3[j] = 0;
                for (i = 0; i <= j; i++)
                    wa3[i] -= fjac[j*m+i] * wa1[ipvt[j]];
            }
            temp1 = SQR( lm_enorm(n, wa3) / fnorm );
            if( !isfinite(temp1) ){
                if( C->verbosity )
                    fprintf(msgfile, "nan case 4\n");
                S->outcome = 12; /* nan */
                goto terminate;
            }
            prered = temp1 + 2 * temp2;
            dirder = -temp1 + temp2; /* scaled directional derivative */

            /* at first call, adjust the initial step bound. */
            if ( !outer && !inner && pnorm < delta )
                delta = pnorm;

/***  [inner]  Evaluate the function at x + p.  ***/

            for (j = 0; j < n; j++)
                wa2[j] = x[j] - wa1[j];

            (*evaluate)( wa2, m, data, wf, &(S->userbreak) );
            ++(S->nfev);
            if ( S->userbreak )
                goto terminate;
            fnorm1 = lm_fnorm(m, wf, y);
            // exceptionally, for this norm we do not test for infinity
            // because we can deal with it without terminating.

/***  [inner]  Evaluate the scaled reduction.  ***/

            /* actual scaled reduction (supports even the case fnorm1=infty) */
	    if (p1 * fnorm1 < fnorm)
		actred = 1 - SQR(fnorm1 / fnorm);
	    else
		actred = -1;

            /* ratio of actual to predicted reduction */
            ratio = prered ? actred/prered : 0;

            if( C->verbosity&32 ){
                if (y)
                    for( i=0; i<m; ++i )
                        fprintf(msgfile, "    i, f, y-f: %4i %18.8g %18.8g\n",
                                i, fvec[i], y[i]-fvec[i]);
                else
                    for( i=0; i<m; ++i )
                        fprintf(msgfile, "    i, f, y-f: %4i %18.8g\n",
                                i, fvec[i]);
            }
            if( C->verbosity&2 ) {
                printf("%3i %2i %9.2g %9.2g %9.2g %14.6g"
                       " %9.2g %10.3e %10.3e %21.15e",
                       outer, inner, lmpar, prered, actred, ratio,
                       dirder, delta, pnorm, fnorm1);
                for (i = 0; i < nout; ++i)
                    fprintf(msgfile, " %16.9g", wa2[i]);
                fprintf(msgfile, "\n");
            }

            /* update the step bound */
	    if (ratio <= 0.25) {
		if (actred >= 0)
		    temp = 0.5;
		else
		    temp = 0.5 * dirder / (dirder + 0.5 * actred);
		if (p1 * fnorm1 >= fnorm || temp < p1)
		    temp = p1;
		delta = temp * MIN(delta, pnorm / p1);
		lmpar /= temp;
	    } else if (lmpar == 0 || ratio >= 0.75) {
		delta = 2 * pnorm;
		lmpar *= 0.5;
	    }

/***  [inner]  On success, update solution, and test for convergence.  ***/

            inner_success = ratio >= p0001;
            if ( inner_success ) {

                /* update x, fvec, and their norms */
                if (C->scale_diag) {
                    for (j = 0; j < n; j++) {
                        x[j] = wa2[j];
                        wa2[j] = diag[j] * x[j];
                    }
                } else {
                    for (j = 0; j < n; j++)
                        x[j] = wa2[j];
                }
                for (i = 0; i < m; i++)
                    fvec[i] = wf[i];
                xnorm = lm_enorm(n, wa2);
                if( !isfinite(xnorm) ){
                    if( C->verbosity )
                        fprintf(msgfile, "nan case 6\n");
                    S->outcome = 12; /* nan */
                    goto terminate;
                }
                fnorm = fnorm1;
            }

            /* convergence tests */
            S->outcome = 0;
            if( fnorm<=LM_DWARF )
                goto terminate;  /* success: sum of squares almost zero */
            /* test two criteria (both may be fulfilled) */
            if (fabs(actred) <= C->ftol && prered <= C->ftol && ratio <= 2)
                S->outcome = 1;  /* success: x almost stable */
            if (delta <= C->xtol * xnorm)
                S->outcome += 2; /* success: sum of squares almost stable */
            if (S->outcome != 0) {
                goto terminate;
            }

/***  [inner]  Tests for termination and stringent tolerances.  ***/

            if ( S->nfev >= maxfev ){
                S->outcome = 5;
                goto terminate;
            }
            if ( fabs(actred) <= LM_MACHEP &&
                 prered <= LM_MACHEP && ratio <= 2 ){
                S->outcome = 6;
                goto terminate;
            }
            if ( delta <= LM_MACHEP*xnorm ){
                S->outcome = 7;
                goto terminate;
            }
            if ( gnorm <= LM_MACHEP ){
                S->outcome = 8;
                goto terminate;
            }

/***  [inner]  End of the loop. Repeat if iteration unsuccessful.  ***/

            ++inner;
        } while ( !inner_success );

/***  [outer]  End of the loop. ***/

    };

terminate:
    S->fnorm = lm_fnorm(m, fvec, y);
    if( C->verbosity&1 )
        fprintf(msgfile, "lmmin terminates with outcome %i\n", S->outcome);
    if( C->verbosity&2 )
        lm_print_pars(nout, x, msgfile);
    if( C->verbosity&8 ){
        if (y)
            for( i=0; i<m; ++i )
                fprintf(msgfile, "    i, f, y-f: %4i %18.8g %18.8g\n",
                        i, fvec[i], y[i]-fvec[i] );
        else
            for( i=0; i<m; ++i )
                fprintf(msgfile, "    i, f, y-f: %4i %18.8g\n", i, fvec[i]);
    }
    if( C->verbosity&2 )
        fprintf(msgfile, "  fnorm=%24.16g xnorm=%24.16g\n", S->fnorm, xnorm);
    if ( S->userbreak ) /* user-requested break */
        S->outcome = 11;

/***  Deallocate the workspace.  ***/
    free(ws);

} /*** lmmin. ***/


/*****************************************************************************/
/*  lm_lmpar (determine Levenberg-Marquardt parameter)                       */
/*****************************************************************************/

void lm_lmpar(
    const int n, double *const r, const int ldr, int *const ipvt,
    double *const diag, double *const qtb, double delta, double *const par,
    double *const x, double *const sdiag, double *const aux, double *const xdi)
{
/*     Given an m by n matrix A, an n by n nonsingular diagonal matrix D,
 *     an m-vector b, and a positive number delta, the problem is to
 *     determine a parameter value par such that if x solves the system
 *
 *          A*x = b  and  sqrt(par)*D*x = 0
 *
 *     in the least squares sense, and dxnorm is the euclidean
 *     norm of D*x, then either par=0 and (dxnorm-delta) < 0.1*delta,
 *     or par>0 and abs(dxnorm-delta) < 0.1*delta.
 *
 *     Using lm_qrsolv, this subroutine completes the solution of the
 *     problem if it is provided with the necessary information from
 *     the QR factorization, with column pivoting, of A. That is, if
 *     A*P = Q*R, where P is a permutation matrix, Q has orthogonal
 *     columns, and R is an upper triangular matrix with diagonal
 *     elements of nonincreasing magnitude, then lmpar expects the
 *     full upper triangle of R, the permutation matrix P, and the
 *     first n components of Q^T*b. On output lmpar also provides an
 *     upper triangular matrix S such that
 *
 *          P^T*(A^T*A + par*D*D)*P = S^T*S.
 *
 *     S is employed within lmpar and may be of separate interest.
 *
 *     Only a few iterations are generally needed for convergence
 *     of the algorithm. If, however, the limit of 10 iterations
 *     is reached, then the output par will contain the best value
 *     obtained so far.
 *
 *     Parameters:
 *
 *      n is a positive integer INPUT variable set to the order of r.
 *
 *      r is an n by n array. On INPUT the full upper triangle
 *        must contain the full upper triangle of the matrix R.
 *        On OUTPUT the full upper triangle is unaltered, and the
 *        strict lower triangle contains the strict upper triangle
 *        (transposed) of the upper triangular matrix S.
 *
 *      ldr is a positive integer INPUT variable not less than n
 *        which specifies the leading dimension of the array R.
 *
 *      ipvt is an integer INPUT array of length n which defines the
 *        permutation matrix P such that A*P = Q*R. Column j of P
 *        is column ipvt(j) of the identity matrix.
 *
 *      diag is an INPUT array of length n which must contain the
 *        diagonal elements of the matrix D.
 *
 *      qtb is an INPUT array of length n which must contain the first
 *        n elements of the vector Q^T*b.
 *
 *      delta is a positive INPUT variable which specifies an upper
 *        bound on the euclidean norm of D*x.
 *
 *      par is a nonnegative variable. On INPUT par contains an
 *        initial estimate of the Levenberg-Marquardt parameter.
 *        On OUTPUT par contains the final estimate.
 *
 *      x is an OUTPUT array of length n which contains the least
 *        squares solution of the system A*x = b, sqrt(par)*D*x = 0,
 *        for the output par.
 *
 *      sdiag is an array of length n needed as workspace; on OUTPUT
 *        it contains the diagonal elements of the upper triangular
 *        matrix S.
 *
 *      aux is a multi-purpose work array of length n.
 *
 *      xdi is a work array of length n. On OUTPUT: diag[j] * x[j].
 *
 */
    int i, iter, j, nsing;
    double dxnorm, fp, fp_old, gnorm, parc, parl, paru;
    double sum, temp;
    static double p1 = 0.1;

/*** lmpar: compute and store in x the gauss-newton direction. if the
     jacobian is rank-deficient, obtain a least squares solution. ***/

    nsing = n;
    for (j = 0; j < n; j++) {
        aux[j] = qtb[j];
        if (r[j * ldr + j] == 0 && nsing == n)
            nsing = j;
        if (nsing < n)
            aux[j] = 0;
    }
    for (j = nsing - 1; j >= 0; j--) {
        aux[j] = aux[j] / r[j + ldr * j];
        temp = aux[j];
        for (i = 0; i < j; i++)
            aux[i] -= r[j * ldr + i] * temp;
    }

    for (j = 0; j < n; j++)
        x[ipvt[j]] = aux[j];

/*** lmpar: initialize the iteration counter, evaluate the function at the
     origin, and test for acceptance of the gauss-newton direction. ***/

    for (j = 0; j < n; j++)
        xdi[j] = diag[j] * x[j];
    dxnorm = lm_enorm(n, xdi);
    fp = dxnorm - delta;
    if (fp <= p1 * delta) {
#ifdef LMFIT_DEBUG_MESSAGES
        printf("debug lmpar nsing %d n %d, terminate (fp<p1*delta)\n",
               nsing, n);
#endif
        *par = 0;
        return;
    }

/*** lmpar: if the jacobian is not rank deficient, the newton
     step provides a lower bound, parl, for the zero of
     the function. otherwise set this bound to zero. ***/

    parl = 0;
    if (nsing >= n) {
        for (j = 0; j < n; j++)
            aux[j] = diag[ipvt[j]] * xdi[ipvt[j]] / dxnorm;

        for (j = 0; j < n; j++) {
            sum = 0;
            for (i = 0; i < j; i++)
                sum += r[j * ldr + i] * aux[i];
            aux[j] = (aux[j] - sum) / r[j + ldr * j];
        }
        temp = lm_enorm(n, aux);
        parl = fp / delta / temp / temp;
    }

/*** lmpar: calculate an upper bound, paru, for the zero of the function. ***/

    for (j = 0; j < n; j++) {
        sum = 0;
        for (i = 0; i <= j; i++)
            sum += r[j * ldr + i] * qtb[i];
        aux[j] = sum / diag[ipvt[j]];
    }
    gnorm = lm_enorm(n, aux);
    paru = gnorm / delta;
    if (paru == 0)
        paru = LM_DWARF / MIN(delta, p1);

/*** lmpar: if the input par lies outside of the interval (parl,paru),
     set par to the closer endpoint. ***/

    *par = MAX(*par, parl);
    *par = MIN(*par, paru);
    if (*par == 0)
        *par = gnorm / dxnorm;

/*** lmpar: iterate. ***/

    for (iter=0; ; iter++) {

        /** evaluate the function at the current value of par. **/

        if (*par == 0)
            *par = MAX(LM_DWARF, 0.001 * paru);
        temp = sqrt(*par);
        for (j = 0; j < n; j++)
            aux[j] = temp * diag[j];

        lm_qrsolv(n, r, ldr, ipvt, aux, qtb, x, sdiag, xdi);
        /* return values are r, x, sdiag */

        for (j = 0; j < n; j++)
            xdi[j] = diag[j] * x[j]; /* used as output */
        dxnorm = lm_enorm(n, xdi);
        fp_old = fp;
        fp = dxnorm - delta;

        /** if the function is small enough, accept the current value
            of par. Also test for the exceptional cases where parl
            is zero or the number of iterations has reached 10. **/

        if (fabs(fp) <= p1 * delta
            || (parl == 0 && fp <= fp_old && fp_old < 0)
            || iter == 10) {
#ifdef LMFIT_DEBUG_MESSAGES
            printf("debug lmpar nsing %d iter %d "
                   "par %.4e [%.4e %.4e] delta %.4e fp %.4e\n",
                   nsing, iter, *par, parl, paru, delta, fp);
#endif
            break; /* the only exit from the iteration. */
        }

        /** compute the Newton correction. **/

        for (j = 0; j < n; j++)
            aux[j] = diag[ipvt[j]] * xdi[ipvt[j]] / dxnorm;

        for (j = 0; j < n; j++) {
            aux[j] = aux[j] / sdiag[j];
            for (i = j + 1; i < n; i++)
                aux[i] -= r[j * ldr + i] * aux[j];
        }
        temp = lm_enorm(n, aux);
        parc = fp / delta / temp / temp;

        /** depending on the sign of the function, update parl or paru. **/

        if (fp > 0)
            parl = MAX(parl, *par);
        else if (fp < 0)
            paru = MIN(paru, *par);
        /* the case fp==0 is precluded by the break condition  */

        /** compute an improved estimate for par. **/

        *par = MAX(parl, *par + parc);

    }

} /*** lm_lmpar. ***/

/*****************************************************************************/
/*  lm_qrfac (QR factorization, from lapack)                                 */
/*****************************************************************************/

void lm_qrfac(
    const int m, const int n, double *const A, int *const Pivot,
    double *const Rdiag, double *const Acnorm, double *const W)
{
/*
 *     This subroutine uses Householder transformations with column pivoting
 *     to compute a QR factorization of the m by n matrix A. That is, qrfac
 *     determines an orthogonal matrix Q, a permutation matrix P, and an
 *     upper trapezoidal matrix R with diagonal elements of nonincreasing
 *     magnitude, such that A*P = Q*R. The Householder transformation for
 *     column k, k = 1,2,...,n, is of the form
 *
 *          I - 2*w*wT/|w|^2
 *
 *     where w has zeroes in the first k-1 positions.
 *
 *     Parameters:
 *
 *      m is an INPUT parameter set to the number of rows of A.
 *
 *      n is an INPUT parameter set to the number of columns of A.
 *
 *      A is an m by n array. On INPUT, A contains the matrix for
 *        which the QR factorization is to be computed. On OUTPUT
 *        the strict upper trapezoidal part of A contains the strict
 *        upper trapezoidal part of R, and the lower trapezoidal
 *        part of A contains a factored form of Q (the non-trivial
 *        elements of the vectors w described above).
 *
 *      Pivot is an integer OUTPUT array of length n that describes the
 *        permutation matrix P:
 *        Column j of P is column ipvt(j) of the identity matrix.
 *
 *      Rdiag is an OUTPUT array of length n which contains the
 *        diagonal elements of R.
 *
 *      Acnorm is an OUTPUT array of length n which contains the norms
 *        of the corresponding columns of the input matrix A. If this
 *        information is not needed, then Acnorm can share storage with Rdiag.
 *
 *      W is a work array of length n.
 *
 */
    int i, j, k, kmax;
    double ajnorm, sum, temp;

#ifdef LMFIT_DEBUG_MESSAGES
    printf("debug qrfac\n");
#endif

    /** Compute initial column norms;
        initialize Pivot with identity permutation. ***/

    for (j = 0; j < n; j++) {
        W[j] = Rdiag[j] = Acnorm[j] = lm_enorm(m, &A[j*m]);
        Pivot[j] = j;
    }

    /** Loop over columns of A. **/

    assert( n <= m );
    for (j = 0; j < n; j++) {

        /** Bring the column of largest norm into the pivot position. **/

        kmax = j;
        for (k = j+1; k < n; k++)
            if (Rdiag[k] > Rdiag[kmax])
                kmax = k;

        if (kmax != j) {
            /* Swap columns j and kmax. */
            k = Pivot[j];
            Pivot[j] = Pivot[kmax];
            Pivot[kmax] = k;
            for (i = 0; i < m; i++) {
                temp = A[j*m+i];
                A[j*m+i] = A[kmax*m+i];
                A[kmax*m+i] = temp;
            }
            /* Half-swap: Rdiag[j], W[j] won't be needed any further. */
            Rdiag[kmax] = Rdiag[j];
            W[kmax] = W[j];
        }

        /** Compute the Householder reflection vector w_j to reduce the
            j-th column of A to a multiple of the j-th unit vector. **/

        ajnorm = lm_enorm(m-j, &A[j*m+j]);
        if (ajnorm == 0) {
            Rdiag[j] = 0;
            continue;
        }

        /* Let the partial column vector A[j][j:] contain w_j := e_j+-a_j/|a_j|,
           where the sign +- is chosen to avoid cancellation in w_jj. */
        if (A[j*m+j] < 0)
            ajnorm = -ajnorm;
        for (i = j; i < m; i++)
            A[j*m+i] /= ajnorm;
        A[j*m+j] += 1;

        /** Apply the Householder transformation U_w := 1 - 2*w_j.w_j/|w_j|^2
            to the remaining columns, and update the norms. **/

        for (k = j+1; k < n; k++) {
            /* Compute scalar product w_j * a_j. */
            sum = 0;
            for (i = j; i < m; i++)
                sum += A[j*m+i] * A[k*m+i];

            /* Normalization is simplified by the coincidence |w_j|^2=2w_jj. */
            temp = sum / A[j*m+j];

            /* Carry out transform U_w_j * a_k. */
            for (i = j; i < m; i++)
                A[k*m+i] -= temp * A[j*m+i];

            /* No idea what happens here. */
            if (Rdiag[k] != 0) {
                temp = A[m*k+j] / Rdiag[k];
                if ( fabs(temp)<1 ) {
                    Rdiag[k] *= sqrt(1-SQR(temp));
                    temp = Rdiag[k] / W[k];
                } else
                    temp = 0;
                if ( temp == 0 || 0.05 * SQR(temp) <= LM_MACHEP ) {
                    Rdiag[k] = lm_enorm(m-j-1, &A[m*k+j+1]);
                    W[k] = Rdiag[k];
                }
            }
        }

        Rdiag[j] = -ajnorm;
    }
} /*** lm_qrfac. ***/


/*****************************************************************************/
/*  lm_qrsolv (linear least-squares)                                         */
/*****************************************************************************/

void lm_qrsolv(
    const int n, double *const r, const int ldr, int *const ipvt,
    double *const diag, double *const qtb, double *const x,
    double *const sdiag, double *const wa)
{
/*
 *     Given an m by n matrix A, an n by n diagonal matrix D, and an
 *     m-vector b, the problem is to determine an x which solves the
 *     system
 *
 *          A*x = b  and  D*x = 0
 *
 *     in the least squares sense.
 *
 *     This subroutine completes the solution of the problem if it is
 *     provided with the necessary information from the QR factorization,
 *     with column pivoting, of A. That is, if A*P = Q*R, where P is a
 *     permutation matrix, Q has orthogonal columns, and R is an upper
 *     triangular matrix with diagonal elements of nonincreasing magnitude,
 *     then qrsolv expects the full upper triangle of R, the permutation
 *     matrix P, and the first n components of Q^T*b. The system
 *     A*x = b, D*x = 0, is then equivalent to
 *
 *          R*z = Q^T*b,  P^T*D*P*z = 0,
 *
 *     where x = P*z. If this system does not have full rank, then a least
 *     squares solution is obtained. On output qrsolv also provides an upper
 *     triangular matrix S such that
 *
 *          P^T*(A^T*A + D*D)*P = S^T*S.
 *
 *     S is computed within qrsolv and may be of separate interest.
 *
 *     Parameters:
 *
 *      n is a positive integer INPUT variable set to the order of R.
 *
 *      r is an n by n array. On INPUT the full upper triangle must
 *        contain the full upper triangle of the matrix R. On OUTPUT
 *        the full upper triangle is unaltered, and the strict lower
 *        triangle contains the strict upper triangle (transposed) of
 *        the upper triangular matrix S.
 *
 *      ldr is a positive integer INPUT variable not less than n
 *        which specifies the leading dimension of the array R.
 *
 *      ipvt is an integer INPUT array of length n which defines the
 *        permutation matrix P such that A*P = Q*R. Column j of P
 *        is column ipvt(j) of the identity matrix.
 *
 *      diag is an INPUT array of length n which must contain the
 *        diagonal elements of the matrix D.
 *
 *      qtb is an INPUT array of length n which must contain the first
 *        n elements of the vector Q^T*b.
 *
 *      x is an OUTPUT array of length n which contains the least
 *        squares solution of the system A*x = b, D*x = 0.
 *
 *      sdiag is an OUTPUT array of length n which contains the
 *        diagonal elements of the upper triangular matrix S.
 *
 *      wa is a work array of length n.
 *
 */
    int i, kk, j, k, nsing;
    double qtbpj, sum, temp;
    double _sin, _cos, _tan, _cot; /* local variables, not functions */

/*** qrsolv: copy R and Q^T*b to preserve input and initialize S.
     In particular, save the diagonal elements of R in x. ***/

    for (j = 0; j < n; j++) {
        for (i = j; i < n; i++)
            r[j * ldr + i] = r[i * ldr + j];
        x[j] = r[j * ldr + j];
        wa[j] = qtb[j];
    }

/*** qrsolv: eliminate the diagonal matrix D using a Givens rotation. ***/

    for (j = 0; j < n; j++) {

/*** qrsolv: prepare the row of D to be eliminated, locating the
     diagonal element using P from the QR factorization. ***/

        if (diag[ipvt[j]] == 0)
            goto L90;
        for (k = j; k < n; k++)
            sdiag[k] = 0;
        sdiag[j] = diag[ipvt[j]];

/*** qrsolv: the transformations to eliminate the row of D modify only
     a single element of Q^T*b beyond the first n, which is initially 0. ***/

        qtbpj = 0;
        for (k = j; k < n; k++) {

            /** determine a Givens rotation which eliminates the
                appropriate element in the current row of D. **/

            if (sdiag[k] == 0)
                continue;
            kk = k + ldr * k;
            if (fabs(r[kk]) < fabs(sdiag[k])) {
                _cot = r[kk] / sdiag[k];
                _sin = 1 / sqrt(1 + SQR(_cot));
                _cos = _sin * _cot;
            } else {
                _tan = sdiag[k] / r[kk];
                _cos = 1 / sqrt(1 + SQR(_tan));
                _sin = _cos * _tan;
            }

            /** compute the modified diagonal element of R and
                the modified element of (Q^T*b,0). **/

            r[kk] = _cos * r[kk] + _sin * sdiag[k];
            temp = _cos * wa[k] + _sin * qtbpj;
            qtbpj = -_sin * wa[k] + _cos * qtbpj;
            wa[k] = temp;

            /** accumulate the tranformation in the row of S. **/

            for (i = k + 1; i < n; i++) {
                temp = _cos * r[k * ldr + i] + _sin * sdiag[i];
                sdiag[i] = -_sin * r[k * ldr + i] + _cos * sdiag[i];
                r[k * ldr + i] = temp;
            }
        }

      L90:
        /** store the diagonal element of S and restore
            the corresponding diagonal element of R. **/

        sdiag[j] = r[j * ldr + j];
        r[j * ldr + j] = x[j];
    }

/*** qrsolv: solve the triangular system for z. If the system is
     singular, then obtain a least squares solution. ***/

    nsing = n;
    for (j = 0; j < n; j++) {
        if (sdiag[j] == 0 && nsing == n)
            nsing = j;
        if (nsing < n)
            wa[j] = 0;
    }

    for (j = nsing - 1; j >= 0; j--) {
        sum = 0;
        for (i = j + 1; i < nsing; i++)
            sum += r[j * ldr + i] * wa[i];
        wa[j] = (wa[j] - sum) / sdiag[j];
    }

/*** qrsolv: permute the components of z back to components of x. ***/

    for (j = 0; j < n; j++)
        x[ipvt[j]] = wa[j];

} /*** lm_qrsolv. ***/


/*****************************************************************************/
/*  lm_enorm (Euclidean norm)                                                */
/*****************************************************************************/

double lm_enorm(const int n, const double *const x)
{
/*     This function calculates the Euclidean norm of an n-vector x.
 *
 *     The Euclidean norm is computed by accumulating the sum of
 *     squares in three different sums. The sums of squares for the
 *     small and large components are scaled so that no overflows
 *     occur. Non-destructive underflows are permitted. Underflows
 *     and overflows do not occur in the computation of the unscaled
 *     sum of squares for the intermediate components.
 *     The definitions of small, intermediate and large components
 *     depend on two constants, LM_SQRT_DWARF and LM_SQRT_GIANT. The main
 *     restrictions on these constants are that LM_SQRT_DWARF**2 not
 *     underflow and LM_SQRT_GIANT**2 not overflow.
 *
 *     Parameters:
 *
 *      n is a positive integer INPUT variable.
 *
 *      x is an INPUT array of length n.
 */
    int i;
    double agiant, s1, s2, s3, xabs, x1max, x3max, temp;

    s1 = 0;
    s2 = 0;
    s3 = 0;
    x1max = 0;
    x3max = 0;
    agiant = LM_SQRT_GIANT / n;

    /** sum squares. **/

    for (i = 0; i < n; i++) {
        xabs = fabs(x[i]);
        if (xabs > LM_SQRT_DWARF) {
            if ( xabs < agiant ) {
                s2 += xabs * xabs;
            } else if ( xabs > x1max ) {
                temp = x1max / xabs;
                s1 = 1 + s1 * SQR(temp);
                x1max = xabs;
            } else {
                temp = xabs / x1max;
                s1 += SQR(temp);
            }
        } else if ( xabs > x3max ) {
            temp = x3max / xabs;
            s3 = 1 + s3 * SQR(temp);
            x3max = xabs;
        } else if (xabs != 0) {
            temp = xabs / x3max;
            s3 += SQR(temp);
        }
    }

    /** calculation of norm. **/

    if (s1 != 0)
        return x1max * sqrt(s1 + (s2 / x1max) / x1max);
    else if (s2 != 0) {
        if (s2 >= x3max)
            return sqrt(s2 * (1 + (x3max / s2) * (x3max * s3)));
        else
            return sqrt(x3max * ((s2 / x3max) + (x3max * s3)));
    } else
        return x3max * sqrt(s3);

} /*** lm_enorm. ***/


/*****************************************************************************/
/*  lm_fnorm (Euclidean norm of difference)                                                */
/*****************************************************************************/

double lm_fnorm(const int n, const double *const x, const double *const y)
{
/*     This function calculates the Euclidean norm of an n-vector x-y.
 *
 *     The Euclidean norm is computed by accumulating the sum of
 *     squares in three different sums. The sums of squares for the
 *     small and large components are scaled so that no overflows
 *     occur. Non-destructive underflows are permitted. Underflows
 *     and overflows do not occur in the computation of the unscaled
 *     sum of squares for the intermediate components.
 *     The definitions of small, intermediate and large components
 *     depend on two constants, LM_SQRT_DWARF and LM_SQRT_GIANT. The main
 *     restrictions on these constants are that LM_SQRT_DWARF**2 not
 *     underflow and LM_SQRT_GIANT**2 not overflow.
 *
 *     Parameters:
 *
 *      n is a positive integer INPUT variable.
 *
 *      x, y are INPUT arrays of length n.
 */
    if (!y)
        return lm_enorm(n, x);
    int i;
    double agiant, s1, s2, s3, xabs, x1max, x3max, temp;

    s1 = 0;
    s2 = 0;
    s3 = 0;
    x1max = 0;
    x3max = 0;
    agiant = LM_SQRT_GIANT / n;

    /** sum squares. **/

    for (i = 0; i < n; i++) {
        xabs = fabs(x[i]-y[i]);
        if (xabs > LM_SQRT_DWARF) {
            if ( xabs < agiant ) {
                s2 += xabs * xabs;
            } else if ( xabs > x1max ) {
                temp = x1max / xabs;
                s1 = 1 + s1 * SQR(temp);
                x1max = xabs;
            } else {
                temp = xabs / x1max;
                s1 += SQR(temp);
            }
        } else if ( xabs > x3max ) {
            temp = x3max / xabs;
            s3 = 1 + s3 * SQR(temp);
            x3max = xabs;
        } else if (xabs != 0) {
            temp = xabs / x3max;
            s3 += SQR(temp);
        }
    }

    /** calculation of norm. **/

    if (s1 != 0)
        return x1max * sqrt(s1 + (s2 / x1max) / x1max);
    else if (s2 != 0) {
        if (s2 >= x3max)
            return sqrt(s2 * (1 + (x3max / s2) * (x3max * s3)));
        else
            return sqrt(x3max * ((s2 / x3max) + (x3max * s3)));
    } else
        return x3max * sqrt(s3);

} /*** lm_fnorm. ***/
