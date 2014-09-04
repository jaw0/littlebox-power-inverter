/*
  Copyright (c) 2013
  Author: Jeff Weisberg <jaw @ tcp4me.com>
  Created: 2013-Aug-31 13:29 (EDT)
  Function: 
*/

#include <conf.h>
#include <proc.h>
#include <i2c.h>
#include <gpio.h>
#include <stm32.h>
#include <userint.h>

#include "hw.h"

#include "lsm303.h"
#include "l3g.h"

#define I2CUNIT		0

static char have_accel=0, have_gyro=0;

static char magbuf[6];	  // x, z, y: hi,lo
static char accbuf[6];	  // x, y, z: lo,hi
static char lsmtmpbuf[2]; // hi,lo
static char gyrbuf[6];	  // x, y, z: lo, hi
static char clickbuf[1];
static char probeaccel[1], probegyro[1];

static short accel_off_x, accel_off_y, accel_off_z;
static short gyro_off_x,  gyro_off_y,  gyro_off_z;


static i2c_msg imuinit[] = {
#ifndef NO_LSM303
    I2C_MSG_C2( LSM303_ADDRESS_MAG,   0, LSM303_REGISTER_MAG_MR_REG_M,      0 ),
    I2C_MSG_C2( LSM303_ADDRESS_MAG,   0, LSM303_REGISTER_MAG_CRA_REG_M,     0x9C ),		// temp enable, 220Hz (QQQ - data rate?)
    I2C_MSG_C2( LSM303_ADDRESS_MAG,   0, LSM303_REGISTER_MAG_CRB_REG_M,     LSM303_MAGGAIN_1_3 ),

    I2C_MSG_C2( LSM303_ADDRESS_ACCEL, 0, LSM303_REGISTER_ACCEL_CTRL_REG1_A, 0x97 ),		// enable, 1.3KHz (QQQ - data rate?)
    I2C_MSG_C2( LSM303_ADDRESS_ACCEL, 0, LSM303_REGISTER_ACCEL_CTRL_REG4_A, 8 ),		// high-res (12 bit) mode
#endif

#ifndef NO_GD20
    I2C_MSG_C2( L3GD20_ADDRESS,       0, L3G_CTRL_REG1, 0xFF ),					// enable. (QQQ - data rate, bandwidth?)
    I2C_MSG_C2( L3GD20_ADDRESS,       0, L3G_CTRL_REG4, 0x20 ),					// full scale = 2000dps
#endif
};

static i2c_msg imuprobe[] = {
    I2C_MSG_C1( LSM303_ADDRESS_ACCEL, 0,             LSM303_REGISTER_ACCEL_CTRL_REG1_A ),
    I2C_MSG_DL( LSM303_ADDRESS_ACCEL, I2C_MSGF_READ, 1, probeaccel ),

    I2C_MSG_C1( L3GD20_ADDRESS,       0,             L3G_CTRL_REG4 ),
    I2C_MSG_DL( L3GD20_ADDRESS,       I2C_MSGF_READ, 1, probegyro ),

};

static i2c_msg imureadall[] = {
#ifndef NO_LSM303
    I2C_MSG_C1( LSM303_ADDRESS_MAG,   0,             LSM303_REGISTER_MAG_OUT_X_H_M ),
    I2C_MSG_DL( LSM303_ADDRESS_MAG,   I2C_MSGF_READ, 6, magbuf ),

    I2C_MSG_C1( LSM303_ADDRESS_ACCEL, 0,             LSM303_REGISTER_ACCEL_OUT_X_L_A | 0x80 ),
    I2C_MSG_DL( LSM303_ADDRESS_ACCEL, I2C_MSGF_READ, 6, accbuf ),

    I2C_MSG_C1( LSM303_ADDRESS_MAG,   0,             LSM303_REGISTER_MAG_TEMP_OUT_H_M ),
    I2C_MSG_DL( LSM303_ADDRESS_MAG,   I2C_MSGF_READ, 2, lsmtmpbuf ),
#endif

#ifndef NO_GD20
    I2C_MSG_C1( L3GD20_ADDRESS,       0,             L3G_OUT_X_L | 0x80 ),
    I2C_MSG_DL( L3GD20_ADDRESS,       I2C_MSGF_READ, 6, gyrbuf),
#endif

};

