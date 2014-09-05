/*
  Copyright (c) 2014
  Author: Jeff Weisberg <jaw @ tcp4me.com>
  Created: 2014-Aug-16 00:32 (EDT)
  Function: 

*/

#ifndef __gpi_hw_h__
#define __gpi_hw_h__

// the big cap
#define GPI_CAP		40	// uF


/* GPIO */

#define HW_GPIO_BUTTON		GPIO_D2
#define HW_GPIO_DPY_CD		GPIO_B12
#define HW_GPIO_SD_CS		GPIO_C6
#define HW_GPIO_DPY_CS		GPIO_C7

#define HW_GPIO_AUDIO		GPIO_B6

#define HW_GPIO_LED_WHITE	GPIO_B5
#define HW_GPIO_LED_RED		GPIO_B4
#define HW_GPIO_LED_GREEN	GPIO_A6

#define HW_GPIO_CARD		GPIO_A11
#define HW_GPIO_SWITCH		GPIO_C5
#define HW_GPIO_FAN1		GPIO_B3
#define HW_GPIO_FAN2		GPIO_A15

#define HW_GPIO_GATE_BOOST_H	GPIO_B1		/* N */
#define HW_GPIO_GATE_BOOST_L	GPIO_C8

#define HW_GPIO_GATE_HBRIDGE_L_H	GPIO_A7	/* N */
#define HW_GPIO_GATE_HBRIDGE_L_L	GPIO_A8
#define HW_GPIO_GATE_HBRIDGE_R_H	GPIO_B0	/* N */
#define HW_GPIO_GATE_HBRIDGE_R_L	GPIO_A9


/* ADC */
#define HW_ADC_VOLTAGE_INPUT	ADC_1_11	/* C1 */
#define HW_ADC_CURRENT_INPUT	ADC_1_0		/* A0 */
#define HW_ADC_VOLTAGE_HIGH	ADC_1_1		/* A1 */
#define HW_ADC_VOLTAGE_OUTPUT	ADC_1_2		/* A2 */
#define HW_ADC_CURRENT_OUTPUT	ADC_1_5		/* A5 */
#define HW_ADC_CURRENT_CTL  	ADC_1_10	/* C0 */	// ctl board current
#define HW_ADC_TEMPER_1		ADC_1_12	/* C2 */	// ctl board
#define HW_ADC_TEMPER_2		ADC_1_3		/* A3 */	// pwr board, side
#define HW_ADC_TEMPER_3		ADC_1_4		/* A4 */	// pwr board, front


/* TIMERS */

#define HW_TIMER_LED_WHITE	TIMER_3_2	/* B4 */
#define HW_TIMER_LED_RED	TIMER_3_1	/* B5  */
#define HW_TIMER_LED_GREEN	TIMER_3_1	/* A6  */

#define HW_TIMER_AUDIO		TIMER_4_1	/* B6  */

#define HW_TIMER_FAN1		TIMER_2_2
#define HW_TIMER_FAN2		TIMER_2_1

#define HW_TIMER_GATE_BOOST	TIMER_8_3
#define HW_TIMER_GATE_HBRIDGE_L	TIMER_1_1
#define HW_TIMER_GATE_HBRIDGE_R	TIMER_1_2


/*
  the accel + gyro point X towards fan, Y towards dpy

  normalize: X is forward
*/

#define ACCEL_ROTATE		270
#define GYRO_ROTATE		270

#define DEGREES2GYRO(x)     	((x) * 1000 * 100 / 7)

extern void set_motors(int, int, int, int);
extern void set_motors_noff(int, int, int, int);
extern void beep(int,int,int);
extern void play(int, const char*);
extern void set_volume(int);

extern void read_imu(void), read_imu_quick(void), read_sensors(void);
extern int  accel_x(void), accel_y(void), accel_z(void);
extern int  gyro_x(void),  gyro_y(void),  gyro_z(void);

extern void read_battery(void);
extern int  battery_voltage(void), battery_current(void);


extern int volume_setting, volume, ivolume;
extern const char const *songs[];
extern int blinky_override;
extern char _ccmem[];



#endif /* __gpi_hw_h__ */
