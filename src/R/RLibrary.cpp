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

#define R_NO_REMAP
#include <R.h>
#include <Rinternals.h>
#include <R_ext/RStartup.h>
#include <R_ext/Parse.h>
#include <R_ext/Rdynload.h>
#include <R_ext/GraphicsEngine.h>
#include <R_ext/GraphicsDevice.h>

#include "RLibrary.h"
#include "Settings.h"

#include <QLibrary>
#include <QFile>

//
// FUNCTION SIGNATURES
//
// Typdefs for each of the functions to get the right signature to
// cast from the (*void)() signature returned by QLibrary::resolve()
//
// R Embedding methods
typedef void (*Prot_GC_R_dot_Last)(void);
typedef void (*Prot_GC_R_CheckUserInterrupt)(void);
typedef void (*Prot_GC_R_RunExitFinalizers)(void);
typedef void (*Prot_GC_R_CleanTempDir)(void);
typedef int  (*Prot_GC_Rf_initEmbeddedR)(int argc, char *argv[]);
typedef void (*Prot_GC_Rf_endEmbeddedR)(int fatal);
typedef void (*Prot_GC_R_ReplDLLinit)(void);
typedef void (*Prot_GC_R_DefParams)(Rstart);
typedef void (*Prot_GC_R_SetParams)(Rstart);
typedef SEXP (*Prot_GC_R_tryEval)(SEXP, SEXP, int *);
typedef void (*Prot_GC_Rf_PrintValue)(SEXP);
typedef DllInfo *(*Prot_GC_R_getEmbeddingDllInfo)(void);
typedef int (*Prot_GC_R_registerRoutines)(DllInfo *info, const R_CMethodDef * const croutines, const R_CallMethodDef * const callRoutines, const R_FortranMethodDef * const fortranRoutines, const R_ExternalMethodDef * const externalRoutines);
#ifndef WIN32
typedef void (*(*Prot_GC_ptr_R_Suicide))(const char *);
typedef void (*(*Prot_GC_ptr_R_ShowMessage))(const char *);
typedef int  (*(*Prot_GC_ptr_R_ReadConsole))(const char *, unsigned char *, int, int);
typedef void (*(*Prot_GC_ptr_R_WriteConsole))(const char *, int);
typedef void (*(*Prot_GC_ptr_R_WriteConsoleEx))(const char *, int, int);
typedef void (*(*Prot_GC_ptr_R_ResetConsole))(void);
typedef void (*(*Prot_GC_ptr_R_FlushConsole))(void);
typedef void (*(*Prot_GC_ptr_R_ClearerrConsole))(void);
typedef void (*(*Prot_GC_ptr_R_ProcessEvents))(void);
typedef void (*(*Prot_GC_ptr_R_Busy))(int);
#endif

// R data
typedef SEXP (*Prot_GC_Rf_allocVector)(SEXPTYPE, R_xlen_t);
typedef SEXP (*Prot_GC_Rf_allocList)(int);
typedef void (*Prot_GC_Rf_unprotect)(int);
typedef SEXP (*Prot_GC_Rf_protect)(SEXP);
typedef SEXP (*Prot_GC_SETCAR)(SEXP x, SEXP y);
typedef void (*Prot_GC_SET_TYPEOF)(SEXP x, int);
typedef SEXP ((*Prot_GC_CDR))(SEXP e);
typedef void (*Prot_GC_SET_STRING_ELT)(SEXP x, R_xlen_t i, SEXP v);
typedef SEXP (*Prot_GC_SET_VECTOR_ELT)(SEXP x, R_xlen_t i, SEXP v);
typedef SEXP ((*Prot_GC_VECTOR_ELT))(SEXP x, R_xlen_t i);
typedef SEXP (*Prot_GC_Rf_mkChar)(const char *);
typedef SEXP (*Prot_GC_Rf_mkString)(const char *);
typedef SEXP (*Prot_GC_Rf_namesgets)(SEXP, SEXP);
typedef SEXP (*Prot_GC_Rf_classgets)(SEXP, SEXP);
typedef R_len_t (*Prot_GC_Rf_length)(SEXP);
typedef SEXP ((*Prot_GC_STRING_ELT))(SEXP x, R_xlen_t i);
typedef SEXP (*Prot_GC_Rf_coerceVector)(SEXP, SEXPTYPE);
typedef SEXP (*Prot_GC_R_ParseVector)(SEXP, int, ParseStatus *, SEXP);
typedef double *((*Prot_GC_REAL))(SEXP x);
typedef int *((*Prot_GC_INTEGER))(SEXP x);
typedef int *((*Prot_GC_LOGICAL))(SEXP x);
typedef SEXP (*Prot_GC_Rf_install)(const char *);
typedef SEXP (*Prot_GC_Rf_setAttrib)(SEXP, SEXP, SEXP);
typedef Rboolean ((*Prot_GC_Rf_isNull))(SEXP s);
typedef char *((*Prot_GC_R_CHAR))(SEXP x);

