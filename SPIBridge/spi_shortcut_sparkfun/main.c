#include "ft245.h"
#include <avr/io.h>
#include "UComMaster.h"
#include "spi.h"
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include "i2c.h"

volatile uint8_t current_phase = '1';     // Mode 1
volatile uint8_t current_dorder = '0';    // MSB First
volatile uint8_t current_frequency = '4'; // 500KHz

volatile uint16_t bits;
volatile uint8_t cnt;
volatile uint16_t fcnt=0;

uint8_t sendonly = 1;
volatile uint16_t send_string[256];


char getc245_blocking(void);
void send_ascii(void);
void toggle_mode(void);
void menu(void);
void actions_menu(void);
void settings_menu(void);
void send_single_ascii(void);
void send_single_hex(void);
void send_file(void);
void continuous_receive(void);
void single_receive(void);
void send_hex_string(void);
void send_ascii_string(void);
void send_command_string(void);
void set_polarity_phase(void);
void my_printf(uint8_t string_num);
void change_frequency(void);
void set_dorder(void);
void show_settings(void);

void send_command_string_custom_l3(void);
void send_command_string_custom_rf(void);
void i2c_command_string(void);
void i2c_addr_search (void);

void bl_spi_proxi (void);

char const main_menu[] PROGMEM = "\n\r\n\r---Custom SparkFun SPI Shortcut v2.3---\n\r\n\rMAIN MENU:\n\r(1) Actions\n\r(2) Settings\n\r(3) Custom: L3 Board\n\r(4) Custom: RF Board\n\r(5) Custom: I2C\n\r(6) Custom: BL\n\r\n\r";	
char const arrow[] PROGMEM = "->";
char const invalid[] PROGMEM = "Invalid Character\n\r";
char const send_single[] PROGMEM = "Enter characters to send, press enter to return to menu\n\r->";
char const action_menu[] PROGMEM = "\n\rACTIONS MENU:\n\r(1) Send command string\n\r(2) Send ASCII characters\n\r(3) Continuous receive\n\r(4) Return to main menu\n\r\n\r";
char const cont_receive[] PROGMEM = "Receiving, CTRL+C to stop\n\r";
char const send_command[] PROGMEM = "Enter hex string of 256 values or less. Press return when finished.\n\rRR = Receive, CH = Chip Select High, CL = Chip Select Low, DY = 10ms Delay\n\r";
char const cs_high [] PROGMEM = "CS High\n\r";
char const cs_low[] PROGMEM = "CS Low\n\r";
char const string_sent[] PROGMEM = "String sent!\n\r";
char const settings[] PROGMEM = "\n\rSETTINGS MENU:\n\r(1) Set clock polarity and phase\n\r(2) Set frequency\n\r(3) Set data order\n\r(4) Show current settings\n\r(5) Return to main menu\n\r";
char const set_polarity_1[] PROGMEM = "\n\rClock settings can be defined in the following ways:\n\r\n\rCPOL/CPHA___LEADING EDGE________TRAILING EDGE________MODE\n\r0/0         Sample (Rising)     Setup (Falling)       (1)\n\r";
char const set_polarity_2[] PROGMEM = "0/1         Setup (Rising)      Sample (Falling)      (2)\n\r1/0         Sample (Falling)    Setup (Rising)        (3)\n\r1/1         Setup (Falling)     Sample(Rising)        (4)\n\r\n\r";
char const set_polarity_3[] PROGMEM = "Change to mode:";
char const mode_changed[] PROGMEM = "\n\rMode changed!\n\r\n\r";
char const frequency_menu_1[] PROGMEM = "\n\rFrequency Options:\n\r\n\rMODE    EFFECTIVE FREQUENCY\n\r(1)          4MHz\n\r(2)          2MHz\n\r(3)          1MHz\n\r(4)          500kHz\n\r(5)          250kHz\n\r";
char const frequency_menu_2[] PROGMEM = "(6)          125kHz\n\r(7)          62.5kHz\n\r\n\rNew frequency mode: ";
char const frequency_changed[] PROGMEM = "Frequency changed!\n\r\n\r";
char const dorder_menu[] PROGMEM = "\n\rData Order Modes:\n\r\n\r(0)   MSB transmitted first\n\r(1)   LSB transmitted first\n\rNew data mode: ";
char const dorder_changed[] PROGMEM = "Data order changed!\n\r\n\r";
char const cur_settings[] PROGMEM = "\n\rCurrent Settings:\n\r";
char const data_order[] PROGMEM = "Data Order: ";
char const msb[] PROGMEM = "MSB\n\r\n\r";
char const lsb[] PROGMEM = "LSB\n\r\n\r";
char const command_delay[] PROGMEM = "Delay 10ms\n\r";

