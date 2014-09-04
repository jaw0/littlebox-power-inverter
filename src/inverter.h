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
    INV_STATE_FAULT,
    INV_STATE_TEST,
};

enum BoostMode {
    BOOST_UNSYNCH,
    BOOST_SYNCH,	// synchronous rectification
};

//extern enum InvState inv_state;

extern int cycle_step, half_cycle_step;
extern utime_t curr_t0, prev_t0;

extern int curr_ii, prev_ii, ccy_ii_sum, pcy_ii_ave, pcy_ii_min, pcy_ii_max, ccy_ii_min, ccy_ii_max;
extern int curr_vi, prev_vi, ccy_vi_sum, pcy_vi_ave;
extern int curr_io, prev_io, ccy_io_sum, pcy_io_ave;
extern int curr_vo, prev_vo, ccy_vo_sum, pcy_vo_ave;
extern int curr_vh, prev_vh, ccy_vh_sum, pcy_vh_ave, pcy_vh_min, pcy_vh_max, ccy_vh_min, ccy_vh_max;
extern int curr_ic;

extern int   output_err, prev_output_err, input_err, prev_input_err;
extern float output_adj, prev_output_adj, input_adj, prev_input_adj;

extern int output_vtarg, input_itarg;

extern void set_boost_pwm(int, int);
extern void set_hbridge_pwm(int);

#endif /* __inverter_h__ */
