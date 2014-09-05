/*
  Copyright (c) 2014
  Author: Jeff Weisberg <jaw @ tcp4me.com>
  Created: 2014-Sep-04 20:30 (EDT)
  Function: adc using overruns^WDMA

*/


#define USE_DMABUF


static void
_enable_dma(int cnt){

    DROP_CRUMB("ena dma", 0,0);

    DMAC->CR   &= ~1;	// clear EN first
    DMAC->PAR   = (u_long) & ADC->CDR;
    DMAC->M0AR  = (u_long) & dmabuf;
    DMAC->CR   &= (0xF<<28) | (1<<20);

    DMAC->NDTR  = cnt;
    DMAC->CR   |= DMASCR_TEIE | DMASCR_TCIE | DMASCR_MINC
        | (0<<25)	// channel 0
        | (3<<16) 	// high prio
        | (2<<13)	// 32 bit memory
        | (2<<11)	// 32 bit periph
        ;

    ADC->CCR |= 2<<14;	// dma mode 2

    DMAC->CR |= 1;	// enable

}

static void
_disable_dma(void){

    ADC->CCR &= ~(3<<14);	// dma mode
    DMAC->CR &= ~(DMASCR_EN | DMASCR_TEIE | DMASCR_TCIE);
    DMA2->LIFCR |= 0x3C;	// clear irq
    DROP_CRUMB("dis dma", 0,0);
}


static void
_adc_init(void){

    ADC1->CR1 |= 1<<8;		// scan mode
    ADC2->CR1 |= 1<<8;

    ADC->CCR  |= 6		// simultaneous regular mode
        | (1<<13)		// dma select
        ;

    RCC->AHB1ENR |= 1<<22;	// DMA2
    nvic_enable( IRQ_DMA2_STREAM0, 0x20 );
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

    ADC1->SQR3 = (SEN_0 & 0x1F) | ((SEN_2 & 0x1f)<<5) | ((SEN_4 & 0x1f)<<10) | ((SEN_0 & 0x1F)<<15) | ((SEN_2 & 0x1F)<<20) | ((SEN_4 & 0x1F)<<25);
    ADC2->SQR3 = (SEN_1 & 0x1F) | ((SEN_3 & 0x1f)<<5) | ((sr & 0x1f)<<10)    | ((SEN_1 & 0x1F)<<15) | ((SEN_3 & 0x1F)<<20) | ((sr & 0x1F)<<25);

    ADC1->SQR2 = (SEN_0 & 0x1F) | ((SEN_2 & 0x1f)<<5) | ((SEN_4 & 0x1f)<<10);
    ADC2->SQR2 = (SEN_1 & 0x1F) | ((SEN_3 & 0x1f)<<5) | ((sr & 0x1f)<<10);

    ADC1->SQR1 = (9-1)<<20;	// 9 conversions
    ADC2->SQR1 = (9-1)<<20;

    _enable_dma(9);
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
            }
            printf("adc: %d, %d, tr %d %d\n", err, adcstate, (int)currproc->timeout, (int)get_time());
            usleep(10000);
        }
    }


    _disable_dma();

    if( adcstate == ADC_ERROR ){
        printf("adc error\n");
        DUMP_CRUMBS();
    }

    sync_unlock( &adclock );
}


void
DMA2_Stream0_IRQHandler(void){
    int isr = DMA2->LISR & 0x3F;
    DMA2->LIFCR |= 0x3C;	// clear irq

    DROP_CRUMB("dmairq", isr, 0);
    if( isr & 8 ){
        // error - done
        _disable_dma();
        adcstate = ADC_ERROR;
        DROP_CRUMB("dma-err", 0,0);
        wakeup( &adc_get_all );
        return;
    }

    if( isr & 0x20 ){
        // dma complete
        if( ((u_long)dmabuf == DMAC->M0AR) && ! DMAC->NDTR ){
            _disable_dma();
            adcstate = ADC_DONE;
            DROP_CRUMB("dma-done", 0,0);
            wakeup( &adc_get_all );
        }
    }
}

