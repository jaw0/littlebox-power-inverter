/*
  Copyright (c) 2014
  Author: Jeff Weisberg <jaw @ tcp4me.com>
  Created: 2014-Sep-04 20:32 (EDT)
  Function: adc busy waiting

*/

// samp = 1 => 21.4



static void
_adc_init(void){

}

static void
adc_get_all(int sr, int si){
    int i=0, n=0;

    sync_lock( &adclock, "adc.L" );

    // bzero(dmabuf, sizeof(dmabuf));

    // 2 new samples + copy previous
    for(i=0; i<9; i++)
        sensor_data[i].hist[2] = sensor_data[i].val;

    // busy-wait
    for(i=0; i<2; i++){
        n = adc_get2( SEN_0, SEN_1 );
        sensor_data[0].hist[i] = n & 0xFFFF;
        sensor_data[1].hist[i] = n >> 16;

        n = adc_get2( SEN_2, SEN_3 );
        sensor_data[2].hist[i] = n & 0xFFFF;
        sensor_data[3].hist[i] = n >> 16;

        n = adc_get2( SEN_4, sr );
        sensor_data[4].hist[i] = n & 0xFFFF;
        sensor_data[si].hist[i] = n >> 16;
    }

    sync_unlock( &adclock );
}

