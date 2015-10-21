/*------------------------------------------------------------------------/
/  MB9BF616/617/618 UART control module
/-------------------------------------------------------------------------/
/
/  Copyright (C) 2012, ChaN, all right reserved.
/
/ * This software is a free software and there is NO WARRANTY.
/ * No restriction on use. You can use, modify and redistribute it for
/   personal, non-profit or commercial products UNDER YOUR RESPONSIBILITY.
/ * Redistributions of source code must retain the above copyright notice.
/
/-------------------------------------------------------------------------*/

#include <string.h>
#include "uart.h"

#define F_PCLK		72000000	/* Bus clock for the MFS module */

#define NUM_MFS 8
#define T_SMR 0
#define T_SCR 1
#define T_ESCR 2
#define T_SSR 3
#define T_TDR 4
#define T_RDR 5
#define T_BGR 6
#define T_RXIRQN 0
#define T_TXIRQN 1

#define ISER ((volatile uint32_t*)0xE000E100)
#define ICER ((volatile uint32_t*)0xE000E180)
#define __enable_irq() asm volatile ("CPSIE i\n")
#define __disable_irq() asm volatile ("CPSID i\n")
#define __enable_irqn(n) ISER[(n) / 32] = 1 << ((n) % 32)
#define __disable_irqn(n) ICER[(n) / 32] = 1 << ((n) % 32)

/* Register address table */
static const uint32_t registerTable_mfs_set[NUM_MFS][7] = {
	/*	MFS_SMR			MFS_SCR			MFS_ESCR		MFS_SSR			MFS_TDR			MFS_RDR			MFS_BGR		*/
	{	0x40038000,		0x40038001,		0x40038004,		0x40038005,		0x40038008,		0x40038008,		0x4003800C	},	/* MFS0 */
	{	0x40038100,		0x40038101,		0x40038104,		0x40038105,		0x40038108,		0x40038108,		0x4003810C	},	/* MFS1 */
	{	0x40038200,		0x40038201,		0x40038204,		0x40038205,		0x40038208,		0x40038208,		0x4003820C	},	/* MFS2 */
	{	0x40038300,		0x40038301,		0x40038304,		0x40038305,		0x40038308,		0x40038308,		0x4003830C	},	/* MFS3 */
	{	0x40038400,		0x40038401,		0x40038404,		0x40038405,		0x40038408,		0x40038408,		0x4003840C	},	/* MFS4 */
	{	0x40038500,		0x40038501,		0x40038504,		0x40038505,		0x40038508,		0x40038508,		0x4003850C	},	/* MFS5 */
	{	0x40038600,		0x40038601,		0x40038604,		0x40038605,		0x40038608,		0x40038608,		0x4003860C	},	/* MFS6 */
	{	0x40038700,		0x40038701,		0x40038704,		0x40038705,		0x40038708,		0x40038708,		0x4003870C	}	/* MFS7 */
};

static const uint32_t registerTable_epfr[NUM_MFS] = {
	0x4003361C,		/* MFS0 */
	0x4003361C,		/* MFS1 */
	0x4003361C,		/* MFS2 */
	0x4003361C,		/* MFS3 */
	0x40033620,		/* MFS4 */
	0x40033620,		/* MFS5 */
	0x40033620,		/* MFS6 */
	0x40033620		/* MFS7 */
};

static const uint32_t registerTable_pfr[NUM_MFS][3] = {
	/*	PIN_LOC0		PIN_LOC1		PIN_LOC2	*/
	{	0x40033008,		0x40033004,		0x4003302C	},	/* MFS0(PFR2, PFR1, PFRB) */
	{	0x40033014,		0x40033004,		0x4003303C	},	/* MFS1(PFR5, PFR1, PFRF) */
	{	0x4003301C,		0x40033008,		0x40033004	},	/* MFS2(PFR7, PFR2, PFR1) */
	{	0x4003301C,		0x40033014,		0x40033010	},	/* MFS3(PFR7, PFR5, PFR4) */
	{	0x40033034,		0x40033004,		0x40033000	},	/* MFS4(PFRD, PFR1, PFR0) */
	{	0x40033018,		0x40033024,		0x4003300C	},	/* MFS5(PFR6, PFR9, PFR3) */
	{	0x40033014,		0x4003300C,		0x4003303C	},	/* MFS6(PFR5, PFR3, PFRF) */
	{	0x40033014,		0x40033010,		0x4003302C	}	/* MFS7(PFR5, PFR4, PFRB) */
};

