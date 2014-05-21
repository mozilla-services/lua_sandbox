/*
* Cephes Math Library Release 2.2: June, 1992
* Copyright 1984, 1987, 1988, 1992 by Stephen L. Moshier
* Direct inquiries to 30 Frost Street, Cambridge, MA 02140
*/

/* extracted the needed function from ntdr.c */

#include <math.h>

const double SQRTH = 0.70710678118654752440; /* sqrt(2)/2 */

double ndtr(double a)
{
  double x, y, z;

  if (isnan(a)) {
    return a;
  }

  x = a * SQRTH;
  z = fabs(x);

  if (z < SQRTH) y = 0.5 + 0.5 * erf(x);

  else {
    y = 0.5 * erfc(z);

    if (x > 0) y = 1.0 - y;
  }

  return (y);
}
