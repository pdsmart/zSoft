#include "math.h"
#include "errno.h"

//#define isnan(__x) (__builtin_isnan (__x))
extern int finite(double);

double fmodf(double x, double y)
{
	int ir,iy;
	double r,w;

	if (y == (double)0
#if !defined(vax) && !defined(tahoe)	/* per "fmod" manual entry, SunOS 4.0 */
		|| isnan(y) || !finite(x)
#endif	/* !defined(vax) && !defined(tahoe) */
	    )
	    return (x*y)/(x*y);

	r = fabsf(x);
	y = fabsf(y);
	(void)frexpf(y,&iy);
	while (r >= y) {
		(void)frexpf(r,&ir);
		w = ldexpf(y,ir-iy);
		r -= w <= r ? w : w*(double)0.5;
	}
	return x >= (double)0 ? r : -r;
}
