# Amiga CD32 CD Drive sniffer and Track Display

* CD32 Akiko-CDROM IF command sniffer and track display
* Arduino pin 13 to Amiga CD32 IF_CLK
* Arduino pin 11 to Amiga CD32 IF_DATA
* Arduino pin  3 to Amiga CD32 IF_DIR
* Arduino GND    to Amiga CD32 GND
* Arduino pin  4 to Amiga CD32 RST via schottky diode, anode to CD32 RST
* The display must be with I2C communication!
* https://www.tztstore.com/goods/show-6267.html
* TZT 2.26 Inch 4PIN White OLED Screen Module IPS 1602 Character OLED Screen KS0066 Drive IC IIC Interface 5V For Arduino
* The display must be with I2C communication!

To work in display mode, leave this enabled:
#define LCDENABLE

![schema](https://github.com/fuseoppl/Amiga_CD32_CD_ROM_sniffer/blob/master/Audio_disk_info_display_002.png)