static i2c_msg imureadmost[] = {
#ifndef NO_LSM303
    I2C_MSG_C1( LSM303_ADDRESS_ACCEL, 0,             LSM303_REGISTER_ACCEL_OUT_X_L_A | 0x80 ),
    I2C_MSG_DL( LSM303_ADDRESS_ACCEL, I2C_MSGF_READ, 6, accbuf ),
#endif

#ifndef NO_GD20
    I2C_MSG_C1( L3GD20_ADDRESS,       0,             L3G_OUT_X_L | 0x80 ),
    I2C_MSG_DL( L3GD20_ADDRESS,       I2C_MSGF_READ, 6, gyrbuf),
#endif

};

static i2c_msg imureadquick[] = {
#ifndef NO_LSM303
    // NB: quicker to read x+y+z than only x+z
    I2C_MSG_C1( LSM303_ADDRESS_ACCEL, 0,             LSM303_REGISTER_ACCEL_OUT_X_L_A | 0x80 ),
    I2C_MSG_DL( LSM303_ADDRESS_ACCEL, I2C_MSGF_READ, 6, accbuf ),
#endif

#ifndef NO_GD20
    I2C_MSG_C1( L3GD20_ADDRESS,       0,             L3G_OUT_Z_L | 0x80 ),
    I2C_MSG_DL( L3GD20_ADDRESS,       I2C_MSGF_READ, 2, gyrbuf+4 ),
#endif

};

/****************************************************************/

void
init_imu(void){
    bootmsg(" imu");
    i2c_xfer(I2CUNIT, ELEMENTSIN(imuinit), imuinit, 1000000);
}

static int i2c_speed[] = { 900000, 800000, 700000, 600000, 500000, 400000, 300000, 200000, 100000 };

DEFUN(imuprobe, "probe imu")
{
    int i;
    int allmax=0, accmax=0, gyrmax=0;

    ui_pause();

    for(i=0; i<ELEMENTSIN(i2c_speed); i++){
        i2c_set_speed(I2CUNIT, i2c_speed[i] );
        i2c_xfer(I2CUNIT, ELEMENTSIN(imuinit), imuinit, 1000000);

        probeaccel[0] = probegyro[0] = 0;
        i2c_xfer(I2CUNIT, ELEMENTSIN(imuprobe), imuprobe, 100000);

        if( probeaccel[0] ){
            have_accel = 1;
            if( i2c_speed[i] > accmax ) accmax = i2c_speed[i];
        }

        if( probegyro[1] ){
            have_gyro = 1;
            if( i2c_speed[i] > gyrmax ) gyrmax = i2c_speed[i];
        }
    }

    ui_resume();
    printf("accel %d max %d\ngyro %d max %d\n", have_accel, accmax, have_gyro, gyrmax);
    return 0;
}

void
read_imu_all(void){
    i2c_xfer(I2CUNIT, ELEMENTSIN(imureadall), imureadall, 100000);
}

void
read_imu_quick(void){
    i2c_xfer(I2CUNIT, ELEMENTSIN(imureadquick), imureadquick, 100000);
}

void
read_imu_most(void){
    i2c_xfer(I2CUNIT, ELEMENTSIN(imureadmost), imureadmost, 100000);
}

/****************************************************************/
static inline int
_accel_x(void){
    short ax = ((accbuf[1]<<8) | accbuf[0]);
    ax >>= 4;
    return ax;
}

static inline int
_accel_y(void){
    short ay = ((accbuf[3]<<8) | accbuf[2]);
    ay >>= 4;
    return ay;
}

static inline int
_accel_z(void){
    short az = ((accbuf[5]<<8) | accbuf[4]);
    az >>= 4;
    return az;
}

static inline int
_gyro_x(void){
    short gz = (gyrbuf[1]<<8) | gyrbuf[0];
    return gz;
}

static inline int
_gyro_y(void){
    short gz = (gyrbuf[3]<<8) | gyrbuf[2];
    return gz;
}

static inline int
_gyro_z(void){
    short gz = (gyrbuf[5]<<8) | gyrbuf[4];
    return gz;
}

static inline int
_compass_x(void){
    short ax = ((magbuf[0]<<8) | magbuf[1]);
    return ax;
}

static inline int
_compass_y(void){
    short ay = ((magbuf[4]<<8) | magbuf[5]);
    return ay;
}

static inline int
_compass_z(void){
    short az = ((magbuf[2]<<8) | magbuf[3]);
    return az;
}
/****************************************************************/

#if ACCEL_ROTATE == 0
int accel_x(void){ return   _accel_x() - accel_off_x; }
int accel_y(void){ return   _accel_y() - accel_off_y; }
int compass_x(void){ return   _compass_x(); }
int compass_y(void){ return   _compass_y(); }

