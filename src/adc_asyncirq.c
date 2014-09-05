/*
  Copyright (c) 2014
  Author: Jeff Weisberg <jaw @ tcp4me.com>
  Created: 2014-Sep-04 20:26 (EDT)
  Function: adc using async irqs

*/

#define USE_DMABUF

// samp = 1 => 36.7 usec

static short adcindex;
static short adccount;
static short adcsr;

static void
_enable_irq(void){

    ADC1->CR1 |= (1<<5) | (1<<26);	// EOCIE, OVRIE
}

static void
_disable_irq(void){

    ADC1->CR1 &= ~( (1<<5) | (1<<26) );
}

static void
_adc_init(void){

    ADC->CCR  |= 6		// simultaneous regular mode
        ;
    nvic_enable( IRQ_ADC1_2, 0x20 );
}


static int8_t _adc_order[] = { SEN_0, SEN_1, SEN_2, SEN_3, SEN_4, SEN_5, SEN_6, SEN_7, SEN_8 };

static inline void
_start_next_adc(void){
    short i = adcindex % 6;
    ADC1->SQR3 = _adc_order[ i ];
    ADC2->SQR3 = (i == 5) ? adcsr : _adc_order[ i + 1 ];
    ADC1->CR2 |= CR2_SWSTART;	// Go!
}


static void
adc_get_all(int sr){
    int i=0, n=0;

    sync_lock( &adclock, "adc.L" );

    int v = ADC1->DR;	// clear any previous result
    v = ADC2->DR;

    ADC1->SR &= ~ SR_OVR;
    ADC2->SR &= ~ SR_OVR;

    RESET_CRUMBS();
    bzero(dmabuf, sizeof(dmabuf));

    ADC1->SQR1 = 0;
    ADC2->SQR1 = 0;

    adcindex = 0;
    adccount = 9 << 1;
    adcsr    = sr;
    _enable_irq();
    DROP_CRUMB("adc start", 0,0);
    adcstate = ADC_BUSY;
    _start_next_adc();


    // tsleep
    while( adcstate == ADC_BUSY ){
        if( currproc ){
            int err = tsleep( &adc_get_all, -1, "adc", 1000);
            if( err ){
                DROP_CRUMB("toed", err, ADC->CSR);
                adcstate = ADC_ERROR;
            }
            //printf("adc: %d, %d, tr %d %d\n", err, adcstate, (int)currproc->timeout, (int)get_time());
            //usleep(10000);
        }
    }

    _disable_irq();

    if( adcstate == ADC_ERROR ){
        printf("adc error\n");
        DUMP_CRUMBS();
    }

    sync_unlock( &adclock );
}

void
ADC_IRQHandler(void){

    int sr = ADC->CSR;

    if( sr & ((1<<5) | (1<<13)) ){
        // overrun
        _disable_irq();
        ADC1->SR &= ~(1<<5);
        ADC2->SR &= ~(1<<5);
        adcstate = ADC_ERROR;
        DROP_CRUMB("adc-ovr", sr, 0);
        wakeup( &adc_get_all );
        return;
    }

    if( sr & ((1<<1) | (1<<9)) ){
        // EOC
        DROP_CRUMB("adc-irq", sr, adcindex);
        dmabuf[ adcindex++ ] = ADC1->DR;
        dmabuf[ adcindex++ ] = ADC2->DR;

        // done?
        if( adcindex > adccount ){
            DROP_CRUMB("adc-done", 0,0);
            adcstate = ADC_DONE;
            _disable_irq();
            wakeup( &adc_get_all );
        }else{
            _start_next_adc();
        }
        return;
    }

}

