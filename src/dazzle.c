/*
  Copyright (c) 2013
  Author: Jeff Weisberg <jaw @ tcp4me.com>
  Created: 2013-Aug-31 13:29 (EDT)
  Function: lights and sounds
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

#include "hw.h"

int volume_setting = 4;
int volume  = 16;	// music
int ivolume = 16;	// UI

int blinky_override = 0;

extern const char *songs[];
extern const char *songnames[];

/****************************************************************/

void
set_volume(int v){
    if( v > 7 ) v = 7;
    volume_setting = v;
    ivolume = volume = 1<<v;
    if( ivolume < 2 ) ivolume = 2;
}

DEFUN(volume, "set volume")
{
    if( argc == 2 ){
        set_volume( atoi(argv[1]) );
    }else if( argv[0][0] == '-' ){
        int v = menu_get_int("Volume", 0, 7, volume_setting);
        if( v != -1 ) set_volume( v );
    }else{
        printf("volume: %d\n", volume_setting);
    }

    return 0;
}

DEFUN(playsong, "play song")
{
    int n;

    if( argv[0][0] == '-' ){
        int s = atoi(argv[1]);
        int ns = atoi(argv[2]);
        n = menu_get_str("Song", ns, songnames + s, 0);
        if( n == -1 ) return 0;
        n += s;
    }else if( argc == 2 ){
        n = atoi(argv[1]);
        printf("song: %s\n", songs[n]);
    }else{
        return 1;
    }

    if( n < 0 || n > 19 ) return 0;
    play( volume, songs[n] );
    return 0;
}

void
beep(int freq, int vol, int dur){
    if( vol > 128 ) vol = 128;
    if( vol < 1  )  vol = 1;

    freq_set(HW_TIMER_AUDIO, freq);
    pwm_set(HW_TIMER_AUDIO,  vol);
    usleep(dur);
    pwm_set(HW_TIMER_AUDIO,  0);
}


void
set_led_white(int v){
    pwm_set( HW_TIMER_LED_WHITE, v & 255 );
}

void
set_led_red(int v){
    pwm_set( HW_TIMER_LED_RED, v & 255 );
}


void
set_led_green(int v){
    pwm_set( HW_TIMER_LED_GREEN, v & 255 );
}

void
set_fan1(int v){
    pwm_set( HW_TIMER_FAN1, v & 255 );
}

void
set_fan2(int v){
    pwm_set( HW_TIMER_FAN2, v & 255 );
}

DEFUN(testleds, "test leds")
{
    short i, j;

    blinky_override = 1;

    for(i=0; i<2; i++){

        for(j=0; j<255; j++){
            set_led_green(j);
            set_led_red(j);
            set_led_white(255-j);
            usleep(2000);
        }
        for(j=0; j<255; j++){
            set_led_green(255-j);
            set_led_red(255-j);
            set_led_white(j);
            usleep(2000);
        }

        play(8, "c");
        set_led_green(0);
        set_led_red(0);
        set_led_white(0);
    }

    blinky_override = 0;
}

DEFUN(testfans, "test fans")
{
    int8_t i;

    for(i=0; i<2; i++){
        set_fan1(127);
        play(8, "a");
        set_fan1(0);
        set_fan2(127);
        play(8, "b");
        set_fan2(0);
    }
}

DEFUN(setled, "set led")
{
    if( argc < 3 ){
        printf(STDERR, "setled led level\n");
        return 1;
    }

    int l = atoi(argv[1]);
    int v = atoi(argv[2]);

    if( l == 1 )      set_led_green( v );
    else if( l == 2 ) set_led_red( v );
    else              set_led_white( v );

    return 0;
}

DEFUN(setfan, "set fan speed")
{
    if( argc < 3 ){
        printf(STDERR, "setfan fan speed\n");
        return 1;
    }

    int f = atoi(argv[1]);
    int v = atoi(argv[2]);

    if( f == 1 ) set_fan1( v );
    else         set_fan2( v );

    return 0;

}

static u_char _blinky_pattern[] = {
    1, 2, 4, 8, 16,
    16, 8, 4, 2, 1,
    0, 0, 0, 0,
    1, 2, 4, 8, 16,
    16, 8, 4, 2, 1,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
};

void
update_blinky(void){
    static int i = 0;
    static int j = 0;

    if( blinky_override ) return;

    if( ++j >= 25 ){
        j = 0;
        if( ++i >= 40 ) i = 0;
    }

    set_led_white( _blinky_pattern[i] );
}

void
blinky(void){
    while(1){
        update_blinky();
        usleep(1000);
    }
}

DEFUN(image, "display image")
{
    FILE *f;

    if( argc < 2 ){
        fprintf(STDERR, "file?\n");
        return -1;
    }

    f = fopen( argv[1], "r" );
    if( !f ){
        fprintf(STDERR, "cannot open file\n");
        return -1;
    }

    FILE *fo = fopen("dev:oled0", "w");
    if( !fo ){
        fprintf(STDERR, "cannot open oled\n");
        fclose(f);
        return -1;
    }

    u_char *buf = (u_char*) fioctl(fo, IOC_GFXBUF, 0);

    if( buf ){
        fread(f, buf, 128 * 32 / 8);
        fprintf(fo, "\xB");	// flush
    }

    fclose(fo);
    fclose(f);
    return 0;
}


DEFUN(debser, "debug serial")
{

    // print wchan
    // ser SR

    sleep(2);
    return 0;
}
