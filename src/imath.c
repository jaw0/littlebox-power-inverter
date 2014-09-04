/*
  Copyright 1998 Jeff Weisberg <jaw@op.net>
  see the file LICENSE for details
*/


// printf "%d, ", 1024 * sin($_ * 2 * 3.14159265359 / 256) for(0 .. 65)

static const short sindata[] = {
    0, 25, 50, 75, 100, 125, 150, 175, 199, 224, 248, 273, 297, 321,
    344, 368, 391, 414, 437, 460, 482, 504, 526, 547, 568, 589, 609,
    629, 649, 668, 687, 706, 724, 741, 758, 775, 791, 807, 822, 837,
    851, 865, 878, 890, 903, 914, 925, 936, 946, 955, 964, 972, 979,
    986, 993, 999, 1004, 1008, 1012, 1016, 1019, 1021, 1022, 1023, 1024,
    /* extra point eliminates need for bounds check in interp. code */
    1023
};

int
isin(int x){
    /* returns 1024*sin( 2pi * x / 4096 ) */
    int p, v;
    char adj, neg=0;

    if (x<0){
        x = -x;
        neg = 1;
    }
    x &= 4095;
    if (x>2047){
        x &= 2047;
        neg = !neg;
    }
    if (x>1024){
        x = 2048 - x;
    }

    p = x>>4;	/* x: [0..1023], p: [0:63] */


    adj = x & 15;
    v = sindata[p];
    if (adj){
        /* interpolate */
        v += ((sindata[p+1]-v) * adj) >> 4;
    }
    return neg?-v:v;
}



// http://www.finesse.demon.co.uk/steven/sqrt.html
#define iter1(N) \
    try = root + (1 << (N)); \
    if (n >= try << (N)){    \
        n -= try << (N);     \
        root |= 2 << (N);    \
    }

int
isqrt(unsigned int n){
    unsigned int root = 0, try;
    iter1 (15);    iter1 (14);    iter1 (13);    iter1 (12);
    iter1 (11);    iter1 (10);    iter1 ( 9);    iter1 ( 8);
    iter1 ( 7);    iter1 ( 6);    iter1 ( 5);    iter1 ( 4);
    iter1 ( 3);    iter1 ( 2);    iter1 ( 1);    iter1 ( 0);
    return root >> 1;
}

