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
#include "stats.h"
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
#define FAULTSONGT      5000000		// 10 sec
#define FAULT_TIME      2000000


DEFVAR(int, inv_state,    INV_STATE_OFF, UV_TYPE_UL, "inverter state")
DEFVAR(int, battle_short, 0,             UV_TYPE_UL | UV_TYPE_CONFIG, "battle short - enable unsafe operation")
// set false for testing, ...
DEFVAR(int, runinverter, 1, UV_TYPE_UL | UV_TYPE_CONFIG, "run the inerter")
// enter diag mode at start?
DEFVAR(int, diag_mode,    0,             UV_TYPE_UL | UV_TYPE_CONFIG, "diag mode: 0=off, 1=on, 2=never")


const char *fault_reason;

static utime_t ondelay_until  = 0;
static utime_t shutdown_until = 0;
utime_t curr_t0 = 0, prev_t0 = 0;

int cycle_step = 0;	// 0 .. 349 = 1 60Hz cycle
int half_cycle_step = 0;

// stats
// voltages Q.4, currents + power Q.8
struct StatAve 		s_vi, s_pi;
struct StatAveMinMax 	s_vo, s_ii, s_io, s_vh, s_po;
int curr_vo, curr_ic;

// control params
int output_err, prev_output_err, input_err, prev_input_err, itarg_err, prev_itarg_err;
int output_adj, prev_output_adj;
int itarg_adj, prev_itarg_adj, input_adj, prev_input_adj;
int output_erri, input_erri, itarg_erri;
int output_vtarg, prev_output_vtarg; 	// target output voltage
int input_itarg,  prev_input_itarg; 	// target input current
int req_ii_lpf;
int itarg_last_kick;

// for reference
int pwm_hbridge, pwm_boost;
int curr_count;
int curr_cycle;

static void reset_hbridge(void), reset_boost(void);
static void calc_itarg(void), calc_output(void);;

extern const struct Menu guitop;

/****************************************************************/