char const l3_prompt[] PROGMEM = "L3->";
char const line_set[] PROGMEM = "Line status is set!";
char const rf_prompt[] PROGMEM = "RF->";
char const i2c_prompt[] PROGMEM = "I2C->";
char const bl_prompt[] PROGMEM = "BL->";

PGM_P const string_table[] PROGMEM = 
{
	main_menu,
	arrow,
	invalid,
	send_single,
	action_menu,
	cont_receive,
	send_command,
	cs_high,
	cs_low,
	string_sent,
	settings,
	set_polarity_1,
	set_polarity_2,
	set_polarity_3,
	mode_changed,
	frequency_menu_1,
	frequency_menu_2,
	frequency_changed,
	dorder_menu,
	dorder_changed,
	cur_settings,
	data_order,
	msb,
	lsb,
	command_delay,
	l3_prompt,
	line_set,
	rf_prompt,
	i2c_prompt,
	bl_prompt
};


char buffer[150];


int main(void)
{
	DDRB |= (1<<LED);
	DDRC |= ((1<<SDA) | (1<<SCL));
	PORTB |= (1<<LED);
	for(int i = 0; i < 3; i++) // Test sequence
	{
		PORTC |= (1<<SDA);
		delay_ms(5);
		PORTC &= ~(1<<SDA);
		PORTC |= (1<<SCL);
		delay_ms(5);
		PORTC &= ~(1<<SCL);
	}
	PORTC |= ((1<<SDA) | (1<<SCL));
	ioinit();
	spi_init(); 
	i2c_init(I2C_400KHZ);
	menu();
	

}

void menu()
{
	char c = 0; 
	deselect();	
	
	my_printf(MAIN_MENU); // Print menu
	my_printf(ARROW);
	
	c = getc245_blocking();
	printf245("%c\n\r",c);
	
	switch(c)
	{
		case '1':
			actions_menu();
			break;
		case '2':
			settings_menu();
			break;
		case '3':
		    
		    // Set Line 1 (PC4/SDA) Line 2 (PC5/SCL) configuration
			DDRC |= ((1<<SDA) | (1<<SCL));  // out/out
		    PORTC |= ((1<<SDA) | (1<<SCL)); // Hi/Hi
		    // Set SPI configuration for L3 board Attenuators 
			current_phase = '1';     // Mode 1
			current_dorder = '1';    // LSB First
			current_frequency = '4'; // 500KHz
			// SPI (+ SPI Irq) is enabled as Master, MODE 1, LBS, 500KHz
			SPCR = (1<<SPIE) | (1<<SPE) | (1<<MSTR) | (1<<DORD) | (1<<SPR0);
			SPSR &= ~(1<<SPI2X);
		    send_command_string_custom_l3();
			break;	
		case '4':
		    // Set Line 1 (PC4/SDA) Line 2 (PC5/SCL) configuration
			DDRC &= ~(1<<SDA); // in
			DDRC &= ~(1<<SCL); // in
		    PORTC |= ((1<<SDA) | (1<<SCL)); // pull-up/pull-up
		    // Set SPI configuration for communication with RFM board
		    current_phase = '1';     // Mode 1
		    current_dorder = '0';    // MSB First
		    current_frequency = '4'; // 500KHz
		    // SPI (+ SPI Irq) is enabled as Master, MODE 1, MBS, 500KHz
		    SPCR = (1<<SPIE) | (1<<SPE) | (1<<MSTR) | (1<<SPR0);
		    SPSR &= ~(1<<SPI2X);
		    send_command_string_custom_rf();
		    break;
		case '6': //BL
		    // Set Line 1 (PC4/SDA) Line 2 (PC5/SCL) configuration
		    DDRC &= ~(1<<SDA); // in
		    DDRC &= ~(1<<SCL); // in
		    PORTC |= ((1<<SDA) | (1<<SCL)); // pull-up/pull-up
		    // Set SPI configuration for communication with RFM board
		    current_phase = '1';     // Mode 1
		    current_dorder = '0';    // MSB First
		    current_frequency = '4'; // 500KHz
		    // SPI (+ SPI Irq) is enabled as Master, MODE 1, MBS, 500KHz
		    SPCR = (1<<SPIE) | (1<<SPE) | (1<<MSTR) | (1<<SPR0);
		    //SPSR &= ~(1<<SPI2X);
			SPSR |= 0x01; // Frequency doubler: 1MHz
		    bl_spi_proxi();
		    break;	
		case '5':
		
		   // activate internal pull-ups for twi
           // as per note from atmega8 manual pg167
           //sbi(PORTC, 4);
           //sbi(PORTC, 5);

           // initialize twi prescaler and bit rate
           //cbi(TWSR, TWPS0); // TWI Status Register - Prescaler bits
           //cbi(TWSR, TWPS1);

           /* twi bit rate formula from atmega128 manual pg 204
           SCL Frequency = CPU Clock Frequency / (16 + (2 * TWBR))
           note: TWBR should be 10 or higher for master mode
           It is 72 for a 16mhz Wiring board with 100kHz TWI */

           //TWBR = ((CPU_FREQ / TWI_FREQ) - 16) / 2; // bitrate register
           // enable twi module, acks, and twi interrupt
		   
		   //TWCR = 1 << TWEN; // enable twi module, no interrupt
		
		    i2c_command_string();
		    break;				
		default:
		{
			my_printf(INVALID);
			menu();
			break;
		}
	}
}

