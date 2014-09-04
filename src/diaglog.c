/*
  Copyright (c) 2014
  Author: Jeff Weisberg <jaw @ tcp4me.com>
  Created: 2014-Apr-06 22:23 (EDT)
  Function: diag log

*/
#include <conf.h>
#include <proc.h>
#include <userint.h>
#include <nstdio.h>
#include <stm32.h>
#include <stdarg.h>

#include "gpiconf.h"
#include "util.h"
#include "hw.h"
#include "inverter.h"
#include "defproto.h"

// store data in ccm (64k)
static u_char *logmem = (u_char*)_ccmem;
static volatile int logpos = 0;
static int logstep = 0;
static int savepos = 0;
static int logend  = 0;
static int8_t logmode = 0;
static volatile proc_t logproc_pid   = 0;
static volatile int8_t logproc_close = 0;
static volatile int8_t logproc_pause = 0;

#define LOGSIZE		65536
#define BUFSIZE		8192
#define ROOMFOR(x)	(logpos < LOGSIZE - (x))

// temp buffer
static u_char bbuf[BUFSIZE];

// current version of the data format
#define LF_VERSION	2

// format:
//   <5bit type> <3bit length>
//   [<8 bit length>]
//   data...

#define MKTAG(tag, len)	(((tag)<<3) | (len))

#define LRL_VAR		0	// 8bit length follows
#define LRL_2B		1
#define LRL_4B		2
#define LRL_6B		3
#define LRL_8B		4
#define LRL_10B		5
#define LRL_12B		6
#define LRL_14B		7

#define LRT_VERSN	0
#define LRT_MSG		1
#define LRT_SENSOR	2
#define LRT_GYRO	3
#define LRT_ACCEL	4
#define LRT_ENV		5
#define LRT_CYCLE	6
#define LRT_CTL		7
#define LRT_USER	8
#define LRT_STAMP	12
#define LRT_SLOW        13
#define LRT_TAG		14
#define LRT_LOGINFO	19


struct control_dat {
    short step,  vtarg;
    short ierr,  oerr;
    short iadj,  oadj;
};

struct sensor_dat {
    short vi, ii, vo, io, vh;
};

struct cycle_dat {
    short ii_ave, ii_min, ii_max;
    short vh_ave, vh_min, vh_max;
    short itarg;
};

struct env_dat {
    short ic, t1, t2, t3;
};

struct version_dat {
    u_short vers;
    // + calibration data
    u_short cell;
    u_short gyro;
};

//****************************************************************

static inline void
_lock(void){
    irq_disable();
}

static inline void
_unlock(void){
    irq_enable();
}

//****************************************************************

static void
_write_version(FILE *f){
    struct version_dat d;

    fputc( MKTAG(LRT_VERSN, LRL_6B), f);

    d.vers = LF_VERSION;
#ifdef M5_471
    d.cell = CELLWIDTH;
#else
    d.cell = 0;
#endif
    d.gyro = DEGREES2GYRO(1);
    fwrite(f, (u_char*)&d, 6);
}

static int
_version_is_ok(void){
    return (bbuf[0] == LF_VERSION) ? 1 : 0;
}

static void
_log_version(void){
    if( !ROOMFOR(7) ) return;
    _lock();
    u_char *lc = logmem + logpos;
    lc[0] = MKTAG(LRT_VERSN, LRL_6B);

    struct version_dat *d = (struct version_dat*)(lc + 1);
    d->vers = LF_VERSION;
    d->gyro = DEGREES2GYRO(1);
    d->cell = 0;

    logpos += 7;
    _unlock();
}

static void
_log_sensor(void){
    if( !ROOMFOR(10) ) return;
    _lock();
    u_char *lc = logmem + logpos;
    lc[0] = MKTAG(LRT_SENSOR, LRL_10B);

    struct sensor_dat *d = (struct sensor_dat*)(lc + 1);
    d->vi = curr_vi;
    d->ii = curr_ii;
    d->vo = curr_vo;
    d->io = curr_io;
    d->vh = curr_vh;

    logpos += 11;
    _unlock();
}

