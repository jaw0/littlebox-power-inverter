/*
  Copyright (c) 2014
  Author: Jeff Weisberg <jaw @ tcp4me.com>
  Created: 2014-Aug-15 23:52 (EDT)
  Function: power inverter control

*/

#include <conf.h>
#include <proc.h>
#include <gpio.h>
#include <adc.h>
#include <pwm.h>
#include <stm32.h>
#include <userint.h>

#include "util.h"
#include "hw.h"
#include "inverter.h"
#include "gpiconf.h"

#if (CONTROLFREQ == 21000)
#  if (GPI_OUTHZ == 50)
#    include "sin50_350.h"
#  endif
#  if (GPI_OUTHZ == 60)
#    include "sin60_350.h"
#  endif
#endif

#define ONDELAY		5000000		// 5 sec, per spec
#define OFFDELAY	 100000		// .1 sec
#define FAULTSONGT     10000000		// 10 sec


DEFVAR(int, inv_state, INV_STATE_OFF, UV_TYPE_UL, "inverter state")

const char *fault_reason;

static utime_t ondelay_until  = 0;
static utime_t shutdown_until = 0;
utime_t curr_t0 = 0;

int cycle_step = 0;	// 0 .. 349 = 1 60Hz cycle
int half_cycle_step = 0;

// voltages Q.4, currents + power Q.8
int curr_ii, prev_ii, ccy_ii_sum, pcy_ii_ave, ppy_ii_ave, pcy_ii_min, pcy_ii_max, ccy_ii_min, ccy_ii_max;
int curr_vi, prev_vi, ccy_vi_sum, pcy_vi_ave;
int ccy_pi_sum, pcy_pi_ave, ppy_pi_ave;
int curr_io, prev_io, ccy_io_sum, pcy_io_ave;
int curr_vo, prev_vo, ccy_vo_sum, pcy_vo_ave;
int curr_vh, prev_vh, ccy_vh_sum, pcy_vh_ave, ppy_vh_ave, pcy_vh_min, pcy_vh_max, ccy_vh_min, ccy_vh_max;
int curr_ic;

int   output_err, prev_output_err, input_err, prev_input_err, itarg_err, prev_itarg_err;
float output_adj, prev_output_adj, input_adj, prev_input_adj, itarg_adj, prev_itarg_adj;

int output_vtarg, prev_output_vtarg;	// target output voltage
int input_itarg,  prev_input_itarg;	// target input current



static void ready_hbridge(void);

/****************************************************************/

/*
  [2s	- double size

  [14m	- 6x10
  [15m	- 6x12
  [16m	- 9x15
  [17m	- 10x20
*/

static inline void
printbig(const char *msg){
    printf("\e[J\e[15mArea791RL - inverter\n\e[17m%s\n", msg);
    // printf("\e[J\e[15m\e[2s%s\e[1s", msg);
}

static inline void
print_ready(void){
    printbig("READY");
}


static void
update_state(void){
    static utime_t fault_song     = 0;
    static utime_t diag_last      = 0;
    static int8_t  diag_counter   = 0;

    int sw = gpio_get( HW_GPIO_SWITCH );
    blinky_override = 0;

    switch( inv_state ){
    case INV_STATE_OFF:
        set_led_green(0);

        if( sw ){
            ondelay_until = get_time() + ONDELAY;
            inv_state = INV_STATE_ONDELAY;
            blinky_override = 1;
            set_led_white(255);
            play(ivolume, "c+3"); 	// debounce delay
            printbig("STARTING...\n");
            syslog("switch on");
        }
        break;

    case INV_STATE_ONDELAY:

        if( !sw ){
            inv_state = INV_STATE_OFF;
            play(ivolume, "a-3");
            diag_counter ++;
            diag_last = get_time();
            printbig("OFF\n");
            usleep(500000);
            print_ready();
            syslog("canceled");
        }else if( get_time() >= ondelay_until ){
            ready_hbridge();
            set_led_green( 255 );
            inv_state = INV_STATE_ONDELAY2;
            diag_counter = 0;
            play(ivolume, "b");
            printbig("ACTIVE\n");
            syslog("ac active");
        }else{
            // flash + chirp; 2Hz
            blinky_override = 1;
            set_led_green(127);
            set_led_white(0);
            play(ivolume, "b+5>>");
            set_led_green(0);
            set_led_white(127);
            usleep(437500);
        }
        break;

    case INV_STATE_ONDELAY2:
        // will move to running at next zero-xing
        break;

    case INV_STATE_RUNNING:
        set_led_green( 255 );

        if( !sw ){
            shutdown_until = get_time() + OFFDELAY;
            inv_state = INV_STATE_SHUTTINGDOWN;
            play(ivolume, "a-3");
            syslog("switch off");
        }
        break;

    case INV_STATE_SHUTTINGDOWN:
        set_led_green( 63 );

        if( get_time() >= shutdown_until ){
            if( sw ){
                // wild switch flipping
                inv_state    = INV_STATE_FAULT;
                fault_song   = 0;
                fault_reason = "SWITCH ERR";
                syslog("switch fault");
            }else{
                inv_state = INV_STATE_OFF;
                play(ivolume, "f-3");
                printbig("AC OFF\n");
                syslog("ac disabled");
                usleep(500000);
                print_ready();
            }
        }
        break;

    case INV_STATE_FAULT:
        set_led_green(0);
        set_led_red(255);

        if( !sw ){
            inv_state = INV_STATE_OFF;
            play(ivolume, "f-3");
            set_led_red(0);
        }else if( get_time() >= fault_song ){
            printbig(fault_reason);
            play(ivolume, "[4 d+3d-3]");
            fault_song = get_time() + FAULTSONGT;
        }

        break;

    case INV_STATE_TEST:
        // ...
        break;
    }

    if( diag_last && diag_last + 5000000 < get_time() ){
        diag_counter = 0;
        diag_last    = 0;
    }
    if( diag_counter >= 7 ){
        inv_state = INV_STATE_TEST;
        diag_counter = 0;
        printbig("DIAG MODE\n");
        syslog("diag mode");
        play(ivolume, "g4e4f4f-3");
        // ...
    }

}

