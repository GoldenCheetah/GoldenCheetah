/*
 * Copyright (c) 2016 Mark Liversedge (liversedge@gmail.com)
 *
 * Additionally, for the original source used as a basis for this (RInside.cpp)
 * Released under the same GNU public license.
 *
 * Copyright (C) 2009         Dirk Eddelbuettel
 * Copyright (C) 2010 - 2012  Dirk Eddelbuettel and Romain Francois
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

#include "REmbed.h"
#include "RTool.h"
#include "Settings.h"
#include <stdexcept>

#include <QMessageBox>

static const char *name = "GoldenCheetah";

// no setenv on windows
#if WIN32
int setenv(QString name, QString value, bool overwrite)
{
    int errcode = 0;
    if(!overwrite) {
        size_t envsize = 0;
        errcode = getenv_s(&envsize, NULL, 0, name.toLatin1().constData());
        if(errcode || envsize) return errcode;
    }

    // make the update
    return _putenv_s(name.toLatin1().constData(), value.toLatin1().constData());
}
#endif

REmbed::~REmbed()
{
    R_dot_Last();
    R_RunExitFinalizers();
    R_CleanTempDir();
    Rf_endEmbeddedR(0);
}

// Windows API defines Read and Write Console macros
// You think they'd realise by now that generic terms
// should be avoided. But no, blissfully unaware that
// there are millions of lines of code out there they
// can wantanly break. Fuckwits.
#ifdef WIN32
#ifdef ReadConsole
#undef ReadConsole
#undef WriteConsole
#endif
#endif

REmbed::REmbed(const bool verbose, const bool interactive) : verbose(verbose), interactive(interactive)
{
    loaded = false;

    // need to load the library
    RLibrary rlib;
    if (!rlib.load()) {
        QMessageBox msg(QMessageBox::Critical,
                    "Failed to load R library",
                    rlib.errors.join("\n"));
        msg.exec();
        return;
    }

    // we need to tell embedded R where to work
    QString envR_HOME(getenv("R_HOME"));
    QString configR_HOME = appsettings->value(NULL,GC_R_HOME,"").toString();
    if (envR_HOME == "") {
        if (configR_HOME == "") {
            qDebug()<<"R HOME not set, R disabled";
            return;
        } else {
            setenv("R_HOME", configR_HOME.toLatin1().constData(), true);
        }
    }
    // fire up R
    const char *R_argv[] = {name, "--gui=none", "--no-save",
                            "--no-readline", "--silent", "--vanilla", "--slave"};
    int R_argc = sizeof(R_argv) / sizeof(R_argv[0]);
    Rf_initEmbeddedR(R_argc, (char**)R_argv);
    R_ReplDLLinit();                    // this is to populate the repl console buffers

    structRstart Rst;
    R_DefParams(&Rst);
#ifdef WIN32
    Rst.rhome = getenv("R_HOME");
    Rst.home = getRUser();
    Rst.CharacterMode = LinkDLL;
    Rst.ReadConsole = &RTool::R_ReadConsoleWin;
    Rst.WriteConsole = &RTool::R_WriteConsole;
    Rst.WriteConsoleEx = &RTool::R_WriteConsoleEx;
    Rst.CallBack = &RTool::R_Callback;
    Rst.ShowMessage = &RTool::R_ShowMessage;
    Rst.YesNoCancel = &RTool::R_YesNoCancel;
    Rst.Busy = &RTool::R_Busy;
#endif
    Rst.R_Interactive = (Rboolean) interactive;       // sets interactive() to eval to false
    R_SetParams(&Rst);
    loaded = true;
}

// this is a non-throwing version returning an error code
int REmbed::parseEval(QString line, SEXP & ans) {
    ParseStatus status;
    SEXP cmdSexp, cmdexpr = R_NilValue;
    int i, errorOccurred;

    program << line;

    PROTECT(cmdSexp = Rf_allocVector(STRSXP, 1));
    SET_STRING_ELT(cmdSexp, 0, Rf_mkChar(program.join(" ").toStdString().c_str()));

    cmdexpr = PROTECT(R_ParseVector(cmdSexp, -1, &status, R_NilValue));

    switch (status){
    case PARSE_OK:
        // Loop is needed here as EXPSEXP might be of length > 1
        for(i = 0; i < Rf_length(cmdexpr); i++){
            ans = R_tryEval(VECTOR_ELT(cmdexpr, i), R_GlobalEnv, &errorOccurred);
            if (errorOccurred) {
                if (verbose) Rf_warning("%s: Error in evaluating R code (%d)\n", name, status);
                UNPROTECT(2);
                program.clear();
                return 1;
            }
            if (verbose) {
                Rf_PrintValue(ans);
            }
        }
        program.clear();
        break;
    case PARSE_INCOMPLETE:
        // need to read another line
        break;
    case PARSE_NULL:
        if (verbose) Rf_warning("%s: ParseStatus is null (%d)\n", name, status);
        UNPROTECT(2);
        program.clear();
        return 1;
        break;
    case PARSE_ERROR:
        if (verbose) Rf_error("Parse Error: \"%s\"\n", line.toStdString().c_str());
        UNPROTECT(2);
        program.clear();
        return 1;
        break;
    case PARSE_EOF:
        if (verbose) Rf_warning("%s: ParseStatus is eof (%d)\n", name, status);
        break;
    default:
        if (verbose) Rf_warning("%s: ParseStatus is not documented %d\n", name, status);
        UNPROTECT(2);
        program.clear();
        return 1;
        break;
    }
    UNPROTECT(2);
    return 0;
}

void REmbed::parseEvalQ(QString line) {
    SEXP ans;
    int rc = parseEval(line, ans);
    if (rc != 0) {
        throw std::runtime_error(std::string("Error evaluating: ") + line.toStdString());
    }
}

void REmbed::parseEvalQNT(QString line) {
    SEXP ans;
    parseEval(line, ans);
}

SEXP REmbed::parseEval(QString line) {
    SEXP ans;
    int rc = parseEval(line, ans);
    if (rc != 0) {
        throw std::runtime_error(std::string("Error evaluating: ") + line.toStdString());
    }
    return ans;
}

SEXP REmbed::parseEvalNT(QString line) {
    SEXP ans;
    parseEval(line, ans);
    return ans;
}
