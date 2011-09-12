/******************************************************************************
 *
 *  File        : dma.c
 *  Description : Direct memory access
 *
 *
 *****************************************************************************/
#include "kernel.h"
#include "dma.h"

/**
 *
 */
void dma_set_address (Uint8 channel, char *address) {
    Uint8 port, extended_port, page;

    if (channel > 8) return;

    // Get page
    page = LO8(HI16((Uint32)address)) & 0x000F;

	switch (channel) {
		case 0: port = DMA0_CHAN0_ADDR_REG; extended_port = 0x00; break;    // NO extended poirt for channel 0
		case 1: port = DMA0_CHAN1_ADDR_REG; extended_port = DMA_PAGE_CHAN1_ADDRBYTE2; break;
		case 2: port = DMA0_CHAN2_ADDR_REG; extended_port = DMA_PAGE_CHAN2_ADDRBYTE2; break;
		case 3: port = DMA0_CHAN3_ADDR_REG; extended_port = DMA_PAGE_CHAN3_ADDRBYTE2; break;
		case 4: port = DMA1_CHAN4_ADDR_REG; extended_port = 0x00; break;    // NO extended poirt for channel 4
		case 5: port = DMA1_CHAN5_ADDR_REG; extended_port = DMA_PAGE_CHAN5_ADDRBYTE2; break;
		case 6: port = DMA1_CHAN6_ADDR_REG; extended_port = DMA_PAGE_CHAN6_ADDRBYTE2; break;
		case 7: port = DMA1_CHAN7_ADDR_REG; extended_port = DMA_PAGE_CHAN7_ADDRBYTE2; break;
	}

    // Output "offset"
	outb (port, LO8(LO16((Uint32)address)));
	outb (port, HI8(LO16((Uint32)address)));
	if (extended_port) outb (extended_port, page);  // Output page
}


/**
 *
 */
void dma_set_size (Uint8 channel, Uint16 size) {
    Uint8 port;

	if (channel > 8) return;

	switch ( channel ) {
		case 0: port = DMA0_CHAN0_COUNT_REG; break;
		case 1: port = DMA0_CHAN1_COUNT_REG; break;
		case 2: port = DMA0_CHAN2_COUNT_REG; break;
		case 3: port = DMA0_CHAN3_COUNT_REG; break;
		case 4: port = DMA1_CHAN4_COUNT_REG; break;
		case 5: port = DMA1_CHAN5_COUNT_REG; break;
		case 6: port = DMA1_CHAN6_COUNT_REG; break;
		case 7: port = DMA1_CHAN7_COUNT_REG; break;
	}

	outb (port, LO8(size));
	outb (port, HI8(size));
}


/**
 *
 */
void dma_mask_channel (Uint8 channel) {

		if (channel <= 4)
		{
			outb(DMA0_CHANMASK_REG, (1 << (channel - 1)));
			return;
		}

	outb(DMA1_CHANMASK_REG, (1 << (channel - 5)));
}

/**
 *
 */
void dma_unmask_channel (Uint8 channel) {

		if (channel <= 4)
		{
			outb (DMA0_CHANMASK_REG, channel);
			return;
		}

	outb (DMA1_CHANMASK_REG, channel);
}


/**
 *
 */
void dma_reset_flipflop (int dma) {

	if (dma < 2) return;

	outb ( (dma==0) ? DMA0_CLEARBYTE_FLIPFLOP_REG : DMA1_CLEARBYTE_FLIPFLOP_REG, 0xff);
}

/**
 *
 */
void dma_reset () {
	outb (DMA0_TEMP_REG, 0xff);
}

/**
 *
 */
void dma_unmask_all (){
    outb (DMA1_UNMASK_ALL_REG, 0xff);
}

/**
 *
 */
void dma_set_mode (Uint8 channel, Uint8 mode) {
	int dma = (channel < 4) ? 0 : 1;
	int chan = (dma == 0) ? channel : channel - 4;

	dma_mask_channel (channel);
	outb ( (channel < 4) ? (DMA0_MODE_REG) : DMA1_MODE_REG, chan | (mode) );
	dma_unmask_all ();
}

/**
 *
 */
void dma_set_read (Uint8 channel) {
	dma_set_mode (channel, DMA_MODE_READ_TRANSFER | DMA_MODE_TRANSFER_SINGLE | DMA_MODE_MASK_AUTO);
}

/**
 *
 */
void dma_set_write (Uint8 channel) {
	dma_set_mode (channel, DMA_MODE_WRITE_TRANSFER | DMA_MODE_TRANSFER_SINGLE | DMA_MODE_MASK_AUTO);
}

/**
 *
 */
void dma_init (void) {
    dma_reset ();
}