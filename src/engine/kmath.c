/***************************************************************************
 kmath.c Copyright (C) 1999 Christoph Reichenbach


 This program may be modified and copied freely according to the terms of
 the GNU general public license (GPL), as long as the above copyright
 notice and the licensing information contained herein are preserved.

 Please refer to www.gnu.org for licensing details.

 This work is provided AS IS, without warranty of any kind, expressed or
 implied, including but not limited to the warranties of merchantibility,
 noninfringement, and fitness for a specific purpose. The author will not
 be held liable for any damage caused by this work or derivatives of it.

 By using this source code, you agree to the licensing terms as stated
 above.


 Please contact the maintainer for bug reports or inquiries.

 Current Maintainer:

    Christoph Reichenbach (CJR) [jameson@linuxgames.com]

***************************************************************************/

#include <engine.h>


void
kRandom(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	s->acc = PARAM(0) + (int) ((PARAM(1) + 1.0 - PARAM(0)) * (rand() / (RAND_MAX + 1.0)));
}


void
kAbs(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	s->acc = abs(PARAM(0));
}


void
kSqrt(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	s->acc = (gint16) sqrt((float) abs(PARAM(0)));
}


int
get_angle(int xrel, int yrel)
{
	if ((xrel == 0) && (yrel == 0))
		return 0;
	else {
		int val = (int) (180.0/PI * atan2(xrel, -yrel) + 0.5);
		if (val < 0)
			val += 360;
		return val;
	}
}

void
kGetAngle(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	int xrel = PARAM(2) - PARAM(0);
	int yrel = PARAM(3) - PARAM(1);

	s->acc = get_angle(xrel, yrel);
}


void
kGetDistance(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	int xrel = (int) (((float) PARAM(1) - PARAM_OR_ALT(3, 0))/cos(PARAM_OR_ALT(5, 0)* PI / 180.0)); /* This works because cos(0)==1 */
	int yrel = PARAM(0) - PARAM_OR_ALT(2, 0); 

	s->acc = sqrt((float) xrel*xrel + yrel*yrel);
}

void
kTimesSin(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	int angle = PARAM(0);
	int factor = PARAM(1);

	s->acc = (int) (factor * 1.0 * sin(angle * PI / 180.0));
}


void
kTimesCos(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	int angle = PARAM(0);
	int factor = PARAM(1);

	s->acc = (int) (factor * 1.0 * cos(angle * PI / 180.0));
}

void
kCosDiv(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	int angle = PARAM(0);
	int value = PARAM(1);
	double cosval = cos(angle * PI / 180.0);

	if ((cosval < 0.0001) && (cosval > 0.0001)) {
		SCIkwarn(SCIkWARNING,"Attepted division by zero\n");
		s->acc = 0x8000;
	} else
		s->acc = (gint16) (value/cosval);
}

void
kSinDiv(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	int angle = PARAM(0);
	int value = PARAM(1);
	double sinval = sin(angle * PI / 180.0);

	if ((sinval < 0.0001) && (sinval > 0.0001)) {
		SCIkwarn(SCIkWARNING,"Attepted division by zero\n");
		s->acc = 0x8000;
	} else
		s->acc = (gint16) (value/sinval);
}

void
kTimesTan(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	int param = PARAM(0);
	int scale = PARAM_OR_ALT(1, 1);

	param -= 90;
	if ((param % 90) == 0) {
		SCIkwarn(SCIkWARNING, "Attempted tan(pi/2)");
		s->acc = 0x8000;
	} else
		s->acc = (gint16) -(tan(param * PI / 180.0) * scale);
}

void
kTimesCot(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	int param = PARAM(0);
	int scale = PARAM_OR_ALT(1, 1);

	if ((param % 90) == 0) {
		SCIkwarn(SCIkWARNING, "Attempted tan(pi/2)");
		s->acc = 0x8000;
	} else
		s->acc = (gint16) (tan(param * PI / 180.0) * scale);
}
