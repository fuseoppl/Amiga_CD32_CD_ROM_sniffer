/*
Custom version of the library:
This is my first Library to make writing to the OLED 1602
Display much easier, the display is based on the SSD1311.
The display has 16 characters and 2 lines.
The library is far from complete and may be prone to errors.
Feedback welcome, visit www.gadjet.co.uk
Phil Grant 2013
Scrolling contributed by Nathan Chantrell http://nathan.chantrell.net/

Updated 06/11/2013 to include the cursPos data in the sendString function
sendString("string", col, row)
*/
#include "OLedI2C.h" 
#include "Wire.h"
#define OLED_Address 0x3c
#define OLED_Command_Mode 0x80
#define OLED_Data_Mode 0x40

OLedI2C::OLedI2C(){}
OLedI2C::~OLedI2C(){}

void OLedI2C::init()
{
  Wire.begin();
  Wire.setClock(400000);
  // *** I2C initial *** //
  sendCommand(0x2A);	  // **** Set "RE"=1	00101010B
  sendCommand(0x71);
  sendCommand(0x5C);
  sendCommand(0x28);

  sendCommand(0x08);	  // **** Set Sleep Mode On
  sendCommand(0x2A);	  // **** Set "RE"=1	00101010B
  sendCommand(0x79);	  // **** Set "SD"=1	01111001B

  sendCommand(0xD5);
  sendCommand(0x70);
  sendCommand(0x78);	  // **** Set "SD"=0  01111000B

  sendCommand(0x08);	  // **** Set 5-dot, 3 or 4 line(0x09), 1 or 2 line(0x08)

  sendCommand(0x06);	  // **** Set Com31-->Com0  Seg0-->Seg99

  // **** Set OLED Characterization *** //
  sendCommand(0x2A);  	// **** Set "RE"=1 
  sendCommand(0x79);  	// **** Set "SD"=1

  // **** CGROM/CGRAM Management *** //
  sendCommand(0x72);  	// **** Set ROM
  sendCommand(0x00);  	// **** Set ROM A and 8 CGRAM

  sendCommand(0x2A);    // **** Additional RE bit security for command reliability

  sendCommand(0xDA); 	  // **** Set Seg Pins HW Config
  sendCommand(0x10);   

  sendCommand(0x81);  	// **** Set Contrast command
  sendCommand(0xFF);

  sendCommand(0xDB);  	// **** Set VCOM deselect level
  sendCommand(0x30);  	// **** VCC x 0.83

  sendCommand(0xDC);  	// **** Set gpio - turn EN for 15V generator on.
  sendCommand(0x03);

  sendCommand(0x78);  	// **** Exiting Set OLED Characterization
  sendCommand(0x28); 
  sendCommand(0x2A); 
  //sendCommand(0x05); 	// **** Set Entry Mode
  sendCommand(0x06); 	  // **** Set Entry Mode
  sendCommand(0x08);  
  sendCommand(0x28); 	  // **** Set "IS"=0 , "RE" =0 //28
  sendCommand(0x01); 
  sendCommand(0x80); 	  // **** Set DDRAM Address to 0x80 (line 1 start)

  delay(2);
  sendCommand(0x0C);  	// **** Turn on Display
}
 
void OLedI2C::setCursor(uint8_t col, uint8_t row) //column, row
{
  int row_offsets[] = { 0x00, 0x40 };
  sendCommand(0x80 | (col + row_offsets[row]));
}
 
void OLedI2C::clearLcd()
{
  sendCommand(0x01);
  
  delay(2); // The DDRAM-clearing command requires a brief pause on the controller.
}

void OLedI2C::lcdOff()
{
  sendCommand(0x08);  	// **** Turn on Off
}

void OLedI2C::lcdOn()
{
  sendCommand(0x0C);  	// **** Turn on On
}

void OLedI2C::sendCommand(unsigned char command)
{
  Wire.beginTransmission(OLED_Address);  // **** Start I2C 
  Wire.write(OLED_Command_Mode);     		 // **** Set OLED Command mode
  Wire.write(command);
  Wire.endTransmission();                // **** End I2C 
  //delay(1);
}

 void OLedI2C::setContrast(unsigned char contrast) // contrast as 0x00 to 0xFF
{
  //Set OLED Command set
	sendCommand(0x2A); 
	sendCommand(0x79); 
	
	sendCommand(0x81);      // Set Contrast
	sendCommand(contrast);  // send contrast value
	sendCommand(0x78);      // Exiting Set OLED Command set
  sendCommand(0x28);
}