static inline void
_show_sensor(FILE *f){
    struct sensor_dat *p = (struct sensor_dat*)bbuf;

    fprintf(f, "sensor\tvi=%d i=%d vo=%d io=%d vh=%d",
            p->vi, p->ii, p->vo, p->io, p->vh
    );
}

_log_cycle(void){
    if( !ROOMFOR(15) ) return;
    _lock();
    u_char *lc = logmem + logpos;
    lc[0] = MKTAG(LRT_CYCLE, LRL_14B);

    struct cycle_dat *d = (struct cycle_dat*)(lc + 1);
    d->ii_ave = pcy_ii_ave;
    d->ii_min = pcy_ii_min;
    d->ii_max = pcy_ii_max;
    d->vh_ave = pcy_vh_ave;
    d->vh_min = pcy_vh_min;
    d->vh_max = pcy_vh_max;
    d->itarg  = input_itarg;

    logpos += 15;
    _unlock();
}

static inline void
_show_cycle(FILE *f){
    struct cycle_dat *p = (struct cycle_dat*)bbuf;

    fprintf(f, "cycle\tit=%d ii=%d/%d/%d vh=%d/%d/%d",
            p->itarg,
            p->ii_min, p->ii_ave, p->ii_max,
            p->vh_min, p->vh_ave, p->vh_max
    );
}

static void
_log_control(void){
    if( !ROOMFOR(11) ) return;
    _lock();
    u_char *lc = logmem + logpos;
    lc[0] = MKTAG(LRT_CTL, LRL_12B);

    struct control_dat *d = (struct control_dat*)(lc + 1);
    d->step  = cycle_step;
    d->vtarg = output_vtarg;
    d->ierr  = input_err;
    d->iadj  = input_adj;
    d->oerr  = output_err;
    d->oadj  = output_adj;

    logpos += 13;
    _unlock();
}

static inline void
_show_control(FILE *f){
    struct control_dat *p = (struct control_dat*)bbuf;

    fprintf(f, "control cs=%d e=%d a=%d, vt=%d e=%e a=%d",
            p->step,  p->ierr, p->iadj,
            p->vtarg, p->oerr, p->oadj
    );
}

static void
_log_env(void){
    if( !ROOMFOR(9) ) return;
    _lock();
    u_char *lc = logmem + logpos;
    lc[0] = MKTAG(LRT_ENV, LRL_8B);

    struct env_dat *d = (struct env_dat*)(lc + 1);
    d->ic = get_curr_ic();
    d->t1 = get_curr_temp1();
    d->t2 = get_curr_temp2();
    d->t3 = get_curr_temp3();

    logpos += 9;
    _unlock();
}

static inline void
_show_env(FILE *f){
    struct env_dat *p = (struct env_dat*)bbuf;

    fprintf(f, "env\tic=%d t=%d %d %d",
            p->ic, p->t1, p->t2, p->t3
    );
}

static void
_log_gyro(void){
    if( !ROOMFOR(7) ) return;
    _lock();
    u_char *lc = logmem + logpos;
    lc[0] = MKTAG(LRT_GYRO, LRL_6B);

    short *ls = (short*)(lc + 1);
    ls[0] = gyro_x();
    ls[1] = gyro_y();
    ls[2] = gyro_z();

    logpos += 7;
    _unlock();
}

static inline void
_show_gyro(FILE *f){
    short *ls = (short*)bbuf;

    fprintf(f, "gyro\t%5d %5d %5d", ls[0], ls[1], ls[2]);
}

static void
_log_accel(void){
    if( !ROOMFOR(5) ) return;
    _lock();
    char *lc = logmem + logpos;
    lc[0] = MKTAG(LRT_ACCEL, LRL_4B);
    lc[1] = accel_x() / 8;
    lc[2] = accel_y() / 8;
    lc[3] = accel_z() / 8;
    // + pad

    logpos += 5;
    _unlock();
}