/*
  [2s	- double size

  [13m  - 5x8
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
printadj(const char *msg){
    int l = strlen(msg);
    int s;

    if( l < 13 ) s = 17;
    else if( l < 15 ) s = 16;
    else if( l < 22 ) s = 15;
    else s = 13;

    printf("\e[J\e[15mArea791RL - inverter\n\e[%dm%s\n", s, msg);

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

    int sw = get_switch();
    int bn = get_button();
    blinky_override = 0;

    switch( inv_state ){
    case INV_STATE_OFF:
        set_led_green(0);

        if( sw ){
            // switch was turned on
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
            // switch was turned off
            inv_state = INV_STATE_OFF;
            play(ivolume, "a-3");
            diag_counter ++;
            diag_last = get_time();
            printbig("OFF\n");
            usleep(500000);
            print_ready();
            syslog("canceled");
        }else if( get_time() >= ondelay_until ){
            // countdown finished
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
            set_led_white(64);
            usleep(437500);

            // reset the clock if Vi drops below 300 (per the spec)
            if( s_vi._curr < MIN_VI_SOFT ){
                ondelay_until = get_time() + ONDELAY;
            }
        }
        break;

    case INV_STATE_ONDELAY2:
        // will move to running at next zero-xing
        break;

    case INV_STATE_RUNNING:
        set_led_green( 127 );

        if( !sw ){
            // switch was turned off
            // quickly bring VH down, and stop at zero-xing
            shutdown_until = get_time() + OFFDELAY;
            inv_state = INV_STATE_SHUTTINGDOWN;
            play(ivolume, "a-3");	// debounce
            syslog("switch off");
        }
        break;

    case INV_STATE_SHUTTINGDOWN:
        set_led_green(255);

        if( get_time() >= shutdown_until ){
            if( sw ){
                // wild switch flipping
                inv_state    = INV_STATE_FAULTEDOFF;
                fault_song   = 0;
                fault_reason = "SWITCH ERR";
                syslog("switch fault");
            }else{
                // done ramping down
                inv_state = INV_STATE_OFF;
                play(ivolume, "f-3");
                printbig("AC OFF\n");
                syslog("ac disabled");
                usleep(500000);
                print_ready();
            }
        }
        break;

    case INV_STATE_FAULTED:
        set_led_green(255);
        set_led_red(255);
        // we ramp down same as a shutdown, but end in faultedoff
        shutdown_until = get_time() + OFFDELAY;
        inv_state = INV_STATE_FAULTINGDOWN;
        printadj(fault_reason);
        break;

    case INV_STATE_FAULTINGDOWN:
        set_led_green(255);
        set_led_red(255);
        if( get_time() >= shutdown_until ){
            inv_state    = INV_STATE_FAULTEDOFF;
            fault_song   = 0;
            syslog("ac disabled (fault)");
        }
        break;

    case INV_STATE_FAULTEDOFF:
        set_led_green(0);
        set_led_red(255);

        if( !sw ){
            // switch turned off, resume normal operation
            inv_state = INV_STATE_OFF;
            play(ivolume, "f-3");
            set_led_red(0);
            print_ready();
        }else if( get_time() >= fault_song ){
            // periodically, sing
            printadj(fault_reason);
            play(128, "t150[4 d+3d-3]");
            fault_song = get_time() + FAULTSONGT;
        }

        break;

    case INV_STATE_DIAG:
        menu(&guitop);
        inv_state = INV_STATE_OFF;
        print_ready();
        break;

    case INV_STATE_TEST:
        break;
    }

    if( diag_last && diag_last + 5000000 < get_time() ){
        diag_counter = 0;
        diag_last    = 0;
    }
    // toggle switch rapidly, or toggle switch once + press button
    if( (diag_mode != 2) && ((diag_counter >= 5) || (bn && diag_counter)) ){
        inv_state = INV_STATE_DIAG;
        diag_counter = 0;
        printbig("DIAG MODE\n");
        syslog("diag mode");
        play(ivolume, "g4e4f4f-3");
        // ...
    }

}

/****************************************************************/
static void
reset_stats(void){

    _stm_reset( &s_ii );
    _stm_reset( &s_io );
    _stm_reset( &s_vh );
    _stm_reset( &s_po );
    _sta_reset( &s_vi );
    _stm_reset( &s_vo );
    _sta_reset( &s_pi );

    output_vtarg = 0;
    itarg_erri   = 0;
    prev_itarg_err = 0;

    curr_count   = 0;
    curr_cycle   = 0;
}

static inline void
rotate_stats(void){
    _stm_cycle( &s_ii );
    _stm_cycle( &s_io );
    _stm_cycle( &s_vh );
    _stm_cycle( &s_po );
    _sta_cycle( &s_vi );
    _stm_cycle( &s_vo );
    _sta_cycle( &s_pi );

    curr_count = 0;
    if( curr_cycle < (1<<30) ) curr_cycle ++;
}

static inline void
add_stats(void){
    short sign = (cycle_step >= HALFCYCLESTEPS) ? -1 : 0;

    int io = get_curr_io();
    int ii = get_curr_ii();
    int vi = get_curr_vi();
    int vo = get_curr_vo(); if(sign) vo = -vo;

    _stm_add( &s_ii, ii );
    _stm_add( &s_io, io );
    _stm_add( &s_vh, get_curr_vh() );
    _sta_add( &s_vi, vi );
    _stm_add( &s_po, (io * vo) >> 4 );
    _sta_add( &s_pi, (ii * vi) >> 4 );

    _stm_add( &s_vo, ABS(vo) );		// abs
    curr_vo = vo;			// signed
    curr_ic = get_curr_ic();

    curr_count ++;
}

