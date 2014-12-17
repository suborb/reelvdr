/*
 * GraphLCD graphics library
 *
 * common.c  -  various functions
 *  
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2004 Andreas Regel <andreas.regel AT powarman.de>
 */

#include "common.h"


namespace GLCD
{

void clip(int & value, int min, int max)
{
	if (value < min)
		value = min;
	if (value > max)
		value = max;
}

void sort(int & value1, int & value2)
{
	if (value2 < value1)
	{
		int tmp;
		tmp = value2;
		value2 = value1;
		value1 = tmp;
	}
}

} // end of namespace
