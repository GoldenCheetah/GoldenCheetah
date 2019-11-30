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

// QFunctionPointer typedef introduced in QT5
#include <QtGlobal>
#if QT_VERSION < 0x050000
typedef void *QFunctionPointer;
#endif

#include <QStringList>
class QString;
class QLibrary;
class RLibrary {

    public:

        // initialise
        RLibrary();

        // we check/message of resolving fails
        QFunctionPointer resolve(const char * symbol);

        // load the library return success or failure
        bool load();

        // it loaded successfully ?
        bool loaded;
        QStringList errors;

        // the library as loaded
        QLibrary *libR;

};

// Must only be included after standard R headers
// in order to redefine the entry points via QLibrary

// R Library Entry Points used by REmbed
extern void GC_R_dot_Last(void);
extern void GC_R_CheckUserInterrupt(void);
extern void GC_R_RunExitFinalizers(void);
extern void GC_R_CleanTempDir(void);
extern int  GC_Rf_initEmbeddedR(int argc, char *argv[]);
extern void GC_Rf_endEmbeddedR(int fatal);
extern void GC_R_ReplDLLinit(void);
extern void GC_R_DefParams(Rstart);
extern void GC_R_SetParams(Rstart);
extern SEXP GC_R_tryEval(SEXP, SEXP, int *);
typedef void (*Prot_GC_Rf_error)(const char *, ...);
typedef void (*Prot_GC_Rf_warning)(const char *, ...);
extern Prot_GC_Rf_error ptr_GC_Rf_error;
extern Prot_GC_Rf_warning ptr_GC_Rf_warning;
extern void GC_Rf_PrintValue(SEXP);
extern DllInfo *GC_R_getEmbeddingDllInfo(void);
extern int GC_R_registerRoutines(DllInfo *info, const R_CMethodDef * const croutines,
                          const R_CallMethodDef * const callRoutines,
                          const R_FortranMethodDef * const fortranRoutines,
                          const R_ExternalMethodDef * const externalRoutines);
#ifndef WIN32
extern void (**ptr_GC_ptr_R_Suicide)(const char *);
extern void (**ptr_GC_ptr_R_ShowMessage)(const char *);
extern int  (**ptr_GC_ptr_R_ReadConsole)(const char *, unsigned char *, int, int);
extern void (**ptr_GC_ptr_R_WriteConsole)(const char *, int);
extern void (**ptr_GC_ptr_R_WriteConsoleEx)(const char *, int, int);
extern void (**ptr_GC_ptr_R_ResetConsole)(void);
extern void (**ptr_GC_ptr_R_FlushConsole)(void);
extern void (**ptr_GC_ptr_R_ClearerrConsole)(void);
extern void (**ptr_GC_ptr_R_ProcessEvents)(void);
extern void (**ptr_GC_ptr_R_Busy)(int);
extern int *pGC_R_interrupts_pending;
#else
typedef char *(*Prot_GC_getDLLVersion)(void);
typedef char *(*Prot_GC_getRUser)(void);
typedef char *(*Prot_GC_get_R_HOME)(void);
extern Prot_GC_getDLLVersion ptr_GC_getDLLVersion;
extern Prot_GC_getRUser ptr_GC_getRUser;
extern Prot_GC_get_R_HOME ptr_GC_get_R_HOME;
extern int *pGC_R_UserBreak;
#endif

// R Data
extern SEXP GC_Rf_allocVector(SEXPTYPE, R_xlen_t);
extern SEXP GC_Rf_allocList(int);
extern void GC_Rf_unprotect(int);
extern SEXP GC_Rf_protect(SEXP);
extern SEXP GC_SETCAR(SEXP x, SEXP y);
extern void GC_SET_TYPEOF(SEXP x, int);
extern SEXP (GC_CDR)(SEXP e);
extern void GC_SET_STRING_ELT(SEXP x, R_xlen_t i, SEXP v);
extern SEXP GC_SET_VECTOR_ELT(SEXP x, R_xlen_t i, SEXP v);
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
extern const char *(GC_R_CHAR)(SEXP x);

// Graphics Device
#ifdef R_RGB // only redo graphics device if its included
extern pGEDevDesc GC_GEcreateDevDesc(pDevDesc dev);
extern void GC_GEaddDevice2(pGEDevDesc, const char *);
extern void GC_Rf_onintr(void);
extern int GC_R_GE_getVersion(void);
extern void GC_R_CheckDeviceAvailable(void);
extern int GC_Rf_selectDevice(int);
extern void GC_Rf_killDevice(int);
extern int GC_Rf_ndevNumber(pDevDesc);
extern Rboolean GC_R_interrupts_suspended;
extern int GC_R_interrupts_pending;
#endif

