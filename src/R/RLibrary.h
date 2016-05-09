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

#ifndef GC_R_Library_H
#define GC_R_Library_H
#ifdef GC_WANT_R_DYNAMIC
// Must only be included after standard R headers
// in order to redefine the entry points via QLibrary

// R Library Entry Points used by REmbed
extern void GC_R_dot_Last(void);
extern void GC_R_RunExitFinalizers(void);
extern void GC_R_CleanTempDir(void);
extern int  GC_Rf_initEmbeddedR(int argc, char *argv[]);
extern void GC_Rf_endEmbeddedR(int fatal);
extern void GC_R_ReplDLLinit(void);
extern void GC_R_DefParams(Rstart);
extern void GC_R_SetParams(Rstart);
extern SEXP GC_R_tryEval(SEXP, SEXP, int *);
extern void NORET GC_Rf_error(const char *, ...);
extern void NORET GC_Rf_warning(const char *, ...);
extern void GC_Rf_PrintValue(SEXP);
extern DllInfo *GC_R_getEmbeddingDllInfo(void);

// R Data
extern SEXP GC_Rf_allocVector(SEXPTYPE, R_xlen_t);
extern SEXP GC_Rf_allocList(int);
extern void GC_Rf_unprotect(int);
extern SEXP GC_Rf_protect(SEXP);
extern SEXP GC_SETCAR(SEXP x, SEXP y);
extern SEXP (GC_CDR)(SEXP e);
extern void GC_SET_STRING_ELT(SEXP x, R_xlen_t i, SEXP v);
extern SEXP (GC_VECTOR_ELT)(SEXP x, R_xlen_t i);
extern SEXP GC_Rf_mkChar(const char *);
extern SEXP GC_Rf_mkString(const char *);
extern SEXP GC_Rf_namesgets(SEXP, SEXP);
extern SEXP GC_Rf_classgets(SEXP, SEXP);
extern R_len_t GC_Rf_length(SEXP);
extern SEXP (GC_STRING_ELT)(SEXP x, R_xlen_t i);
extern SEXP GC_Rf_coerceVector(SEXP, SEXPTYPE);
extern SEXP GC_R_ParseVector(SEXP, int, ParseStatus *, SEXP);
extern double *(GC_REAL)(SEXP x);
extern int *(GC_INTEGER)(SEXP x);
extern int *(GC_LOGICAL)(SEXP x);
extern SEXP GC_Rf_install(const char *);
extern SEXP GC_Rf_setAttrib(SEXP, SEXP, SEXP);
extern Rboolean (GC_Rf_isNull)(SEXP s);
const char *(GC_R_CHAR)(SEXP x);

// globals
extern SEXP GC_R_GlobalEnv, GC_R_NilValue,
            GC_R_RowNamesSymbol, GC_R_ClassSymbol;
extern double GC_R_NaReal;

// embedding
#define R_dot_Last                  GC_R_dot_Last
#define R_RunExitFinalizers         GC_R_RunExitFinalizers
#define R_CleanTempDir              GC_R_CleanTempDir
#define Rf_initEmbeddedR            GC_Rf_initEmbeddedR
#define Rf_endEmbeddedR             GC_Rf_endEmbeddedR
#define R_ReplDLLinit               GC_R_ReplDLLinit
#define R_SetParams                 GC_R_SetParams
#define R_DefParams                 GC_R_DefParams
#define R_tryEval                   GC_R_tryEval
#define R_ParseVector               GC_R_ParseVector
#define Rf_error                    GC_Rf_error
#define Rf_warning                  GC_Rf_warning
#define Rf_PrintValue               GC_Rf_PrintValue
#define R_getEmbeddingDllInfo       GC_R_getEmbeddingDllInfo


// data wrangling and manipulation
#define Rf_allocVector              GC_Rf_allocVector
#define Rf_allocList                GC_Rf_allocList
#define Rf_coerceVector             GC_Rf_coerceVector
#define Rf_protect                  GC_Rf_protect
#define Rf_unprotect                GC_Rf_unprotect
#define Rf_namesgets                GC_Rf_namesgets
#define Rf_classgets                GC_Rf_classgets
#define Rf_setAttrib                GC_Rf_setAttrib
#define Rf_mkChar                   GC_Rf_mkChar
#define Rf_mkString                 GC_Rf_mkString
#define Rf_length                   GC_Rf_length
#define Rf_install                  GC_Rf_install
#define Rf_isNull                   GC_Rf_isNull
#define R_RowNamesSymbol            GC_R_RowNamesSymbol
#define R_ClassSymbol               GC_R_ClassSymbol
#define SETCAR                      GC_SETCAR
#define CDR                         GC_CDR
#define SET_STRING_ELT              GC_SET_STRING_ELT
#define STRING_ELT                  GC_STRING_ELT
#define VECTOR_ELT                  GC_VECTOR_ELT
#define REAL                        GC_REAL
#define INTEGER                     GC_INTEGER
#define LOGICAL                     GC_LOGICAL
#define R_CHAR                      GC_R_CHAR

// globals
#define R_GlobalEnv                 GC_R_GlobalEnv
#define R_NilValue                  GC_R_NilValue
#define R_NaReal                    GC_R_NaReal
#endif
#endif
