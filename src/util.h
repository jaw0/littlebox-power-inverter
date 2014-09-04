/*
  Copyright (c) 2014
  Author: Jeff Weisberg <jaw @ tcp4me.com>
  Created: 2014-May-15 13:31 (EDT)
  Function: 

*/
#ifndef __util_h__
#define __util_h__

#define ABS(a)                	(((a)<0)?-(a):(a))

#define random_n(n)		(random() % (n))


typedef char bool_t;


extern unsigned int random(void);

#endif /* __util_h__ */
