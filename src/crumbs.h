/*
  Copyright (c) 2014
  Author: Jeff Weisberg <jaw @ tcp4me.com>
  Created: 2014-Aug-19 19:24 (EDT)
  Function: 

*/

#ifndef __crumbs_h__
#define __crumbs_h__

#ifdef CRUMBS


struct crumb {
    const char *event;
    int when;
    int arg0;
    int arg1;
};

#define NR_CRUMBS       128
static struct crumb crumbs[NR_CRUMBS];
static u_long cur_crumb = 0;

static inline void
_drop_crumb(const char *event, u_long arg0, u_long arg1) {
    struct crumb *crumb = &crumbs[cur_crumb % NR_CRUMBS];
    crumb->event = event;
    crumb->when  = get_hrtime();
    crumb->arg0  = arg0;
    crumb->arg1  = arg1;
    cur_crumb++;
}

static void
_dump_crumb(void){
    int i;

    int st = (cur_crumb > NR_CRUMBS) ? cur_crumb - NR_CRUMBS : 0;
    int wh = crumbs[st].when;

    for(i=st; i<cur_crumb; i++){
        int n = i % NR_CRUMBS;
        unsigned int dt = crumbs[n].when - wh;
        wh = crumbs[n].when;

        kprintf("[" CRUMBS "] %d\t%s\t%x %x\n", dt, crumbs[n].event, crumbs[n].arg0, crumbs[n].arg1);
    }
}
# define DROP_CRUMB(event, arg0, arg1) _drop_crumb(event, (u_long)arg0, (u_long)arg1)
# define DUMP_CRUMBS()	_dump_crumb()
# define RESET_CRUMBS()	cur_crumb = 0

#else
# define DROP_CRUMB(a,b,c)	/**/
# define DUMP_CRUMBS()		/**/
# define RESET_CRUMBS()		/**/

#endif

#endif /* __crumbs_h__ */
