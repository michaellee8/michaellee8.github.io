/******************************************************************
 * CS61C                       CACHE LAB                          *
 * Using this program, on as many different kinds of computers as *
 * possible, investigate these cache parameters:                  *
 *   -- total cache size                                          *
 *   -- cache width                                               *
 *   -- cache replacement policy                                  *
 *                                                                *
 * Make sure you compile this code without optimizations on       *
 * otherwise, the overhead calculations will not be correct       *
 *                                                                *
 * ChangeLog:                                                     *
 *   2014.04.10:
 *   HS: Use clock_gettime() in Linux for time measurement
 *       Add support to run on OSX
 *   2004.03.20:                                                  *
 *   Heavily rewritten by Jeremy Huddleston <jeremyhu@cory>       *
 *     Changed CACHE* names to ARRAY* names                       *
 *     Using (1 << X) notation for sizes of 2^X                   *            
 *     #ifndef around CLK_TCK since it's set by POSIX headers     *
 *     broke up main() into sub functions                         *
 *     do everything except output using clock ticks              *
 ******************************************************************/

#include <stdio.h>
#include <sys/times.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

/* Note that (1 << X) is 2^X ... the MIN/MAX macros below MUST be powers of 2. */
#define ARRAY_MIN (1 << 8)  /* smallest cache (in words) */
#define ARRAY_MAX (1 << 21)  /* largest cache */
#define STRIDE_MIN (1 << 0)  /* smallest stride (in words) */
#define STRIDE_MAX (1 << 16) /* largest stride */
#define SAMPLE 50000000      /* Approximately how many accesses to the array do we want
                              * to make in our analysis.  Larger generates better
                              * statistics.  Adjust for speed on your system.  This
                              * number is by no means exact.  We always do at-least this
                              */

#ifdef __MACH__ // OS X does not have clock_gettime, use clock_get_time
#define SET_TIMER(ts) {				\
clock_serv_t cclock;\
mach_timespec_t mts;\
host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);\
clock_get_time(cclock, &mts);\
mach_port_deallocate(mach_task_self(), cclock);\
ts.tv_sec = mts.tv_sec;\
ts.tv_nsec = mts.tv_nsec;\
  }
#else
#define SET_TIMER(ts) clock_gettime(CLOCK_MONOTONIC_RAW, &ts)
#endif


/* This macro creates a function which iterates through the passed array of size asize striding
 * by stride.  We return the time (in ns) it takes to perform the operation.
 */

#define ARRAY_ACCESS_FUN(func_name, vars, operation) \
static unsigned func_name(unsigned array[], unsigned asize, unsigned stride, unsigned steps) { \
	vars; \
	unsigned register tmp;\
	unsigned register i, j; \
	struct timespec ts_start,ts_stop;\
\
	/* Initialize the time count */ \
	SET_TIMER(ts_start);\
\
	/* Iterate through our array */ \
	for(j=0; j < steps; j++) { \
		for(i=0; i < asize; i += stride) { \
			tmp++;\
			operation; \
		} \
	} \
\
	/* Calculate the final time */ \
	SET_TIMER(ts_stop);\
	return (ts_stop.tv_sec - ts_start.tv_sec)*1e9 + (ts_stop.tv_nsec - ts_start.tv_nsec);\
}

/* Make some functions using that macro */
ARRAY_ACCESS_FUN(rwArray, , array[i] = array[i])
ARRAY_ACCESS_FUN(rwArrayOverhead, , )

/* array going to stride through */
unsigned array[ARRAY_MAX];

/* Global swtich on output format */
int s_csvOut = 0;

/* And... begin... */
int main (int argc, char** argv) {
	int asize, stride, steps, accesses;
	unsigned rwtime;
	unsigned actual,overhead;
	double rwsec;

	if (argc > 1) {
		if (argv[1][0] == '-' && argv[1][1] == 'c') {
			s_csvOut = 1;
		}
	}

	if (s_csvOut) {
		printf(",");
		for (stride = STRIDE_MIN; stride <= STRIDE_MAX ; stride = stride << 1) {
			printf("%lu,",stride * sizeof(int));
		}
		printf("\n");
	}

	for (asize = ARRAY_MIN; asize <= ARRAY_MAX; asize = asize << 1) {
		if (s_csvOut) {
			printf("%lu,",asize * sizeof(int));
		} else {
			/* Print an extra \n here to separate the groups with the same array size */
			printf("\n");
		}
		for (stride = STRIDE_MIN; 
		     stride <= STRIDE_MAX && stride <= asize /2 ; stride = stride << 1) {
			/* How many total accesses do we do on each iteration */
			accesses = asize / stride;
			steps = 1 + ( SAMPLE / accesses );

			/**************** NOW READ/WRITE ****************/

			rwtime = 0;
			/* Do it once to initialize the cache before we start timing */
			rwArray(array, asize, stride, 1);

			/* We want to average the overhead too... */
			overhead = rwArrayOverhead(array, asize, stride, steps);
			actual = rwArray(array, asize, stride, steps);
			rwtime = (actual > overhead)? (actual - overhead): 0;

			/* Calculate our time */
			rwsec = (double) rwtime / ((double) steps * accesses);

			/**************** PRINT RESULTS ****************/

			if (s_csvOut) { 
				printf("%6.3f,", rwsec);
				fflush(stdout);
			} else {
				printf("Size (bytes): %7lu Stride (bytes): %4lu read+write: %6.3f ns\n",
				       asize * sizeof (int),
				       stride * sizeof (int),
				       rwsec);
			}
		}
		if (s_csvOut) { 
			printf("\n");
		}
	}
}