static void
update_stats(void){

    if( !half_cycle_step || (half_cycle_step < curr_count) ){

        rotate_stats();

        calc_itarg();
        calc_output();
    }

    add_stats();

    // RSN - determine power factor from po_min + po_max
    //
    //               max + min
    // angle = acos( --------- )
    //               max - min

    // lead/lag : look at sign of io

}

static void
update_stats_dc(void){

    if( !half_cycle_step || (half_cycle_step < curr_count) ){
        rotate_stats();
        calc_output();
    }

    add_stats();
}

/****************************************************************/

static void
reset_itarg(void){

    itarg_erri       = 0;
    prev_input_itarg = input_itarg  = 100;
    prev_itarg_err   = itarg_err = 0;
    prev_itarg_adj   = itarg_adj = 0;
    itarg_last_kick  = 0;
    cycle_step       = 0;
    req_ii_lpf       = 0;
}

static int
est_req_ii(void){

    if( curr_cycle ){
        // estimate required input current
        // QQQ - may need to adj for power factor efficency loss
        int vi = s_vi.ave ? s_vi.ave : s_vi._curr;
        int pi = s_po.ave;
#if 1
        if( pi < P_DCM )
            pi = (pi * 1167) >> 10;	// DCM mode, 14% power loss
        else
            pi = (pi * 1042) >> 10;	// 1.8% power loss

        pi += get_lpf_ic() * 12;	// power to low voltage systems
#endif
        int it = (pi<<4) / vi;		// estimated required input current

        if( curr_cycle == 1 )
            req_ii_lpf = it;
        else
            req_ii_lpf = (it + req_ii_lpf) >> 1;
        return req_ii_lpf;
    }

    req_ii_lpf = 0;
    return 0;
}

// determine target input current
static void
calc_itarg(void){

    // calculate new target input current
    // previous ii +- vh move
    int it  = curr_cycle ? s_ii.ave : 0;

    // try to stay centered
    int va = ((Q_VOLTS(MAX_VH_SOFT) - s_vh.max) + (Q_VOLTS(MIN_VH_SOFT) - s_vh.min)) >> 1;
    // RSN - cap va to max - to avoid large I

    if( s_vh.max >= Q_VOLTS(MAX_VH_HARD) ){
        // shift down - safety first!
        int va2 = Q_VOLTS(MAX_VH_SOFT) - s_vh.max;
        if( va2 < va ) va = va2;
        // shift harder to avoid clipping or tripping a fault
        it -= it>>2;
        reset_boost();
        itarg_last_kick = curr_cycle;
    }else if( s_vh.min < Q_VOLTS(MIN_VH_HARD) ){
        // NB - this cannot happen under the "normal" conditions (Vin > Vout)
        // if we drop too low, the output clips
        int va2 = Q_VOLTS(MIN_VH_SOFT) - s_vh.min;
        if( va2 > va ) va = va2;
        // if we clipped, increase current by the estimated clippage
        it += it>>2;
    }


    if( !(curr_cycle % 5) ) syslog("pi %d po %d it %d va %d, eri %d", s_pi.ave, s_po.ave, it, va, est_req_ii());

    // i = C dv f
#if (GPI_CAP == 40) && (GPI_OUTHZ == 60)
    itarg_err = (va * 1258) >> 14;
#else
    itarg_err = va * (GPI_CAP * GPI_OUTHZ * 2) / (1000000>>4);
#   error "make me faster!"
#endif

    // kick - improves convergence
    if( (-va > (Q_VOLTS(MAX_VH_SOFT - MIN_VH_SOFT)>> 4) && s_vh.max >= Q_VOLTS(MAX_VH_SOFT) )
        || ( -itarg_err > (input_itarg>>4)) ){
        // but too often (avoid any 120Hz spikes)

        if( curr_cycle - itarg_last_kick > 2 ){
            reset_boost();
            itarg_last_kick = curr_cycle;
        }
    }

    int errd;

    if( curr_cycle > 1 ){
        errd = itarg_err - prev_itarg_err;
        itarg_erri += itarg_err;
    }else{
        // avoid windup when device is first powered on
        errd = 0;
        itarg_erri = 0;
    }

    // PI control
    itarg_adj  = (KIP * itarg_err + KII * itarg_erri) >> 8;

    prev_input_itarg  = input_itarg;
    input_itarg       = it + itarg_adj;

    prev_itarg_err    = itarg_err;
    prev_itarg_adj    = itarg_adj;
}

