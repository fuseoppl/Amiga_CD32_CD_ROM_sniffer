//CD32 Akiko-CDROM IF command sniffer v.3.1
//Arduino pin 13 to Amiga CD32 IF_CLK
//Arduino pin 11 to Amiga CD32 IF_DATA
//Arduino pin  2 to Amiga CD32 IF_DIR
//Arduino GND    to Amiga CD32 GND
//The display must be with I2C communication!

//Display on, sniffer off
#define LCDENABLE

#include "pins_arduino.h"

#if defined(LCDENABLE)
  #include <ST7032_asukiaaa.h>
  ST7032_asukiaaa lcd;
  int contrast = 10;
#endif

#define IF_DIR 2
volatile unsigned long start_time;
volatile uint8_t read_byte[255];
volatile bool IF_DIR_level[255];
volatile bool IF_DIR_int_occurred;
volatile uint8_t position;
volatile bool brandAndModelPrinted;

void setup (void)
{
  pinMode(IF_DIR, INPUT);

  Serial.begin (2000000);

  // have to send on master in, *slave out*
  pinMode(MISO, OUTPUT);
  
  // SPI on (MSTR = 0 so slave mode)
  bitSet(SPCR, SPE);

  // SPI MODE 3
  bitSet(SPCR, CPOL);
  bitSet(SPCR, CPHA);

  // SPI LSB first
  bitSet(SPCR, DORD);

#if defined(LCDENABLE)
  lcd.begin(16, 2); // columns and rows
  lcd.setContrast(contrast);
  lcd.clear();
#endif

  // SPI interrupts on
  bitSet(SPCR, SPIE);

  // clear interrupt register INT0
  //bitSet(EIFR, INTF0);
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
  bitClear(SPCR, SPE);
// SPI on   
  bitSet(SPCR, SPE);

  IF_DIR_int_occurred = true;
}

// not needed, all you need is a change in the DORD SPI register
/*
byte reverseBits(byte x) {
  x = (x & 0xF0) >> 4 | (x & 0x0F) << 4;
  x = (x & 0xCC) >> 2 | (x & 0x33) << 2;
  x = (x & 0xAA) >> 1 | (x & 0x55) << 1;
  return x;
}
*/

uint8_t TwoComplementChecksum8(const byte *data, size_t dataLength, size_t startData)
{
  uint8_t value = 0;
  for (size_t i = startData; i < dataLength; i++)
  {
    value += (uint8_t)data[i];
  }
  return ~value;
}

