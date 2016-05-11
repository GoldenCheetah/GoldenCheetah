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
extern int GC_R_registerRoutines(DllInfo *info, const R_CMethodDef * const croutines,
                          const R_CallMethodDef * const callRoutines,
                          const R_FortranMethodDef * const fortranRoutines,
                          const R_ExternalMethodDef * const externalRoutines);
#ifndef WIN32
extern void (*GC_ptr_R_Suicide)(const char *);
extern void (*GC_ptr_R_ShowMessage)(const char *);
extern int  (*GC_ptr_R_ReadConsole)(const char *, unsigned char *, int, int);
extern void (*GC_ptr_R_WriteConsole)(const char *, int);
extern void (*GC_ptr_R_WriteConsoleEx)(const char *, int, int);
extern void (*GC_ptr_R_ResetConsole)(void);
extern void (*GC_ptr_R_FlushConsole)(void);
extern void (*GC_ptr_R_ClearerrConsole)(void);
extern void (*GC_ptr_R_Busy)(int);
extern void (*GC_ptr_R_CleanUp)(SA_TYPE, int, int);
extern int  (*GC_ptr_R_ChooseFile)(int, char *, int);
extern int  (*GC_ptr_R_EditFile)(const char *);
extern void (*GC_ptr_R_loadhistory)(SEXP, SEXP, SEXP, SEXP);
extern void (*GC_ptr_R_savehistory)(SEXP, SEXP, SEXP, SEXP);
extern void (*GC_ptr_R_addhistory)(SEXP, SEXP, SEXP, SEXP);
extern int  (*GC_ptr_R_EditFiles)(int, const char **, const char **, const char *);
extern void (*GC_ptr_R_ProcessEvents)();
#endif
extern FILE * GC_R_Outputfile;
extern FILE * GC_R_Consolefile;

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

// Graphics Device
#ifdef R_RGB // only redo graphics device if its included
extern pGEDevDesc GC_GEcreateDevDesc(pDevDesc dev);
extern void GC_GEaddDevice2(pGEDevDesc, const char *);
extern void GC_Rf_onintr(void);
extern int GC_R_GE_getVersion(void);
extern void GC_R_CheckDeviceAvailable(void);
extern Rboolean GC_R_interrupts_suspended;
extern int GC_R_interrupts_pending;
#endif

// globals
extern SEXP GC_R_GlobalEnv, GC_R_NilValue,
            GC_R_RowNamesSymbol, GC_R_ClassSymbol;
extern double GC_R_NaReal;


//
// Map R functions to GC proxy to access QLibrary versions
//

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
#define ptr_R_Suicide               GC_ptr_R_Suicide
#define ptr_R_ShowMessage           GC_ptr_R_ShowMessage
#define ptr_R_ReadConsole           GC_ptr_R_ReadConsole
#define ptr_R_WriteConsole          GC_ptr_R_WriteConsole
#define ptr_R_WriteConsoleEx        GC_ptr_R_WriteConsoleEx
#define ptr_R_ResetConsole          GC_ptr_R_ResetConsole
#define ptr_R_FlushConsole          GC_ptr_R_FlushConsole
#define ptr_R_ClearerrConsole       GC_ptr_R_ClearerrConsole
#define ptr_R_Busy                  GC_ptr_R_Busy
#define ptr_R_CleanUp               GC_ptr_R_CleanUp
#define ptr_R_ShowFiles             GC_ptr_R_ShowFiles
#define ptr_R_ChooseFile            GC_ptr_R_ChooseFile
#define ptr_R_EditFile              GC_ptr_R_EditFile
#define ptr_R_loadhistory           GC_ptr_R_loadhistory
#define ptr_R_savehistory           GC_ptr_R_savehistory
#define ptr_R_addhistory            GC_ptr_R_addhistory
#define ptr_R_EditFiles             GC_ptr_R_EditFiles
#define ptr_R_ProcessEvents         GC_ptr_R_ProcessEvents
#define R_registerRoutines          GC_R_registerRoutines
#define R_Consolefile               GC_R_Consolefile
#define R_Outputfile                GC_R_Outputfile

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

// Graphics device
#define GEcreateDevDesc             GC_GEcreateDevDesc
#define GEaddDevice2                GC_GEaddDevice2
#define Rf_onintr                   GC_Rf_onintr
#define R_GE_getVersion             GC_R_GE_getVersion
#define R_CheckDeviceAvailable      GC_R_CheckDeviceAvailable
#define R_interrupts_suspended      GC_R_interrupts_suspended
#define R_interrupts_pending        GC_R_interrupts_pending

// globals
#define R_GlobalEnv                 GC_R_GlobalEnv
#define R_NilValue                  GC_R_NilValue
#define R_NaReal                    GC_R_NaReal
#endif
#endif