/* EPFR setting table */
static const uint8_t settingTable_epfr[] = {
	4,		/* MFS0, MFS4 */
	10,		/* MFS1, MFS5 */
	16,		/* MFS2, MFS6 */
	22		/* MFS3, MFS7 */
};

/* Pin location table */
static const uint8_t settingTable_pinloc[NUM_MFS][3] = {
	/*	PIN_LOC0	PIN_LOC1	PIN_LOC2	*/
	{	1,			4,			4			},	/* MFS0 */
	{	6,			1,			0			},	/* MFS1 */
	{	2,			4,			7			},	/* MFS2 */
	{	5,			0,			8			},	/* MFS3 */
	{	1,			10,			5			},	/* MFS4 */
	{	0,			2,			6			},	/* MFS5 */
	{	3,			2,			3			},	/* MFS6 */
	{	9,			13,			0			}	/* MFS7 */
};

/* MFS_IRQn table */
static const uint8_t settingTable_irqn[NUM_MFS][2] = {
	/*	MFS_RX_IRQn		MFS_TX_IRQn	*/
	{	7,				8			},	/* MFS0 */
	{	9,				10			},	/* MFS1 */
	{	11,				12			},	/* MFS2 */
	{	13,				14			},	/* MFS3 */
	{	15,				16			},	/* MFS4 */
	{	17,				18			},	/* MFS5 */
	{	19,				20			},	/* MFS6 */
	{	21,				22			}	/* MFS7 */
};

static
volatile UARTFIFO	*Uart0_0, *Uart0_1, *Uart0_2, *Uart0_3,
					*Uart0_4, *Uart0_5, *Uart0_6, *Uart0_7;		/* UART control structure */

void MFS_TX_IRQHandler_uart (uint8_t mfs)
{
	volatile UARTFIFO *uart;
	volatile uint8_t *add_scr;
	volatile uint16_t *add_tdr;
	uint16_t i;
	
	switch(mfs)
	{
		case 0:
			uart = Uart0_0;		break;
		case 1:
			uart = Uart0_1;		break;
		case 2:
			uart = Uart0_2;		break;
		case 3:
			uart = Uart0_3;		break;
		case 4:
			uart = Uart0_4;		break;
		case 5:
			uart = Uart0_5;		break;
		case 6:
			uart = Uart0_6;		break;
		case 7:
			uart = Uart0_7;		break;
		default:
			return;
	}
	
	if (!uart) {		/* Spurious Interrupt */
		return;
	}
	
	add_scr = (uint8_t *)(registerTable_mfs_set[mfs][T_SCR]);
	add_tdr = (uint16_t *)(registerTable_mfs_set[mfs][T_TDR]);

	i = uart->txri;
	if (i != uart->txwi) {		/* There is one or more byte in the Tx buffer */
		*add_tdr = uart->txbuff[i++];
		i %= BUFF_SIZE;
	}
	if (i == uart->txwi) {		/* No data in the Tx buffer */
		*add_scr &= ~0x08;		/* Clear TIE (disable Tx ready irq) */
	}
	uart->txri = i;
}

void MFS_RX_IRQHandler_uart (uint8_t mfs)
{
	volatile UARTFIFO *uart;
	volatile uint8_t *add_ssr;
	volatile uint16_t *add_rdr;
	uint8_t d;
	uint16_t i, ni;
	
	switch(mfs)
	{
		case 0:
			uart = Uart0_0;		break;
		case 1:
			uart = Uart0_1;		break;
		case 2:
			uart = Uart0_2;		break;
		case 3:
			uart = Uart0_3;		break;
		case 4:
			uart = Uart0_4;		break;
		case 5:
			uart = Uart0_5;		break;
		case 6:
			uart = Uart0_6;		break;
		case 7:
			uart = Uart0_7;		break;
		default:
			return;
	}
	
	if (!uart) {		/* Spurious Interrupt */
		return;
	}
	
	add_ssr = (uint8_t *)(registerTable_mfs_set[mfs][T_SSR]);
	add_rdr = (uint16_t *)(registerTable_mfs_set[mfs][T_RDR]);
	
	if (*add_ssr & 0x38) {	/* Error occured */
		*add_ssr = 0x80;
	}

	if (*add_ssr & 0x04) {	/* Data arrived */
		d = *add_rdr;	/* Get received data */
		i = uart->rxwi; ni = (i + 1) % BUFF_SIZE;
		if (ni != uart->rxri) {	/* Store it into the Rx buffer if not full */
			uart->rxbuff[i] = d;
			uart->rxwi = ni;
		}
	}
}

