//https://github.com/JChristensen/tinySPI/blob/master/tinySPI.cpp


#include <stdint.h>
#include <avr/io.h>
#include <util/atomic.h>

#define USCK_DD_PIN DDB2
#define DO_DD_PIN DDB1


void adc_init();
unsigned short adc_read(unsigned char pin);
uint8_t spi_transfer(uint8_t data);
void usi_init();


uint8_t spi_transfer(uint8_t data)
{
	USIDR = data;
	USISR = _BV(USIOIF); //clear counter and counter overflow interrupt flag
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)   //ensure a consistent clock period
	{
		while ( !(USISR & _BV(USIOIF)) ) USICR |= _BV(USITC);
	}
	return USIDR;
}



void setup()
{
	adc_init();
	usi_init();

	for(int i = 0; i < 4; i++)
	{
		spi_transfer( 1 << i);
		delay(50);
	}
}

void loop()
{

	//the adc pins had to be tied to the GND with a 1MOhm resistor
	delay(10);
	//only the higher values are interesting for the game
	int energy1 = (int)((adc_read(2)) / 110);
	int energy2 = (int)((adc_read(3)) / 127);
	//better not work with negative numbers in the 2 lines above!!!
	
	//tweaking the Ljusa energy a bit
	energy1 = (int)(energy1-2);
	if (energy1 < 0) energy1 = 0;
	if (energy1 > 3) energy1 = 3;
	
	if ( energy2 < 4)  
		spi_transfer( (1<<energy1)|(1 << (4) ));
		else
		spi_transfer( (1<<energy1)|(1 << (energy2/2 + 3) ));
	
	
	/*
	
	if ( (energy1 < 0)  && (energy2 > -1 )) { 
		energy1 = 0;
		spi_transfer( (1 << energy1) | (1 << (energy2 + 4) ));
	}
	
	else if  ( (energy1 > -1)  && (energy2 < 1 ))
	{
		//energy2 = 0;
		spi_transfer( (1 << energy1) | (1 << 4 ));
		
	}
	else{
			spi_transfer( (1 << energy1) | (1 << (energy2 + 4) ));
			
	}

 */
	

	delay(500);

}

void usi_init()
{

	USICR &= ~(_BV(USISIE) | _BV(USIOIE) | _BV(USIWM1));
	USICR |= _BV(USIWM0) | _BV(USICS1) | _BV(USICLK);

	DDRB |= _BV(USCK_DD_PIN); //set the USCK pin as output
	DDRB |= _BV(DO_DD_PIN); //set the DO pin as output

}



void adc_init()
{
	/* internal pull-ups interfere with the ADC. disable the
	* pull-up on the pin if it's being used for ADC. either
	* writing 0 to the port register or setting it to output
	* should be enough to disable pull-ups. */
	PORTB &= ~_BV(PB3);
	DDRB &= ~_BV(PB3);
	PORTB &= ~_BV(PB4);
	DDRB &= ~_BV(PB4);
	/* unless otherwise configured, arduinos use the internal Vcc
	* reference. MUX 0x0f samples the ground (0.0V). */
	ADMUX = 0x0D;//B1101 = GND attiny45 page 139
	/*
	* Enable the ADC system, use 128 as the clock divider on a 16MHz
	* arduino (ADC needs a 50 - 200kHz clock) and start a sample. the
	* AVR needs to do some set-up the first time the ADC is used; this
	* first, discarded, sample primes the system for later use.
	*/
	ADCSRA |= _BV(ADEN) | _BV(ADPS1) | _BV(ADPS0) | _BV(ADSC);
	/* wait for the ADC to return a sample */
	loop_until_bit_is_clear(ADCSRA, ADSC);
}
unsigned short adc_read(unsigned char pin)
{
	unsigned char l, h, r;
	r = (ADMUX & 0xf0) | (pin & 0x0f);
	ADMUX = r; /* select the input channel */
	ADCSRA |= _BV(ADSC);
	loop_until_bit_is_clear(ADCSRA, ADSC);
	/* must read the low ADC byte before the high ADC byte */
	l = ADCL;
	h = ADCH;
	return ((unsigned short)h << 8) | l;
}