void bl_spi_proxi(void)
{
  char byte_char, l1, h, l;
  
  // Put out BL-> prompt
  my_printf(BL_PROMPT);
  
  h = 0;
  l = 0;
  while (1)
  {
	  byte_char = getchar245();
	  if (byte_char)
	  {
		 if (h)
		 {
			 l = byte_char;
		 }
		 else
		 {
			 h = byte_char;
		 }
		 
		 if((h == 13) || (h == 13))
		 {  
			 // Put out BL-> prompt
			 my_printf(BL_PROMPT);
			 h = 0;
			 l = 0;
		 }
		 
		 if ((h == 'E') && (l == 'X'))
		 {
			 // Exit Custom BL-> mode
			 menu();
			 return;
		 }
		 
		 if (h && l)
		 {
			 if(h >= 48 && h <= 57){ h -= 48; } 	  // h is a number
			 else if(h >= 65 && h <= 70){ h -= 55; } // h is a letter
			 else if(h >= 97 && h <= 102){ h -= 87; } // h is a letter
			 else{ h = 0; }
			 
			 if(l >= 48 && l <= 57){ l -= 48; } 	  // l is a number
			 else if(l >= 65 && l <= 70){ l -= 55; } // l is a letter
			 else if(l >= 97 && l <= 102){ l -= 87; } // l is a letter
			 else{ l = 0; }
			 
			 h = (h<<4) + l;
			 
			 select();
			 send_spi_byte(h);
			 deselect();
			 
			 h = 0;
			 l = 0;
		 }
		  
	  }
	  
	  
	  l1 = PINC & (1<<SDA);
	  if (!l1)
	  {
		 select(); // CS LOW
		 send_spi_byte(0x00);
		 byte_char = SPDR;
		 deselect(); 
		 
		 printf245("%x ", byte_char);
	  }
	  
  }	
}

void send_command_string_custom_l3(void)
{
	char h = 0, l = 0;
	uint16_t i = 0, j = 0; //, receive = 1;
	
	// Put out L3> prompt
	my_printf(L3_PROMPT);
	while(i < 256)
	{
		h = getc245_blocking();
		if(h == 13){ break; }
		printf245("%c",h);
		l = getc245_blocking();
		if(l == 13){ break; }
		printf245("%c ",l);
		
		if ((h == 'E') && (l == 'X'))
		{
			// Exit Custom L3> mode
			menu();
			return;
		}

		
		if(h == 'R') // Receive
		{
			send_string[i] = 0x0100;
		}
		else if(l == 'H') // CS High
		{
			send_string[i] = 0x0101;
		}
		
		else if(l == 'L') // CS Low
		{
			send_string[i] = 0x0102;
		}
		else if(l == 'Y')
		{
			send_string[i] = 0x0103;
		}
		else if ((h == 'L') && (l == '1'))
		{
			// Line 1 set LOW
			send_string[i] = 0x0104;
		}
		else if ((h == 'L') && (l == '2'))
		{
			// Line 2 set LOW
			send_string[i] = 0x0105;
		}
		else if ((h == 'H') && (l == '1'))
		{
			// Line 1 set HIGH
			send_string[i] = 0x0106;
		}
		else if ((h == 'H') && (l == '2'))
		{
			// Line 2 set HIGH
			send_string[i] = 0x0107;
		}
		else
		{
			if(h >= 48 && h <= 57){ h -= 48; } 	  // h is a number
			else if(h >= 65 && h <= 70){ h -= 55; } // h is a letter
			else if(h >= 97 && h <= 102){ h -= 87; } // h is a letter
			else{ my_printf(INVALID); send_command_string_custom_l3(); }
			
			if(l >= 48 && l <= 57){ l -= 48; } 	  // l is a number
			else if(l >= 65 && l <= 70){ l -= 55; } // l is a letter
			else if(l >= 97 && l <= 102){ l -= 87; } // l is a letter
			else{ my_printf(INVALID); send_command_string_custom_l3(); }
			
			h = (h<<4) + l;
			send_string[i] = h;
		}
		
		i++;
		
	}
	printf245("\n\r");
	
	if (i == 0) 
	{
		send_command_string_custom_l3();
	}
	
	if(i == 256){ i--; }
	for(j = 0; j < i; j++)
	{
		
		if(send_string[j] == 0x0100) // Receive
		{
			send_spi_byte(0x00);
			send_string[j] = 0x0200 + SPDR;
		}
		else if(send_string[j] == 0x0101){ deselect(); }// CS HIGH
		else if(send_string[j] == 0x0102){ select(); } // CS LOW
		else if(send_string[j] == 0x0103){ delay_ms(10); } // DELAY
		else if(send_string[j] == 0x0104){ PORTC &= ~(1<<SDA); } // Line 1 set LOW
		else if(send_string[j] == 0x0105){ PORTC &= ~(1<<SCL); } // Line 2 set LOW
		else if(send_string[j] == 0x0106){ PORTC |= (1<<SDA); } // Line 1 set HIGH
		else if(send_string[j] == 0x0107){ PORTC |= (1<<SCL); } // Line 2 set HIGH				
		else{ send_spi_byte(send_string[j]); }
	}
	deselect();
	
	// PRINT RESULTS
	//for(j = 0; j < i; j++)
	//{
		
	//	if(send_string[j] >= 0x0200){ printf245("R%d = 0x%x\n\r",receive,(send_string[j] & 0x00FF)); receive++; }
	//	else if(send_string[j] == 0x0101){ my_printf(CS_HIGH); }
	//	else if(send_string[j] == 0x0102){ my_printf(CS_LOW); }
	//	else if(send_string[j] == 0x0103){ my_printf(DELAY); }
	//	else{ printf245("Sent 0x%x\n\r", send_string[j]); }
	//}
	
	my_printf(STRING_SENT);
	
	//actions_menu();
	
	send_command_string_custom_l3();
}