/*--------------------------------------------------------------------------

   Private

---------------------------------------------------------------------------*/

void uart_attach_mfs (uint8_t mfs, uint8_t loc)
{
	volatile uint32_t *add_epfr, *add_pfr;
	uint8_t epfr_no, pinloc_no;
	
	add_epfr = (uint32_t *)(registerTable_epfr[mfs]);
	add_pfr = (uint32_t *)(registerTable_pfr[mfs][loc]);
	epfr_no = settingTable_epfr[mfs%4];
	pinloc_no = settingTable_pinloc[mfs][loc];
	
	switch(loc)
	{
		case 0:		/* PIN_LOC0 */
			*add_epfr = (*add_epfr & ~(15 << epfr_no)) | (4 << epfr_no);
			break;
		case 1:		/* PIN_LOC1 */
			*add_epfr = (*add_epfr & ~(15 << epfr_no)) | (10 << epfr_no);
			break;
		case 2: 	/* PIN_LOC2 */
			*add_epfr = (*add_epfr & ~(15 << epfr_no)) | (15 << epfr_no);
			break;
		default:
			break;
	}
	*add_pfr |= 3 << pinloc_no;
}

void uart_detach_mfs (uint8_t mfs, uint8_t loc)
{
	volatile uint32_t *add_epfr, *add_pfr;
	uint8_t epfr_no, pinloc_no;
	
	add_epfr = (uint32_t *)(registerTable_epfr[mfs]);
	add_pfr = (uint32_t *)(registerTable_pfr[mfs][loc]);
	epfr_no = settingTable_epfr[mfs%4];
	pinloc_no = settingTable_pinloc[mfs][loc];
	
	*add_epfr = *add_epfr & ~(15 << epfr_no);
	*add_pfr = *add_pfr & ~(3 << pinloc_no);
}

/*--------------------------------------------------------------------------

   Public Functions

---------------------------------------------------------------------------*/

int uart_test (volatile UARTFIFO *uart)	/* 0:Empty, 1:Not empty */
{
	return uart->rxri != uart->rxwi;
}

uint8_t uart_getc (volatile UARTFIFO *uart)
{
	uint8_t d;
	uint16_t i;
 
	/* Wait while Rx FIFO is empty */
	i = uart->rxri;
	while (i == uart->rxwi) ;
 
	d = uart->rxbuff[i++];	/* Get a byte from Rx FIFO */
	uart->rxri = i % BUFF_SIZE;
 
	return d;
}

void uart_putc (volatile UARTFIFO *uart, uint8_t d)
{
	volatile uint8_t *add_scr;
	uint16_t i, ni;
	uint8_t mfs;
	
	mfs = uart->mfsch;
	add_scr = (uint8_t *)(registerTable_mfs_set[mfs][T_SCR]);
 
	/* Wait for Tx FIFO ready */
	i = uart->txwi; ni = (i + 1) % BUFF_SIZE;
	while (ni == uart->txri) ;
 
	uart->txbuff[i] = d;	/* Put a byte into Tx byffer */
	__disable_irq();
	uart->txwi = ni;
	*add_scr |= 0x08;	/* Set TIE (enable TX ready irq) */
	__enable_irq();
}

uint8_t uart_received(volatile UARTFIFO *uart)
{
	uint16_t num_ri, num_wi;
	
	num_ri = uart->rxri;
	num_wi = uart->rxwi;
	
	if(num_ri == num_wi)
		return(0);
	else
		return(1);
}

void uart_flush(volatile UARTFIFO *uart)
{
	/* Clear Rx buffer */
	uart->rxri = 0; uart->rxwi = 0;
	memset(uart->rxbuff, 0, BUFF_SIZE);
}