static inline void
_show_accel(FILE *f){

    fprintf(f, "accel\t%5d %5d %5d", bbuf[0] * 8, bbuf[1] * 8, bbuf[2] * 8);
}


static void
_log_stamp(void){
    static utime_t prevt = 0;
    utime_t now = get_hrtime();

    if( now < prevt + 500 ) return;
    prevt = now;

    if( !ROOMFOR(5) ) return;
    _lock();
    u_char *lc = logmem + logpos;
    lc[0] = MKTAG(LRT_STAMP, LRL_4B);

    u_long *ll = (u_long*)(lc + 1);
    ll[0] = now;

    logpos += 5;
    _unlock();
}

static inline void
_show_stamp(FILE *f){
    u_long *ll = (u_long*)bbuf;

    fprintf(f, "tstamp\t%x", ll[0]);
}

static void
_log_tag(int tag){
    static int prevtag = 0;
    if( tag == prevtag ) return;

    if( !ROOMFOR(3) ) return;
    _lock();
    u_char *lc = logmem + logpos;
    lc[0] = MKTAG(LRT_TAG, LRL_2B);

    u_short *ls = (u_short*)(lc + 1);
    ls[0] = tag;

    prevtag = tag;
    logpos += 3;
    _unlock();
}

static inline void
_show_tag(FILE *f){
    u_short *ls = (u_short*)bbuf;

    fprintf(f, "tag\t%3d", ls[0]);
}

static void
_log_loginfo(void){
    if( !ROOMFOR(3) ) return;
    _lock();
    u_char *lc = logmem + logpos;

    lc[0] = MKTAG(LRT_LOGINFO, LRL_2B);

    u_short *ls = (u_short*)(lc + 1);

    if( logend ){
        ls[0] = logend - savepos + logpos;
    }else{
        ls[0] = logpos - savepos;
    }

    logpos += 3;
    _unlock();
}

static inline void
_show_loginfo(FILE *f){
    u_short *ls = (u_short*)bbuf;
    fprintf(f, "logsz\t%d", ls[0]);
}

static void
_log_slow(void){
    static int prevdt = 0;
    int dt = get_hrtime() - curr_t0;

    if( ABS(dt - prevdt) < 10 ) return;

    if( !ROOMFOR(3) ) return;
    _lock();
    u_char *lc = logmem + logpos;
    lc[0] = MKTAG(LRT_SLOW, LRL_2B);

    short *ls = (u_short*)(lc + 1);
    ls[0] = dt;

    prevdt = dt;
    logpos += 3;
    _unlock();
}

static inline void
_show_slow(FILE *f){
    short *ls = (short*)bbuf;

    fprintf(f, "dtime\t%3d", ls[0]);
}



//****************************************************************

void
diaglog_user(int len, u_char *dat){
    if( !ROOMFOR(len+2) ) return;

    _lock();
    u_char *lc = logmem + logpos;

    lc[0] = MKTAG(LRT_USER, LRL_VAR);
    lc[1] = len;

    memcpy(lc + 2, dat, len);

    logpos += len + 2;
    _unlock();
}


//****************************************************************

static int
printffnc(char **a, char c){

    **a = c;
    (*a) ++;

    return 1;
}

// NB - try to keep messages short
void
syslog(const char *fmt, ...){
    va_list ap;

    _log_stamp();
    if( !ROOMFOR(64) ) return;	// XXX ~
    _lock();
    u_char *lc = logmem + logpos;
    lc[0] = MKTAG(LRT_MSG, LRL_VAR);
    lc[1] = 0;

    u_char *p = lc+2;

    va_start(ap,fmt);
    int l = vprintf( printffnc, &p, fmt, ap);
    // remove newline
    if( logmem[logpos + l + 2 - 1] == '\n' ) l --;

    lc[1] = l;
    logpos += l + 2;
    _unlock();
}


