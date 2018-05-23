/*
 * Library:   lmfit (Levenberg-Marquardt least squares fitting)
 *
 * File:      lmmin.h
 *
 * Contents:  Declarations for Levenberg-Marquardt minimization.
 *
 * Copyright: Joachim Wuttke, Forschungszentrum Juelich GmbH (2004-2013)
 *
 * License:   see ../COPYING (FreeBSD)
 *
 * Homepage:  apps.jcns.fz-juelich.de/lmfit
 */

#ifndef LMMIN_H
#define LMMIN_H
#undef __BEGIN_DECLS
#undef __END_DECLS
#ifdef __cplusplus
#define __BEGIN_DECLS extern "C" {
#define __END_DECLS }
#else
#define __BEGIN_DECLS /* empty */
#define __END_DECLS   /* empty */
#endif

#include "lmstruct.h"

__BEGIN_DECLS

/* Levenberg-Marquardt minimization. */
void lmmin(
    const int n_par, double* par, const int m_dat, const double* y,
    const void* data,
    void (*evaluate)(
        const double* par, const int m_dat, const void* data,
        double* fvec, int* userbreak),
    const lm_control_struct* control, lm_status_struct* status);
/*
 *   This routine contains the core algorithm of our library.
 *
 *   It minimizes the sum of the squares of m nonlinear functions
 *   in n variables by a modified Levenberg-Marquardt algorithm.
 *   The function evaluation is done by the user-provided routine 'evaluate'.
 *   The Jacobian is then calculated by a forward-difference approximation.
 *
 *   Parameters:
 *
 *      n_par is the number of variables (INPUT, positive integer).
 *
 *      par is the solution vector (INPUT/OUTPUT, array of length n).
 *        On input it must be set to an estimated solution.
 *        On output it yields the final estimate of the solution.
 *
 *      m_dat is the number of functions to be minimized (INPUT, positive integer).
 *        It must fulfill m>=n.
 *
 *      y contains data to be fitted. Use a null pointer if there are no data.
 *
 *      data is a pointer that is ignored by lmmin; it is however forwarded
 *        to the user-supplied functions evaluate and printout.
 *        In a typical application, it contains experimental data to be fitted.
 *
 *      evaluate is a user-supplied function that calculates the m functions.
 *        Parameters:
 *          n, x, m, data as above.
 *          fvec is an array of length m; on OUTPUT, it must contain the
 *            m function values for the parameter vector x.
 *          userbreak is an integer pointer. When *userbreak is set to a
 *            nonzero value, lmmin will terminate.
 *
 *      control contains INPUT variables that control the fit algorithm,
 *        as declared and explained in lmstruct.h
 *
 *      status contains OUTPUT variables that inform about the fit result,
 *        as declared and explained in lmstruct.h
 */

/* Refined calculation of Eucledian norm. */
double lm_enorm(const int, const double*);
double lm_fnorm(const int, const double*, const double*);

__END_DECLS
#endif /* LMMIN_H */