void uart_init (volatile UARTFIFO *uart, uint32_t bps)
{
	volatile uint8_t *add_smr, *add_scr, *add_escr;
	volatile uint16_t *add_bgr;
	uint8_t rx_irqn, tx_irqn;
	uint8_t mfs, loc;
	
	/* Clear Tx/Rx buffers */
	uart->txri = 0; uart->txwi = 0;
	uart->rxri = 0; uart->rxwi = 0;
	
	mfs = uart->mfsch;
	loc = uart->locno;
	
	switch(mfs)
	{
		case 0:		/* MFS0 */
			if(Uart0_0)		return;
			Uart0_0 = uart;		break;
		case 1:		/* MFS1 */
			if(Uart0_1)		return;
			Uart0_1 = uart;		break;
		case 2:		/* MFS2 */
			if(Uart0_2)		return;
			Uart0_2 = uart;		break;
		case 3:		/* MFS3 */
			if(Uart0_3)		return;
			Uart0_3 = uart;		break;
		case 4:		/* MFS4 */
			if(Uart0_4)		return;
			Uart0_4 = uart;		break;
		case 5:		/* MFS5 */
			if(Uart0_5)		return;
			Uart0_5 = uart;		break;
		case 6:		/* MFS6 */
			if(Uart0_6)		return;
			Uart0_6 = uart;		break;
		case 7:		/* MFS7 */
			if(Uart0_7)		return;
			Uart0_7 = uart;		break;
		default:
			return;
	}

	add_smr = (uint8_t *)(registerTable_mfs_set[mfs][T_SMR]);
	add_scr = (uint8_t *)(registerTable_mfs_set[mfs][T_SCR]);
	add_escr = (uint8_t *)(registerTable_mfs_set[mfs][T_ESCR]);
	add_bgr = (uint16_t *)(registerTable_mfs_set[mfs][T_BGR]);
	rx_irqn = settingTable_irqn[mfs][T_RXIRQN];
	tx_irqn = settingTable_irqn[mfs][T_TXIRQN];
	
	__disable_irqn(rx_irqn);
	__disable_irqn(tx_irqn);

	/* Initialize MFS (UART0 mode, N81)*/
	*add_scr = 0x80;		/* Disable MFS */
	*add_smr = 0x01;		/* Enable SOT output */
	*add_escr = 0;
	*add_bgr = F_PCLK / bps - 1;
	*add_scr = 0x13;	/* Enable MFS: Set RIE/RXE/TXE */
	
	/* Attach MFS module to I/O pads */
	uart_attach_mfs(mfs, loc);	/* Attach MFS module to I/O pads */
	
	/* Enable Tx/Rx/Error interrupts */
//	__set_irqn_priority(rx_irqn, 192);
//	__set_irqn_priority(tx_irqn, 192);
	__enable_irqn(rx_irqn);
	__enable_irqn(tx_irqn);
}

void uart_detach_buffer(volatile UARTFIFO *uart)
{
	uint8_t mfs;
	
	mfs = uart->mfsch;
	
	switch(mfs)
	{
		case 0:		/* MFS0 */
			Uart0_0 = 0;		break;
		case 1:		/* MFS1 */
			Uart0_1 = 0;		break;
		case 2:		/* MFS2 */
			Uart0_2 = 0;		break;
		case 3:		/* MFS3 */
			Uart0_3 = 0;		break;
		case 4:		/* MFS4 */
			Uart0_4 = 0;		break;
		case 5:		/* MFS5 */
			Uart0_5 = 0;		break;
		case 6:		/* MFS6 */
			Uart0_6 = 0;		break;
		case 7:		/* MFS7 */
			Uart0_7 = 0;		break;
		default:
			return;
	}
}

void uart_close(volatile UARTFIFO *uart)
{
	volatile uint8_t *add_smr, *add_scr, *add_escr;
	volatile uint16_t *add_bgr;
	uint8_t rx_irqn, tx_irqn;
	uint8_t mfs, loc;
	
	uart_detach_buffer(uart);
	
	mfs = uart->mfsch;
	loc = uart->locno;

	add_smr = (uint8_t *)(registerTable_mfs_set[mfs][T_SMR]);
	add_scr = (uint8_t *)(registerTable_mfs_set[mfs][T_SCR]);
	add_escr = (uint8_t *)(registerTable_mfs_set[mfs][T_ESCR]);
	add_bgr = (uint16_t *)(registerTable_mfs_set[mfs][T_BGR]);
	rx_irqn = settingTable_irqn[mfs][T_RXIRQN];
	tx_irqn = settingTable_irqn[mfs][T_TXIRQN];
	
	__disable_irqn(rx_irqn);
	__disable_irqn(tx_irqn);
	
	*add_scr = 0x00;		/* Disable MFS */
	*add_smr = 0;
	*add_escr = 0;
	*add_bgr = 0;
	
	uart_detach_mfs(mfs, loc);
}
