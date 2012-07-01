/* nolintest.f -- translated by f2c (version 19951021).
*/

#include "f2c.h"

/* *********************************************************************** */
/*     Main program with nonlinear test */
/* *********************************************************************** */
/* Main program */ MAIN__()
{
    /* Builtin functions */
    /* Subroutine */ int s_copy();

    /* Local variables */
    extern /* Subroutine */ int pprn1_();
    static integer nolin;
    extern /* Subroutine */ int objfun_();
    static integer iestat;
    static char strdat[80];

    s_copy(strdat, "ntest.dat", 80L, 9L);
    nolin = 1;
    iestat = 0;
    pprn1_(&nolin, strdat, &iestat, objfun_, 80L);
/*      write(*,*) 'exit=',iestat */
} /* MAIN__ */

/* *********************************************************************** */
/*     Objective function */
/* *********************************************************************** */
/* Subroutine */ int objfun_(nar, nat, x, f, g)
integer *nar, *nat;
doublereal *x, *f, *g;
{
    /* System generated locals */
    integer x_dim1, x_offset, g_dim1, g_offset, i__1, i__2;

    /* Builtin functions */
    double sqrt();

    /* Local variables */
    static integer j, k;
    static doublereal r__, f1, f2, f3, r2, r3, r4, xx, xx2, u_u__, sqr, xxn, 
	    xnx;


/*     constants per divisions */
/*     variables locals */
    /* Parameter adjustments */
    g_dim1 = *nar + 1;
    g_offset = g_dim1;
    g -= g_offset;
    x_dim1 = *nar + 1;
    x_offset = x_dim1;
    x -= x_offset;

    /* Function Body */
    *f = 0.;
    i__1 = *nat;
    for (k = 1; k <= i__1; ++k) {
	i__2 = *nar;
	for (j = 0; j <= i__2; ++j) {
	    g[j + k * g_dim1] = 0.;
	}
    }

    i__1 = *nat;
    for (k = 1; k <= i__1; ++k) {
/*     inicialitzacions per cada article */
	f1 = 0.;
	f2 = 0.;
	f3 = 0.;
	u_u__ = 1.;
	xx = x[k * x_dim1 + 1];
/*     bucle per article k (arc 1...n-1) */
	i__2 = *nar - 1;
	for (j = 1; j <= i__2; ++j) {
	    xxn = x[j + 1 + k * x_dim1];
	    xx2 = xx * xx;
	    xnx = xx - xxn;
	    u_u__ = -u_u__;
	    f1 += xx2;
	    sqr = sqrt(xx2 + 1. + xnx * xnx);
	    f2 += sqr;
	    f3 += u_u__ * xx;
	    g[j + k * g_dim1] += xx * 2. / 1e3;
/* part f1 */
	    r__ = sqr * 2. * 1e3;
	    g[j + k * g_dim1] += (xx + xnx) * 2. / r__;
/* part f2 */
	    g[j + 1 + k * g_dim1] -= xnx * 2. / r__;
/* part f2 */
	    xx = xxn;
	}
/*     tractament ultim arc */
	u_u__ = -u_u__;
	f1 += xx * xx;
	f3 += u_u__ * xx;
	g[*nar + k * g_dim1] += xx * 2. / 1e3;
/*     valor funobj */
/* part f1 */
	r__ = f3 + 10.;
	r2 = r__ * r__;
	r4 = r2 * r2;
	*f = *f + f1 / 1e3 + f2 / 1e3 + r4 / 1.2e6;
/*     gradient part f3 */
	r3 = r2 * r__;
	u_u__ = 1.;
	i__2 = *nar;
	for (j = 1; j <= i__2; ++j) {
	    u_u__ = -u_u__;
	    g[j + k * g_dim1] += r3 * 4. * u_u__ / 1.2e6;
	}
    }
} /* objfun_ */

