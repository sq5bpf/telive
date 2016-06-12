//#include <sys/time.h>
//#include <time.h>

#include "telive_util.h"

/* Subtract the struct timevalvalues X and Y, return result in miliseconds
 *
 * This function is shamelessly ripped from the glibc manual
 */

long timeval_subtract_ms (struct timeval *x, struct timeval *y)
{
	struct timeval result;
	/* Perform the carry for the later subtraction by updating y. */
	if (x->tv_usec < y->tv_usec) { 
		int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
		y->tv_usec -= 1000000 * nsec;
		y->tv_sec += nsec;
	}
	if (x->tv_usec - y->tv_usec > 1000000) {
		int nsec = (x->tv_usec - y->tv_usec) / 1000000;
		y->tv_usec += 1000000 * nsec;
		y->tv_sec -= nsec;
	}

	/* Compute the time remaining to wait.
	 * tv_usec is certainly positive. */
	result.tv_sec = x->tv_sec - y->tv_sec;
	result.tv_usec = x->tv_usec - y->tv_usec;

	return((result.tv_usec/1000)+(result.tv_sec*1000));
}



