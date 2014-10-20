

// needs to be: 60 * 7 * ( 2^N * 5^M )
// this value also works for 50Hz
#define CONTROLFREQ	21000
// also possible: 16800, 26880, 33600

// R = 2 L F / 0.15 - margin
#define R_DCM		384	// Ohms. DCM-CCM point

#define VOLTSHIFT	4	// volts are Q.4
#define AMPSHIFT	8	// amperes are Q.8

#define Q_VOLTS(x)	((x) << VOLTSHIFT)
#define Q_AMPS(x)	((x) << AMPSHIFT)


#define VDIODE		1	// generically
#define Q_V_DIODEOUT	18	// output voltage drop
#define Q_V_ELEVATOR	100	// VH must be at least this much more than VO


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
#define	GPI_OUTV	48

#define MAX_VH_HARD	100
#define MAX_VH_SOFT	90
#define MIN_VH_SOFT	60
#define MIN_VH_HARD	50

#define MIN_VI_SOFT	6

#define STOP_HARD_VH	200

#endif


#define P_DCM		(GPI_OUTV * GPI_OUTV / R_DCM / 2)

// control params
//  output voltage
#define KOP	1035
#define KOI	354

//  input current
#define KIP	144
#define KII	29