// Graphics Device
typedef pGEDevDesc (*Prot_GC_GEcreateDevDesc)(pDevDesc dev);
typedef void (*Prot_GC_GEaddDevice2)(pGEDevDesc, const char *);
typedef void (*Prot_GC_Rf_onintr)(void);
typedef int (*Prot_GC_Rf_selectDevice)(int);
typedef void (*Prot_GC_Rf_killDevice)(int);
typedef int (*Prot_GC_Rf_ndevNumber)(pDevDesc);
typedef int (*Prot_GC_R_GE_getVersion)(void);
typedef void (*Prot_GC_R_CheckDeviceAvailable)(void);

//
// FUNCTION POINTERS
//
// The actual function pointers. These are set once the
// library is loaded, and are de-referenced by the proxy
// methods at the bottom of this source file
//
// R Embedding methods
Prot_GC_R_dot_Last ptr_GC_R_dot_Last;
Prot_GC_R_CheckUserInterrupt ptr_GC_R_CheckUserInterrupt;
Prot_GC_R_RunExitFinalizers ptr_GC_R_RunExitFinalizers;
Prot_GC_R_CleanTempDir ptr_GC_R_CleanTempDir;
Prot_GC_Rf_initEmbeddedR ptr_GC_Rf_initEmbeddedR;
Prot_GC_Rf_endEmbeddedR ptr_GC_Rf_endEmbeddedR;
Prot_GC_R_ReplDLLinit ptr_GC_R_ReplDLLinit;
Prot_GC_R_DefParams ptr_GC_R_DefParams;
Prot_GC_R_SetParams ptr_GC_R_SetParams;
Prot_GC_R_tryEval ptr_GC_R_tryEval;
Prot_GC_Rf_error ptr_GC_Rf_error;
Prot_GC_Rf_warning ptr_GC_Rf_warning;
Prot_GC_Rf_PrintValue ptr_GC_Rf_PrintValue;
Prot_GC_R_getEmbeddingDllInfo ptr_GC_R_getEmbeddingDllInfo;
Prot_GC_R_registerRoutines ptr_GC_R_registerRoutines;
#ifndef WIN32
Prot_GC_ptr_R_Suicide ptr_GC_ptr_R_Suicide;
Prot_GC_ptr_R_ShowMessage ptr_GC_ptr_R_ShowMessage;
Prot_GC_ptr_R_ReadConsole ptr_GC_ptr_R_ReadConsole;
Prot_GC_ptr_R_WriteConsole ptr_GC_ptr_R_WriteConsole;
Prot_GC_ptr_R_WriteConsoleEx ptr_GC_ptr_R_WriteConsoleEx;
Prot_GC_ptr_R_ResetConsole ptr_GC_ptr_R_ResetConsole;
Prot_GC_ptr_R_FlushConsole ptr_GC_ptr_R_FlushConsole;
Prot_GC_ptr_R_ClearerrConsole ptr_GC_ptr_R_ClearerrConsole;
Prot_GC_ptr_R_ProcessEvents ptr_GC_ptr_R_ProcessEvents;
Prot_GC_ptr_R_Busy ptr_GC_ptr_R_Busy;
int *pGC_R_interrupts_pending;
#else
Prot_GC_getDLLVersion ptr_GC_getDLLVersion;
Prot_GC_getRUser ptr_GC_getRUser;
Prot_GC_get_R_HOME ptr_GC_get_R_HOME;
int *pGC_R_UserBreak;
#endif