/****************************************************************/

static void
update_stats(void){
    short sign = (cycle_step > HALFCYCLESTEPS) ? -1 : 0;

    if( !half_cycle_step ) {
        // rotate stats
        ppy_ii_ave = pcy_ii_ave;
        ppy_vh_ave = pcy_vh_ave;
        pcy_pi_ave = pcy_pi_ave;
        pcy_ii_ave = ccy_ii_sum / HALFCYCLESTEPS;
        pcy_vi_ave = ccy_vi_sum / HALFCYCLESTEPS;
        pcy_io_ave = ccy_io_sum / HALFCYCLESTEPS;
        pcy_vo_ave = ccy_vo_sum / HALFCYCLESTEPS;	// ave(abs)
        pcy_vh_ave = ccy_vh_sum / HALFCYCLESTEPS;
        pcy_pi_ave = ccy_pi_sum / HALFCYCLESTEPS;

        pcy_ii_min = ccy_ii_min;
        pcy_ii_max = ccy_ii_max;
        pcy_vh_min = ccy_vh_min;
        pcy_vh_max = ccy_vh_max;

        // clear
        ccy_ii_sum = 0;
        ccy_vh_sum = 0;
        ccy_vi_sum = 0;
        ccy_vo_sum = 0;
        ccy_vi_sum = 0;
        ccy_pi_sum = 0;
    }

    prev_ii = curr_ii; curr_ii = get_curr_ii(); ccy_ii_sum += curr_ii;
    prev_vi = curr_vi; curr_vi = get_curr_vi(); ccy_vi_sum += curr_vi;
    prev_io = curr_io; curr_io = get_curr_io(); ccy_io_sum += curr_io;
    prev_vo = curr_vo; curr_vo = get_curr_vo(); ccy_vo_sum += curr_vo;	if(sign) curr_vo = -curr_vo;
    prev_vh = curr_vh; curr_vh = get_curr_vh(); ccy_vh_sum += curr_vh;
    ccy_pi_sum += (curr_ii * curr_vi) >> 4;	// Q.8
    curr_ic = get_curr_ic();

    if( !half_cycle_step ){
        ccy_ii_min = ccy_ii_max = curr_ii;
        ccy_vh_min = ccy_vh_max = curr_vh;
    }
    if( curr_ii < ccy_ii_min ) ccy_ii_min = curr_ii;
    if( curr_ii > ccy_ii_max ) ccy_ii_max = curr_ii;
    if( curr_vh < ccy_vh_min ) ccy_vh_min = curr_vh;
    if( curr_vh > ccy_vh_max ) ccy_vh_max = curr_vh;

    // RSN - determine power phase
}


/****************************************************************/

