

// needs to be: 60 * 7 * ( 2^N * 5^M )
// this value also works for 50Hz
#define CONTROLFREQ	21000
// also possible: 16800, 26880, 33600


#define VOLTSHIFT	4	// volts are Q.4
#define AMPSHIFT	8	// amperes are Q.8

#define Q_VOLTS(x)	((x) << VOLTSHIFT)
#define Q_AMPS(x)	((x) << AMPSHIFT)

#if 0
#define	GPI_OUTHZ	60
#define	GPI_OUTV	340	// 240Vrms = 240*sqrt(2)
#define VOLTSINIT    	835

// high enough, so that discontinuous mode is one-sided
// at light loads, high duty cycles
#define VH_TARGET	700
#define MAX_VH_HARD	950
#define MAX_VH_SOFT	900
#define MIN_VH_SOFT	500

// boost stage shuts down if Vh gets this high
// this should be set higher than MAX_VH_HARD
#define STOP_HARD_VH	1000

#else
// testing
#define	GPI_OUTHZ	60
#define	GPI_OUTV	16
#define VOLTSINIT    	12

#define VH_TARGET	30
#define MAX_VH_HARD	48
#define MAX_VH_SOFT	40
#define MIN_VH_SOFT	24
#define MIN_VH_HARD	18

#define STOP_HARD_VH	100

#endif

// control params
#define KO1	(8*339.41)
#define KO2	(8*287.81)
#define KO3	0.96184

#define KI1	27
#define KI2	20
#define KI3	233

#define KT1	27
#define KT2	20
#define KT3	233