// R data
Prot_GC_Rf_allocVector ptr_GC_Rf_allocVector;
Prot_GC_Rf_allocList ptr_GC_Rf_allocList;
Prot_GC_Rf_unprotect ptr_GC_Rf_unprotect;
Prot_GC_Rf_protect ptr_GC_Rf_protect;
Prot_GC_SETCAR ptr_GC_SETCAR;
Prot_GC_SET_TYPEOF ptr_GC_SET_TYPEOF;
Prot_GC_CDR ptr_GC_CDR;
Prot_GC_SET_STRING_ELT ptr_GC_SET_STRING_ELT;
Prot_GC_SET_VECTOR_ELT ptr_GC_SET_VECTOR_ELT;
Prot_GC_VECTOR_ELT ptr_GC_VECTOR_ELT;
Prot_GC_Rf_mkChar ptr_GC_Rf_mkChar;
Prot_GC_Rf_mkString ptr_GC_Rf_mkString;
Prot_GC_Rf_namesgets ptr_GC_Rf_namesgets;
Prot_GC_Rf_classgets ptr_GC_Rf_classgets;
Prot_GC_Rf_length ptr_GC_Rf_length;
Prot_GC_STRING_ELT ptr_GC_STRING_ELT;
Prot_GC_Rf_coerceVector ptr_GC_Rf_coerceVector;
Prot_GC_R_ParseVector ptr_GC_R_ParseVector;
Prot_GC_REAL ptr_GC_REAL;
Prot_GC_INTEGER ptr_GC_INTEGER;
Prot_GC_LOGICAL ptr_GC_LOGICAL;
Prot_GC_Rf_install ptr_GC_Rf_install;
Prot_GC_Rf_setAttrib ptr_GC_Rf_setAttrib;
Prot_GC_Rf_isNull ptr_GC_Rf_isNull;
Prot_GC_R_CHAR ptr_GC_R_CHAR;

// Graphics Device
Prot_GC_GEcreateDevDesc ptr_GC_GEcreateDevDesc;
Prot_GC_GEaddDevice2 ptr_GC_GEaddDevice2;
Prot_GC_Rf_onintr ptr_GC_Rf_onintr;
Prot_GC_Rf_selectDevice ptr_GC_Rf_selectDevice;
Prot_GC_Rf_ndevNumber ptr_GC_Rf_ndevNumber;
Prot_GC_Rf_killDevice ptr_GC_Rf_killDevice;
Prot_GC_R_GE_getVersion ptr_GC_R_GE_getVersion;
Prot_GC_R_CheckDeviceAvailable ptr_GC_R_CheckDeviceAvailable;

//
// ENTRY POINTS USED FROM GC CODEBASE
//
// These proxy methods will call the resolved functions once the
// dynamic libraries have been loaded. Below you will find the
// typdef and actual pointers that are set via QLibrary::resolve()
//

// R Embedding methods
void GC_R_dot_Last(void) { (*ptr_GC_R_dot_Last)(); }
void GC_R_CheckUserInterrupt(void) { (*ptr_GC_R_CheckUserInterrupt)(); }
void GC_R_RunExitFinalizers(void) { (*ptr_GC_R_RunExitFinalizers)(); }
void GC_R_CleanTempDir(void) { (*ptr_GC_R_CleanTempDir)(); }
int  GC_Rf_initEmbeddedR(int argc, char *argv[]) { return (*ptr_GC_Rf_initEmbeddedR)(argc,argv); }
void GC_Rf_endEmbeddedR(int fatal) { (*ptr_GC_Rf_endEmbeddedR)(fatal); }
void GC_R_ReplDLLinit(void) { (*ptr_GC_R_ReplDLLinit)(); }
void GC_R_DefParams(Rstart x) { (*ptr_GC_R_DefParams)(x); }
void GC_R_SetParams(Rstart x) { (*ptr_GC_R_SetParams)(x); }
SEXP GC_R_tryEval(SEXP a, SEXP b, int *c) { return (*ptr_GC_R_tryEval)(a,b,c); }
void GC_Rf_error(const char *, ...) { } //XXX how to pass ... ?
void GC_Rf_warning(const char *, ...) { }
void GC_Rf_PrintValue(SEXP x) { (*ptr_GC_Rf_PrintValue)(x); }
DllInfo *GC_R_getEmbeddingDllInfo(void) { return (*ptr_GC_R_getEmbeddingDllInfo)(); }
int GC_R_registerRoutines(DllInfo *a, const R_CMethodDef * const b, const R_CallMethodDef * const c, const R_FortranMethodDef * const d, const R_ExternalMethodDef * const e)
                          { return (*ptr_GC_R_registerRoutines)(a,b,c,d,e); }