#elif ACCEL_ROTATE == 180
int accel_x(void){ return - _accel_x() + accel_off_x; }
int accel_y(void){ return - _accel_y() + accel_off_y; }
int compass_x(void){ return - _compass_x(); }
int compass_y(void){ return - _compass_y(); }

#elif ACCEL_ROTATE == 90
int accel_x(void){ return - _accel_y() + accel_off_y; }
int accel_y(void){ return   _accel_x() - accel_off_x; }
int compass_x(void){ return - _compass_y(); }
int compass_y(void){ return   _compass_x(); }

#elif ACCEL_ROTATE == 270
int accel_x(void){ return   _accel_y() - accel_off_y; }
int accel_y(void){ return - _accel_x() + accel_off_x; }
int compass_x(void){ return   _compass_y(); }
int compass_y(void){ return - _compass_x(); }


#endif

int accel_z(void){ return _accel_z() - accel_off_z; }
int compass_z(void){ return _compass_z(); }

/****************************************************************/
// An excellent angler, and now with God.
//   -- The Complete Angler., Izaak Walton.

#if GYRO_ROTATE == 0
int gyro_x(void){ return   _gyro_x() - gyro_off_x; }
int gyro_y(void){ return   _gyro_y() - gyro_off_y; }

#elif GYRO_ROTATE == 180
int gyro_x(void){ return - _gyro_x() + gyro_off_x; }
int gyro_y(void){ return - _gyro_y() + gyro_off_y; }

#elif GYRO_ROTATE == 90
int gyro_x(void){ return - _gyro_y() + gyro_off_y; }
int gyro_y(void){ return   _gyro_x() - gyro_off_x; }

#elif GYRO_ROTATE == 270
int gyro_x(void){ return   _gyro_y() - gyro_off_y; }
int gyro_y(void){ return - _gyro_x() + gyro_off_x; }

#endif

int gyro_z(void){ return _gyro_z() - gyro_off_z; }

/****************************************************************/

int compass_temp(void){
    short at = ((lsmtmpbuf[0]<<8) | lsmtmpbuf[3]);
    return at;
}

/****************************************************************/
void
imu_calibrate(void){
    int gxtot=0, gytot=0, gztot=0;
    int axtot=0, aytot=0, aztot=0;
    int n;

    gyro_off_x  = 0;
    gyro_off_y  = 0;
    gyro_off_z  = 0;
    accel_off_x = 0;
    accel_off_y = 0;
    accel_off_z = 0;

    for(n=0; n<1000; n++){
        read_imu_all();
        gxtot += _gyro_x();
        gytot += _gyro_y();
        gztot += _gyro_z();
        axtot += _accel_x();
        aytot += _accel_y();
        aztot += _accel_z() - 1000;
        usleep(1000);
    }

    gyro_off_x  = gxtot/1000;
    gyro_off_y  = gytot/1000;
    gyro_off_z  = gztot/1000;
    accel_off_x = axtot/1000;
    accel_off_y = aytot/1000;
    accel_off_z = aztot/1000;
}

/****************************************************************/

DEFUN(testimu, "test imu")
{
    while(1){
        read_imu_all();
#ifndef NO_GD20
        printf("gyro %6d %6d %6d\n", gyro_x(),    gyro_y(),    gyro_z());
#endif
        printf("accl %6d %6d %6d\n", accel_x(),   accel_y(),   accel_z());
#ifndef IGNORE_COMPASS
        printf("magn %6d %6d %6d\n", compass_x(), compass_y(), compass_z());
        printf("temp %6d\n",         compass_temp());
#endif
        printf("\n");

        if( check_button() ) break;
        if( clickbuf[0] & 0x40 ){
            play(16, "a4");
            sleep(1);
        }
        usleep(250000);
    }
    return 0;
}

DEFUN(testimu2, "raw i2c")
{

    gpio_init( GPIO_B10,  GPIO_OUTPUT | GPIO_PUSH_PULL | GPIO_SPEED_25MHZ );
    gpio_init( GPIO_B11,  GPIO_OUTPUT | GPIO_PUSH_PULL | GPIO_SPEED_25MHZ );

    while(1){
        gpio_set( GPIO_B10 );
        gpio_set( GPIO_B11 );
        sleep(1);
        gpio_clear( GPIO_B10 );
        gpio_clear( GPIO_B11 );
        sleep(1);
    }

    return 0;
}