void
diaglog(int tag){

    if( logmode == 2 ) return;

    if( logmode == 1 ){
        // high-res mode
        _log_tag(tag);
        _log_control();
        _log_sensor();
        _log_cycle();
        _log_env();
        _log_accel();
        _log_gyro();
        _log_slow();
        return;
    }

    _log_tag(tag);

    if( inv_state == INV_STATE_RUNNING || inv_state == INV_STATE_ONDELAY2 || inv_state == INV_STATE_SHUTTINGDOWN ){
        short hcs  = half_cycle_step;

        if( !hcs ) _log_cycle();
        _log_sensor();
        if( !(logstep % 10) )   _log_control();
        if( !(logstep % 3500) ) _log_env();
    }else{
        if( !(logstep % 21000) ){
            _log_sensor();
            _log_env();
        }
    }

    _log_slow();
    logstep ++;
}

//****************************************************************
void
diaglog_dump(int mask, FILE *fin, FILE *fout){
    u_char tag, len;
    int8_t i, line=0;

    while(1){
        // read record header
        int l = fread(fin, (char*)&tag, 1);
        if( l < 1 ) break;
        len = (tag & 7) << 1;
        tag >>= 3;

        if( !len ){
            l = fread(fin, (char*)&len, 1);
            if( l < 1 ) break;
        }

        if( len > sizeof(bbuf) ) break;
        // read data
        l = fread(fin, bbuf, len);
        if( l < len ) break;

        if(!( mask & (1<<tag)) ) continue;

        // dump data
        switch( tag ){
        case LRT_VERSN:
            if( !_version_is_ok() ) fprintf(STDERR, "incompatible version\n");
            continue;
        case LRT_MSG:
            fprintf(fout, "msg\t");
            fwrite(fout, bbuf, len);
            break;
        case LRT_USER:
            fprintf(fout, "data\t");
            // hexdump
            for(i=0; i<len; i++){
                fprintf(fout, "%02.2x", bbuf[i]);
                if(i & 1) fprintf(fout, " ");
            }
            break;
        case LRT_SENSOR:	_show_sensor(fout);	break;
        case LRT_GYRO:		_show_gyro(fout);	break;
        case LRT_ACCEL:		_show_accel(fout);	break;
        case LRT_STAMP:		_show_stamp(fout);	break;
        case LRT_CYCLE:		_show_cycle(fout);	break;
        case LRT_ENV:		_show_env(fout);	break;
        case LRT_SLOW:		_show_slow(fout);	break;
        case LRT_TAG:		_show_tag(fout);	break;
        case LRT_CTL:		_show_control(fout);	break;
        case LRT_LOGINFO:	_show_loginfo(fout);	break;

        default:
            fprintf(fout, "#<type %d>", tag);
            break;
        }
        fwrite(fout, "\n", 1);

        // paginate?
        if( fout == STDOUT ){
            line ++;
            if( line == 23 ){
                fputs("--More--", STDOUT);
                while(1){
                    char c = getchar();
                    switch(c){
                    case ' ':  line = 0;	break;
                    case '\n': line--;		break;
                    case 'q':  goto done;
                    default:
                        fputc('\a', STDOUT);
                        continue;
                    }
                    break;
                }
                fputs("\r        \b\b\b\b\b\b\b\b", STDOUT);
            }
        }
    }

done:
    return;
}

DEFUN(diaglog, "dump diaglog")
{
    FILE *fin, *fout=0;
    int mask = 0;

    if( argc < 2 ){
        fprintf(STDERR, "diaglog [-type ...] infile [outfile]\n");
        return -1;
    }

    while( argc > 2 ){
        // only display requested types - build bitmask
        if( argv[1][0] == '-' ){
            int t = atoi( argv[1] + 1 );
            mask |= 1<<t;
            argc --;
            argv ++;
        }
    }

    // default - show all
    if( !mask ) mask = 0xFFFFFFFF;

    fin = fopen(argv[1], "r");
    if( !fin ){
        fprintf(STDERR, "cannot open input file\n");
        return -1;
    }

    if( argc > 2 ){
        fout = fopen(argv[2], "w");
        if( !fout ){
            fprintf(STDERR, "cannot open output file\n");
            fclose(fin);
            return -1;
        }
    }

    diaglog_dump(mask, fin, fout?fout : STDOUT);

    fclose(fin);
    if(fout) fclose(fout);

    return 0;
}

