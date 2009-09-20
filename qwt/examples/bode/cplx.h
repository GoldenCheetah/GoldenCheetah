#ifndef BODE_CPLX_H
#define BODE_CPLX_H

#include <math.h>


class cplx {
private:
    double re,im;

public:

    double real() {return re;}
    double imag() {return im;}

    cplx() {
        re = 0.0;
        im  = -0.0;
    }

    cplx& operator= (cplx a) {
        re = a.re;
        im = a.im;
        return *this;
    }

    cplx(double r, double i = 0.0) {
        re = r;
        im = i;
    }

    friend cplx operator * (cplx x1, cplx x2);
    friend cplx operator + (cplx x1, cplx x2);
    friend cplx operator - (cplx x1, cplx x2);
    friend cplx operator / (cplx x1, cplx x2);

};

inline cplx operator+(cplx x1, cplx x2)
{
    return cplx(x1.re + x2.re, x1.im + x2.im);
}

inline cplx operator-(cplx x1, cplx x2)
{
    return cplx(x1.re - x2.re, x1.im - x2.im);
}

inline cplx operator*(cplx x1, cplx x2)
{
    return cplx(x1.re * x2.re - x1.im * x2.im,
        x1.re * x2.im + x2.re * x1.im);
}

inline cplx operator/(cplx x1, cplx x2)
{
    double denom = x2.re * x2.re + x2.im * x2.im;
    return cplx( (x1.re * x2.re + x1.im * x2.im) /denom,
            (x1.im * x2.re - x2.im * x1.re) / denom);
}

#endif
