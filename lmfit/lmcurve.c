/*
 * Library:   lmfit (Levenberg-Marquardt least squares fitting)
 *
 * File:      lmcurve.c
 *
 * Contents:  Implements lmcurve, a simplified API for curve fitting
 *            using the generic Levenberg-Marquardt routine lmmin.
 *
 * Copyright: Joachim Wuttke, Forschungszentrum Juelich GmbH (2004-2013)
 *
 * License:   see ../COPYING (FreeBSD)
 *
 * Homepage:  apps.jcns.fz-juelich.de/lmfit
 *
 * Note to programmers: Don't patch and fork, but copy and modify!
 *   If you need to compute residues differently, then please do not patch
 * lmcurve.h and lmcurve.c, but copy them, and create differently named
 * versions of lmcurve_data_struct, lmcurve_evaluate, and lmcurve of your own.
 */

#include "lmmin.h"


typedef struct {
    const double *const t;
    const double *const y;
    double (*const g) (const double t, const double *par);
} lmcurve_data_struct;


void lmcurve_evaluate(
    const double *const par, const int m_dat, const void *const data,
    double *const fvec, int *const info)
{
    (void)(info);
    for (int i = 0; i < m_dat; i++ )
        fvec[i] =
            ((lmcurve_data_struct*)data)->y[i] -
            ((lmcurve_data_struct*)data)->g(
                ((lmcurve_data_struct*)data)->t[i], par );
}


void lmcurve(
    const int n_par, double *const par, const int m_dat,
    const double *const t, const double *const y,
    double (*const g)(const double t, const double *const par),
    const lm_control_struct *const control, lm_status_struct *const status)
{
    lmcurve_data_struct data = {t, y, g};
    lmmin(n_par, par, m_dat, NULL, (const void *const) &data,
          lmcurve_evaluate, control, status);
}
