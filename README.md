# Amiga CD32 CD Drive sniffer and Track Display

* Arduino pin 13 to Amiga CD32 IF_CLK (TP7-pin 1)
* Arduino pin 11 to Amiga CD32 IF_DATA (TP7-pin 2)
* Arduino pin  2 to Amiga CD32 IF_DIR (TP7-pin 3)
* Arduino GND    to Amiga CD32 GND (TP7-pin 4)
* The display must be with I2C communication!

To work in display mode, leave this enabled:
#define LCDENABLE

![schema](https://github.com/fuseoppl/Amiga_CD32_CD_ROM_sniffer/blob/938fb55ecff812e342e18234801e03014686bb03/Audio_disk_info_display.png)