void loop (void)
{
  // 5 ms have passed since the last change of the IF_DIR signal, maybe nothing will happen,
  // so we print the collected data
  if (IF_DIR_int_occurred && millis() > start_time + 5)
  {
    //noInterrupts();

    for (int i = 0; i < position; i++)
    {
#if defined(LCDENABLE)
      if (i < 244 && read_byte[i] == 0x27 && read_byte[i+1] == 0xD8 && read_byte[i+2] == 0x27 && read_byte[i+3] == 0x1)
      {
        String brandName = "";
        String modelName = "";

        for (int ib = i+4; ib < i+12; ib++)
        {
          brandName += (char)read_byte[ib];
        }

        for (int im = i+12; im < i+22; im++)
        {
          modelName += (char)read_byte[im];
        }

        String DX = String(read_byte[i+6], HEX);
        if (DX.length() == 1) DX = " " + DX;

        lcd.setCursor(0,0);
        lcd.print(brandName);
        lcd.print("        ");
        lcd.setCursor(0,1);
        lcd.print(modelName);
        lcd.print("       ");

        brandAndModelPrinted = true;
      }

      if (read_byte[i] == 0x6 && read_byte[i+1] == 0xA && read_byte[i+2] == 0x0 && read_byte[i+3] == 0x1 && read_byte[i+4] == 0x0 && read_byte[i+5] == 0xA0 && TwoComplementChecksum8(read_byte, 14, 0) == read_byte[15])
      {
        if (brandAndModelPrinted)
        {
          lcd.setCursor(0, 0);
          lcd.print("DISK  i  m  s  f");
          lcd.setCursor(0, 1);
          lcd.print("TR.      m  s  f");

          brandAndModelPrinted = false;
        }

        String DX = String(read_byte[i+6], HEX);
        if (DX.length() == 1) DX = " " + DX;

        lcd.setCursor(4,0);
        lcd.print(DX);
      }

      if (read_byte[i] == 0x6 && read_byte[i+1] == 0xA && read_byte[i+2] == 0x0 && read_byte[i+3] == 0x1 && read_byte[i+4] == 0x0 && read_byte[i+5] == 0xA1 && TwoComplementChecksum8(read_byte, 14, 0) == read_byte[15])
      {
        String TN = String(read_byte[i+10], HEX);
        if (TN.length() == 1) TN = " " + TN;

        lcd.setCursor(4,1);
        lcd.print(TN);
      }

      if (read_byte[i] == 0x6 && read_byte[i+1] == 0xA && read_byte[i+2] == 0x0 && read_byte[i+3] == 0x1 && read_byte[i+4] == 0x0 && read_byte[i+5] == 0xA2 && TwoComplementChecksum8(read_byte, 14, 0) == read_byte[15])
      {
        String DM = String(read_byte[i+10], HEX);
        if (DM.length() == 1) DM = " " + DM;
        String DS = String(read_byte[i+11], HEX);
        if (DS.length() == 1) DS = " " + DS;
        String DF = String(read_byte[i+12], HEX);
        if (DF.length() == 1) DF = " " + DF;

        lcd.setCursor(7,0);
        lcd.print(DM);
        lcd.setCursor(10,0);
        lcd.print(DS);
        lcd.setCursor(13,0);
        lcd.print(DF);

        lcd.setCursor(7,1);
        lcd.print("  ");
        lcd.setCursor(10,1);
        lcd.print("  ");
        lcd.setCursor(13,1);
        lcd.print("  ");
      }
#else
      Serial.print (i);
      Serial.print (";");
      Serial.print (IF_DIR_level[i]);
      Serial.print (";");
      Serial.println (read_byte[i], HEX);
#endif
    }

#if defined(LCDENABLE)
    if ((read_byte[0] & 0xF) == 0x6 && read_byte[1] == (uint8_t)~read_byte[0] && read_byte[2] == read_byte[0] && read_byte[3] == 0x2 && read_byte[4] == 0x0 && read_byte[11] == 0x0 && read_byte[15] == 0x0 && read_byte[16] == 0x0 && TwoComplementChecksum8(read_byte, 16, 2) == read_byte[17])
    {
      String TN = String(read_byte[6], HEX);
      if (TN.length() == 1) TN = " " + TN;
      String TM = String(read_byte[8], HEX);
      if (TM.length() == 1) TM = " " + TM;
      String TS = String(read_byte[9], HEX);
      if (TS.length() == 1) TS = " " + TS;
      String TF = String(read_byte[10], HEX);
      if (TF.length() == 1) TF = " " + TF;
      String DX = String(read_byte[7], HEX);
      if (DX.length() == 1) DX = " " + DX;
      String DM = String(read_byte[12], HEX);
      if (DM.length() == 1) DM = " " + DM;
      String DS = String(read_byte[13], HEX);
      if (DS.length() == 1) DS = " " + DS;
      String DF = String(read_byte[14], HEX);
      if (DF.length() == 1) DF = " " + DF;

      lcd.setCursor(4,0);
      lcd.print(DX);
      lcd.setCursor(7,0);
      lcd.print(DM);
      lcd.setCursor(10,0);
      lcd.print(DS);
      lcd.setCursor(13,0);
      lcd.print(DF);

      lcd.setCursor(4,1);
      lcd.print(TN);
      lcd.setCursor(7,1);
      lcd.print(TM);
      lcd.setCursor(10,1);
      lcd.print(TS);
      lcd.setCursor(13,1);
      lcd.print(TF);
    }
#endif

    position = 0;
    IF_DIR_int_occurred = false;
    memset(read_byte, 0, sizeof read_byte);
    //interrupts();
  }
}