void send_command_string_custom_rf(void)
{
	char h = 0, l = 0, l1=0,l2=0;
	uint16_t i = 0, j = 0, receive = 0;
	
	// Put out RF-> prompt
	my_printf(RF_PROMPT);
	while(i < 256)
	{
		h = getc245_blocking();
		if(h == 13){ break; }
		printf245("%c",h);
		l = getc245_blocking();
		if(l == 13){ break; }
		printf245("%c ",l);
		
		if ((h == 'E') && (l == 'X'))
		{
			// Exit Custom RF-> mode
			menu();
			return;
		}

		
		if(h == 'R') // Receive
		{
			send_string[i] = 0x0100;
		}
		else if(l == 'H') // CS High
		{
			send_string[i] = 0x0101;
		}
		
		else if(l == 'L') // CS Low
		{
			send_string[i] = 0x0102;
		}
		else if(l == 'Y')
		{
			send_string[i] = 0x0103;
		}
		else if ((h == 'L') && (l == '1'))
		{
			// Line 1 set LOW
			send_string[i] = 0x0104;
		}
		else if ((h == 'L') && (l == '2'))
		{
			// Line 2 set LOW
			send_string[i] = 0x0105;
		}
		else if ((h == 'H') && (l == '1'))
		{
			// Line 1 set HIGH
			send_string[i] = 0x0106;
		}
		else if ((h == 'H') && (l == '2'))
		{
			// Line 2 set HIGH
			send_string[i] = 0x0107;
		}
		else if ((h == '?') && (l == '1'))
		{
			// Check status Line 1
			send_string[i] = 0x0108;
		}
		else if ((h == '?') && (l == '2'))
		{
			// Check status Line 2
			send_string[i] = 0x0109;
		}
		else
		{
			if(h >= 48 && h <= 57){ h -= 48; } 	  // h is a number
			else if(h >= 65 && h <= 70){ h -= 55; } // h is a letter
			else if(h >= 97 && h <= 102){ h -= 87; } // h is a letter
			else{ my_printf(INVALID); send_command_string_custom_rf(); }
			
			if(l >= 48 && l <= 57){ l -= 48; } 	  // l is a number
			else if(l >= 65 && l <= 70){ l -= 55; } // l is a letter
			else if(l >= 97 && l <= 102){ l -= 87; } // l is a letter
			else{ my_printf(INVALID); send_command_string_custom_rf(); }
			
			h = (h<<4) + l;
			send_string[i] = h;
		}
		
		i++;
		
	}
	printf245("\n\r");
	
	if (i == 0)
	{
		send_command_string_custom_rf();
	}
	
	if(i == 256){ i--; }
	for(j = 0; j < i; j++)
	{
		
		if(send_string[j] == 0x0100) // Receive
		{
			send_spi_byte(0x00);
			send_string[j] = 0x0200 + SPDR;
		}
		else if(send_string[j] == 0x0101){ deselect(); }// CS HIGH
		else if(send_string[j] == 0x0102){ select(); } // CS LOW
		else if(send_string[j] == 0x0103){ delay_ms(10); } // DELAY
		else if(send_string[j] == 0x0104){ PORTC &= ~(1<<SDA); } // Line 1 set LOW
		else if(send_string[j] == 0x0105){ PORTC &= ~(1<<SCL); } // Line 2 set LOW
		else if(send_string[j] == 0x0106){ PORTC |= (1<<SDA); } // Line 1 set HIGH
		else if(send_string[j] == 0x0107){ PORTC |= (1<<SCL); } // Line 2 set HIGH
		else if(send_string[j] == 0x0108){ l1 = PINC & (1<<SDA); break; } // Check status Line 1 
		else if(send_string[j] == 0x0109){ l2 = PINC & (1<<SCL); break; } // Check status Line 2		
		else{ send_spi_byte(send_string[j]); }
	}
	deselect();
	

	if (send_string[j] == 0x0108) 
	{
		if (l1)
		{
			printf245("%c%c\n\r", 'H','1');
		}
		else
		{
		   printf245("%c%c ", 'L','1');	
		   
		   select(); // CS LOW
		   do 
		   {	   
			   send_spi_byte(0x00);
			   printf245("%x ", SPDR);
			   receive++;
			   l1 = PINC & (1<<SDA);
			   
		   } while ((l1 == 0) && (receive < 256));
		   deselect();
		   printf245("%c%c\n\r", 'H','1');
		   
		}
		
	} 
	else if (send_string[j] == 0x0109)
	{
		if (l2)
		{
			printf245("%c%c\n\r", 'H','2');
		}
		else
		{
			printf245("%c%c\n\r", 'L','2');
		}
	}  
	else
	{
		my_printf(STRING_SENT);
	}
	
	send_command_string_custom_rf();
}