// globals .. don't resolve with QLibrary (!)
//         .. so we fake them and create our own
extern FILE ** pGC_R_Outputfile;
extern FILE ** pGC_R_Consolefile;
extern SEXP * pGC_R_GlobalEnv,         // XXX TODO
            * pGC_R_NilValue,          // NILSXP
            * pGC_R_RowNamesSymbol,    // "row.names"
            * pGC_R_ClassSymbol;       // "class"
extern double *pGC_R_NaReal;          // XXX TODO NaReal value


//
// Map R functions to GC proxy to access QLibrary versions
//

// embedding
#define R_dot_Last                  GC_R_dot_Last
#define R_CheckUserInterrupt        GC_R_CheckUserInterrupt
#define R_RunExitFinalizers         GC_R_RunExitFinalizers
#define R_CleanTempDir              GC_R_CleanTempDir
#define Rf_initEmbeddedR            GC_Rf_initEmbeddedR
#define Rf_endEmbeddedR             GC_Rf_endEmbeddedR
#define R_ReplDLLinit               GC_R_ReplDLLinit
#define R_SetParams                 GC_R_SetParams
#define R_DefParams                 GC_R_DefParams
#define R_tryEval                   GC_R_tryEval
#define R_ParseVector               GC_R_ParseVector
#define Rf_error                    (*ptr_GC_Rf_error) // dereference directly due to varargs pass thru
#define Rf_warning                  (*ptr_GC_Rf_warning) // dereference directly due to varargs pass thru
#define Rf_PrintValue               GC_Rf_PrintValue
#define R_getEmbeddingDllInfo       GC_R_getEmbeddingDllInfo
#define ptr_R_Suicide               (*ptr_GC_ptr_R_Suicide)
#define ptr_R_ShowMessage           (*ptr_GC_ptr_R_ShowMessage)
#define ptr_R_ReadConsole           (*ptr_GC_ptr_R_ReadConsole)
#define ptr_R_WriteConsole          (*ptr_GC_ptr_R_WriteConsole)
#define ptr_R_WriteConsoleEx        (*ptr_GC_ptr_R_WriteConsoleEx)
#define ptr_R_ResetConsole          (*ptr_GC_ptr_R_ResetConsole)
#define ptr_R_FlushConsole          (*ptr_GC_ptr_R_FlushConsole)
#define ptr_R_ClearerrConsole       (*ptr_GC_ptr_R_ClearerrConsole)
#define ptr_R_ProcessEvents         (*ptr_GC_ptr_R_ProcessEvents)
#define ptr_R_Busy                  (*ptr_GC_ptr_R_Busy)
#define R_registerRoutines          GC_R_registerRoutines
#ifdef WIN32
#define getDLLVersion				(*ptr_GC_getDLLVersion)
#define getRUser                    (*ptr_GC_getRUser)
#define get_R_HOME                  (*ptr_GC_get_R_HOME)
#define UserBreak                   (*pGC_R_UserBreak)
#else
#define R_interrupts_pending        (*pGC_R_interrupts_pending)
#endif

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
#define SETCAR                      GC_SETCAR
#define CDR                         GC_CDR
#define SET_STRING_ELT              GC_SET_STRING_ELT
#define SET_VECTOR_ELT              GC_SET_VECTOR_ELT
#define SET_TYPEOF                  GC_SET_TYPEOF
#define STRING_ELT                  GC_STRING_ELT
#define VECTOR_ELT                  GC_VECTOR_ELT
#define REAL                        GC_REAL
#define INTEGER                     GC_INTEGER
#define LOGICAL                     GC_LOGICAL
#define R_CHAR                      GC_R_CHAR

// Graphics device
#define GEcreateDevDesc             GC_GEcreateDevDesc
#define GEaddDevice2                GC_GEaddDevice2
#define Rf_selectDevice             GC_Rf_selectDevice
#define Rf_ndevNumber               GC_Rf_ndevNumber
#define Rf_killDevice               GC_Rf_killDevice
#define Rf_onintr                   GC_Rf_onintr
#define R_GE_getVersion             GC_R_GE_getVersion
#define R_CheckDeviceAvailable      GC_R_CheckDeviceAvailable
#define R_interrupts_suspended      GC_R_interrupts_suspended

// globals
#define R_GlobalEnv                 (*pGC_R_GlobalEnv)
#define R_NilValue                  (*pGC_R_NilValue)
#define R_NaReal                    (*pGC_R_NaReal)
#define R_RowNamesSymbol            (*pGC_R_RowNamesSymbol)
#define R_ClassSymbol               (*pGC_R_ClassSymbol)
#define R_Consolefile               (*pGC_R_Consolefile)
#define R_Outputfile                (*pGC_R_Outputfile)
#endif