// in discont mode
// E = vin^2 t^t / 2L = vout^2 C/2 = vin iin t
// Iinpk = vin t / L
// Iinav = Iinpk D / 2

static void
reset_output(){
    // reset vars
    output_err  = input_err = prev_output_err = prev_input_err = 0;
    output_adj  = input_adj = prev_output_adj = prev_input_adj = 0;
    output_erri = 0;
}


// adjust vout to compensate for VH resistor drift
static void
calc_output(void){

    output_err   = s_vo.max - Q_VOLTS(GPI_OUTV);
    output_erri += output_err;
    output_adj   = (KOP * output_err + KOI * output_erri) >> 8;
    // ...
}

/****************************************************************/

static void
reset_boost(void){

    input_err = prev_input_err = input_erri = 0;
    input_adj = prev_input_adj = 0;
}

static void
update_boost(void){

    // d(vh)/dt = I / C

    int itarg = input_itarg;

    int vi  = s_vi._curr;
    int lvh = get_lpf_vh();

    input_err   = itarg - get_lpf_ii();
    input_erri += input_err;
    input_adj   = (input_err + input_erri + prev_input_adj) >> 1;

    // this much current to the output
    int oi = (output_vtarg * s_io._curr) / vi;

    // QQQ - power factor?

    // this much current to the caps
    int ci = itarg - oi + input_adj;

    // NB: V units are Q.4, I are Q.8
#if (CONTROLFREQ == 21000) && (GPI_CAP == 40)
    int dvh = (ci * 1219) >> 14;
#else
    int dvh = ci * (1000000>>4) / (CONTROLFREQ * GPI_CAP);
#   error "make me faster!"
#endif

    int vh  = lvh + dvh;

    if( inv_state == INV_STATE_SHUTTINGDOWN || inv_state == INV_STATE_FAULTINGDOWN ){
        // bring vh down
        int tr  = (shutdown_until - get_time()) >> 4;
        int max = (Q_VOLTS(MAX_VH_SOFT) - vi) * tr / OFFDELAY + vi;
        if( vh > max ) vh = max;
    }else{
        // make sure we have enough
        if( vh < output_vtarg + Q_V_ELEVATOR ) vh = output_vtarg + Q_V_ELEVATOR;
        // avoid runaway at no load
        if( s_vh._curr > Q_VOLTS(MAX_VH_SOFT) && !input_itarg ) vh = 0;
    }

    pwm_boost = (vh > vi) ? 65535 - 65535 * vi / vh : 0;

    // safety limit
    if( s_vh._curr > Q_VOLTS(MAX_VH_HARD) )
        pwm_boost = 0;

    set_boost_pwm( pwm_boost, BOOST_UNSYNCH );

    prev_input_err = input_err;
    prev_input_adj = input_adj;

}

// regulate VH
static void
update_boost_dc(void){

    int vi  = s_vi._curr;

    int vht = output_vtarg + 2 * Q_V_ELEVATOR;
    int vh  = get_lpf_vh();

    input_err   = vht - get_lpf_vh();
    if( !curr_cycle ) input_err = 0;	// too soon

    input_erri += input_err;
    int adj     = (28 * input_err + 2 * input_erri) >> 8;
    input_adj   = adj;

    // prevent runaway
    if( vh > vht + (vht>>2) ) vht = 0;

    if( vht ) vht += input_adj;
    pwm_boost = (vht > vi) ? 65535 - 65535 * vi / vht : 0;

    // RSN - if vht => control + smooth
    // vh is smooth+clean, but low

    set_boost_pwm( pwm_boost, BOOST_UNSYNCH );

    prev_input_err = input_err;
    prev_input_adj = input_adj;

}