void i2c_command_string(void)
{
	char h = 0, l = 0;
	uint16_t i = 0, j = 0;
	char i2c_addr_flag = 1, i2c_error = 0;
	
	// Put out I2C-> prompt
	my_printf(I2C_PROMPT);
	while(i < 256)
	{
		h = getc245_blocking();
		if(h == 13){ break; }
		printf245("%c",h);
		l = getc245_blocking();
		if(l == 13){ break; }
		printf245("%c ",l);
		
		if ((h == 'E') && (l == 'X'))
		{
			// Exit I2C-> mode
			menu();
			return;
		}

		if ((h == 'S')&& (l == 'T')) //ST
		{   //I2C Start 
			send_string[i] = 0x0100;
			
			i2c_addr_flag = 1;
		}
		else if ((h == 'S')&& (l == 'P')) //SP
		{   //I2C Stop
			send_string[i] = 0x0101;
		}
		else if ((h == 'R') && (l == 'A'))
		{   // Receive with ACK
			send_string[i] = 0x0102;
		}
		else if ((h == 'R') && (l == 'N'))
		{   // Receive with NACK
			send_string[i] = 0x0103;
		}
		else if ((h == 'R') && (l == 'S'))
		{   // Re-Start
			send_string[i] = 0x0104;
			i2c_addr_flag = 1;
		}
		else if((h == 'D') && (l == 'Y'))
		{   // Delay 10ms
			send_string[i] = 0x0105;
		}
		//else if ((h == 'A') && (l == 'T')) //AT
		//{   // Address Test
		//	send_string[i] = 0x0106;
		//}
		else
		{
			if(h >= 48 && h <= 57){ h -= 48; } 	  // h is a number
			else if(h >= 65 && h <= 70){ h -= 55; } // h is a letter
			else if(h >= 97 && h <= 102){ h -= 87; } // h is a letter
			else{ my_printf(INVALID); i2c_command_string(); }
			
			if(l >= 48 && l <= 57){ l -= 48; } 	  // l is a number
			else if(l >= 65 && l <= 70){ l -= 55; } // l is a letter
			else if(l >= 97 && l <= 102){ l -= 87; } // l is a letter
			else{ my_printf(INVALID); i2c_command_string(); }
			
			h = (h<<4) + l;
			send_string[i] = h;
			
			if (i2c_addr_flag)
			{
				send_string[i] |= 0x0200;
				i2c_addr_flag = 0;
			}
		}
		
		i++;
		
	} //End of while(i < 256)
	printf245("\n\r");
	
	if (i == 0)
	{
		i2c_command_string();
	}
	
	if(i == 256){ i--; }
	for(j = 0; j < i; j++)
	{
		switch (send_string[j])
		{
			//case 0x0106: // I2C Address Test
			//{
			//	i2c_addr_search();
			//	break;
			//}
			case 0x0105: // DELAY
			{ 
				delay_ms(10); 
				printf245("%c%c ", 'D','Y');
				break;
			} 
			case 0x0100: // I2C Start
			{
				TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
				printf245("%c%c ", 'S','T');
				while (!(TWCR & (1<<TWINT)));
				if ((TWSR & 0xF8) != I2C_STATUS_START)
				{
					printf245("%c%x ",'N', TWSR & 0xF8); //Error
					i2c_error++;
				}
				
				break;
			}
			case 0x0104: // I2C Re-Start
			{
				TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWSTO)|(1<<TWEN);
				printf245("%c%c ", 'R','S');
				while (!(TWCR & (1<<TWINT)));
				if ((TWSR & 0xF8) != I2C_STATUS_START) //I2C_STATUS_RSTART
				{
					printf245("%c%x ",'N', TWSR & 0xF8);//Error
					i2c_error++;
				}
				
				break;
			}
			case 0x0102: // Receive with ACK
			{
				TWCR = (1<<TWINT)|(1<<TWEA)|(1<<TWEN);
				while (!(TWCR & (1<<TWINT)));
				
				send_string[j] = TWDR;
				printf245("%x ", send_string[j]); 
				
				if ((TWSR & 0xF8) != I2C_STATUS_DATA_R_ACK)
				{
					printf245("%c%x ",'N', TWSR & 0xF8); //Error
					i2c_error++;
				}
				
				break;
			}
			case 0x0103: // Receive with NACK (the last byte)
			{
				TWCR = (1<<TWINT)|(1<<TWEN);
				while (!(TWCR & (1<<TWINT)));
				
				send_string[j] = TWDR;
				printf245("%x ", send_string[j]);
				
				if ((TWSR & 0xF8) != I2C_STATUS_DATA_R_NACK)
				{
					printf245("%c%x ",'N', TWSR & 0xF8);//Error
					i2c_error++;
				}
				
				break;
			}
			case 0x0101: // I2C Stop
			{
				TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);
				printf245("%c%c ", 'S','P');
				break;
			}
			default:
			{
				if ((send_string[j] & 0xFF00) == 0x0200)
				{  // Slave Addr + W/R
					if (send_string[j] & 0x0001)
					{  // Read
					   TWDR = (char)(send_string[j]&0x00FF);
					   TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA);
					   
					   printf245("%x ", send_string[j]); 
					   
					   while (!(TWCR & (1<<TWINT)));
					   if ((TWSR & 0xF8) != I2C_STATUS_SLA_R_ACK)
					   {
						  printf245("%c%X ",'N', TWSR & 0xF8);//Error
						  i2c_error++;
					   }	
					}
					else
					{ // Write
					  TWDR = (char)(send_string[j]&0x00FF);
					  TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA);	
					  
					  printf245("%x ", send_string[j]); 
					  
					  while (!(TWCR & (1<<TWINT)))
					  if ((TWSR & 0xF8) != I2C_STATUS_SLA_W_ACK)
					  {
						 printf245("%c%x ",'N', TWSR & 0xF8);//Error
						  i2c_error++;
					  }
					}
				   	
				}
				else
				{  // Transmit data
				   TWDR = (char)(send_string[j]&0x00FF);
				   TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA);
				   
				   printf245("%x ", send_string[j]); 
				   
				   while (!(TWCR & (1<<TWINT)));
				   if ((TWSR & 0xF8) != I2C_STATUS_DATA_W_ACK)
				   {
					   printf245("%c%x ",'N', TWSR & 0xF8);//Error
					   i2c_error++;
					   
				   }
				}
				
				break;
			}
			
		}// End of switch (send_string[j])
		
		if (i2c_error)
		{
			TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO)|(1<<TWEA);
			printf245("%c%c ", 'S','P');
			break;
		}
		
	}// End of for(j = 0; j < i; j++)
	
	printf245("\n\r");
	i2c_command_string();
}

