/*
  Copyright (c) 2014
  Author: Jeff Weisberg <jaw @ tcp4me.com>
  Created: 2014-Sep-04 20:30 (EDT)
  Function: adc using overruns^WDMA

*/


/*
  when it overruns, it has always transfered 4 sets of 32bit transfers

*/

#define USE_DMABUF


#define DMAC		DMA2_Stream0

#define DMASCR_MINC	(1<<10)
#define DMASCR_DIR_M2P  (1<<6)
#define DMASCR_PFCTRL	(1<<5)
#define DMASCR_TEIE	(1<<2)
#define DMASCR_TCIE	(1<<4)
#define DMASCR_HIPRIO	(3<<16)
#define DMASCR_MEM32	(2<<13)
#define DMASCR_DEV32	(2<<11)
#define DMASCR_EN	1


static void
_enable_dma(int cnt){

    DMAC->CR    &= ~1;		// clear EN first
    DMA2->LIFCR |= 0x3D;	// clear irq

    // wait for it to disable
    while( DMAC->CR & 1 )
        ;

    // DMAC->CR   &= (0xF<<28) | (1<<20);
    DMAC->CR   = 0;

    DMAC->FCR   = DMAC->FCR & ~3;
    DMAC->FCR  |= 1<<2;
    DMAC->PAR   = (u_long) & ADC->CDR;
    DMAC->M0AR  = (u_long) & dmabuf;
    DMAC->NDTR  = cnt;

    DMAC->CR   |= DMASCR_TEIE | DMASCR_TCIE | DMASCR_MINC | DMASCR_HIPRIO | DMASCR_MEM32 | DMASCR_DEV32
        | (0<<25)	// channel 0
        ;

    ADC->CCR |= 3<<14;	// dma mode
    ADC->CCR |= 2<<14;	// dma mode 2
    ADC->CCR |= 6;	// regular simultaneous mode
    DMAC->CR |= 1;	// enable

    DROP_CRUMB("ena dma", DMA2->LISR, ADC->CSR);

}

static void
_disable_dma(void){

    DROP_CRUMB("dis dma", DMA2->LISR,DMAC->NDTR);
    DMAC->CR &= ~(DMASCR_EN | DMASCR_TEIE | DMASCR_TCIE);
    DMA2->LIFCR |= 0x3D;	// clear irq

    ADC->CCR = ADC->CCR & ~(
        (3<<14)		// dma mode
        | (0x1F)	// multi-mode
        );
}


static void
_adc_init(void){

    ADC1->CR1 |= 1<<8;		// scan mode
    ADC2->CR1 |= 1<<8;

    RCC->AHB1ENR |= 1<<22;	// DMA2
    nvic_enable( IRQ_DMA2_STREAM0, 0x40 );
}


static void
adc_get_all(int sr, int si){
    int i=0, n=0, v;

    sync_lock( &adclock, "adc.L" );

    //if( ADC1->SR & 2 ) v = ADC1->DR;	// clear any previous result
    //if( ADC2->SR & 2 ) v = ADC2->DR;

    //ADC1->SR &= ~ SR_OVR | (1<<4) | (1<<1);
    //ADC2->SR &= ~ SR_OVR | (1<<4) | (1<<1);
    ADC1->SR = ADC1->SR & ~0x3f;
    ADC2->SR = ADC2->SR & ~0x3f;

    RESET_CRUMBS();
    bzero(dmabuf, sizeof(dmabuf));

    ADC1->SQR3 = (SEN_0 & 0x1F) | ((SEN_2 & 0x1f)<<5) | ((SEN_4 & 0x1f)<<10) | ((SEN_0 & 0x1F)<<15) | ((SEN_2 & 0x1F)<<20) | ((SEN_4 & 0x1F)<<25);
    ADC2->SQR3 = (SEN_1 & 0x1F) | ((SEN_3 & 0x1f)<<5) | ((sr & 0x1f)<<10)    | ((SEN_1 & 0x1F)<<15) | ((SEN_3 & 0x1F)<<20) | ((sr & 0x1F)<<25);

    ADC1->SQR2 = (SEN_0 & 0x1F) | ((SEN_2 & 0x1f)<<5) | ((SEN_4 & 0x1f)<<10);
    ADC2->SQR2 = (SEN_1 & 0x1F) | ((SEN_3 & 0x1f)<<5) | ((sr & 0x1f)<<10);

    ADC1->SQR1 = (9-1)<<20;	// 9 conversions
    ADC2->SQR1 = (9-1)<<20;

    _enable_dma(9);
    DROP_CRUMB("adc start", ADC->CSR, DMA2->LISR);
    adcstate = ADC_BUSY;
    ADC1->CR2 |= CR2_SWSTART;	// Go!
    DROP_CRUMB("adc started", ADC->CSR, DMA2->LISR);

    // tsleep
    utime_t to = get_hrtime() + 10000;
    while( adcstate == ADC_BUSY ){
        if( currproc ){
            int err = tsleep( &adc_get_all, -1, "adc", 1000);
            DROP_CRUMB("adc-wake", DMA2->LISR, ADC->CSR);
            if( get_hrtime() >= to ){
                DROP_CRUMB("toed", err, ADC->CSR);
                adcstate = ADC_ERROR;
            }
            //printf("adc: %d, %d, tr %d %d\n", err, adcstate, (int)currproc->timeout, (int)get_time());
            //usleep(10000);
        }
    }


    _disable_dma();

    if( adcstate == ADC_ERROR ){
        printf("adc error\n");
    }
    DUMP_CRUMBS();

    sync_unlock( &adclock );
}


void
DMA2_Stream0_IRQHandler(void){
    int isr = DMA2->LISR & 0x3F;
    DMA2->LIFCR |= 0x3D;	// clear irq

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