/****************************************************************/
static int output_k;

static void
reset_hbridge(void){

}

void
update_hbridge(void){

    // table is: +- 16k
    int st = sin_data[ cycle_step ];

    if( inv_state == INV_STATE_SHUTTINGDOWN || inv_state == INV_STATE_FAULTINGDOWN ){
        // shutting down - ramp down voltage
        int tr = (shutdown_until - get_time()) >> 4;
        st = (st * tr) / (OFFDELAY >> 4);
    }

    int pvh = get_lpf_vh();

    // target voltage
    // adjust for VH measurement error, temperature drift
    int adj = 0; // (s_vh.min > Q_VOLTS(GPI_OUTV)) ? (s_vh.ave - Q_VOLTS(GPI_OUTV)) / 7 : 0;
    int vam = Q_VOLTS(GPI_OUTV) - adj;
    int vt  = vam * st;
    output_vtarg = vt >> 14;	// Q.4

    // output
    int pwm = (vt << 2) / pvh ;
    if( pwm >  0xFE00 ) pwm =  0xFE00;
    if( pwm < -0xFE00 ) pwm = -0xFE00;

    // smooth
    pwm_hbridge = (pwm + 3 * pwm_hbridge) >> 2;

    set_hbridge_pwm( pwm_hbridge );

    // sync output
    if( st >= 0 ) gpio_set( HW_GPIO_SYNC );
    else          gpio_clear( HW_GPIO_SYNC );


    prev_output_vtarg = output_vtarg;
    prev_output_err   = output_err;
    prev_output_adj   = output_adj;

}

static void
update_hbridge_dc(void){
    // output_vtarg should be set by caller

    int pvh = get_lpf_vh();

    // update_stats flips the sign
    if( curr_vo < 0 ) curr_vo = - curr_vo;
#if 0
    output_err   = output_vtarg - curr_vo;
    output_erri += output_err;
    output_adj   = (1035 * output_err + 207 * output_erri) >> 8;
#endif

    // output
    int pwm = ((output_vtarg) << 16) / pvh ;
    if( pwm >  65000 ) pwm =  65000;
    if( pwm < -65000 ) pwm = -65000;

    // smooth
    pwm_hbridge = (pwm + 3 * pwm_hbridge) >> 2;

    // flip sign, so H2 is +, H1 is -
    set_hbridge_pwm( - pwm_hbridge );

    prev_output_vtarg = output_vtarg;
    prev_output_err   = output_err;
    prev_output_adj   = output_adj;

}

/****************************************************************/