void OLedI2C::sendString(const char *String, uint8_t col, uint8_t row)
{
  setCursor(col, row);
  
  Wire.beginTransmission(OLED_Address);
  Wire.write(OLED_Data_Mode); 
  
  unsigned char i = 0;
  while(String[i])
  {
    Wire.write(String[i]); 
    i++;
  }
  Wire.endTransmission();
}

void OLedI2C::print(const char *String)
{
  Wire.beginTransmission(OLED_Address);
  Wire.write(OLED_Data_Mode);
  
  unsigned char i = 0;
  while(String[i])
  {
    Wire.write(String[i]);  
    i++;
  }
  Wire.endTransmission();
}

void OLedI2C::sendData(unsigned char data)
{
  Wire.beginTransmission(OLED_Address);   // **** Start I2C 
  Wire.write(OLED_Data_Mode);             // **** Set OLED Data mode
  Wire.write(data);
  Wire.endTransmission();                 // **** End I2C 
}

void OLedI2C::printBytes(const uint8_t *data, uint8_t length)
{
  Wire.beginTransmission(OLED_Address);
  Wire.write(OLED_Data_Mode);
  for (uint8_t i = 0; i < length; i++)
  {
    Wire.write(data[i]);
  }
  Wire.endTransmission();
}

void OLedI2C::printTime(const char* m, uint8_t sepM, const char* s, uint8_t sepS, const char* f, uint8_t sepF)
{
  Wire.beginTransmission(OLED_Address);
  Wire.write(OLED_Data_Mode);

  uint8_t i = 0;
  while(m[i]) { Wire.write(m[i++]); }
  Wire.write(sepM); // minute separator (e.g., ':')

  i = 0;
  while(s[i]) { Wire.write(s[i++]); }
  Wire.write(sepS); // seconds separator (e.g., '.')

  i = 0;
  while(f[i]) { Wire.write(f[i++]); }
  Wire.write(sepF); // frame separator (e.g., ' ')

  Wire.endTransmission();
}

void OLedI2C::printTrackInfo(const char* tn, const char* tm, const char* ts, const char* tf, 
                             uint8_t trackNrChar, uint8_t minChar, uint8_t secChar, uint8_t frameChar, uint8_t spaceChar)
{
  Wire.beginTransmission(OLED_Address);
  Wire.write(OLED_Data_Mode);
  
  uint8_t i;

  // Track number (TN) + separator
  i = 0; while(tn[i]) { Wire.write(tn[i++]); }
  if (tn[0] == ' ' && tn[1] == ' ') { Wire.write(spaceChar); }
  else { Wire.write(trackNrChar); }
  Wire.write(spaceChar); // Fixed space after the track number

  // Minutes (TM) + separator
  i = 0; while(tm[i]) { Wire.write(tm[i++]); }
  if (tm[0] == ' ' && tm[1] == ' ') { Wire.write(spaceChar); }
  else { Wire.write(minChar); }

  // Seconds (TS) + separator
  i = 0; while(ts[i]) { Wire.write(ts[i++]); }
  if (ts[0] == ' ' && ts[1] == ' ') { Wire.write(spaceChar); }
  else { Wire.write(secChar); }

  // Frames (TF) + separator
  i = 0; while(tf[i]) { Wire.write(tf[i++]); }
  if (tf[0] == ' ' && tf[1] == ' ') { Wire.write(spaceChar); }
  else { Wire.write(frameChar); }

  Wire.endTransmission();
}

void OLedI2C::scrollString(const char* message, byte row, unsigned int time)
{ 
  char buffer[17];
  size_t msgLen = strlen(message);

  for (byte i=0; i<msgLen+16; i++)
  {
    byte pos = i+1;
    for (byte j=0; j<16; j++)
    {
      if ((pos<16)||(pos>msgLen+15))
      { 
        buffer[j]=' ';
      }
      else buffer[j]=message[pos-16];
      pos++;
    }
    buffer[16] = '\0';
    sendString(buffer, 0, row);
    delay(time);
  }
}

// Allows us to fill the first 8 CGRAM locations
// with custom characters
void OLedI2C::createChar(uint8_t location, uint8_t charmap[]) {
  location &= 0x7; // we only have 8 locations 0-7

  Wire.beginTransmission(OLED_Address);
  Wire.write(OLED_Data_Mode | (location << 3));
  for (int i=0; i<8; i++)
  {
    Wire.write(charmap[i]);
  }
  Wire.endTransmission();
}


