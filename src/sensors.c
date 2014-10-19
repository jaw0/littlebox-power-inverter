/*
  Copyright (c) 2014
  Author: Jeff Weisberg <jaw @ tcp4me.com>
  Created: 2014-Aug-14 23:03 (EDT)
  Function: analog instrumentation

*/

#include <conf.h>
#include <proc.h>
#include <gpio.h>
#include <adc.h>
#include <nvic.h>
#include <pwm.h>
#include <locks.h>
#include <stm32.h>
#include <userint.h>

#include "util.h"
#include "hw.h"

#define CRUMBS	"adc"
#include "crumbs.h"

#define SR_EOC		2
#define SR_OVR		(1<<5)
#define CR2_SWSTART	0x40000000
#define CR2_ADON	1
#define ADC_VREFINT	ADC_1_17


static lock_t adclock;
static short dmabuf[30];
static short adcstate;
#define ADC_DONE	 0
#define ADC_BUSY	 1
#define ADC_ERROR	-1


// sensor order
#define SEN_0		HW_ADC_VOLTAGE_HIGH
#define SEN_1		HW_ADC_VOLTAGE_OUTPUT
#define SEN_2		HW_ADC_VOLTAGE_INPUT
#define SEN_3		HW_ADC_CURRENT_INPUT
#define SEN_4		HW_ADC_CURRENT_OUTPUT
#define SEN_5		HW_ADC_CURRENT_CTL
#define SEN_6		HW_ADC_TEMPER_1
#define SEN_7		HW_ADC_TEMPER_2
#define SEN_8		HW_ADC_TEMPER_3

/****************************************************************/

// recent sensor values
struct SensorData {
    u_short val;
    u_short lpf;
    u_short hist[3];
};

static struct SensorData sensor_data[9];

int offset_io, offset_ii;


/****************************************************************/

#include "adc_wait.c"
//#include "adc_asyncirq.c"	- too slow
//#include "adc_syncirq.c"	- overruns
//#include "adc_dma.c"	     // - overruns
//#include "adc_dma2.c"	     // - scrambled data

/****************************************************************/


static void
median_filter(struct SensorData *sd){
    short *hist = sd->hist;
#if 0
    sd->val = hist[0];
#else
    // val = median of 3
    if( hist[0] > hist[1] ){
        if( hist[2] > hist[0] )      sd->val = hist[0];
        else if( hist[2] > hist[1] ) sd->val = hist[2];
        else                         sd->val = hist[1];
    }else{
        if( hist[2] > hist[1] )      sd->val = hist[1];
        else if( hist[2] > hist[0] ) sd->val = hist[2];
        else                         sd->val = hist[0];
    }
#endif
}

static inline void
lowpass_filter(struct SensorData *sd){
    sd->lpf = (sd->val + 3 * sd->lpf) >> 2;
}

void
read_sensors(void){
    // read 3x, median filter

    // pick one low-prio sensor to read
    char r = random() & 3, sr;
    switch(r){
    case 0: sr = SEN_5; break;
    case 1: sr = SEN_6; break;
    case 2: sr = SEN_7; break;
    case 3: sr = SEN_8; break;
    }

    adc_get_all(sr, 5+r);

    // unbundle data
    int8_t i,j;
    for(i=0; i<6; i++){
        int si = (i==5) ? 5 + r : i;
#ifdef USE_DMABUF
        for(j=0; j<3; j++){
            sensor_data[si].hist[j] = dmabuf[j*6+i];
        }
#endif
        median_filter(sensor_data + si);
        lowpass_filter(sensor_data + si);
    }

    // emergency fail-safe
    safety_check_boost();
}

static void
calibrate(void){
    int i, to=0, ti=0, tc=0;

    // prime
    for(i=0;i<100;i++) read_sensors();

    // actual io is 0 while idle
    // actual ii ~ ic while idle
    // in Rev.1 ic should be precisely known (but scaled differently)

    for(i=0; i<100; i++){
        read_sensors();
        ti += sensor_data[3].val;	// ii
        to += sensor_data[4].val;	// io
        tc += sensor_data[5].val;	// ic
    }
    offset_io = to / 100;
    offset_ii = ti / 100; // RSN (ti - tc) / 100;

    // bootmsg("ical %d %d\n", offset_ii, offset_io);
}


// returns .1 degrees C
static inline int
temperature(int v){
    // Rev 0
    // sensor is a TC1047
    // 10mV/C
    // 500mV = 0C
    // Rev 1 => LM61, 10mV/C, +600mV

    return ((v * 3300 + 2048) >> 12) - 600;
}

/*     adc * Rt * 3.3
  V  = --------------  << 20   =  Rt(in k) * 256
        3.3k * 4096


        where Rt = Rx + 3.3
*/

