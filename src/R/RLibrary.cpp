/*
 * Copyright (c) 2016 Mark Liversedge (liversedge@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef GC_WANT_R_DYNAMIC
#include <R.h>
#include <Rinternals.h>
#include <R_ext/RStartup.h>
#include <R_ext/Parse.h>
#include <R_ext/Rdynload.h>
#include <R_ext/GraphicsEngine.h>
#include <R_ext/GraphicsDevice.h>

// R Embedding methods
void GC_R_dot_Last(void){ }
void GC_R_RunExitFinalizers(void){ }
void GC_R_CleanTempDir(void){ }
int  GC_Rf_initEmbeddedR(int argc, char *argv[]){ }
void GC_Rf_endEmbeddedR(int fatal){ }
void GC_R_ReplDLLinit(void){ }
void GC_R_DefParams(Rstart){ }
void GC_R_SetParams(Rstart){ }
SEXP GC_R_tryEval(SEXP, SEXP, int *){ }
void NORET GC_Rf_error(const char *, ...){ }
void NORET GC_Rf_warning(const char *, ...){ }
void GC_Rf_PrintValue(SEXP) { }
DllInfo *GC_R_getEmbeddingDllInfo(void) { }
int GC_R_registerRoutines(DllInfo *info, const R_CMethodDef * const croutines,
                          const R_CallMethodDef * const callRoutines,
                          const R_FortranMethodDef * const fortranRoutines,
                          const R_ExternalMethodDef * const externalRoutines) { }
#ifndef WIN32
void (*GC_ptr_R_Suicide)(const char *);
void (*GC_ptr_R_ShowMessage)(const char *);
int  (*GC_ptr_R_ReadConsole)(const char *, unsigned char *, int, int);
void (*GC_ptr_R_WriteConsole)(const char *, int);
void (*GC_ptr_R_WriteConsoleEx)(const char *, int, int);
void (*GC_ptr_R_ResetConsole)(void);
void (*GC_ptr_R_FlushConsole)(void);
void (*GC_ptr_R_ClearerrConsole)(void);
void (*GC_ptr_R_Busy)(int);
void (*GC_ptr_R_CleanUp)(SA_TYPE, int, int);
int  (*GC_ptr_R_ChooseFile)(int, char *, int);
int  (*GC_ptr_R_EditFile)(const char *);
void (*GC_ptr_R_loadhistory)(SEXP, SEXP, SEXP, SEXP);
void (*GC_ptr_R_savehistory)(SEXP, SEXP, SEXP, SEXP);
void (*GC_ptr_R_addhistory)(SEXP, SEXP, SEXP, SEXP);
int  (*GC_ptr_R_EditFiles)(int, const char **, const char **, const char *);
void (*GC_ptr_R_ProcessEvents)();
#endif
FILE * GC_R_Outputfile;
FILE * GC_R_Consolefile;

// R data
SEXP GC_Rf_allocVector(SEXPTYPE, R_xlen_t){ }
SEXP GC_Rf_allocList(int){ }
void GC_Rf_unprotect(int){ }
SEXP GC_Rf_protect(SEXP){ }
SEXP GC_SETCAR(SEXP x, SEXP y){ }
SEXP (GC_CDR)(SEXP e){ }
void GC_SET_STRING_ELT(SEXP x, R_xlen_t i, SEXP v){ }
SEXP (GC_VECTOR_ELT)(SEXP x, R_xlen_t i){ }
SEXP GC_Rf_mkChar(const char *){ }
SEXP GC_Rf_mkString(const char *){ }
SEXP GC_Rf_namesgets(SEXP, SEXP){ }
SEXP GC_Rf_classgets(SEXP, SEXP){ }
R_len_t GC_Rf_length(SEXP){ }
SEXP (GC_STRING_ELT)(SEXP x, R_xlen_t i){ }
SEXP GC_Rf_coerceVector(SEXP, SEXPTYPE){ }
SEXP GC_R_ParseVector(SEXP, int, ParseStatus *, SEXP){ }
double *(GC_REAL)(SEXP x){ }
int *(GC_INTEGER)(SEXP x){ }
int *(GC_LOGICAL)(SEXP x){ }
SEXP GC_Rf_install(const char *){ }
SEXP GC_Rf_setAttrib(SEXP, SEXP, SEXP) { }
Rboolean (GC_Rf_isNull)(SEXP s) { }
char *(GC_R_CHAR)(SEXP x) { }

// Graphics Device
pGEDevDesc GC_GEcreateDevDesc(pDevDesc dev) { }
void GC_GEaddDevice2(pGEDevDesc, const char *) { }
void GC_Rf_onintr(void) { }
int GC_R_GE_getVersion(void) { }
void GC_R_CheckDeviceAvailable(void) { }
Rboolean GC_R_interrupts_suspended;
int GC_R_interrupts_pending;

// globals
SEXP GC_R_GlobalEnv, GC_R_NilValue,
     GC_R_RowNamesSymbol, GC_R_ClassSymbol;
double GC_R_NaReal;
#endif
