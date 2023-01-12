/*
 * mdio_drv.c
 *
 * Created: 2022-12-20 13:38:33
 * Author : pejo
 */ 

#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "uart.h"

/* Define CPU frequency in Hz in Makefile or toolchain compiler configuration */
#ifndef F_CPU
#error "F_CPU undefined, please define CPU frequency in Hz in Makefile or compiler configuration"
#endif

/* Define UART baud rate here */
#define UART_BAUD_RATE 9600


#define MDIO_START 0x40
#define MDIO_CMDWR 0x10
#define MDIO_CMDRD 0x20


// Initialize SPI Master Device (with SPI interrupt)
void spi_init_master (void)
{
	// Enable pullup
	PORTB|=(1<<3);
	// Set MOSI, SCK, SS as Output
	DDRB=(1<<5)|(1<<2);
	
	// Enable SPI, Set as Master
	// Prescaler: Fosc/16, Enable Interrupts
	//The MOSI, SCK pins are as per ATMega8
	SPCR=(1<<SPE)|(1<<MSTR)|(1<<CPOL)|(1<<CPHA)|(1<<SPR0)|(0<<SPIE);
	

}

//Function to send and receive data
static inline unsigned char spi_tranceiver (unsigned char data)
{
	SPDR = data;                       //Load data into the buffer
	while(!(SPSR & (1<<SPIF) ));       //Wait until transmission complete
	return(SPDR);                      //Return received data
}

void mdio_write (uint8_t phyAddr, uint8_t regAddr, uint16_t data)
{
	uint8_t tmpa;
	uint8_t tmpb;
	
	// Precalculate
	tmpa= MDIO_START | MDIO_CMDWR | ((phyAddr >> 1) & 0x0F);
	tmpb= (phyAddr << 7) | ((regAddr << 2 ) & 0x7C) | 0x02;
	
	cli();
	
	// Set pin to output
	DDRB|=(1<<3);

	// Send 32 bits preamble
	for (uint8_t fcc=0; fcc<4; fcc++)
	{
		spi_tranceiver (0xff);
	}
	
	// Send address 	
	spi_tranceiver (tmpa);
	spi_tranceiver (tmpb);
	// Send data
	spi_tranceiver (data >> 8);
	spi_tranceiver (data & 0xFF);
	
	// Disable driver
	DDRB&=~(1<<3);
	
	sei();
}

void mdio_read (uint8_t phyAddr, uint8_t regAddr, uint16_t *data)
{
	uint8_t tmpa;
	uint8_t tmpb;
	
	// Precalculate
	tmpa= MDIO_START | MDIO_CMDRD | ((phyAddr >> 1) & 0x0F);
	tmpb= (phyAddr << 7) | ((regAddr << 2 ) & 0x7C) | 0x02;

	// Set pin to output
	DDRB|=(1<<3);
	
	cli();
	
	// Send 32 bits preamble
	for (uint8_t fcc=0; fcc<4; fcc++)
	{
		spi_tranceiver (0xff);
	}
	
	// Send address
	spi_tranceiver (tmpa);
	spi_tranceiver (tmpb);
	// Disable driver (should have been done one bit ago)
	DDRB&=~(1<<3);

	// Read data
	tmpa = spi_tranceiver (0x00);
	tmpb = spi_tranceiver (0x00);
	
	sei();
	
	*data=(tmpa << 8) | tmpb;
}

int main(void)
{
	char buffer[7];
	uint16_t c;
	uart_init(UART_BAUD_SELECT(UART_BAUD_RATE, F_CPU));
	spi_init_master();
	sei();
	uint16_t tmp;
	// Enable Global Interrupts
	
	uart_puts("String stored in SRAM\n");
	
	//sei();
    /* Replace with your application code */
    while (1) 
    {
//		mdio_write(0x10,0xff, 0x55AA);

		mdio_read(0x0c, 0x02, &tmp); // Read ADIN1200
		itoa(tmp, buffer, 16); // convert integer into string (decimal format)
		uart_puts(buffer);     // and transmit string to UART
		uart_putc(',');
		uart_putc(' ');
		
		mdio_read(0x0c, 0x03, &tmp); // Read ADIN1200
		itoa(tmp, buffer, 16); // convert integer into string (decimal format)
		uart_puts(buffer);     // and transmit string to UART
		uart_putc(',');
		uart_putc(' ');
		
		mdio_read(0x0c, 0x01, &tmp); // Read ADIN1200
		itoa(tmp, buffer, 16); // convert integer into string (decimal format)
		uart_puts(buffer);     // and transmit string to UART
		uart_putc('\r');
		for (unsigned int i=0; i < 0xFFFF; i++);
		
		c = uart_getc();
        if (!(c & UART_NO_DATA))
        {
			//uart_putc('-');
			itoa(c, buffer, 16); // convert integer into string (decimal format)
			//uart_puts(buffer);     // and transmit string to UART
			if (c & UART_FRAME_ERROR)
            {
                /* Framing Error detected, i.e no stop bit detected */
            //    uart_puts_P("UART Frame Error: ");
            }
            if (c & UART_OVERRUN_ERROR)
            {
                /*
                 * Overrun, a character already present in the UART UDR register was
                 * not read by the interrupt handler before the next character arrived,
                 * one or more received characters have been dropped
                 */
            //    uart_puts_P("UART Overrun Error: ");
            }
            if (c & UART_BUFFER_OVERFLOW)
            {
                /*
                 * We are not reading the receive buffer fast enough,
                 * one or more received character have been dropped
                 */
            //    uart_puts_P("Buffer overflow error: ");
            }
			
			//uart_putc((unsigned char)c);
			if ((unsigned char)c == 'x')
			{
				uart_putc('0');
				mdio_write(0x00,0x00, 0x1900);		
			}
			else if ((unsigned char)c == 'X')
			{
				uart_putc('1');
				mdio_write(0x00,0x00, 0x1100);
			}
        }
    }
}

