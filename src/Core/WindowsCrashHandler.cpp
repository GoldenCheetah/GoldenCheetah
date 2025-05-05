/*
 * Copyright (c) 2017 Stefan Schake
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


// Only for Windows 64 Bit and MSVC
#if defined(_MSC_VER) && defined(_WIN64)

#include <fstream>
#include <cstdint>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <string>
#include <sstream>

#define NOMINMAX // prevents windows.h defining max & min macros
#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")

static std::string installation_crash_path = "";

static std::string getCrashFileName()
{
    using namespace std::chrono;
    auto now = system_clock::now();
    auto nowTimeT = system_clock::to_time_t(now);
    std::stringstream ret;
    if (installation_crash_path.length() > 0 ) {
      ret << installation_crash_path << "\\";
    }
    ret << "crash_" << std::put_time(std::localtime(&nowTimeT), "%H%M_%d%m%y");
    return ret.str();
}

static void setCrashFilePath(std::string file_path)
{
   installation_crash_path = file_path;
}

static LONG WINAPI ExceptionHandler(LPEXCEPTION_POINTERS exception)
{
    auto crashFileName = getCrashFileName();
    HANDLE process = GetCurrentProcess();

    auto dumpFileName = crashFileName + ".dmp";
    HANDLE dumpFile = CreateFileA(dumpFileName.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (dumpFile != INVALID_HANDLE_VALUE)
    {
        MINIDUMP_EXCEPTION_INFORMATION minidumpException = {0};
        minidumpException.ThreadId = GetCurrentThreadId();
        minidumpException.ExceptionPointers = exception;
        minidumpException.ClientPointers = false;

        MiniDumpWriteDump(process, GetCurrentProcessId(), dumpFile,
                          (MINIDUMP_TYPE)(MiniDumpNormal | MiniDumpFilterMemory | MiniDumpFilterModulePaths | MiniDumpWithoutOptionalData),
                          &minidumpException, NULL, NULL);

        FlushFileBuffers(dumpFile);
        CloseHandle(dumpFile);
    }

    SymInitialize(process, NULL, TRUE);
    SymSetOptions(SYMOPT_LOAD_LINES);

    PCONTEXT context = exception->ContextRecord;
    STACKFRAME64 curStackFrame = {0};
    curStackFrame.AddrPC.Offset = context->Rip;
    curStackFrame.AddrPC.Mode = AddrModeFlat;
    curStackFrame.AddrFrame.Offset = context->Rbp;
    curStackFrame.AddrFrame.Mode = AddrModeFlat;
    curStackFrame.AddrStack.Offset = context->Rsp;
    curStackFrame.AddrStack.Mode = AddrModeFlat;

    auto crashLogName = crashFileName + ".log";
    std::ofstream logFile(crashLogName, std::ofstream::trunc);
    logFile << "Unhandled exception 0x" << std::hex << exception->ExceptionRecord->ExceptionCode << std::dec << std::endl;
    logFile << "RAX=" << std::hex << context->Rax << " RCX=" << context->Rcx << " RDX=" << context->Rdx << " RBX=" << context->Rbx << std::endl;
    logFile << "RSP=" << context->Rsp << " RBP=" << context->Rbp << " RSI=" << context->Rsi << " RDI=" << context->Rdi << std::endl;
    logFile << "R8=" << context->R8 << " R9=" << context->R9 << " R10=" << context->R10 << " R11=" << context->R11 << std::endl;
    logFile << "R12=" << context->R12 << " R13=" << context->R13 << " R14=" << context->R14 << " R15=" << context->R15 << std::dec << std::endl;
    logFile << std::endl;

    while (StackWalk64(IMAGE_FILE_MACHINE_AMD64, process, GetCurrentThread(),
                       &curStackFrame, context, NULL, SymFunctionTableAccess, SymGetModuleBase, NULL))
    {
        DWORD64 module = SymGetModuleBase64(process, curStackFrame.AddrPC.Offset);
        char moduleBuffer[MAX_PATH] = {0};
        const char* moduleName = "[unknown module]";
        int fullLength = 0;
        if (module != 0 && (fullLength = GetModuleFileNameA((HMODULE) module, moduleBuffer, MAX_PATH)) != 0) {
            moduleName = moduleBuffer;
            // we're only interested in the name, not the full path
            const char* shortName = strrchr(moduleName, '\\');
            if (shortName != NULL && shortName < (moduleBuffer+fullLength-1))
                moduleName = shortName + 1;
        }

        char symbolBuffer[sizeof(SYMBOL_INFO) + 512];
        PSYMBOL_INFO symbol = (PSYMBOL_INFO) symbolBuffer;
        DWORD64 symbolOffset = 0;
        const char* symbolName = NULL;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = sizeof(symbolBuffer) - sizeof(SYMBOL_INFO);
        if (SymFromAddr(process, curStackFrame.AddrPC.Offset, NULL, symbol)) {
            symbolName = symbol->Name;
            symbolOffset = curStackFrame.AddrPC.Offset - symbol->Address;
        }

        IMAGEHLP_LINE64 line;
        line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
        const char* fileName = NULL;
        int lineNumber = 0;
        DWORD lineOffset = 0;
        if (SymGetLineFromAddr64(process, curStackFrame.AddrPC.Offset, &lineOffset, &line))
        {
            fileName = line.FileName;
            lineNumber = line.LineNumber;
            // only include text after "\src\" if applicable
            const char* shortFileName = strstr(fileName, "\\src\\");
            if (shortFileName != NULL)
                fileName = shortFileName+5;
        }

        if (symbolName == NULL)
            logFile << moduleName << "@0x" << std::hex << module << "!+0x" << (curStackFrame.AddrPC.Offset-module) << std::dec;
        else
            logFile << moduleName << "!" << symbolName << "+0x" << std::hex << symbolOffset << std::dec;

        if (fileName != NULL)
            logFile << " " << fileName << ":" << lineNumber;
        logFile << std::endl;
    }

    logFile.close();
    SymCleanup(process);
    return EXCEPTION_EXECUTE_HANDLER;
}

class WindowsCrashHandler
{
public:
    WindowsCrashHandler() {
        SetUnhandledExceptionFilter(ExceptionHandler);
    }

    ~WindowsCrashHandler() {
        SetUnhandledExceptionFilter(NULL);
    }
};

// static object constructors don't run super early, but still before main
static WindowsCrashHandler __windowsCrashHandler;

#endif