void i2c_addr_search (void)
{
	unsigned char i2c_addr = 0;
	
	printf245("\n\r");
	
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN)|(1<<TWEA);
	printf245("%c%c ", 'S','T');
	while (!(TWCR & (1<<TWINT)));
	if ((TWSR & 0xF8) != I2C_STATUS_START)
	{
		printf245("%c%X ",'N', TWSR & 0xF8); //Error
		//i2c_error++;
		TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO)|(1<<TWEA);
		printf245("%c%c\n\r", 'S','P');		
		return;
	}
	
	
	for (i2c_addr=0x00; i2c_addr < 0x80; i2c_addr++)
	{
		TWDR = i2c_addr << 1;
		TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA);
		while (!(TWCR & (1<<TWINT)));
		if ((TWSR & 0xF8) != I2C_STATUS_SLA_W_ACK)
		{
			printf245("%x %c%x\n\r", i2c_addr, 'N', TWSR & 0xF8);//Error
			
			TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN)|(1<<TWEA);
			printf245("%c%c ", 'R','S');
			while (!(TWCR & (1<<TWINT)));
			if ((TWSR & 0xF8) != I2C_STATUS_RSTART)
			{
				printf245("%c%X ",'N', TWSR & 0xF8); //Error
				break;
				//i2c_error++;
			}
			
			//i2c_error++;
		}
		else
		{
			printf245("%x", i2c_addr);
			break;
		}
		
	}
	
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO)|(1<<TWEA);
	printf245("%c%c\n\r", 'S','P');
	
	return;
	
}