static void
precharge_boost(void){
#if 0
    // over 5 secs, ramp Vh to VINIT
    // this does not need to be precise

    int tr = ondelay_until - get_time();
    if( tr <= 0 ) return;
    int vh = get_curr_vh();
    int vr = VOLTSINIT - vh;
    if( vr < 0 ) return;

    vh += vr * (1000000/CONTROLFREQ) / tr;
    if( vh > VOLTSINIT ) vh = VOLTSINIT;

    // D = 1 - Vi / Vo
    int vi  = curr_vi;
    int pwm = (vh > vi) ? 65535 - 65535 * vi / vh : 0;

    set_boost_pwm( pwm, BOOST_UNSYNCH );
#else
    // or not, this'll be easier to measure needed itarg
    set_boost_pwm(0, BOOST_UNSYNCH);
#endif
}

static inline void
calc_itarg(void){

    // calculate new target input current
    // previous ii +- vh move
    int it = (pcy_pi_ave << 4) / pcy_vi_ave;
    int va = 0;

    if( pcy_vh_max > MAX_VH_SOFT && pcy_vh_min < MIN_VH_SOFT ){
        // ouch
        // QQQ - intentionally add ripple to ii? pick a dir?

    }else if( pcy_vh_max > MAX_VH_SOFT ){
        // shift down
        va = (pcy_vh_max - MAX_VH_SOFT) - (pcy_vh_min - MIN_VH_SOFT);

    }else if( pcy_vh_min < MIN_VH_SOFT ){
        // shift up
        va = (MAX_VH_SOFT - pcy_vh_max) - (MIN_VH_SOFT - pcy_vh_min);
        int va2 = VH_TARGET - pcy_vh_max;
        if( va2 > va ) va = va2;

    }else{
        // try to keep the max near the top
        va = VH_TARGET - pcy_vh_max;
    }

    // i = C dv f
    itarg_err = GPI_CAP * va * 120 / (1000000>>4);
    itarg_adj = KT1 * itarg_err - KT2 * prev_itarg_err + KT3 * prev_itarg_adj;

    prev_input_itarg  = input_itarg;
    input_itarg      += itarg_adj;

    prev_itarg_err = itarg_err;
    prev_itarg_adj = itarg_adj;
}

// in discont mode
// E = vin^2 t^t / 2L = vout^2 C/2 = vin iin t
// Iinpk = vin t / L
// Iinav = Iinpk D / 2


static void
update_boost(void){
    short hcs  = cycle_step % HALFCYCLESTEPS;

    // QQQ - update once per cycle, or half-cycle?
    // half-cycle will converge faster
    // but risks increased 120Hz ripple
    if( !hcs ) calc_itarg();

    // d(vh)/dt = I / C

    input_err += input_itarg - curr_ii;
    input_adj  = KI1 * input_err - KI2 * prev_input_err + KI3 * prev_input_adj;

    // this much current to the output
    // QQQ - account for losses?
    int oi = (curr_vo * curr_io) / curr_vi;
    // this much current to the caps
    int ci = input_itarg - oi;

    // NB: V units are Q.4, I are Q.8
    int dvh = ci * (1000000>>4) / (CONTROLFREQ * GPI_CAP);
    int vi  = curr_vi;
    int vh  = curr_vh + dvh + input_adj;

    if( inv_state == INV_STATE_SHUTTINGDOWN ){
        // bring vh down
        int tr = (shutdown_until - get_time()) >> 4;
        int max = (MAX_VH_SOFT - vi) * tr / OFFDELAY + vi;
        if( vh > max ) vh = max;
    }else{
        if( vh > MAX_VH_SOFT ) vh = MAX_VH_SOFT + (vh - MAX_VH_SOFT) >> 1;
        if( vh > MAX_VH_HARD ) vh = MAX_VH_HARD;
    }

    int pwm = (vh > vi) ? 65535 - 65535 * vi / vh : 0;

    set_boost_pwm( pwm, BOOST_UNSYNCH );

    prev_input_err = input_err;
    prev_input_adj = input_adj;

}

/****************************************************************/

static void
ready_hbridge(void){

    // reset vars
    output_err = input_err = prev_output_err = prev_input_err = 0;
    output_adj = input_adj = prev_output_adj = prev_input_adj = 0;
}

void
update_hbridge(void){

    // table is: +- 16k
    int st = sin_data[ cycle_step + 1 ];

    if( inv_state == INV_STATE_SHUTTINGDOWN ){
        // shutting down - ramp down voltage
        int tr = (shutdown_until - get_time()) >> 4;
        st = (st * tr) / (OFFDELAY >> 4);
    }

    // predicted vh (Q:11.4)
    int pvh = curr_vh + (curr_vh - prev_vh);

    // target voltage
    int vt  = GPI_OUTV * st;
    output_vtarg = vt >> 10;	// Q.4

    // control error
    output_err = prev_output_vtarg - curr_vo;

    output_adj = KO1 * output_err - KO2 * prev_output_err + KO3 * prev_output_adj;

    // output
    // QQQ - or adj/pvh ?

    int pwm = (vt << 6) / pvh + (int)output_adj;
    set_hbridge_pwm( pwm );

    prev_output_vtarg = output_vtarg;
    prev_output_err   = output_err;
    prev_output_adj   = output_adj;

}


