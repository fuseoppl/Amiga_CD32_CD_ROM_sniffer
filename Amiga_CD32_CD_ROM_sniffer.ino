//Arduino pin 13 to Amiga CD32 IF_CLK
//Arduino pin 11 to Amiga CD32 IF_DATA
//Arduino pin  2 to Amiga CD32 IF_DIR
//Arduino GND    to Amiga CD32 GND

#include "pins_arduino.h"

#define IF_DIR 2
volatile unsigned long start_time;
volatile unsigned long actual_time;
volatile uint8_t read_byte[255];
volatile bool IF_DIR_level[255];
volatile bool IF_DIR_int;
volatile uint8_t pos;
volatile bool proced_it;



void setup (void)
{
  pinMode(IF_DIR, INPUT);

  Serial.begin (2000000);

  // have to send on master in, *slave out*
  pinMode(MISO, OUTPUT);
  
  // turn on SPI in slave mode
  SPCR |= _BV(SPE);

  // SPI to MODE 3
  SPCR |= _BV(CPOL);
  SPCR |= _BV(CPHA);

  // turn on SPI interrupts
  SPCR |= _BV(SPIE);

  // clear interrupt register INT0
  EIFR |= (1<<INTF0);
  // attach interrupt INT0
  attachInterrupt(digitalPinToInterrupt(IF_DIR), ISR0, RISING);
}

// here SPI Buffer, I have something
ISR (SPI_STC_vect)
{
  IF_DIR_level[pos] = digitalRead(IF_DIR);
  read_byte[pos] = SPDR;
  pos++;
}

// here interrupt INT0, a change in the monitored signal occurred
void ISR0()
{
  start_time = millis();
  IF_DIR_int = true;
}

void loop (void)
{
  // 5 ms have passed since the last change of the IF_DIR signal, maybe nothing will happen, we print the collected data
  if (IF_DIR_int && millis() > start_time + 5)
  {
    noInterrupts();
    // turn off SPI interrupts
    //SPCR &= ~(1 << SPIE);

    for (int i = 0; i < pos; i++)
    {
      Serial.print (IF_DIR_level[i]);
      Serial.print (";");
      Serial.println (read_byte[i], HEX);
    }

    pos = 0;
    IF_DIR_int = false;
    // turn on SPI interrupts
    //SPCR |= _BV(SPIE);
    interrupts();
  }
}