// RSN - convert to proper calibrated units V:Q.4, A:Q.8
// RSN - calibrate + temperature compensate
int get_curr_vh(void){    return (sensor_data[0].val * 507660) >> 16; }		// measured 20141018
int get_lpf_vh(void){     return (sensor_data[0].lpf * 507660) >> 16; }		// "
int get_curr_vo(void){    return (sensor_data[1].val * 150678) >> 16; } 	// measured 20141015
int get_lpf_vo(void){     return (sensor_data[1].lpf * 150678) >> 16; } 	// "
int get_curr_vi(void){    return 192;    return sensor_data[2].val; }		// not hooked up
int get_lpf_vi(void){     return 192;    return sensor_data[2].val; }		// not hooked up

int get_curr_ii(void){    return ((sensor_data[3].val - offset_ii) * 100663) >> 14; }
int get_lpf_ii(void){     return ((sensor_data[3].lpf - offset_ii) * 100663) >> 14; }
int get_curr_io(void){    return ((sensor_data[4].val - offset_io) * 100663) >> 14; }
int get_lpf_io(void){     return ((sensor_data[4].lpf - offset_io) * 100663) >> 14; }

int get_curr_ic(void){
    // LMP8640 Gain = 100, R = 0.01
    // mA = v * 3.3/4096 * 1000
    // Q8 = v * 3.3/4096 * 256
    return (sensor_data[5].val * 845 + 2048) >> 12;
}
int get_lpf_ic(void){	return (sensor_data[5].lpf * 845 + 2048) >> 12; }


int get_curr_temp1(void){ return temperature(sensor_data[6].val); }
int get_curr_temp2(void){ return temperature(sensor_data[7].val); }
int get_curr_temp3(void){ return temperature(sensor_data[8].val); }

int
get_max_temp(void){
    // determine hottest temp
    int t  = get_curr_temp1();
    int tt = get_curr_temp2();
    if( tt > t ) t = tt;
    tt = get_curr_temp3();
    if( tt > t ) t = tt;
    return t;
}

DEFUN(testsensors, "test sensors")
{
    short i, j;

    while(1){
        read_sensors();
        printf(" vh   vo   vi   ii   io   ic   t1   t2   t3\n");
        for(i=0; i<3; i++){
            for(j=0; j<9; j++){
                printf("%04.4x ", sensor_data[j].hist[i]);
            }
            printf("\n");
        }
        for(i=0; i<3; i++){
            for(j=0; j<6; j++){
                printf("%04.4x ", dmabuf[i*6 + j]);
            }
            printf("\n");
        }

        printf("\n");
        sleep(1);
    }
}

DEFUN(testpowersensors, "test power sensors")
{
    while(1){
        read_sensors();
        printf("vi %4d ii %4d\n", get_curr_vi(), get_curr_ii() );
        printf("vo %4d io %4d\n", get_curr_vo(), get_curr_io() );
        printf("vh %4d ic %4d\n", get_curr_vh(), get_curr_ic() );
        printf("\n");
        usleep(10000);
    }
}

DEFUN(testtempsensors, "test temperature sensors")
{
    while(1){
        read_sensors();
        printf("%4d\n", get_curr_temp1());
        printf("%4d\n", get_curr_temp2());
        printf("%4d\n", get_curr_temp3());
        printf("\n");
        usleep(10000);
    }
}

DEFUN(testctlsens, "test ctl board sensors")
{
    while(1){
        read_sensors();
        printf("%4d\n", get_curr_ic());
        printf("%4d\n", get_curr_vi());
        printf("%4d\n", get_curr_temp1());
        printf("\n");
        usleep(10000);
    }
}

void
init_sensors(void){

    // 0 => 3 cycles
    // 1 => 15 cycles
    // 2 => 28 cycles
    // 3 => 56 cycles
    // 4 => 84 cycles

    //  3        => 1.8
    // 15 cycles => 2.3 usec
    // 28        => 2.8

    bootmsg(" sensors");

    adc_init2( HW_ADC_VOLTAGE_INPUT,    1);
    adc_init2( HW_ADC_CURRENT_INPUT,    1);
    adc_init2( HW_ADC_VOLTAGE_OUTPUT,   1);
    adc_init2( HW_ADC_CURRENT_OUTPUT,   1);
    adc_init2( HW_ADC_VOLTAGE_HIGH,     1);
    adc_init2( HW_ADC_CURRENT_CTL,      1);
    adc_init2( HW_ADC_TEMPER_1,         1);
    adc_init2( HW_ADC_TEMPER_2,         1);
    adc_init2( HW_ADC_TEMPER_3,         1);

    ADC->CCR |= 0x00C00000;	// enable Vrefint
    adc_init( ADC_VREFINT, 1 );

    _adc_init();

    calibrate();
}