void
update_fans(void){

    int t = get_max_temp();

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

void
check_for_bad(void){
    char *soft=0, *hard=0;
    static utime_t fault_start = 0;
    static logged = 0;

    // Vh too high?
    // II, IO too high?
    if( get_lpf_vh() > Q_VOLTS(MAX_VH_SOFT) ) soft = "VH OVERLOAD";
    if( get_lpf_vh() > Q_VOLTS(MAX_VH_HARD) ) hard = "VH OVERLOAD";
#if 0
    if( pcy_vi_ave    > Q_VOLTS(MIN_VI_SOFT))  soft = "VI UNDERVOLT";
    if( pcy_vi_ave    > Q_VOLTS(MIN_VI_HARD))  hard = "VI UNDERVOLT";
    if( pcy_ii_ave    > Q_AMPS(MAX_II_SOFT) )  soft = "II OVERLOAD";
    if( pcy_ii_ave    > Q_AMPS(MAX_II_HARD) )  hard = "II OVERLOAD";
    if( pcy_io_ave    > Q_AMPS(MAX_IO_SOFT) )  soft = "IO OVERLOAD";
    if( pcy_io_ave    > Q_AMPS(MAX_IO_HARD) )  hard = "IO OVERLOAD";

    if( output_err > Q_VOLTS(MAX_OE_SOFT) )    soft = "OUT ERR";
    if( output_err > Q_VOLTS(MAX_OE_HARD) )    hard = "OUT ERR";

    int mt = get_max_temp();
    if( mt > MAX_TEMP_SOFT ) soft = "TOO HOT";
    if( mt > MAX_TEMP_HARD ) hard = "TOO HOT";

#endif

    // soft limit => flash LED
    // hard limit => start timer, time exceeded? shutdown

    if( !hard ){
        fault_start = 0;
        logged = 0;
    }

    if( hard ){
        set_led_red( 255 );

        if( get_time() > fault_start + FAULT_TIME ){
            if( ! battle_short ){
                inv_state = INV_STATE_FAULTED;
            }

            fault_reason = hard;
            if( !logged ) syslog("hard fault: %s [vh %d]", hard, get_lpf_vh());
            logged = 1;
        }

    }else if( soft ){
        set_led_red( 127 );
    }else{
        set_led_red( 0 );
    }
}

/****************************************************************/

// real-time, high-frequency control

void
inverter_ctl(void){

    currproc->flags |= PRF_REALTIME;
    currproc->prio = 0;

    while(1){
        // timer7 => 21kHz
        tsleep( TIM7, -1, "timer", 10000);

        curr_t0 = get_hrtime();		// for logging
        read_sensors();
        update_stats();
        update_gate_freq();

        switch( inv_state ){
        case INV_STATE_ONDELAY:
            break;

        case INV_STATE_ONDELAY2:
            if( cycle_step ) break;

            // start running at zero-xing
            inv_state = INV_STATE_RUNNING;
            reset_stats();
            reset_itarg();
            reset_output();
            reset_boost();
            reset_hbridge();
            // fall thru

        case INV_STATE_RUNNING:
        case INV_STATE_SHUTTINGDOWN:
        case INV_STATE_FAULTED:
        case INV_STATE_FAULTINGDOWN:

            update_boost();
            update_hbridge();
            break;

        case INV_STATE_TEST:
            while( inv_state == INV_STATE_TEST ){
                usleep(100000);
            }
            break;

        case INV_STATE_DIAG:
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

    if( battle_short ){
        syslog("battle short enabled");
        //printadj("BATTLE SHORT ON");
        set_led_red( 255 );
        usleep(250000);
    }

    print_ready();

    while(1){
        update_fans();
        update_state();
        update_pwm_freq();	// LEDs + fans

        if( inv_state == INV_STATE_RUNNING ){
            check_for_bad();
            // update display
        }

        usleep(1000);
    }
}

void
init_inverter(void){

    if( diag_mode == 1 ) inv_state = INV_STATE_DIAG;
    if( !runinverter )   return;

    // bootmsg("hw init: inverter\n");
    start_proc( 8192, inverter_mon, "invmon" );
    start_proc( 8192, inverter_ctl, "invctl" );

}

/****************************************************************/

static const char * _short_mode[] = { "disable", "enable" };
DEFUN(set_battle_short, "")
{
    if( argc == 2 ){
        battle_short = atoi(argv[1]);
    }else if( argv[0][0] == '-' ){
        int v = menu_get_str("battle short", 2, _short_mode, battle_short);
        if( v != -1 ) battle_short = v;
    }

    return 0;
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

/****************************************************************/

// on, off at various levels

DEFUN(profhpwm, "profile hbridge pwm")
{

    currproc->flags |= PRF_REALTIME;
    currproc->prio = 0;

    diaglog_open( "log/hbrpwm.log");
    diaglog_testres();
    inv_state = INV_STATE_TEST;

    printf("flip switch on\n");
    while( !get_switch() ) usleep( 100000 );
    play(ivolume, "ab");
    set_led_green( 255 );

    int pwm, i, n;
    for(pwm=4; pwm<65535; pwm=(pwm+(pwm>>1))&~1){

        output_vtarg = pwm;
        for(n=0; n<500; n++){
            tsleep( TIM7, -1, "timer", 10000);

            curr_t0 = get_hrtime();		// for logging
            read_sensors();

            set_hbridge_pwm( pwm );
            diaglog(0);
        }

        output_vtarg = 0;
        for(n=0; n<500; n++){
            tsleep( TIM7, -1, "timer", 10000);

            curr_t0 = get_hrtime();		// for logging
            read_sensors();

            set_hbridge_pwm( 0 );
            diaglog(0);
        }

        diaglog_flush();
    }

    set_gates_safe();
    diaglog_close();
    play(ivolume, "ba");
    while( get_switch() ) usleep( 100000 );

    set_led_green( 0 );
    inv_state = INV_STATE_OFF;

    return 0;
}

DEFUN(profsine, "profile sine wave")
{

    currproc->flags |= PRF_REALTIME;
    currproc->prio = 0;

    diaglog_open( "log/sine.log");
    diaglog_testres();
    inv_state = INV_STATE_TEST;

    printf("flip switch on\n");
    while( !get_switch() ) usleep( 100000 );
    play(ivolume, "ab");
    set_led_green( 255 );

    utime_t tend = get_time() + 1000000;
    cycle_step = 0;
    reset_stats();
    reset_itarg();
    reset_output();
    reset_boost();
    reset_hbridge();
    fault_reason = 0;
    input_itarg  = 100;
    output_k = 0;

    while(1){

        curr_t0 = get_hrtime();		// for logging
        read_sensors();

        update_stats();
        //check_for_bad();
        update_boost();
        update_hbridge();
        output_k ++;

        if( !get_switch() ) break;
        //if( fault_reason )  break;

        if( get_time() <= tend )
            diaglog(0);

        tsleep( TIM7, -1, "timer", 1000);
    }

    set_gates_safe();
    if( fault_reason ) printf("ERR: %s\n", fault_reason);
    diaglog_close();
    play(ivolume, "ba");
    set_led_green( 0 );
    inv_state = INV_STATE_OFF;

    return 0;
}

static const char *_dc_voltage[] = {
    "24", "48", "72", "100", "150", "200", "250", "300", "350", "400", "450", /* "500", "600", "700", "800", */
};
// generate high-voltage DC to feed into 2nd unit
DEFUN(testdc, "generate DC output")
{

    int di = menu_get_str("Vout", ELEMENTSIN(_dc_voltage), _dc_voltage, 0);
    if( di == -1 ) return 0;
    int vout = atoi( _dc_voltage[di] );

    currproc->flags |= PRF_REALTIME;
    currproc->prio = 0;

    diaglog_open( "log/dc.log");
    diaglog_testres();
    inv_state = INV_STATE_TEST;

    printf("vout %d DC\n", vout);
    printf("flip switch on\n");
    while( !get_switch() ) usleep( 100000 );
    play(ivolume, "ab");
    set_led_green( 255 );

    utime_t tend = get_time() + 1000000;
    cycle_step = 0;
    reset_stats();
    reset_itarg();
    reset_output();
    reset_boost();
    reset_hbridge();
    input_itarg = 0;
    output_vtarg = Q_VOLTS( vout );
    output_k     = 0;

    while(1){

        curr_t0 = get_hrtime();		// for logging
        read_sensors();

        update_stats_dc();
        //check_for_bad();
        update_boost_dc();
        update_hbridge_dc();
        output_k ++;

        if( !get_switch() ) break;

        if( get_time() <= tend )
            diaglog(0);

        tsleep( TIM7, -1, "timer", 1000);
    }

    set_gates_safe();
    diaglog_close();
    printf("vh %d\n", s_vh.ave);
    play(ivolume, "ba");
    set_led_green( 0 );
    inv_state = INV_STATE_OFF;

    sleep(5);
    return 0;
}

DEFUN(testloop, "test loop")
{

    currproc->flags |= PRF_REALTIME;
    currproc->prio = 0;

    diaglog_open( "log/loop.log");
    diaglog_testres();
    inv_state = INV_STATE_TEST;

    play(ivolume, "ab");

    utime_t tend = get_time() + 1000000;
    cycle_step = 0;
    // synchronize
    tsleep( TIM7, -1, "timer", 1000);
    curr_t0 = get_hrtime();

    while(1){
        tsleep( TIM7, -1, "timer", 1000);
        prev_t0 = curr_t0;
        curr_t0 = get_hrtime();
        read_sensors();

        if( get_time() >= tend ) break;

        // NB: sd card cannot keep up
        diaglog( curr_t0 - prev_t0 );
    }

    set_gates_safe();
    diaglog_close();
    play(ivolume, "ba");
    inv_state = INV_STATE_OFF;

    return 0;
}


DEFUN(testsin, "test sine pwm")
{
    int step;

    FILE *f = fopen("log/sin.tab", "w");
    if(!f) return -1;

    inv_state = INV_STATE_TEST;

    for(step=0; step<CYCLESTEPS; step++){

        // table is: +- 16k
        int st = sin_data[ step ];

        // predicted vh (Q:11.4)
        int pvh = Q_VOLTS(GPI_OUTV);

        // target voltage
        int vt  = GPI_OUTV * st;
        output_vtarg = vt >> 10;	// Q.4


        int pwm = (vt << 6) / pvh ;
        int pv  = hbridge_pwm_val(pwm);

        //set_hbridge_pwm(pwm);
        //usleep(1000);

        fprintf(f, "%d\t%d\t%d\t%d\t%d,%d\n", step, output_vtarg, pwm, pv, TIM1->CCR1, TIM1->CCR2);

    }

    fclose(f);
    inv_state = INV_STATE_OFF;
    return 0;
}

DEFUN(testsboo, "test sine pwm+boost")
{
    int step;

    FILE *f = fopen("log/sin.tab", "w");
    if(!f) return -1;

    inv_state = INV_STATE_TEST;

    for(step=0; step<CYCLESTEPS; step++){

        // table is: +- 16k
        int st = sin_data[ step ];

        // predicted vh (Q:11.4)
        int pvh = Q_VOLTS(12);

        // target voltage
        int vt  = 12 * st;
        output_vtarg = vt >> 10;	// Q.4

        int hpwm = (vt << 6) / pvh ;
        int pv   = hbridge_pwm_val(hpwm);

        // boost
        int c_vi  = Q_VOLTS(12);
        int c_vh  = Q_VOLTS(12);
        int c_io  = (output_vtarg << 4) / 50;	// 50 Ohm load
        int itarg = 44 + 120;			// 170 mA load + overhead

        // this much current to the output
        int oi = (output_vtarg * c_io) / c_vi;

        // this much current to the caps
        int ci = itarg - oi;

        // NB: V units are Q.4, I are Q.8
        int dvh = ci * (1000000>>4) / (CONTROLFREQ * GPI_CAP);
        int vh  = c_vh + dvh + input_adj;

        int bpwm = (vh > c_vi) ? 65535 - 65535 * c_vi / vh : 0;


        //set_hbridge_pwm(pwm);
        //usleep(1000);
        // fprintf(f, "%d\t%d\t%d\t%d\t",     step, output_vtarg, hpwm, pv);

        fprintf(f, "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", step, output_vtarg,  c_io, oi, ci, dvh, vh, bpwm);

    }

    fclose(f);
    inv_state = INV_STATE_OFF;
    return 0;
}

DEFUN(spidump, "spi dump crumbs")
{
    spi_dump_crumb();
    return 0;
}
