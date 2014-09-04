

// needs to be: 60 * 7 * ( 2^N * 5^M )
// this value also works for 50Hz
#define CONTROLFREQ	21000
// also possible: 16800, 26880, 33600

#if 0
#define	GPI_OUTHZ	60
#define	GPI_OUTV	340	// 240Vrms = 240*sqrt(2)
#define VOLTSINIT    	835

// high enough, so that discontinuous mode is one-sided
// at light loads, high duty cycles
#define VH_TARGET	(875 <<4)
#define MAX_VH_HARD	(950 <<4)
#define MAX_VH_SOFT	(900 <<4)
#define MIN_VH_SOFT	(500 <<4)

#else
// testing
#define	GPI_OUTHZ	60
#define	GPI_OUTV	12
#define VOLTSINIT    	12

#define VH_TARGET	20
#define MAX_VH_HARD	24
#define MAX_VH_SOFT	20
#define MIN_VH_SOFT	12

#endif

// control params
#define KO1	1
#define KO2	1
#define KO3	1

#define KI1	1
#define KI2	1
#define KI3	1

#define KT1	1
#define KT2	1
#define KT3	1
