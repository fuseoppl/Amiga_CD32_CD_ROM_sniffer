//CD32 Akiko-CDROM IF command sniffer and track display
//Arduino pin 13 to Amiga CD32 IF_CLK
//Arduino pin 11 to Amiga CD32 IF_DATA
//Arduino pin  3 to Amiga CD32 IF_DIR
//Arduino GND    to Amiga CD32 GND
//Arduino RST    to Amiga CD32 RST
//The display must be with I2C communication!
//https://www.tztstore.com/goods/show-6267.html
//TZT 2.26 Inch 4PIN White OLED Screen Module IPS 1602 Character OLED Screen KS0066 Drive IC IIC Interface 3.3V For Arduino

//I was wondering why the CD drive has three microcontroller pins connected to handle the IF_DATA signal, and now it's all clear to me.
//The CD drive operates in MASTER mode.
//When the CD drive wants to send data to Akiko, it sets the IF_DIR signal to a logical 0.
//After about 15 µs, it transmits the data. This means it starts the clock on the IF_CLK signal, and on the rising edge of the clock signal, the IF_DATA signal is set accordingly.
//Nothing special, IF_DIR acts a bit like a chip select signal.
//The problem arises when Akiko needs to send data; it can't control the clock signal or the IF_DIR signal, as these are inputs to the Akiko.
//In this situation, it sets the IF_DATA pin to a logical 0. However, this doesn't provide any information for the SPI interface for the master device, such as the CD drive.
//So the third pin in the CD drive's microcontroller is simply an interrupt input, activated when the CD drive microcontroller does not send data to Akiko.
//After detecting a signal change on this pin, the CD drive's microcontroller enables the SPI interface and begins transmitting the clock signal.
//It all makes sense now, although in my opinion it was a poor solution, but that doesn't matter now, what matters is that in the microcontroller that will support the new CD drive,
//the MISO, MOSI and INT pins need to be connected together. 

//#define DEBUGGER

//Firmware revision number
String firmwareRevision  = "     4.0"; //Firmware revision
String STRING_DISK_AUDIO = "AUDIO  "; //Disk type audio string
String STRING_DISK_DATA  = "DATA   "; //Disk type data string
String STRING_OPEN       = "OPEN   "; //Open string
String STRING_CLOSE      = "CLOSE  "; //Close string
String STRING_PLAY       = "PLAY   "; //Stop string
String STRING_STOP       = "STOP   "; //Stop string
String STRING_PAUSE      = "PAUSE  "; //Stop string

String STRING_SET_PLAY   = "SET P. "; //Set Play string
uint8_t STRING_TRACK_NR  = 0x74;      //Track string
uint8_t STRING_MINUTES   = 0x27;      //Minutes string
uint8_t STRING_SECONDS   = 0x22;      //Seconds string
uint8_t STRING_FRAMES    = 0x66;      //Frames string

String STRING_X1         = " x1 ";      //Speed x1 string
String STRING_X2         = " x2 ";      //Speed x2 string

String STRING_H1_CLEAR   = "       ";      //Speed x1 string
String STRING_H2_CLEAR   = "    ";      //Speed x2 string

#define DISK_AUDIO 1
#define DISK_DATA  41

#include "pins_arduino.h"

#include "OLedI2C.h"
OLedI2C lcd;

#define IF_DIR 3
volatile unsigned long start_time = 0;
volatile uint8_t read_byte[255];
volatile uint8_t position;

uint8_t mediaType   = 0;
uint8_t setPlayType = 0;
uint8_t playMode    = 0;
uint8_t playSpeed   = 0;
uint8_t indexNR     = 0;

String H0 = ""; //Header row 0
String H1 = ""; //Header row 1
String TN = ""; //Track number
String TM = ""; //Track minutes
String TS = ""; //Track seconds
String TF = ""; //Trank frame
String DX = ""; //Disk  index ???
String DM = ""; //Disk  minutes
String DS = ""; //Disk  seconds
String DF = ""; //Disk  frames

void setup (void)
{
  pinMode(IF_DIR, INPUT);

  // have to send on master in, *slave out*
  pinMode(MISO,   OUTPUT);
  pinMode(MOSI,   INPUT);
  pinMode(SCK,    INPUT);
  
  // SPI ON (MSTR = 0 so slave mode)
  bitSet(SPCR, SPE);

  // SPI MODE 3
  bitSet(SPCR, CPOL);
  bitSet(SPCR, CPHA);

  // SPI LSB first
  bitSet(SPCR, DORD);

  lcd.init();

  // SPI interrupts on
  bitSet(SPCR, SPIE);

  // clear interrupt register INT0
  //bitSet(EIFR, INTF0);
  // attach interrupt INT0
  attachInterrupt(digitalPinToInterrupt(IF_DIR), ISR1, RISING);
}

