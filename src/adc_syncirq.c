/*
  Copyright (c) 2014
  Author: Jeff Weisberg <jaw @ tcp4me.com>
  Created: 2014-Sep-04 21:07 (EDT)
  Function: adc using irq in one sequence

*/

#define USE_DMABUF

static short adcstate;
static short adcindex;
static short adccount;


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


    ADC1->CR1 |= 1<<8;		// scan mode
    ADC2->CR1 |= 1<<8;

    ADC1->CR2 |= 1<<10;		// EOC for each conversion
    ADC2->CR2 |= 1<<10;

    ADC->CCR  |= 6		// simultaneous regular mode
        ;
    nvic_enable( IRQ_ADC1_2,       0x40 );

}



static void
adc_get_all(int sr){
    int i=0, n=0;

    sync_lock( &adclock, "adc.L" );

    int v = ADC1->DR;	// clear any previous result
    v = ADC2->DR;

    RESET_CRUMBS();
    bzero(dmabuf, sizeof(dmabuf));

    ADC1->SR &= ~ SR_OVR;
    ADC2->SR &= ~ SR_OVR;

    ADC1->SQR3 = (SEN_0 & 0x1F) | ((SEN_2 & 0x1f)<<5) | ((SEN_4 & 0x1f)<<10) | ((SEN_0 & 0x1F)<<15) | ((SEN_2 & 0x1F)<<20) | ((SEN_4 & 0x1F)<<25);
    ADC2->SQR3 = (SEN_1 & 0x1F) | ((SEN_3 & 0x1f)<<5) | ((sr & 0x1f)<<10)    | ((SEN_1 & 0x1F)<<15) | ((SEN_3 & 0x1F)<<20) | ((sr & 0x1F)<<25);

    ADC1->SQR2 = (SEN_0 & 0x1F) | ((SEN_2 & 0x1f)<<5) | ((SEN_4 & 0x1f)<<10);
    ADC2->SQR2 = (SEN_1 & 0x1F) | ((SEN_3 & 0x1f)<<5) | ((sr & 0x1f)<<10);

    ADC1->SQR1 = (9-1)<<20;	// 9 conversions
    ADC2->SQR1 = (9-1)<<20;

    adcindex = 0;
    adccount = 9 << 1;
    _enable_irq();

    DROP_CRUMB("adc start", 0,0);
    adcstate = ADC_BUSY;
    ADC1->CR2 |= CR2_SWSTART;	// Go!

    // tsleep
    while( adcstate == ADC_BUSY ){
        if( currproc ){
            int err = tsleep( &adc_get_all, -1, "adc", 1000);
            if( err ){
                DROP_CRUMB("toed", err, ADC->CSR);
                adcstate = ADC_ERROR;
                printf("adc: %d, %d, tr %d %d\n", err, adcstate, (int)currproc->timeout, (int)get_time());
            }
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
            wakeup( &adc_get_all );
        }
        return;
    }

}