// R data
SEXP GC_Rf_allocVector(SEXPTYPE a, R_xlen_t b) { return (*ptr_GC_Rf_allocVector)(a,b); }
SEXP GC_Rf_allocList(int x) { return (*ptr_GC_Rf_allocList)(x); }
void GC_Rf_unprotect(int x) { (*ptr_GC_Rf_unprotect)(x); }
SEXP GC_Rf_protect(SEXP x) { return (*ptr_GC_Rf_protect)(x); }
SEXP GC_SETCAR(SEXP x, SEXP y) { return (*ptr_GC_SETCAR)(x,y); }
void GC_SET_TYPEOF(SEXP x, int y) { return (*ptr_GC_SET_TYPEOF)(x,y); }
SEXP (GC_CDR)(SEXP e) { return (*ptr_GC_CDR)(e); }
void GC_SET_STRING_ELT(SEXP x, R_xlen_t i, SEXP v) { (*ptr_GC_SET_STRING_ELT)(x,i,v); }
SEXP GC_SET_VECTOR_ELT(SEXP x, R_xlen_t i, SEXP v) { return (*ptr_GC_SET_VECTOR_ELT)(x,i,v); }
SEXP (GC_VECTOR_ELT)(SEXP x, R_xlen_t i) { return (*ptr_GC_VECTOR_ELT)(x,i); }
SEXP GC_Rf_mkChar(const char *a) { return (*ptr_GC_Rf_mkChar)(a); }
SEXP GC_Rf_mkString(const char *b) { return (*ptr_GC_Rf_mkString)(b); }
SEXP GC_Rf_namesgets(SEXP a, SEXP b) { return (*ptr_GC_Rf_namesgets)(a, b); }
SEXP GC_Rf_classgets(SEXP a, SEXP b) { return (*ptr_GC_Rf_classgets)(a,b); }
R_len_t GC_Rf_length(SEXP a) { return (*ptr_GC_Rf_length)(a); }
SEXP (GC_STRING_ELT)(SEXP x, R_xlen_t i) { return (*ptr_GC_STRING_ELT)(x,i); }
SEXP GC_Rf_coerceVector(SEXP a, SEXPTYPE b) { return (*ptr_GC_Rf_coerceVector)(a,b); }
SEXP GC_R_ParseVector(SEXP a, int b, ParseStatus *c, SEXP d) { return (*ptr_GC_R_ParseVector)(a,b,c,d); }
double *(GC_REAL)(SEXP x) { return (*ptr_GC_REAL)(x); }
int *(GC_INTEGER)(SEXP x) { return (*ptr_GC_INTEGER)(x); }
int *(GC_LOGICAL)(SEXP x) { return (*ptr_GC_LOGICAL)(x); }
SEXP GC_Rf_install(const char *a) { return (*ptr_GC_Rf_install)(a); }
SEXP GC_Rf_setAttrib(SEXP a, SEXP b, SEXP c) { return (*ptr_GC_Rf_setAttrib)(a,b,c); }
Rboolean (GC_Rf_isNull)(SEXP s) { return (*ptr_GC_Rf_isNull)(s); }
const char *(GC_R_CHAR)(SEXP x) { return (*ptr_GC_R_CHAR)(x); }

// Graphics Device
pGEDevDesc GC_GEcreateDevDesc(pDevDesc dev) { return (*ptr_GC_GEcreateDevDesc)(dev); }
void GC_GEaddDevice2(pGEDevDesc a, const char *b) { (*ptr_GC_GEaddDevice2)(a,b); }
void GC_Rf_onintr(void) { (*ptr_GC_Rf_onintr)(); }
int GC_R_GE_getVersion(void) { return (*ptr_GC_R_GE_getVersion)(); }
void GC_R_CheckDeviceAvailable(void) { (*ptr_GC_R_CheckDeviceAvailable)(); }
int GC_Rf_selectDevice(int x) { return (*ptr_GC_Rf_selectDevice)(x); }
void GC_Rf_killDevice(int x) { (*ptr_GC_Rf_killDevice)(x);}
int GC_Rf_ndevNumber(pDevDesc x) { return (*ptr_GC_Rf_ndevNumber)(x); }
Rboolean GC_R_interrupts_suspended;
int GC_R_interrupts_pending;

