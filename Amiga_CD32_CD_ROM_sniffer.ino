//CD32 Akiko-CDROM track display
//Arduino pin 13 to Amiga CD32 IF_CLK
//Arduino pin 11 to Amiga CD32 IF_DATA
//Arduino pin  3 to Amiga CD32 IF_DIR
//Arduino GND    to Amiga CD32 GND
//Arduino RST    to Amiga CD32 RST
//The display must be with I2C communication!
//https://www.tztstore.com/goods/show-6267.html
//TZT 2.26 Inch 4PIN White OLED Screen Module IPS 1602 Character OLED Screen KS0066 Drive IC IIC Interface 3.3V For Arduino
//operates in the 3.3V to 5.0V range

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
#include "pins_arduino.h"
#include "OLedI2C.h"

//#define DEBUGGER
#define IF_DIR 3
#define DISK_AUDIO 1
#define DISK_DATA  41

const char hexChars[]          = "0123456789ABCDEF";
uint16_t bufforUint16;

const char firmwareRevision[]  = "     5.0"; //Firmware revision
const char STRING_DISK_AUDIO[] = "AUDIO   "; //Disk type audio string
const char STRING_DISK_DATA[]  = "DATA    "; //Disk type data string
const char STRING_OPEN[]       = "OPEN    "; //Open string
const char STRING_CLOSE[]      = "CLOSE   "; //Close string
const char STRING_PLAY[]       = "PLAY    "; //Stop string
const char STRING_STOP[]       = "STOP    "; //Stop string
const char STRING_PAUSE[]      = "PAUSE   "; //Stop string
const char STRING_SET_PLAY[]   = "SET PLAY"; //Set Play string

uint8_t STRING_TRACK_NR        = 0x74;       //Track string   0x74 = 't'
uint8_t STRING_MINUTES         = 0x3A;       //Minutes string 0x27 = ''' , 0x3A = ':'
uint8_t STRING_SECONDS         = 0x2E;       //Seconds string 0x22 = '"' , 0x2E = '.'
uint8_t STRING_FRAMES          = 0x20;       //Frames string  0x66 = 'f' , 0x20 = ' '
uint8_t STRING_SPACE           = 0x20;       //Space string   0x20 = ' '

const char STRING_X1[]         = " x1 ";     //Speed x1 string
const char STRING_X2[]         = " x2 ";     //Speed x2 string

const char STRING_H0_CLEAR[]   = "        "; //Clear Header row 0
const char STRING_H1_CLEAR[]   = "    ";     //Clear Header row 1

volatile unsigned long start_time = 0;
volatile uint8_t position         = 0;
volatile uint8_t read_byte[255];

uint8_t mediaType   = 0;
uint8_t setPlayType = 0;
uint8_t playMode    = 0;
uint8_t playSpeed   = 0;
uint8_t indexNR     = 0;
uint8_t totalTrack  = 99;

const char* H0 = ""; //Header row 0
const char* H1 = ""; //Header row 1
char TN[3]     = ""; //Track number
char TM[3]     = ""; //Track minutes
char TS[3]     = ""; //Track seconds
char TF[3]     = ""; //Track frames
char DX[3]     = ""; //Disk  index ???
char DM[3]     = ""; //Disk  minutes
char DS[3]     = ""; //Disk  seconds
char DF[3]     = ""; //Disk  frames

OLedI2C lcd;

void setup (void)
{
  pinMode(IF_DIR, INPUT);
  // have to send on master in, *slave out*
  pinMode(MISO,   OUTPUT);
  pinMode(MOSI,   INPUT);
  pinMode(SCK,    INPUT);
  
  lcd.init();

#if defined(DEBUGGER)
  H0 = STRING_PLAY;
  H1 = STRING_X1;
  ToHex(33, TN);
  ToHex(50, TM);
  ToHex(67, TS);
  ToHex(84, TF);
  ToHex(101, DX);
  ToHex(118, DM);
  ToHex(135, DS);
  ToHex(152, DF);
  RedrawLCD();
#endif

  //SPI MODE 3
  bitSet(SPCR, CPOL);
  //SPI MODE 3
  bitSet(SPCR, CPHA);
  //SPI LSB first
  bitSet(SPCR, DORD);
  //SPI interrupts on
  bitSet(SPCR, SPIE);   
  //SPI ON (MSTR = 0 so slave mode)
  bitSet(SPCR, SPE);
  //attach interrupt INT0
  attachInterrupt(digitalPinToInterrupt(IF_DIR), ISR1, RISING);
}

//here SPI Buffer, I have something
ISR (SPI_STC_vect)
{
  read_byte[position] = SPDR;
  position++;
}

//here interrupt INT1, a change in the monitored signal occurred
void ISR1()
{
  start_time = millis();
// clear SPI buffer
  //bitClear(SPCR, SPE); //SPI off
  //bitSet(SPCR, SPE);   //SPI on  
}