/*************************************************************************** 
	ACTIONS SECTION:       
		Actions include
			-Send command string
			-Send ASCII one at a time
			-Continuous Receive 
****************************************************************************/
void actions_menu(void)
{
	char c = 0;
	
	my_printf(ACTIONS_MENU);
	my_printf(ARROW);
	c = getc245_blocking();
	printf245("%c\n\r\n\r",c);
	switch(c)
	{
		case '1':
			send_command_string();
			break;
		case '2':
			send_single_ascii();
			break;
		case '3':
			continuous_receive();
			break;
		case '4':
			menu();
			break;
		default:
		{
			my_printf(INVALID);
			actions_menu();
			break;
		}
	}
}

void send_single_ascii(void)
{
	char c = 0;
	my_printf(SEND_SINGLE);
	while(1)  
	{
		c = getc245_blocking();
		if(c == 13){ printf245("\n\r\n\r"); actions_menu(); }
		printf245("Sent %c\n\r",c);
		select();
		send_spi_byte(c);
		deselect();
	}
}



void continuous_receive(void)
{
	char c = 0;
	uint8_t i = 0;
	my_printf(CONT_RECEIVE);
	while(c != 3)
	{
		send_spi_byte(0x00);
		printf245("%x ",SPDR);
		i++;
		c = getchar245();
		if(i == 15){ printf245("\n\r"); i = 0; }
	}
	printf245("\n\r\n\r");
	actions_menu();
}

void send_command_string(void)
{
	char h = 0, l = 0;
	uint16_t i = 0, j = 0, receive = 1;
	my_printf(SEND_COMMAND);
	while(i < 256)
	{
		h = getc245_blocking();
		if(h == 13){ break; } 
		printf245("%c",h);
		l = getc245_blocking();
		if(l == 13){ break; }
		printf245("%c ",l);
		
		if(h == 'R') // Receive
		{
			send_string[i] = 0x0100;
		}
		else if(l == 'H') // CS High
		{
			send_string[i] = 0x0101;
		}
		
		else if(l == 'L') // CS Low
		{
			send_string[i] = 0x0102;
		}
		else if(l == 'Y')
		{
			send_string[i] = 0x0103;
		}
		else
		{
			if(h >= 48 && h <= 57){ h -= 48; } 	  // h is a number
			else if(h >= 65 && h <= 70){ h -= 55; } // h is a letter
			else if(h >= 97 && h <= 102){ h -= 87; } // h is a letter
			else{ my_printf(INVALID); send_command_string(); }
		
			if(l >= 48 && l <= 57){ l -= 48; } 	  // l is a number
			else if(l >= 65 && l <= 70){ l -= 55; } // l is a letter
			else if(l >= 97 && l <= 102){ l -= 87; } // l is a letter
			else{ my_printf(INVALID); send_command_string(); }
		
			h = (h<<4) + l; 
			send_string[i] = h;
		}
		
		i++;
			
	}
	printf245("\n\r");
	if(i == 256){ i--; }
	for(j = 0; j < i; j++)
	{
	
		if(send_string[j] == 0x0100) // Receive
		{
			send_spi_byte(0x00);
			send_string[j] = 0x0200 + SPDR;
		}
		else if(send_string[j] == 0x0101){ deselect(); }// CS HIGH
		else if(send_string[j] == 0x0102){ select(); } // CS LOW
		else if(send_string[j] == 0x0103){ delay_ms(10); } // DELAY
		else{ send_spi_byte(send_string[j]); }
	}
	deselect();
	// PRINT RESULTS
	for(j = 0; j < i; j++)
	{
	
		if(send_string[j] >= 0x0200){ printf245("R%d = 0x%x\n\r",receive,(send_string[j] & 0x00FF)); receive++; }
		else if(send_string[j] == 0x0101){ my_printf(CS_HIGH); }
		else if(send_string[j] == 0x0102){ my_printf(CS_LOW); }
		else if(send_string[j] == 0x0103){ my_printf(DELAY); }
		else{ printf245("Sent 0x%x\n\r", send_string[j]); }
	}
	my_printf(STRING_SENT);
	
	actions_menu();
}

