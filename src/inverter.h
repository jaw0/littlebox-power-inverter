/*
  Copyright (c) 2014
  Author: Jeff Weisberg <jaw @ tcp4me.com>
  Created: 2014-Aug-16 00:32 (EDT)
  Function: 

*/

#ifndef __inverter_h__
#define __inverter_h__

#define CYCLESTEPS	(CONTROLFREQ/GPI_OUTHZ)
#define HALFCYCLESTEPS	(CYCLESTEPS/2)


enum InvState {
    INV_STATE_OFF,
    INV_STATE_ONDELAY,		// switch was turned on, waiting > 5 sec (per spec)
    INV_STATE_ONDELAY2,		// waiting for zero-xing to start
    INV_STATE_RUNNING,
    INV_STATE_SHUTTINGDOWN,	// switch was turned off, ramping down
    INV_STATE_FAULTED,		// fault detected
    INV_STATE_FAULTINGDOWN,	// ramping down
    INV_STATE_FAULTEDOFF,	// power removed
    INV_STATE_DIAG,		// diagnostic menu
    INV_STATE_TEST		// control taken over by test code
};

enum BoostMode {
    BOOST_UNSYNCH,
    BOOST_SYNCH,	// synchronous rectification
};

//extern enum InvState inv_state;

extern int cycle_step, half_cycle_step;
extern utime_t curr_t0, prev_t0;

extern struct StatAve 		s_vi, s_pi;
extern struct StatAveMinMax 	s_vo, s_ii, s_io, s_vh, s_po;
extern int curr_vo, curr_ic;

extern int   output_err, prev_output_err, input_err, prev_input_err, itarg_err, prev_itarg_err;
extern int   output_adj, prev_output_adj;
extern int   itarg_adj, prev_itarg_adj, input_adj, prev_input_adj;

extern int output_vtarg, input_itarg;
extern int pwm_hbridge, pwm_boost;

extern void set_boost_pwm(int, int);
extern int  set_hbridge_pwm(int);

#endif /* __inverter_h__ */
