/*
  Copyright (c) 2014
  Author: Jeff Weisberg <jaw @ tcp4me.com>
  Created: 2014-Aug-14 23:03 (EDT)
  Function: pwm gate drive

*/

#include <conf.h>
#include <proc.h>
#include <gpio.h>
#include <pwm.h>
#include <stm32.h>
#include <userint.h>
#include <error.h>

#include "defproto.h"
#include "util.h"
#include "gpiconf.h"
#include "hw.h"
#include "inverter.h"

#define APB2CLOCK	84000000
#define HBRFREQ_LO	300000
#define HBRFREQ_HI	350000
#define BOOSTFREQ_LO	300000
#define BOOSTFREQ_HI	350000
#define DEADTIME	30	// ns.  ~ Toff + Tfall - Ton

#define HBRFREQ_VMIN	(APB2CLOCK / HBRFREQ_HI)
#define HBRFREQ_VMAX	(APB2CLOCK / HBRFREQ_LO)
#define BOOSTFREQ_VMIN	(APB2CLOCK / BOOSTFREQ_HI)
#define BOOSTFREQ_VMAX	(APB2CLOCK / BOOSTFREQ_LO)


static int maxval_h, maxval_b;
static int pwm_h, mode_b, pwm_b;

static inline int
pwmval(int v, int max){
    // p = v * maxval / 64k
    v *= max;
    // dither
    int d = v & 0xFFFF;
    v >>= 16;
    if( (v < max-1) && (random() & 0xFFFF) <= d ) v ++;
    if( v > max-1 ) v = max-1;
    if( v < 0 ) v = 0;
    return v;
}

// set( 0 .. 64k )
// 0 => power pass thru
// 64k => short out the battery (very bad!)
void
set_boost_pwm(int v, int m){

    // skip check if we are turning it off (to avoid recursion)
    if( v && safety_check_boost() ) return;

    if( v < 0 ) v = 0;

    if( m == BOOST_UNSYNCH ){
        if( m != mode_b ){
            // high-side gate off
            gpio_init( HW_GPIO_GATE_BOOST_H,     GPIO_OUTPUT | GPIO_PUSH_PULL | GPIO_SPEED_50MHZ );
            gpio_clear( HW_GPIO_GATE_BOOST_H );
        }
        // adjust freq - to avoid saturation + heat
        // Tsat = Isat L / Vin
        // pwmmax = Isat LuH F 64k / 1e6 / Vin
        // run at slowest freq possible without saturating
        // RSN - only if temp > x
        int lim = ((GPI_LSAT * GPI_IND * BOOSTFREQ_LO) >> 4) / get_lpf_vi();
        int div = v ? lim / v : 32;
        if( div > 32 ) div = 32;	// QQQ - limit?
        if( div < 1  ){
            // saturates at highest freq - truncate
            div = 1;
            v   = lim;
        }
        TIM8->PSC = div - 1;
    }else{
        if( m != mode_b ){
            // enable high-side gate
            gpio_init( HW_GPIO_GATE_BOOST_H,     GPIO_AF(3) | GPIO_SPEED_50MHZ );
            TIM8->PSC = 0;	// normal freq
        }
    }

    pwm_set( HW_TIMER_GATE_BOOST, pwmval(v, maxval_b) );
    pwm_b  = v;
    mode_b = m;
}


// set( 0 .. 64k )
// 0,0 => both sides ground
// 64k,64k => both sides high
// normal operation: x,0 and 0,x
int
set_hbridge_pwm(int v){

    pwm_h = v;

    if( v >= 0 ){
        v = pwmval(v, maxval_h);
        pwm_set( HW_TIMER_GATE_HBRIDGE_L, v );
        pwm_set( HW_TIMER_GATE_HBRIDGE_R, 0);
        return v;
    }else{
        v = pwmval(-v, maxval_h);
        pwm_set( HW_TIMER_GATE_HBRIDGE_L, 0 );
        pwm_set( HW_TIMER_GATE_HBRIDGE_R, v );
        return -v;
    }
}

int
hbridge_pwm_val(int v){

    if( v >= 0 )
        return pwmval(v, maxval_h);
    else
        return pwmval(-v, maxval_h);
}

void
set_gates_safe(void){

    set_boost_pwm(0, BOOST_UNSYNCH);
    set_hbridge_pwm(0);
}

// this is called from set_boost_pwm and read_sensors
int
safety_check_boost(void){
    static char ovct = 0;

    if( battle_short ) return 0;

    int v = get_curr_vh();
    if( (v >= Q_VOLTS(STOP_HARD_VH)) && (++ovct > 10) ){
        // last-resort emergency fail-safe cut-off
        // internal voltage is too high, shut down the boost
        set_gates_safe();
        // QQQ - set fault mode or halt device?
        // presumably, the inverter control system has failed, the fault check has failed
        kprintf("VH %d\n", v>>4);
        PANIC("OVER VOLTAGE");
        return 1;
    }

    ovct = 0;
    return 0;
}