/****************************************************************/

void
update_fans(void){

    // determine hottest temp
    int t = get_curr_temp1();
    int tt = get_curr_temp2();
    if( tt > t ) t = tt;
    tt = get_curr_temp3();
    if( tt > t ) t = tt;

    // hot? - all on
    if( t > 500 ){
        set_fan1( 255 );
        set_fan2( 255 );
        return;
    }
    // cool? - minimum spin
    if( t < 300 ){
        set_fan1( 16 );
        set_fan2( 16 );
        return;
    }

    // cheese control
    int p = (t - 300) + 32;
    if( p > 255 ) p = 255;
    set_fan1(p);
    set_fan2(p);
}

/****************************************************************/

// real-time, high-frequency control

void
inverter_ctl(void){

    currproc->flags |= PRF_REALTIME;
    currproc->prio = 0;

    while(1){
        tsleep( TIM7, -1, "timer", 10000);

        curr_t0 = get_hrtime();		// for logging
        read_sensors();
        update_stats();
        update_gate_freq();

        switch( inv_state ){
        case INV_STATE_ONDELAY:
            precharge_boost();
            break;

        case INV_STATE_ONDELAY2:
            if( cycle_step ) break;

            // start running at zero-xing
            inv_state = INV_STATE_RUNNING;
            // fall thru

        case INV_STATE_RUNNING:
        case INV_STATE_SHUTTINGDOWN:
            update_boost();
            update_hbridge();
            break;

        default:
            set_gates_safe();
            break;
        }

        diaglog(inv_state);

    }
}


// slow control + monitoring

void
inverter_mon(void){

    syslog("device starting");

    FILE *f = fopen("dev:oled0", "w");
    STDOUT = f;
    STDIN  = 0;

    print_ready();

    while(1){
        update_fans();
        update_state();
        update_pwm_freq();	// LEDs + fans

        if( inv_state == INV_STATE_RUNNING ){
            // check for overload/faults
            // update display
        }

        usleep(1000);
    }
}

void
init_inverter(void){

    // bootmsg("hw init: inverter\n");
    start_proc( 8192, inverter_mon, "invmon" );
    start_proc( 8192, inverter_ctl, "invctl" );

}

/****************************************************************/

#define TESTTIME      get_hrtime()

DEFUN(testtiming, "test timing")
{
    int i;
    cycle_step = 0;

    ui_pause();
    usleep(1);
    int t0 = TESTTIME;
    for(i=10; i!=0; i--) read_sensors();
    int t1 = TESTTIME;
    for(i=10; i!=0; i--) calc_itarg();
    int t2 = TESTTIME;
    for(i=10; i!=0; i--) update_stats();
    int t3 = TESTTIME;
    cycle_step = 1;
    for(i=10; i!=0; i--) update_boost();
    int t4 = TESTTIME;
    for(i=10; i!=0; i--) update_hbridge();
    int t5 = TESTTIME;
    for(i=10; i!=0; i--) diaglog(0);
    int t6 = TESTTIME;


    set_gates_safe();
    ui_resume();

    printf("sens: %d calc: %d stats: %d boost: %d hbr: %d dlog: %d\n",
           t1-t0, t2-t1, t3-t2, t4-t3, t5-t4, t6-t5
        );

}

DEFUN(testswitch, "test switch")
{
    int s = gpio_get( HW_GPIO_SWITCH );

    while(1){
        int n = gpio_get( HW_GPIO_SWITCH );
        if( n != s ){
            play(ivolume, "a");
            s = n;
        }else{
            usleep(250000);
        }
    }
    return 0;
}

DEFUN(testsenstiming, "test sensor timing")
{
    int i;
    cycle_step = 0;

    ui_pause();
    usleep(1);
    int t0 = TESTTIME;
    for(i=10; i!=0; i--) read_sensors();
    int t1 = TESTTIME;

    ui_resume();

    printf("sens: %d \n",
           t1-t0
        );

}

DEFUN(logging, "enable/disable diaglog")
{
    if( argc > 1 ){
        diaglog_open( argv[1] );
    }else{
        diaglog_close();
    }

    return 0;
}
