
#include <math.h>

double round(double num, int percision)
{

	int val = num * ( (10 * percision) + 10 );
	val+= 5;
	
	return (double ) (val / ( (10 * percision) + 10 ));

}