void
update_gate_freq(void){
    int v;
    return; // XXX

    // boost
    v = BOOSTFREQ_VMIN + random_n(BOOSTFREQ_VMAX - BOOSTFREQ_VMIN );
    TIM8->ARR = v;
    maxval_b = v + 1;
    set_boost_pwm( pwm_b, mode_b );

    // h-bridge
    v = HBRFREQ_VMIN + random_n(HBRFREQ_VMAX - HBRFREQ_VMIN );
    TIM1->ARR = v;
    maxval_h = v + 1;
    set_hbridge_pwm( pwm_h );
}

static void
_init_timer(TIM_TypeDef *t){

    t->CR1 |= 1<<7;	// ARR is buffered
    t->CR1 |= 3<<5;	// center aligned mode
    //CR2 - idle states

    // CR1 prescaler for dead time?

    // dead time
    // 2 bits in CR1 - prescale apb2 by 1,2,4 => Tdts
    // BDTR 7 bits
    // DTG[7:5]=0xx => DT=DTG[7:0]x tdtg with tdtg=tDTS.
    // DTG[7:5]=10x => DT=(64+DTG[5:0])xtdtg with Tdtg=2xtDTS.
    // DTG[7:5]=110 => DT=(32+DTG[4:0])xtdtg with Tdtg=8xtDTS. DTG[7:5]=111 => DT=(32+DTG[4:0])xtdtg with Tdtg=16xtDTS.

    // no prescalers up to ~372 ns
    int dtg = (DEADTIME * (APB2CLOCK / 1000000) + 999) / 1000;	// ceil( DT * APB2 / 1e9 )

    t->BDTR &= 0xFF;
    t->BDTR |= 1<<11;	// OSSR - does not seem to help
    t->BDTR |= dtg;

}

void
init_gates(void){

    bootmsg(" gates");

    //gpio_init( HW_GPIO_GATE_BOOST_H,     GPIO_AF(3) | GPIO_SPEED_50MHZ );
    gpio_init( HW_GPIO_GATE_BOOST_H,     GPIO_OUTPUT | GPIO_PUSH_PULL | GPIO_SPEED_50MHZ );
    gpio_clear(HW_GPIO_GATE_BOOST_H);

    gpio_init( HW_GPIO_GATE_BOOST_L,     GPIO_AF(3) | GPIO_SPEED_50MHZ );
    gpio_init( HW_GPIO_GATE_HBRIDGE_L_H, GPIO_AF(1) | GPIO_SPEED_50MHZ );
    gpio_init( HW_GPIO_GATE_HBRIDGE_L_L, GPIO_AF(1) | GPIO_SPEED_50MHZ );
    gpio_init( HW_GPIO_GATE_HBRIDGE_R_H, GPIO_AF(1) | GPIO_SPEED_50MHZ );
    gpio_init( HW_GPIO_GATE_HBRIDGE_R_L, GPIO_AF(1) | GPIO_SPEED_50MHZ );

    _init_timer( TIM1 );
    _init_timer( TIM8 );

    pwm_init(  HW_TIMER_GATE_BOOST,     BOOSTFREQ_HI, BOOSTFREQ_VMIN - 1 );
    pwm_init(  HW_TIMER_GATE_HBRIDGE_L, HBRFREQ_HI,   HBRFREQ_VMIN - 1 );

    // CCER must be set after pwm_init
    TIM1->CCER = 0x0FF;		// channels 1 + 2, inverted
    TIM8->CCER = 0x500; 	// channel 3

    maxval_h = HBRFREQ_VMIN;
    maxval_b = BOOSTFREQ_VMIN;

    set_gates_safe();
}

DEFUN(safe, "set gates to safe state (may take several minutes for existing voltages to bleed)")
{
    set_gates_safe();
    return 0;
}

DEFUN(testgates, "test gates")
{
    if( argc < 4 ) return;

    pwm_set( HW_TIMER_GATE_BOOST, 	atoi(argv[1]) );
    pwm_set( HW_TIMER_GATE_HBRIDGE_L, 	atoi(argv[2]) );
    pwm_set( HW_TIMER_GATE_HBRIDGE_R, 	atoi(argv[3]) );

    return 0;
}

DEFUN(testhbr, "test hbridge")
{
    int i;

    for(i=0; i<10; i++){
        set_hbridge_pwm(8192);
        sleep(1);
        set_hbridge_pwm(-8192);
        sleep(1);
    }

    set_hbridge_pwm(0);
    return 0;
}

DEFUN(testboost, "test boost")
{
    int i;

    set_hbridge_pwm(65535);
    set_boost_pwm(50000, BOOST_UNSYNCH);
    for(i=0; i<10; i++){
        read_sensors();
        printf("vi %4d ii %4d\n", get_curr_vi(), get_curr_ii() );
        printf("vo %4d io %4d\n", get_curr_vo(), get_curr_io() );
        printf("vh %4d ic %4d\n", get_curr_vh(), get_curr_ic() );
        printf("\n");

        usleep(500000);
    }

    set_boost_pwm(0, BOOST_UNSYNCH);
    set_hbridge_pwm(0);

    return 0;
}
