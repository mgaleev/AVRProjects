
#include <avr/io.h>
#include "UComMaster.h"
#include "spi.h"

extern uint8_t current_phase;     
extern uint8_t current_dorder;    
extern uint8_t current_frequency;

void spi_init()
{
	// Setup SPI master for programming	
	DDRB = (1<<MOSI) | (1<<SCK);        // Output: MOSI, SCK
    DDRB |= (1<<CS);                    // Output: CS
	DDRC |= (1<<4);                     // ???
	DDRB &= ~(1<<MISO);                 // Input : MISO
	
	// SPI (+ SPI Irq) is enabled as Master
	SPCR = (1<<SPIE) | (1<<SPE) | (1<<MSTR);
	
	// Default Configuration Settings
	
	if(current_frequency == '1'|| current_frequency == '2'){ SPCR &= ~((1<<SPR0) | (1<<SPR1)); }
	else if(current_frequency == '3'|| current_frequency == '4'){ SPCR &= ~(1<<SPR1); SPCR |= (1<<SPR0); }
	else if(current_frequency == '5'|| current_frequency == '6'){ SPCR &= ~(1<<SPR0); SPCR |= (1<<SPR1); }
	else if(current_frequency == '7'){ SPCR |= ((1<<SPR0) | (1<<SPR1)); }

	if(current_frequency == '1' || current_frequency == '3' || current_frequency == '5'){ SPSR |= 0x01; } // Frequency doubler
	else{ SPSR &= ~(1<<SPI2X); }
	
	switch(current_phase)
	{
		case '1': // 0/0
		SPCR &= ~(1<<CPHA);
		SPCR &= ~(1<<CPOL);
		break;
		case '2': //0/1
		SPCR |= (1<<CPHA);
		SPCR &= ~(1<<CPOL);
		break;
		case '3': //1/0
		SPCR &= ~(1<<CPHA);
		SPCR |= (1<<CPOL);
		break;
		case '4': //1/1
		SPCR |= ((1<<CPHA) | (1<<CPOL));
		break;
		default:
		break;
	}
	
	if(current_dorder == '1')
	    { SPCR |= (1<<DORD); }    //LSB First
	else{ SPCR &= ~(1<<DORD); }   //MSB First
}

void select(void){ cbi(PORTB, CS); } //cbi(PORTC, 4); }

void reselect(void) { deselect(); delay_ms(1); select(); }

void deselect(void) { sbi(PORTB, CS); } //sbi(PORTC, 4); }

void send_spi_byte(char c)
{
	SPDR = c;
	while(!(SPSR & (1<<SPIF)));
}


char read_spi_byte(void)
{
	while(!(SPSR & (1<<SPIF)));
	return SPDR;
}

