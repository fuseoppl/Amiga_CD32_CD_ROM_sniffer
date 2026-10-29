/*
	Custom version of the library
	OLedI2C.h
	OLED 1602 Library for the I2C
	OLED display from WIDE.HK
	Also my first attempt at a Library
	Phil Grant Sept 2013
	www.gadjet.co.uk
	Scrolling contributed by Nathan Chantrell http://nathan.chantrell.net/
	Updated 06/11/2013 to include the cursPos data within the sendString function.
*/
#ifndef OLedI2C_h
#define OLedI2C_h	
#include "Arduino.h"
#include "Wire.h"
/*
class OLedI2C {
public:
		OLedI2C();
		~OLedI2C();
		void init();
		void sendCommand(unsigned char command); // unsigned char
		void print(const char *String); // *String
		void sendString(const char *String, uint8_t col, uint8_t row); // *String, column, row
		void sendData(unsigned char data);
		void clearLcd();
		void setCursor(uint8_t, uint8_t); // column, row
		void scrollString(char* message, byte row, unsigned int time); //written by Nathan Chantrell http://nathan.chantrell.net/
		void lcdOff();
		void lcdOn();
		void setContrast(unsigned char contrast); // contrast should be the hex value between 0x00 and 0xFF
		void createChar(uint8_t, uint8_t[]);
};
#endif
*/
/*
	OLedI2C.h - Zoptymalizowana wersja dla CD32 Sniffer
*/

class OLedI2C {
public:
		OLedI2C();
		~OLedI2C();
		void init();
		void sendCommand(unsigned char command); 
		void print(const char *String);
		void printBytes(const uint8_t *data, uint8_t length);
		void printTime(const char* m, uint8_t sepM, const char* s, uint8_t sepS, const char* f, uint8_t sepF);
		void printTrackInfo(const char* tn, const char* tm, const char* ts, const char* tf, uint8_t trackNrChar, uint8_t minChar, uint8_t secChar, uint8_t frameChar, uint8_t spaceChar);
		void sendString(const char *String, uint8_t col, uint8_t row); 
		void sendData(unsigned char data);
		void clearLcd();
		void setCursor(uint8_t col, uint8_t row); 
		void scrollString(const char* message, byte row, unsigned int time); 
		void lcdOff();
		void lcdOn();
		void setContrast(unsigned char contrast); 
		void createChar(uint8_t location, uint8_t charmap[]);
};
#endif
