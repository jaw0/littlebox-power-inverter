/*
  Copyright (c) 2013
  Author: Jeff Weisberg <jaw @ tcp4me.com>
  Created: 2013-Apr-18 20:28 (EDT)
  Function: menus, accelerometer for user input
*/


#include <conf.h>
#include <proc.h>
#include <gpio.h>
#include <pwm.h>
#include <sys/types.h>
#include <ioctl.h>
#include <userint.h>

#include "menu.h"

#define AUTOREPEAT_X	250000
#define AUTOREPEAT_Y	1000000
#define TILT_MIN	500		// ~ 30 degree tilt


/*
u+80	arrowleft
u+81	arrowup
u+82	arrowright
u+83	arrowdown
u+84	arrowupdn
u+85	arrowboth
*/

extern void beep(int, int, int);
extern int ivolume;

static int menu_paused = 0;


void
ui_sleep(void){
    // low power

    FILE *f = fopen("dev:oled0", "w");
    fioctl(f, IOC_GFXSLEEP, 1);		// oled sleep mode
    RCC->AHB1ENR &= ~(1<<21);		// disable DMA1
    menu_paused = 2;
}

void
ui_awake(void){

    RCC->AHB1ENR |= 1<<21;
    FILE *f = fopen("dev:oled0", "w");
    fioctl(f, IOC_GFXSLEEP, 0);
    menu_paused = 0;
}

void
ui_pause(void){
    // prevent imu usage during disk access
    menu_paused = 1;
}

void
ui_resume(void){
    if( menu_paused > 1 ) ui_awake();
    menu_paused = 0;
}

/****************************************************************/

DEFUN(uisleep, "test ui sleep")
{
    ui_sleep();
}
DEFUN(uiwake, "test ui sleep")
{
    ui_awake();
}

/****************************************************************/

static clear_line(int dir){
    int i;

    for(i=0; i<26; i++){
        if( dir > 0 )
            printf("\e[<");
        else
            printf("\e[>");
        printf("\xB");
        usleep(10000);
    }
}

static int
get_input(void){
    static int     prevach = 0;
    static utime_t prevat  = 0;
    utime_t now;
    char flatct = 0;
    char leftct = 0;
    char rightct = 0;
    char upct = 0;
    char dnct = 0;

    while(1){
        if( menu_paused ){
            usleep(100000);
            continue;
        }

        // button = enter
        if( check_button() ){
            return '*';
        }

        read_imu_all();
        int ax = accel_x();
        int ay = accel_y();
        now = get_time();

        // flat?
        if( ax < TILT_MIN && ax > -TILT_MIN && ay < TILT_MIN && ay > -TILT_MIN )
            flatct ++;
        else
            flatct = 0;

        if( ax > TILT_MIN )
            upct ++;
        else
            upct = 0;

        if( ax < -TILT_MIN )
            dnct ++;
        else
            dnct = 0;

        if( ay > TILT_MIN )
            leftct ++;
        else
            leftct = 0;

        if( ay < -TILT_MIN )
            rightct ++;
        else
            rightct = 0;

        if( flatct > 10 ){
            prevach = 0;
            prevat  = 0;
        }

        if( rightct > 10 )
            if( !prevach || (prevach == '>' && now - prevat > AUTOREPEAT_X) ){
                beep(400, ivolume, 100000);
                prevat  = now;
                return prevach = '>';
            }

        if( leftct > 10 )
            if( !prevach || (prevach == '<' && now - prevat > AUTOREPEAT_X) ){
                beep(400, ivolume, 100000);
                prevat  = now;
                return prevach = '<';
            }

        if( dnct > 10 )
            if( !prevach || (prevach == '*' && now - prevat > AUTOREPEAT_Y) ){
                beep(600, ivolume, 100000);
                beep(300, ivolume, 150000);
                prevat  = now;
                return prevach = '*';
            }

        if( upct > 10 )
            if( !prevach || (prevach == '^' && now - prevat > AUTOREPEAT_Y) ){
                beep(300, ivolume, 100000);
                beep(320, ivolume, 150000);
                prevat  = now;
                return prevach = '^';
            }

        usleep(10000);
    }
}



