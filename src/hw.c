/*
  Copyright (c) 2014
  Author: Jeff Weisberg <jaw @ tcp4me.com>
  Created: 2014-Aug-14 21:06 (EDT)
  Function: 

*/

#include <conf.h>
#include <proc.h>
#include <gpio.h>
#include <pwm.h>
#include <adc.h>
#include <spi.h>
#include <i2c.h>
#include <ioctl.h>
#include <stm32.h>
#include <userint.h>

#include "util.h"
#include "hw.h"



/****************************************************************/

DEFVAR(int, bootseqno, 0, UV_TYPE_UL, "boot sequence number")

DEFUN(updateboot, "update boot data")
{
    FILE *f;

    f = fopen("bootdata.rc", "w!");
    if( f ){
        fprintf(f, "bootseqno = %d\n", bootseqno + 1);
        fprintf(f, "time      = \"%T\"\n", systime);
        fclose(f);
    }
}

unsigned int
random(void){
    return rng_get();
}

/****************************************************************/
void
update_pwm_freq(void){

    // 10kHz +/- 2kHz

    pwm_init(  HW_TIMER_LED_WHITE, 10000 - 2000 + random_n(4000), 255 );
    pwm_init(  HW_TIMER_FAN1,      10000 - 2000 + random_n(4000), 255 );

}


/****************************************************************/

/* NB: timers1+2 => AF(1), timers3+4+5 => AF(2) */

void
hw_init(void){

    bootmsg("hw init");

    // enable i+d cache, prefecth=off => faster + lower adc noise
    // nb: prefetch=on => more faster, less power, more noise
    FLASH->ACR  |= 0x600;

    // T1,2 => AF(1); T3,4,5 => AF(2), else AF(3)

    // beeper
    gpio_init( HW_GPIO_AUDIO, GPIO_AF(2) | GPIO_SPEED_2MHZ );
    pwm_init(  HW_TIMER_AUDIO, 440, 255 );
    pwm_set(   HW_TIMER_AUDIO, 0);

    // LEDs
    gpio_init( HW_GPIO_LED_WHITE,  GPIO_AF(2) | GPIO_SPEED_2MHZ );
    pwm_init(  HW_TIMER_LED_WHITE, 10000, 255 );
    pwm_set(   HW_TIMER_LED_WHITE, 0);
    gpio_init( HW_GPIO_LED_RED,    GPIO_AF(2) | GPIO_SPEED_2MHZ );
    pwm_set(   HW_TIMER_LED_RED,   0);
    /*
    gpio_init( HW_GPIO_LED_GREEN,  GPIO_AF(2) | GPIO_SPEED_2MHZ );
    pwm_set(   HW_TIMER_LED_GREEN, 0);
    */
    gpio_init( HW_GPIO_LED_GREEN,  GPIO_OUTPUT | GPIO_PUSH_PULL | GPIO_SPEED_2MHZ );

    // fans
    gpio_init( HW_GPIO_FAN1, GPIO_AF(1) | GPIO_SPEED_2MHZ );
    gpio_init( HW_GPIO_FAN2, GPIO_AF(1) | GPIO_SPEED_2MHZ );
    pwm_init(  HW_TIMER_FAN1, 10000, 255 );

    // ...

    init_gates();	// start me first
    init_sensors();
    init_timer();
    init_imu();
    init_inverter();	// start me last
    bootmsg("\n");

    if( run_script("bootdata.rc") ){
        // could not run rc - play warning tone
        play(32, "[3 d+4>>d-4>> ]");
    }
    ui_f_updateboot(0,0,0);
    mkdir("log");
}

void
onpanic(const char *msg){
    int i;

    set_gates_safe();

    set_led_white( 0xFF );
    set_led_red(   0xFF );
    set_led_green( 0 );

    freq_set( HW_TIMER_AUDIO, 200 );
    pwm_set(  HW_TIMER_AUDIO, 0x7F );

    currproc = 0;
    splproc();
    ssd13060_puts("\e[J\e[16mPANIC\r\n\e[15m");
    ssd13060_puts(msg);
    ssd13060_puts("\r\n");
    ssd13060_puts(0);
    splhigh();

    while(1){
        set_led_white( 0xFF );
        freq_set( HW_TIMER_AUDIO, 150 );
        for(i=0; i<20000000; i++){ asm("nop"); }

        set_led_white( 0 );
        freq_set( HW_TIMER_AUDIO, 250 );
        for(i=0; i<20000000; i++){ asm("nop"); }
    }
}

/****************************************************************/

void
main(void){

    hw_init();

    extern void blinky(void);
    start_proc( 1024, blinky, "blinky" );

    // return to shell
}
