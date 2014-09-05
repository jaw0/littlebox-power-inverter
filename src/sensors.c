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

#define DMAC		DMA2_Stream0

#define SR_EOC		2
#define SR_OVR		(1<<5)
#define CR2_SWSTART	0x40000000
#define CR2_ADON	1

#define DMASCR_MINC	(1<<10)
#define DMASCR_DIR_M2P  (1<<6)
#define DMASCR_PFCTRL	(1<<5)
#define DMASCR_TEIE	4
#define DMASCR_TCIE	16
#define DMASCR_EN	1


static lock_t adclock;
static short dmabuf[27];
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
    u_short hist[3];
};

static struct SensorData sensor_data[9];

/****************************************************************/

#include "adc_wait.c"
//#include "adc_asyncirq.c"	- too slow
//#include "adc_syncirq.c"	- overruns
//#include "adc_dma.c"		- overruns

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
    }
}

// returns .1 degrees C
static inline int
temperature(int v){
    // sensor is a TC1047
    // 10mV/C
    // 500mV = 0C

    return v; //return ((v * 3300 + 2048) >> 12) - 500;
}


// RSN - convert to proper calibrated units V:Q.4, A:Q.8
int get_curr_vh(void){    return sensor_data[0].val; }
int get_curr_vo(void){    return sensor_data[1].val; }
int get_curr_vi(void){    return sensor_data[2].val; }

int get_curr_ii(void){    return sensor_data[3].val; }
int get_curr_io(void){    return sensor_data[4].val; }
int get_curr_ic(void){    return sensor_data[5].val; }

int get_curr_temp1(void){ return temperature(sensor_data[6].val); }
int get_curr_temp2(void){ return temperature(sensor_data[7].val); }
int get_curr_temp3(void){ return temperature(sensor_data[8].val); }

DEFUN(testsensors, "test sensors")
{
    short i, j;

    while(1){
        read_sensors();
        for(i=0; i<3; i++){
            for(j=0; j<9; j++){
                printf("%04.4x ", sensor_data[j].hist[i]);
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

    adc_init2( HW_ADC_VOLTAGE_INPUT,    2);
    adc_init2( HW_ADC_CURRENT_INPUT,    2);
    adc_init2( HW_ADC_VOLTAGE_OUTPUT,   2);
    adc_init2( HW_ADC_CURRENT_OUTPUT,   2);
    adc_init2( HW_ADC_VOLTAGE_HIGH,     2);
    adc_init2( HW_ADC_CURRENT_CTL,      2);
    adc_init2( HW_ADC_TEMPER_1,         2);
    adc_init2( HW_ADC_TEMPER_2,         2);
    adc_init2( HW_ADC_TEMPER_3,         2);

    _adc_init();
}
