//CD32 Akiko-CDROM IF command sniffer v.1.1 (SPI buffer clearing not tested for proper operation)
//Arduino pin 13 to Amiga CD32 IF_CLK
//Arduino pin 11 to Amiga CD32 IF_DATA
//Arduino pin  2 to Amiga CD32 IF_DIR
//Arduino GND    to Amiga CD32 GND

#include "pins_arduino.h"

#define IF_DIR 2
volatile unsigned long start_time;
volatile uint8_t read_byte[255];
volatile bool IF_DIR_level[255];
volatile bool IF_DIR_int_occurred;
volatile uint8_t position;

void setup (void)
{
  pinMode(IF_DIR, INPUT);

  Serial.begin (2000000);

  // have to send on master in, *slave out*
  pinMode(MISO, OUTPUT);
  
  // SPI slave mode on (MSTR = 0)
  SPCR |= _BV(SPE);

  // SPI MODE 3
  SPCR |= _BV(CPOL);
  SPCR |= _BV(CPHA);

  // SPI LSB first
  SPCR |= _BV(DORD);

  // SPI interrupts on
  SPCR |= _BV(SPIE);

  // clear interrupt register INT0
  //EIFR |= (1 << INTF0);
  // attach interrupt INT0
  attachInterrupt(digitalPinToInterrupt(IF_DIR), ISR0, RISING);
}

// here SPI Buffer, I have something
ISR (SPI_STC_vect)
{
  IF_DIR_level[position] = digitalRead(IF_DIR);
  read_byte[position] = SPDR;
  position++;
}

// here interrupt INT0, a change in the monitored signal occurred
void ISR0()
{
  start_time = millis();

// clear SPI buffer
// SPI off
  SPCR &= ~(1 << SPE);
// SPI on   
  SPCR |= _BV(SPE);
  
  IF_DIR_int_occurred = true;
}

// not needed, change in DORD SPI register
/*
byte reverseBits(byte x) {
  x = (x & 0xF0) >> 4 | (x & 0x0F) << 4;
  x = (x & 0xCC) >> 2 | (x & 0x33) << 2;
  x = (x & 0xAA) >> 1 | (x & 0x55) << 1;
  return x;
}
*/

void loop (void)
{
  // 5 ms have passed since the last change of the IF_DIR signal, maybe nothing will happen,
  // so we print the collected data
  if (IF_DIR_int_occurred && millis() > start_time + 5)
  {
    noInterrupts();
    // turn off SPI interrupts
    //SPCR &= ~(1 << SPIE);

    for (int i = 0; i < position; i++)
    {
      Serial.print (IF_DIR_level[i]);
      Serial.print (";");
      Serial.println (read_byte[i], HEX);//(reverseBits(read_byte[i]), HEX);
    }

    position = 0;
    IF_DIR_int_occurred = false;

    // turn on SPI interrupts
    //SPCR |= _BV(SPIE);
    interrupts();
  }
}