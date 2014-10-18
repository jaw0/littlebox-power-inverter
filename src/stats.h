/*
  Copyright (c) 2014
  Author: Jeff Weisberg <jaw @ tcp4me.com>
  Created: 2014-Oct-06 22:59 (EDT)
  Function: 

*/


#ifndef __gpi_stats_h__
#define __gpi_stats_h__


struct StatAve {
    int _curr, _prev;
    int _sum;
    int _count;
    int ave, p_ave;

};

static inline void _sta_reset(struct StatAve *s){ s->_sum  = 0; s->_count = 0; }
static inline void _sta_add(struct StatAve *s,int v){  s->_sum += v; s->_count ++; s->_prev = s->_curr; s->_curr = v; }
static inline void _sta_cycle(struct StatAve *s){
    s->p_ave  = s->ave;
    s->ave    = s->_sum / s->_count;
    s->_sum   = 0;
    s->_count = 0;
}

struct StatAveMinMax {
    int _curr, _prev;
    int _sum;
    int _count;
    int ave, p_ave;
    int _min, _max;
    int min, max;

};

static inline void _stm_reset(struct StatAveMinMax *s){ s->_sum  = 0; s->_count = 0; }

static inline void
_stm_add(struct StatAveMinMax *s, int v) {
    if( !s->_count )
        s->_min = s->_max = v;
    else{
        if( v < s->_min ) s->_min = v;
        if( v > s->_max ) s->_max = v;
    }
    s->_sum += v; s->_count ++; s->_prev = s->_curr; s->_curr = v;
}

static inline void _stm_cycle(struct StatAveMinMax *s){
    s->p_ave  = s->ave;
    s->ave    = s->_sum / s->_count;
    s->_sum   = 0;
    s->_count = 0;
    s->min = s->_min;
    s->max = s->_max;
}



#endif /* __gpi_stats_h__ */
