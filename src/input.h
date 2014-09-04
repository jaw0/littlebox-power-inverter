/*
  Copyright (c) 2014
  Author: Jeff Weisberg <jaw @ tcp4me.com>
  Created: 2014-Apr-29 12:28 (EDT)
  Function: 

*/

#ifndef __input_h__
#define __input_h__

#define WAITFOR_SENSOR_L_F	(1<<SENSOR_L_F)
#define WAITFOR_SENSOR_R_F	(1<<SENSOR_R_F)
#define WAITFOR_SENSOR_L_S	(1<<SENSOR_L_S)
#define WAITFOR_SENSOR_R_S	(1<<SENSOR_R_S)
#define WAITFOR_SENSOR_L_D	(1<<SENSOR_L_D)
#define WAITFOR_SENSOR_R_D	(1<<SENSOR_R_D)

#define WAITFOR_BUTTON		0x100
#define WAITFOR_DTAP		0x200
#define WAITFOR_TTAP		0x400
#define WAITFOR_UPDN		0x800

extern int  wait_for_action(int, int);

#define wait_for_wave()		wait_for_action( WAITFOR_SENSOR_L_F | WAITFOR_SENSOR_R_F, -1 )
#define wait_for_tap()		wait_for_action( WAITFOR_DTAP, -1 )
#define wait_for_3tap()		wait_for_action( WAITFOR_TTAP, -1 )
#define wait_for_button()	wait_for_action( WAITFOR_BUTTON, -1 )


#endif /* __input_h__ */