// globals
SEXP *pGC_R_GlobalEnv, *pGC_R_NilValue,
     *pGC_R_RowNamesSymbol, *pGC_R_ClassSymbol;
double *pGC_R_NaReal;
FILE ** pGC_R_Outputfile;
FILE ** pGC_R_Consolefile;


//
// Dynamic Loader
//
//
RLibrary::RLibrary()
{
    loaded=false;
    libR = NULL;
}

QFunctionPointer
RLibrary::resolve(const char *symbol)
{
    QFunctionPointer returning = libR->resolve(symbol);
    if (returning) return returning;
    else {
        errors << tr("R lib: failed to resolve symbol: '%1'").arg(symbol);
        loaded = false;
        return NULL;
    }
}

// by default any dependant libs will be loaded only if they
// are in the relevant search path. R libs are rarely in this
// path, so we update it just whilst we load the libs
#ifdef WIN32
const char *gcSearchPath = "PATH";
const char *gcSearchSep = ";";
// its in REmbed.h, but we don't want to include it here
extern int setenv(QString name,  QString value, bool overwrite);
#else
const char *gcSearchPath = "LD_LIBRARY_PATH";
const char *gcSearchSep = ":";
#endif


bool
RLibrary::load()
{
    // if there were any from last time, clear them
    errors.clear();

    // get location from environment or config - try config then environment
    // since user may override the value in configuration
    QString name, home =  appsettings->value(NULL,GC_R_HOME,"").toString();
    if (home == "") home = getenv("R_HOME");

    // if home still blanks
    bool sethome = false;
    if (home == "") {
        errors << tr("R_HOME has not been configured in options or the system environment"
                  " so we looked in the common places to find the R install.\n\n");
        sethome = true;
    }

    // PLATFORM SPECIFIC PATH AND NAME
#ifdef Q_OS_LINUX
    name = "lib/libR.so";
    // look in the standard locations for debian/ubuntu or fedora if not set
    if (home == "") {
        home = "/usr/lib/R";

        // look in the usual place for debian/ubuntu, if not try fedora
        QString path = QString("%1/%2").arg(home).arg(name);
        if (!QFile(path).exists())  home = "/usr/lib64/R";
    }
#endif
#ifdef WIN32
    if (home == "") {
        QSettings reg("HKEY_LOCAL_MACHINE\\SOFTWARE\\R-core\\R", QSettings::NativeFormat);
        home = reg.value("InstallPath", "").toString();
    }
#if defined(_M_X64) || defined (WIN64)
    name = QString("bin/x64/R.dll");
#else
    name = QString("bin/i386/R.dll");
#endif
#endif
#ifdef Q_OS_MAC
    if (home == "") home= "/Library/Frameworks/R.framework/Resources";
    name += "lib/libR.dylib";
#endif

    loaded = false;

    // load if its found
    QString full = QString("%1/%2").arg(home).arg(name);
    if (QFile(full).exists()) {

        errors << tr("The R library was found at '%1'\n").arg(full);

        // we need to make sure the dependants are loaded so update
        // LD_LIBRARY_PATH or PATH so they can be found - this only
        // affects us, it is not retained outside our process scope.
        QString search = getenv(gcSearchPath);
        QString dir = QFileInfo(QFile(full)).absolutePath();
        if (search != "") search = dir + gcSearchSep + search;
        else search = dir;
        setenv(gcSearchPath, search.toLatin1().constData(), true);

        //qDebug()<<"setenv"<<gcSearchPath<<search;

        // Now load the library
        libR = new QLibrary(full);
        loaded = libR->load();

        // snaffle away errors
        errors <<libR->errorString();
    } else {
        errors << tr("We failed to find the R shared libraries.");
    }

    if (!loaded) {
        errors <<tr("The dynamic library load failed.");
        return loaded;
    }

    // ok, now its loaded we need to set all the function pointers
    ptr_GC_Rf_initEmbeddedR = Prot_GC_Rf_initEmbeddedR(resolve("Rf_initEmbeddedR"));
    ptr_GC_R_dot_Last = Prot_GC_R_dot_Last(resolve("R_dot_Last"));
    ptr_GC_R_CheckUserInterrupt = Prot_GC_R_CheckUserInterrupt(resolve("R_CheckUserInterrupt"));
    ptr_GC_R_RunExitFinalizers = Prot_GC_R_RunExitFinalizers(resolve("R_RunExitFinalizers"));
    ptr_GC_R_CleanTempDir = Prot_GC_R_CleanTempDir(resolve("R_CleanTempDir"));
    ptr_GC_Rf_endEmbeddedR = Prot_GC_Rf_endEmbeddedR(resolve("Rf_endEmbeddedR"));
    ptr_GC_R_ReplDLLinit = Prot_GC_R_ReplDLLinit(resolve("R_ReplDLLinit"));
    ptr_GC_R_DefParams = Prot_GC_R_DefParams(resolve("R_DefParams"));
    ptr_GC_R_SetParams = Prot_GC_R_SetParams(resolve("R_SetParams"));
    ptr_GC_R_tryEval = Prot_GC_R_tryEval(resolve("R_tryEval"));
    ptr_GC_Rf_error = Prot_GC_Rf_error(resolve("Rf_error"));
    ptr_GC_Rf_warning = Prot_GC_Rf_warning(resolve("Rf_warning"));
    ptr_GC_Rf_PrintValue = Prot_GC_Rf_PrintValue(resolve("Rf_PrintValue"));
    ptr_GC_R_getEmbeddingDllInfo = Prot_GC_R_getEmbeddingDllInfo(resolve("R_getEmbeddingDllInfo"));
    ptr_GC_R_registerRoutines = Prot_GC_R_registerRoutines(resolve("R_registerRoutines"));

    // R data
    ptr_GC_Rf_allocVector = Prot_GC_Rf_allocVector(resolve("Rf_allocVector"));
    ptr_GC_Rf_allocList = Prot_GC_Rf_allocList(resolve("Rf_allocList"));
    ptr_GC_Rf_unprotect = Prot_GC_Rf_unprotect(resolve("Rf_unprotect"));
    ptr_GC_Rf_protect = Prot_GC_Rf_protect(resolve("Rf_protect"));
    ptr_GC_SETCAR = Prot_GC_SETCAR(resolve("SETCAR"));
    ptr_GC_SET_TYPEOF = Prot_GC_SET_TYPEOF(resolve("SET_TYPEOF"));
    ptr_GC_CDR = Prot_GC_CDR(resolve("CDR"));
    ptr_GC_SET_STRING_ELT = Prot_GC_SET_STRING_ELT(resolve("SET_STRING_ELT"));
    ptr_GC_SET_VECTOR_ELT = Prot_GC_SET_VECTOR_ELT(resolve("SET_VECTOR_ELT"));
    ptr_GC_VECTOR_ELT = Prot_GC_VECTOR_ELT(resolve("VECTOR_ELT"));
    ptr_GC_Rf_mkChar = Prot_GC_Rf_mkChar(resolve("Rf_mkChar"));
    ptr_GC_Rf_mkString = Prot_GC_Rf_mkString(resolve("Rf_mkString"));
    ptr_GC_Rf_namesgets = Prot_GC_Rf_namesgets(resolve("Rf_namesgets"));
    ptr_GC_Rf_classgets = Prot_GC_Rf_classgets(resolve("Rf_classgets"));
    ptr_GC_Rf_length = Prot_GC_Rf_length(resolve("Rf_length"));
    ptr_GC_STRING_ELT = Prot_GC_STRING_ELT(resolve("STRING_ELT"));
    ptr_GC_Rf_coerceVector = Prot_GC_Rf_coerceVector(resolve("Rf_coerceVector"));
    ptr_GC_R_ParseVector = Prot_GC_R_ParseVector(resolve("R_ParseVector"));
    ptr_GC_REAL = Prot_GC_REAL(resolve("REAL"));
    ptr_GC_INTEGER = Prot_GC_INTEGER(resolve("INTEGER"));
    ptr_GC_LOGICAL = Prot_GC_LOGICAL(resolve("LOGICAL"));
    ptr_GC_Rf_install = Prot_GC_Rf_install(resolve("Rf_install"));
    ptr_GC_Rf_setAttrib = Prot_GC_Rf_setAttrib(resolve("Rf_setAttrib"));
    ptr_GC_Rf_isNull = Prot_GC_Rf_isNull(resolve("Rf_isNull"));
    ptr_GC_R_CHAR = Prot_GC_R_CHAR(resolve("R_CHAR"));

    // Graphics Device
    ptr_GC_GEcreateDevDesc = Prot_GC_GEcreateDevDesc(resolve("GEcreateDevDesc"));
    ptr_GC_GEaddDevice2 = Prot_GC_GEaddDevice2(resolve("GEaddDevice2"));
    ptr_GC_Rf_onintr = Prot_GC_Rf_onintr(resolve("Rf_onintr"));
    ptr_GC_Rf_selectDevice = Prot_GC_Rf_selectDevice(resolve("Rf_selectDevice"));
    ptr_GC_Rf_ndevNumber = Prot_GC_Rf_ndevNumber(resolve("Rf_ndevNumber"));
    ptr_GC_Rf_killDevice = Prot_GC_Rf_killDevice(resolve("Rf_killDevice"));
    ptr_GC_R_GE_getVersion = Prot_GC_R_GE_getVersion(resolve("R_GE_getVersion"));
    ptr_GC_R_CheckDeviceAvailable = Prot_GC_R_CheckDeviceAvailable(resolve("R_CheckDeviceAvailable"));

    // set the globals
    //loaded = false; // not resolved yet!!

    // lets try and find a symbol (not a function)
    pGC_R_NilValue = (SEXP*)(resolve("R_NilValue"));
    pGC_R_RowNamesSymbol = (SEXP*)(resolve("R_RowNamesSymbol"));
    pGC_R_ClassSymbol = (SEXP*)(resolve("R_ClassSymbol"));
    pGC_R_GlobalEnv = (SEXP*)(resolve("R_GlobalEnv"));
    pGC_R_NaReal = (double*)(resolve("R_NaReal"));
    pGC_R_Consolefile = (FILE**)(resolve("R_Consolefile"));
    pGC_R_Outputfile = (FILE**)(resolve("R_Outputfile"));

    #ifndef WIN32
    ptr_GC_ptr_R_Suicide = Prot_GC_ptr_R_Suicide(resolve("ptr_R_Suicide"));
    ptr_GC_ptr_R_ShowMessage = Prot_GC_ptr_R_ShowMessage(resolve("ptr_R_ShowMessage"));
    ptr_GC_ptr_R_ReadConsole = Prot_GC_ptr_R_ReadConsole(resolve("ptr_R_ReadConsole"));
    ptr_GC_ptr_R_WriteConsole = Prot_GC_ptr_R_WriteConsole(resolve("ptr_R_WriteConsole"));
    ptr_GC_ptr_R_WriteConsoleEx = Prot_GC_ptr_R_WriteConsoleEx(resolve("ptr_R_WriteConsoleEx"));
    ptr_GC_ptr_R_ResetConsole = Prot_GC_ptr_R_ResetConsole(resolve("ptr_R_ResetConsole"));
    ptr_GC_ptr_R_FlushConsole = Prot_GC_ptr_R_FlushConsole(resolve("ptr_R_FlushConsole"));
    ptr_GC_ptr_R_ClearerrConsole = Prot_GC_ptr_R_ClearerrConsole(resolve("ptr_R_ClearerrConsole"));
    ptr_GC_ptr_R_ProcessEvents = Prot_GC_ptr_R_ProcessEvents(resolve("ptr_R_ProcessEvents"));
    ptr_GC_ptr_R_Busy = Prot_GC_ptr_R_Busy(resolve("ptr_R_Busy"));
    pGC_R_interrupts_pending = (int*)(resolve("R_interrupts_pending"));
    #else
    ptr_GC_getDLLVersion = Prot_GC_getDLLVersion(resolve("getDLLVersion"));
    ptr_GC_getRUser = Prot_GC_getRUser(resolve("getRUser"));
    ptr_GC_get_R_HOME = Prot_GC_get_R_HOME(resolve("get_R_HOME"));
    pGC_R_UserBreak = (int*)(resolve("UserBreak"));
    #endif

    // did it work -- resolve sets to false if symbols won't load
    if (loaded && sethome) {
        // we searched for it and found it, so lets set R_HOME to what we found
        // it only affects this process and means users can get away with not
        // configuring where R is installed if its in a standard place.
        // sadly, there is no standard place on windows
        setenv("R_HOME", home.toLatin1().constData(), true);
    }

    return loaded;
}