static const struct MenuOption *
domenu(const struct Menu *m){
    int nopt  = 0;
    int nopts = 0;

    // count options
    for(nopts=0; m->el[nopts].type; nopts++) {}

    // display in big font, top line: "title (count)     arrow"
    printf("\e[J\e[16m%s(%d)\e[-1G\x84\n\e[16m", m->title, nopts);

    if( m->startval ) nopt = * m->startval;

    while(1){
        // display current choice, bottom line: "choice     arrow"
        printf("\e[0G%s\e[99G%c\xB", m->el[nopt].text,
               (nopt==0 && nopts==1) ? ' ' :
               (nopt==0) ? '\x82' :
               (nopt==nopts-1) ? '\x80' : '\x85'
            );

        // get input
        int ch = get_input();

        switch(ch){
        case '<':
            nopt --;
            if( nopt < 0 ){
                nopt = 0;
                beep(200, ivolume, 500000);
            }
            clear_line(-1);
            break;
        case '>':
            nopt ++;
            if( nopt > nopts - 1 ){
                nopt = nopts - 1;
                beep(200, ivolume, 500000);
            }
            clear_line(1);
            break;
        case '^':
            return (void*)-1;
        case '*':
            return (void*) & m->el[nopt];
        }
    }
}



void
menu(const struct Menu *m){
    const struct MenuOption *opt;
    const void *r;

    // QQQ - wait until level?

    while(1){
        if( !m ) return;

        opt = domenu(m);

        if( opt == (void*)-1 || opt == 0 ){
            r = (void*)opt;
        }else{
            switch(opt->type){
            case MTYP_EXIT:
                return;
            case MTYP_MENU:
                r = opt->action;
                break;
            case MTYP_FUNC:
                printf("\e[10m\e[J");
                r = ((void*(*)(int,const char**,void*))opt->action)(opt->argc, opt->argv, 0);
                // stay or back
                if( r ) r = (void*)-1;
                break;
            }
        }

        if( r == 0 ){
            // stay here
        }
        else if( r == (void*)-1 ){
            // go back
            m = m->prev;
        }else{
            // new menu
            m = (const struct Menu*)r;
        }
    }
}

/****************************************************************/

/* return user entered number */
int
menu_get_int(const char *prompt, int min, int max, int start){
    int nopt  = start;
    int nopts = max - min + 1;

    // display in big font, top line: "title (count)     arrow"
    printf("\e[J\e[16m%s(%d)\e[-1G\x84\n\e[16m", prompt, nopts);

    while(1){
        // display current choice, bottom line: "choice     arrow"
        printf("\e[0G%d\e[99G%c\xB", nopt,
               (nopt==0 && nopts==1) ? ' ' :
               (nopt==0) ? '\x82' :
               (nopt==nopts-1) ? '\x80' : '\x85'
            );

        // get input
        int ch = get_input();

        switch(ch){
        case '<':
            nopt --;
            if( nopt < 0 ){
                nopt = 0;
                beep(200, ivolume, 500000);
            }
            clear_line(-1);
            break;
        case '>':
            nopt ++;
            if( nopt > nopts - 1 ){
                nopt = nopts - 1;
                beep(200, ivolume, 500000);
            }
            clear_line(1);
            break;
        case '^':
            return -1;
        case '*':
            return nopt;
        }
    }
}

/* return user entered string */
int
menu_get_str(const char *prompt, int nopts, const char **argv, int start){
    int nopt  = start;

    // display in big font, top line: "title (count)     arrow"
    printf("\e[J\e[16m%s(%d)\e[-1G\x84\n\e[16m", prompt, nopts);

    while(1){
        // display current choice, bottom line: "choice     arrow"
        printf("\e[0G%s\e[99G%c\xB", argv[nopt],
               (nopt==0 && nopts==1) ? ' ' :
               (nopt==0) ? '\x82' :
               (nopt==nopts-1) ? '\x80' : '\x85'
            );

        // get input
        int ch = get_input();

        switch(ch){
        case '<':
            nopt --;
            if( nopt < 0 ){
                nopt = 0;
                beep(200, ivolume, 500000);
            }
            clear_line(-1);
            break;
        case '>':
            nopt ++;
            if( nopt > nopts - 1 ){
                nopt = nopts - 1;
                beep(200, ivolume, 500000);
            }
            clear_line(1);
            break;
        case '^':
            return -1;
        case '*':
            return nopt;
        }
    }
}
