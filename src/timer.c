/*
  Copyright (c) 2013
  Author: Jeff Weisberg <jaw @ tcp4me.com>
  Created: 2013-Dec-01 00:19 (EST)
  Function: use timer for delay loops

*/

#include <conf.h>
#include <proc.h>
#include <gpio.h>
#include <pwm.h>
#include <stm32.h>
#include <userint.h>

#include "hw.h"
#include "gpiconf.h"
#include "inverter.h"

//#define CRUMBS "timer"
#include "crumbs.h"

#define TIMCLOCK	84000000

#define TIMER	TIM7
#define CR1_CEN	1
#define CR1_URS 4
#define CR1_OPM 8


void
init_timer(void){

    bootmsg(" timer");

    RCC->APB1ENR |= 1<<5;	// T7
    nvic_enable(55, 0x60);	// T7

    TIMER->SR &= ~1;
    TIMER->DIER = 1;		// interrupt enable

    TIMER->PSC = 0;		// no prescaler
    // NB - controlfreq was chosen to make this exact:
    TIMER->ARR = TIMCLOCK / CONTROLFREQ - 1;

    TIMER->CR1 |= (1<<7)	// ARR is buffered
        | (1<<2)		// int on over/under flow
        ;

    TIMER->CR1 |= CR1_CEN;	// GO!

}

//         tsleep( TIMER, -1, "timer", 100000);


void
TIM7_IRQHandler(void){

    DROP_CRUMB("irq", TIMER->SR, 0);
    if( !(TIMER->SR & 1) ) return ;

    TIMER->SR  &= ~1;		// clear UIF

    cycle_step = (cycle_step + 1) % CYCLESTEPS;
    half_cycle_step = cycle_step % HALFCYCLESTEPS;

    wakeup( TIMER );
}

DEFUN(testtimer, "test ctl timer")
{
    int i;
    int tt=0;
    int nl=0, nto=0, ny=0;

    currproc->flags |= PRF_REALTIME;
    currproc->prio = 1;

    utime_t t0 = get_hrtime();
    RESET_CRUMBS();

    for(i=0; i<100000; i++){
        irq_disable();
        utime_t t1t = get_time();
        int     t1s = SysTick->VAL;
        utime_t t1  = get_hrtime();
        irq_enable();
        //currproc->nyield = 0;
        DROP_CRUMB("tsleep", 0,0);
        int ts = tsleep( TIMER, -1, "timer", 100000);

        irq_disable();
        utime_t t2t = get_time();
        int     t2s = SysTick->VAL;
        utime_t t2  = get_hrtime();
        irq_enable();

        int dt = t2 - t1;
        if( ts ) nto ++;
        //if( currproc->nyield != 1 ){
        //    ny ++;
        //    //printf("ny %d %s\n", currproc->nyield, currproc->prevwchan);
        //}
        tt += dt;
        if( dt > 100 ){
            nl ++;
            printf("dt %d; %d[%d %d] %d[%d %d]\n", dt, (int)t1, (int)t1t, t1s,
                   (int)t2, (int)t2t, (int) t2s);
        //    usleep(1000);
            DUMP_CRUMBS();
            RESET_CRUMBS();
        }

    }

    int dt = get_hrtime() - t0;
    printf("TT: %d; %d; nl %d, nto %d, ny %d\n", tt, dt, nl, nto, ny);
    return 0;
}
