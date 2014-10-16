

// needs to be: 60 * 7 * ( 2^N * 5^M )
// this value also works for 50Hz
#define CONTROLFREQ	21000
// also possible: 16800, 26880, 33600


#define VOLTSHIFT	4	// volts are Q.4
#define AMPSHIFT	8	// amperes are Q.8

#define Q_VOLTS(x)	((x) << VOLTSHIFT)
#define Q_AMPS(x)	((x) << AMPSHIFT)


#define VDIODE		1	// generically
#define Q_V_DIODEOUT	18	// output voltage drop


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

#define MIN_VI_SOFT	300	// from spec

// boost stage shuts down if Vh gets this high
// this should be set higher than MAX_VH_HARD
#define STOP_HARD_VH	1000

#else
// testing
#define	GPI_OUTHZ	60
#define	GPI_OUTV	24
#define VOLTSINIT    	12

#define VH_TARGET	30
#define MAX_VH_HARD	72
#define MAX_VH_SOFT	60
#define MIN_VH_SOFT	32
#define MIN_VH_HARD	28

#define MIN_VI_SOFT	6

#define STOP_HARD_VH	100

#endif

// control params
//  output voltage
#define KOP	1035
#define KOI	354

//  input current
#define KIP	144
#define KII	29