//****************************************************************

void
diaglog_hires(void){
    logmode = 1;
}

void
diaglog_quiet(void){
    logmode = 2;
}

void
diaglog_reset(void){
    logpos  = 0;
    logstep = 0;
    logmode = 0;
    savepos = 0;
    logend  = 0;
    bzero(logmem, LOGSIZE);
    _log_version();
}

void
diaglog_save(FILE *f){
    int i=0;

    while(i < logpos){
        // copy to buffer - cannot dma from ccm
        int sz = logpos - i;
        if( sz > sizeof(bbuf) ) sz = sizeof(bbuf);
        memcpy(bbuf, logmem+i, sz);

        // write
        fwrite(f, bbuf, sz);

        i += sz;
    }
}

//****************************************************************

static FILE *logfd = 0;

static void
_reset_buf(void){


}

static void
_save_buf(int pos){
    int wl, l1=0, l2=0;

    if( logend ){
        l1 = logend - savepos;
        if( l1 > BUFSIZE ) l1 = BUFSIZE;
        memcpy(bbuf, logmem + savepos, l1);
        savepos += l1;
        wl = l1;

        if( savepos == logend ){
            savepos = logend = 0;
            l2 = pos;
            if( l2 > BUFSIZE - l1 ) l2 = BUFSIZE - l1;

            memcpy(bbuf + l1, logmem + savepos, l2);
            savepos += l2;
            wl += l2;
        }

    }else{
        l1 = pos - savepos;
        if( l1 > BUFSIZE ) l1 = BUFSIZE;

        memcpy(bbuf, logmem + savepos, l1);
        savepos += l1;
        wl = l1;
    }

    //printf("save log %d=%d+%d; pos %d save %d end %d\n", wl, l1, l2, logpos, savepos, logend);
    fwrite(logfd, bbuf, wl);
    fflush(logfd);
}

static void
_save_all(void){

    while( logpos != savepos ){
        _save_buf(logpos);
    }
}

static void
_log_proc(void){
    int slp;

    while( !logproc_close ){
        usleep(1000);
        if( logproc_pause ) continue;

        // reset buf?
        _lock();
        slp = logpos;
        _unlock();

        if( (slp - savepos < BUFSIZE) && (LOGSIZE - slp < BUFSIZE/2) ){
            _lock();
            logend = logpos;
            slp = logpos = 0;
            _unlock();
        }


        // is there data to save
        if( logend ){
            if( logend - savepos + slp > BUFSIZE ) _save_buf(slp);
        }else{
            if( slp - savepos > BUFSIZE ) _save_buf(slp);
        }
    }

    _save_all();
    fclose(logfd);
    logproc_pid = 0;
}


int
diaglog_open(const char *file){
    if( logproc_pid ) return 0;

    logfd = fopen(file, "w");
    if( !logfd ) return 0;

    diaglog_reset();
    logproc_close = 0;
    logproc_pid   = start_proc( 1024, _log_proc, "logger");
    yield();

    return 1;
}

int
diaglog_close(void){
    if( !logproc_pid ) return 0;

    proc_t pid = logproc_pid;
    int wp = pid->mommy->flags & PRF_AUTOREAP;
    logproc_close = 1;

    if( !wp ) wait(pid);

    return 1;
}

void
diaglog_pause(void){
    logproc_pause = 1;
}

void
diaglog_resume(void){
    logproc_pause = 0;
}


DEFUN(syslog, "log message")
{
    if( argc < 2 ) return 0;
    syslog("%s", argv[1]);
    return 0;
}
