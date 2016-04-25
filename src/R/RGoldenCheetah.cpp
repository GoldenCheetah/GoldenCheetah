// R API is C
extern "C" {

#include <R.h>
#include <Rinternals.h>
#include <R_ext/Rdynload.h>

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
    fprintf(stderr, "INIT RGOLDENCHEETAH DYNAMIC LIB\n");
}

}