/*************************************************************************** 
	SETTINGS SECTION:       
		Settings include
			-Set clock polarity and phase
			-Change frequency
			-Change parity
****************************************************************************/

void settings_menu(void)
{
	char c = 0;
	
	my_printf(SETTINGS);
	my_printf(ARROW);
	
	c = getc245_blocking();
	printf245("%c\n\r",c);
	
	switch(c)
	{
		case '1':
			set_polarity_phase();
			break;
		case '2':
			change_frequency();
			break;
		case '3':
			set_dorder();
			break;
		case '4':
			show_settings();
			break;
		case '5':
			menu();
			break;
		default:
		{
			my_printf(INVALID);
			menu();
			break;
		}
	}
}

void set_polarity_phase(void)
{
	uint8_t c = 0;
	my_printf(SET_POLARITY_1);
	my_printf(SET_POLARITY_2);
	my_printf(SET_POLARITY_3);
	
	c = getc245_blocking();
	printf245(" %c\n\r",c);
	current_phase = c;
	switch(c)
	{
		case '1':
		    SPCR &= ~(1<<CPHA);
		    SPCR &= ~(1<<CPOL);
			SPCR |= ((1<<SPE) | (1<<MSTR) | (1<<SPR0) | (1<<SPIE));
			break;
		case '2':
		    SPCR |= (1<<CPHA);
		    SPCR &= ~(1<<CPOL);
			SPCR |= ((1<<SPE) | (1<<MSTR) | (1<<SPR0) | (1<<SPIE));
			break;
		case '3':
		    SPCR &= ~(1<<CPHA);
		    SPCR |= (1<<CPOL);
			SPCR |= ((1<<SPE) | (1<<MSTR) | (1<<SPR0) | (1<<SPIE));
			break;
		case '4':
		    SPCR |= ((1<<CPHA) | (1<<CPOL));
			SPCR |= ((1<<SPE) | (1<<MSTR) | (1<<SPR0) | (1<<SPIE));
			break;
		default:
		{
			my_printf(INVALID);
			set_polarity_phase();
			break;
		}
	}
	my_printf(MODE_CHANGED);
	settings_menu();
}
	
void change_frequency(void)
{
	uint8_t c;
	my_printf(FREQUENCY_MENU_1);
	my_printf(FREQUENCY_MENU_2);
	c = getc245_blocking();
	printf245("%c\n\r",c);
	current_frequency = c;
	
	if(c == '1'|| c == '2'){ SPCR &= ~((1<<SPR0) | (1<<SPR1)); }	
	else if(c == '3'|| c == '4'){ SPCR &= ~(1<<SPR1); SPCR |= (1<<SPR0); }
	else if(c == '5'|| c == '6'){ SPCR &= ~(1<<SPR0); SPCR |= (1<<SPR1); }
	else if(c == '7'){ SPCR |= ((1<<SPR0) | (1<<SPR1)); }
	else{ my_printf(INVALID); }

	if(c == '1' || c == '3' || c == '5'){ SPSR |= 0x01; } // Frequency doubler
	else{ SPSR &= ~(1<<SPI2X); } 
	
	my_printf(FREQUENCY_CHANGED);
	settings_menu();
		
}

void set_dorder(void)
{
	uint8_t c;
	my_printf(DORDER_MENU);
	c = getc245_blocking();
	printf245("%c\n\r",c);
	current_dorder = c;
	if(c == '1'){ SPCR |= (1<<DORD); }
	else{ SPCR &= ~(1<<DORD); }
	my_printf(DORDER_CHANGED);
	settings_menu();
}

void show_settings(void)
{	
	my_printf(CURRENT_SETTINGS);
	printf245("Polarity/Phase Mode: %c\n\r", current_phase);
	printf245("Clock frequency: ");
	switch(current_frequency)
	{
		case '1': printf245("4MHz\n\r"); break;
		case '2': printf245("2MHz\n\r"); break;
		case '3': printf245("1MHz\n\r"); break;
		case '4': printf245("500kHz\n\r"); break;
		case '5': printf245("250kHz\n\r"); break;
		case '6': printf245("125kHz\n\r"); break;
		case '7': printf245("62.5kHz\n\r"); break; 
	}
	my_printf(DATA_ORDER);
	if(current_dorder == '1'){ my_printf(LSB); }
	else{ my_printf(MSB); }
	settings_menu();
}

char getc245_blocking()
{
	char c = 0;
	while(c == 0){ c = getchar245(); }
	return c;
}

void my_printf(uint8_t string_num)
{
	strcpy_P(buffer, (PGM_P)pgm_read_word(&(string_table[string_num])));
	printf245(buffer);
}