//not needed, all you need is a change in the DORD SPI register
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
    //detachInterrupt(digitalPinToInterrupt(IF_DIR));
    //bitClear(SPCR, SPIE); //SPI interrupts off
    bitClear(SPCR, SPE); //SPI off
    start_time = 0;

    //Play packet
    if ((read_byte[0] & 0xF) == 0x6 && read_byte[1] == (uint8_t)~read_byte[0] && read_byte[2] == read_byte[0] && (read_byte[3] == 0x2 || read_byte[3] == 0x3) && TwoComplementChecksum8(read_byte, 16, 2) == read_byte[17])//((read_byte[0] & 0xF) == 0x6 && read_byte[1] == (uint8_t)~read_byte[0] && read_byte[2] == read_byte[0] && TwoComplementChecksum8(read_byte, 16, 2) == read_byte[17])
    {
      playMode = read_byte[3]; //0x2 = Play, 0x3 = Pause
      indexNR  = read_byte[7];

      if      (playMode  == 0x2)  H0 = STRING_PLAY;
      else if (playMode  == 0x3)  H0 = STRING_PAUSE;
      else                        H0 = STRING_H1_CLEAR;

      if      (playSpeed == 0x0)  H1 = STRING_X1;
      else if (playSpeed == 0x40) H1 = STRING_X2;
      else                        H1 = STRING_H1_CLEAR;

      ToHex(read_byte[6], TN);
      ToHex(read_byte[8], TM);
      ToHex(read_byte[9], TS);
      byte _TF_ = read_byte[10];
      bufforUint16 = ((((_TF_ >> 4) * 10) + (_TF_ & 0x0F)) * 4 + 1) / 3;
      sprintf(TF, "%02d", bufforUint16);
      ToHex(read_byte[12], DM);
      ToHex(read_byte[13], DS);
      byte _DF_ = read_byte[14];
      bufforUint16 = ((((_DF_ >> 4) * 10) + (_DF_ & 0x0F)) * 4 + 1) / 3;
      sprintf(DF, "%02d", bufforUint16);

      if (read_byte[6] <= totalTrack && read_byte[6] > 0)
        RedrawLCD();
    }
    //Read content
    else if (read_byte[0] == 0x6 && read_byte[1] == 0xA && read_byte[2] == 0x0 && read_byte[4] == 0x0 && read_byte[5] > 0x9F && TwoComplementChecksum8(read_byte, 14, 0) == read_byte[15]) //(read_byte[0] == 0x6 && read_byte[1] == 0xA && read_byte[2] == 0x0 && read_byte[4] == 0x0 && read_byte[5] > 0x9F && TwoComplementChecksum8(read_byte, 14, 0) == read_byte[15])
    {  
      mediaType = read_byte[3]; //0x1 = Audio, 0x41 = Data

      if      (mediaType == 0x1)  H0 = STRING_DISK_AUDIO;
      else if (mediaType == 0x41) H0 = STRING_DISK_DATA;
      else                        H0 = STRING_H0_CLEAR;

      H1 = STRING_H1_CLEAR;

      if      (read_byte[5] == 0xA0) //Disk index ???
      {
        indexNR = read_byte[9];
      }
      else if (read_byte[5] == 0xA1) //Total disk track
      {
        totalTrack = read_byte[10];
        if (totalTrack == 0) totalTrack = 0x99;
        ToHex(read_byte[10], TN);
      }
      else if (read_byte[5] == 0xA2) //Total disk time
      {
        strcpy(TM, "  ");
        strcpy(TS, "  ");
        strcpy(TF, "  ");

        ToHex(read_byte[10], DM);
        ToHex(read_byte[11], DS);
        ToHex(read_byte[12], DF);
      }

      RedrawLCD();
    }
    //CD drive version
    else if ((read_byte[0] & 0xF) == 0x5 && read_byte[1] == 0x0 && read_byte[2] == (uint8_t)~read_byte[0] && (read_byte[3] & 0xF) == 0x7 && read_byte[4] == (uint8_t)~read_byte[3])
    {
      lcd.setCursor(0,0);
      // Wysyła bajty od 7 do 14 w jednej paczce I2C (długość: 15 - 7 = 8)
      lcd.printBytes(&read_byte[7], 8);

      lcd.print(firmwareRevision);

      lcd.setCursor(0,1);
      // Wysyła bajty od 15 do 24 w jednej paczce I2C (długość: 25 - 15 = 10)
      lcd.printBytes(&read_byte[15], 10);
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
      lcd.print(STRING_CLOSE);
    }
    //Open
    else if (read_byte[0] == 0xA && read_byte[1] == 0x0 && read_byte[2] == 0xF5)
    {
      lcd.clearLcd();
      lcd.print(STRING_OPEN);
    }

    position = 0;
//    memset(read_byte, 0, sizeof read_byte);
    
    bitSet(EIFR, INTF0); // clear interrupt register INT0
    //attachInterrupt(digitalPinToInterrupt(IF_DIR), ISR1, RISING);
    bitSet(SPCR, SPE);  //SPI on
    //bitSet(SPCR, SPIE); //SPI interrupts off
  }
}

void RedrawLCD()
{
  lcd.setCursor(0,0);
  lcd.print(H0);
  lcd.printTime(DM, STRING_MINUTES, DS, STRING_SECONDS, DF, STRING_FRAMES);

  lcd.setCursor(0,1);
  lcd.print(H1);
  lcd.printTrackInfo(TN, TM, TS, TF, STRING_TRACK_NR, STRING_MINUTES, STRING_SECONDS, STRING_FRAMES, STRING_SPACE);
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

//input data, starting point
void readSetPlay (const byte *data, uint8_t startPoint)
{
  setPlayType = read_byte[startPoint+6];
  playSpeed   = read_byte[startPoint+7];

  H0 = STRING_SET_PLAY;

  if      (playSpeed == 0x0)  H1 = STRING_X1;
  else if (playSpeed == 0x40) H1 = STRING_X2;

  strcpy(TN, "  ");

  ToHex(read_byte[startPoint], TM);
  ToHex(read_byte[startPoint+1], TS);
  ToHex(read_byte[startPoint+2], TF);

  ToHex(read_byte[startPoint+3], DM);
  ToHex(read_byte[startPoint+4], DS);
  ToHex(read_byte[startPoint+5], DF);

  RedrawLCD();
}

void ToHex(uint8_t readByte, char* output) {
  output[0] = hexChars[(readByte >> 4) & 0x0F];
  output[1] = hexChars[readByte & 0x0F];
  output[2] = '\0';
}