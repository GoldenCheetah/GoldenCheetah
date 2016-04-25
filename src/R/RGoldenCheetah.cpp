// R API is C
extern "C" {

#include <R.h>
#include <Rinternals.h>
#include <R_ext/Rdynload.h>

// an array of pointers to the actual functions
// in the native GC code -- these are initialised
// once the shared library has been loaded via
// a call to initialiseFunctions()
//
// for this test we only have one function
// GC.display() to make sure the concept works
static SEXP (*fn[1])();

// if we haven't been initialised don't even try
// to dereference the function pointers !!
static bool initialised = false;

// not it is static and will call GC once the symbol table
// has been initialised.
SEXP GCdisplay()
{
    //fprintf(stderr, "GC.display() called\n");

    if (initialised) return fn[0]();
    else return NULL;
}

SEXP GCinitialiseFunctions(SEXP (*functions[1])())
{
    //fprintf(stderr, "RGoldenCheetah initialise functions\n");

    // initialise all the function pointers
    for(int i=0; i<1; i++) fn[i] = functions[i];
    initialised = true;

    return SEXP(0);
}

// this function is called by R when the dynamic library
// is loaded via the R command > dyn.load("RGoldenCheetah.so")
// and is expected to register all the C functions to make
// available via the C interface
//
// By return, a number of functions will be created that provide
// wrappers in R (i.e. they call with the .C(xxx) notation and
//
// We so this to avoid using Rcpp / RInside which are not
// compatible with MSVC
void
R_init_RGoldenCheetah(DllInfo *info)
{
    //fprintf(stderr, "RGoldenCheetah init is called!\n");

    // initialise the function table
    R_CallMethodDef callMethods[]  = {
        { "GCinitialiseFunctions", (DL_FUNC) &GCinitialiseFunctions, 0 },
        { "GC.display", (DL_FUNC) &GCdisplay, 0 },
        { NULL, NULL, 0 }
    };

    // initialise the parameter table
    R_CMethodDef cMethods[] = {
        { "GCinitialiseFunctions", (DL_FUNC) &GCinitialiseFunctions, 0 },
        { "GC.display", (DL_FUNC) &GCdisplay, 0 },
        { NULL, NULL, 0 }
    };

    // set them up
    R_registerRoutines(info, cMethods, NULL, NULL, NULL);

    // make the initialiseFunctions callable from C
    R_RegisterCCallable("RGoldenCheetah", "GCinitialiseFunctions", (DL_FUNC)&GCinitialiseFunctions);
}

}
