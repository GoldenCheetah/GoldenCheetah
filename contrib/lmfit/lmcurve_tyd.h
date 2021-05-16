/*
 * Library:   lmfit (Levenberg-Marquardt least squares fitting)
 *
 * File:      lmcurve_tyd.h
 *
 * Contents:  Declares lmcurve_tyd(), a variant of lmcurve() that weighs
 *            data points y(t) with the inverse of the standard deviations dy.
 *
 * Copyright: Joachim Wuttke, Forschungszentrum Juelich GmbH (2004-2013)
 *
 * License:   see ../COPYING (FreeBSD)
 *
 * Homepage:  apps.jcns.fz-juelich.de/lmfit
 */

#ifndef LMCURVETYD_H
#define LMCURVETYD_H
#undef __BEGIN_DECLS
#undef __END_DECLS
#ifdef __cplusplus
#define __BEGIN_DECLS extern "C" {
#define __END_DECLS }
#else
#define __BEGIN_DECLS /* empty */
#define __END_DECLS   /* empty */
#endif

#include <lmstruct.h>

__BEGIN_DECLS

void lmcurve_tyd(
    const int n_par, double* par, const int m_dat,
    const double* t, const double* y, const double* dy,
    double (*f)(double t, const double* par),
    const lm_control_struct* control, lm_status_struct* status);

__END_DECLS
#endif /* LMCURVETYD_H */