// here SPI Buffer, I have something
ISR (SPI_STC_vect)
{
  read_byte[position] = SPDR;
  position++;
}

// here interrupt INT1, a change in the monitored signal occurred
void ISR1()
{
  start_time = millis();
// clear SPI buffer
  bitClear(SPCR, SPE); //SPI off
  bitSet(SPCR, SPE);   //SPI on  
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

void loop (void)
{
  // 1 ms have passed since the last change of the IF_DIR signal, maybe nothing will happen,
  // so we print the collected data
  if (start_time > 0 && millis() > start_time + 1)//(IF_DIR_int_occurred && millis() > start_time + 2)
  {
    bitSet(EIFR, INTF0);
    //detachInterrupt(digitalPinToInterrupt(IF_DIR));

    bitClear(SPCR, SPIE);

    start_time = 0;

    //Play packet
    if (TwoComplementChecksum8(read_byte, 16, 2) == read_byte[17])//((read_byte[0] & 0xF) == 0x6 && read_byte[1] == (uint8_t)~read_byte[0] && read_byte[2] == read_byte[0] && TwoComplementChecksum8(read_byte, 16, 2) == read_byte[17])
    {
      playMode = read_byte[3]; //0x2 = Play, 0x3 = Pause
      indexNR  = read_byte[7];

      if      (playMode  == 0x2)  H0 = STRING_PLAY;
      else if (playMode  == 0x3)  H0 = STRING_PAUSE;

      if      (playSpeed == 0x0)  H1 = STRING_X1;
      else if (playSpeed == 0x40) H1 = STRING_X2;

      TN = String(read_byte[6], HEX);
      if (TN.length() == 1) TN = " " + TN;
      TM = String(read_byte[8], HEX);
      if (TM.length() == 1) TM = " " + TM;
      TS = String(read_byte[9], HEX);
      if (TS.length() == 1) TS = " " + TS;
      TF = String(read_byte[10], HEX);
      if (TF.length() == 1) TF = " " + TF;
      DM = String(read_byte[12], HEX);
      if (DM.length() == 1) DM = " " + DM;
      DS = String(read_byte[13], HEX);
      if (DS.length() == 1) DS = " " + DS;
      DF = String(read_byte[14], HEX);
      if (DF.length() == 1) DF = " " + DF;

      RedrawLCD(H0, H1);
    }
    //Read content
    else if (read_byte[5] > 0x9F && TwoComplementChecksum8(read_byte, 14, 0) == read_byte[15]) //(read_byte[0] == 0x6 && read_byte[1] == 0xA && read_byte[2] == 0x0 && read_byte[4] == 0x0 && read_byte[5] > 0x9F && TwoComplementChecksum8(read_byte, 14, 0) == read_byte[15])
    {  
      mediaType = read_byte[3]; //0x1 = Audio, 0x41 = Data

      if      (mediaType == 0x1)  H0 = STRING_DISK_AUDIO;
      else if (mediaType == 0x41) H0 = STRING_DISK_DATA;

      H1 = STRING_H2_CLEAR;

      if      (read_byte[5] == 0xA0) //Disk index ???
      {
        indexNR = read_byte[9];
      }
      else if (read_byte[5] == 0xA1) //Total disk track
      {
        TN = String(read_byte[10], HEX);
        if (TN.length() == 1) TN = " " + TN;
      }
      else if (read_byte[5] == 0xA2) //Total disk time
      {
        TM = "  ";
        TS = "  ";
        TF = "  ";

        DM = String(read_byte[10], HEX);
        if (DM.length() == 1) DM = " " + DM;
        DS = String(read_byte[11], HEX);
        if (DS.length() == 1) DS = " " + DS;
        DF = String(read_byte[12], HEX);
        if (DF.length() == 1) DF = " " + DF;
      }

      RedrawLCD(H0, H1);
    }
    //Command asking for the CD drive version
    else if ((read_byte[0] & 0xF) == 0x5 && read_byte[1] == 0x0 && read_byte[2] == (uint8_t)~read_byte[0] && (read_byte[3] & 0xF) == 0x7 && read_byte[4] == (uint8_t)~read_byte[3])
    {
      lcd.setCursor(0,0);
      for (int i = 7; i < 15; i++)
      {
        lcd.sendData(read_byte[i]);
      }

      lcd.print(firmwareRevision.c_str());

      lcd.setCursor(1,1);
      for (int i = 15; i < 25; i++)
      {
        lcd.sendData(read_byte[i]);
      }
    }
    //Set Play Audio
    else if ((read_byte[0] & 0xF) == 0x5 && read_byte[1] == 0x0 && read_byte[2] == (uint8_t)~read_byte[0] && (read_byte[3] & 0xF) == 0x5 && read_byte[4] == 0x1 && read_byte[5] == (uint8_t)~(read_byte[2]+read_byte[4]) && (read_byte[6] & 0xF) == 0x4)
    {
      readSetPlay(read_byte, 7);
    }
    //Set Play Data
    else if ((read_byte[0] & 0xF) == 0x5 && read_byte[1] == 0x0 && read_byte[2] == (uint8_t)~read_byte[0] && (read_byte[8] & 0xF) == 0x5 && read_byte[9] == 0x1 && read_byte[10] == (uint8_t)~(read_byte[8]+read_byte[9]) && (read_byte[11] & 0xF) == 0x4)
    {
      readSetPlay(read_byte, 12);
    }
    //Close
    else if (read_byte[0] == 0xA && read_byte[1] == 0x1 && read_byte[2] == 0xF4 && (read_byte[3] & 0xF) == 0x5 && read_byte[4] == 0x1 && read_byte[5] == (uint8_t)~(read_byte[3] + read_byte[4]))//(read_byte[0] == 0xA && read_byte[1] == 0x1 && read_byte[2] == 0xF4 && (read_byte[3] & 0xF) == 0x5 && read_byte[4] == 0x1 && read_byte[5] == (uint8_t)~(read_byte[3] + read_byte[4]) && TwoComplementChecksum8(read_byte, 17, 6) == read_byte[18])
    {
      lcd.clearLcd();
      lcd.print(STRING_CLOSE.c_str());
    }
    //Open
    else if (read_byte[0] == 0xA && read_byte[1] == 0x0 && read_byte[2] == 0xF5)
    {
      lcd.clearLcd();
      lcd.print(STRING_OPEN.c_str());
    }
#if defined(DEBUGGER)
    else 
    {
      lcd.clearLcd();
      lcd.print(String(read_byte[0], HEX).c_str());
      lcd.print(" ");
      lcd.print(String(read_byte[1], HEX).c_str());
      lcd.print(" ");
      lcd.print(String(read_byte[2], HEX).c_str());
      lcd.print(" ");
      lcd.print(String(read_byte[3], HEX).c_str());
      lcd.print(" ");
      lcd.print(String(read_byte[4], HEX).c_str());
    }
#endif
    position = 0;
    memset(read_byte, 0, sizeof read_byte);
    
    //attachInterrupt(digitalPinToInterrupt(IF_DIR), ISR1, RISING);
    bitSet(SPCR, SPIE);
  }
}

//readraw LCD
void RedrawLCD(String header0, String header1)
{
  lcd.setCursor(0,0);
  lcd.print(H0.c_str());
  lcd.print(DM.c_str());
  lcd.sendData(STRING_MINUTES);
  lcd.print(DS.c_str());
  lcd.sendData(STRING_SECONDS);
  lcd.print(DF.c_str());
  lcd.sendData(STRING_FRAMES);

  lcd.setCursor(0,1);
  lcd.print(H1.c_str());
  lcd.print(TN.c_str());
  if (TN == "  ") lcd.sendData(0x20); //space
  else lcd.sendData(STRING_TRACK_NR);
  lcd.print(TM.c_str());
  lcd.sendData(STRING_MINUTES);
  lcd.print(TS.c_str());
  lcd.sendData(STRING_SECONDS);
  lcd.print(TF.c_str());
  lcd.sendData(STRING_FRAMES);
}

//input data, end point, starting point
uint8_t TwoComplementChecksum8(const byte *data, uint8_t endPoint, uint8_t startPoint)
{
  uint8_t value = 0;
  for (uint8_t i = startPoint; i < endPoint; i++)
  {
    value += (uint8_t)data[i];
  }
  return ~value;
}

//read Set Play Data
void readSetPlay (const byte *data, uint8_t startPoint)
{
  setPlayType = read_byte[startPoint+6];
  playSpeed   = read_byte[startPoint+7];

  H0 = STRING_SET_PLAY;

  if      (playSpeed == 0x0)  H1 = STRING_X1;
  else if (playSpeed == 0x40) H1 = STRING_X2;

  TN = String(" ");
  if (TN.length() == 1) TN = " " + TN;

  TM = String(read_byte[startPoint], HEX);
  if (TM.length() == 1) TM = " " + TM;
  TS = String(read_byte[startPoint+1], HEX);
  if (TS.length() == 1) TS = " " + TS;
  TF = String(read_byte[startPoint+2], HEX);
  if (TF.length() == 1) TF = " " + TF;

  DM = String(read_byte[startPoint+3], HEX);
  if (DM.length() == 1) DM = " " + DM;
  DS = String(read_byte[startPoint+4], HEX);
  if (DS.length() == 1) DS = " " + DS;
  DF = String(read_byte[startPoint+5], HEX);
  if (DF.length() == 1) DF = " " + DF;

  RedrawLCD(H0, H1);
}
