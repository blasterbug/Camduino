Camduino
===========

Camduino is an interface to speak with Arduino over 
[I2C](https://en.wikipedia.org/wiki/I2c "I2C - Wikipedia"). You can get a red 
ball position and state of proximity sensors.
The main goal of this project is to supply an easy-to-use interface to 
communicate with Arduino as part of 
[ImpRo](http://anr-impro.irccyn.ec-nantes.fr/#description "ANR ImpRo") project 
demonstrator, *Pongduino*.

I2C bus is also called Two Wire Interface (TWI) as in the Arduino 
reference/librairy.


## Arduino setup

Arduino board must be connected to CMUcam4 and five presence sensors on pins 3
to 7 and run *CMU_Tracker_I2C.ino*.

#### The wire

To connect an Arduino with a NXT, I made my own wire.

![Custom NXT-to-Arduino wire](../../doc/CustomNXT-to-ArduinoWire.jpg 
	"Custom NXT-to-Arduino wire")

-	white : ground
-	black : ground
-	red : analog
-	green : 5V
-	blue : SDA (Arduino pin A4)
-	yellow : SCL (Arduino pin A5)

NXT can't supply enought energy for Arduino, so use an external source. 

## Trampoline settings

The Trampoline revision running on the NXT must use the custom I2C driver "from
Armel". This driver is supplied with the Camduino library.

Modifying the driver at your own risk.

#### Arduino port and NXT

When calling `init_camduino(NXT_PORT_S#)` if you're 
**NOT** using **NXT_PORT_S4**, you should change it on the driver file 
i2c.c in *Trampolie_folder*/machines/arm/nxt/drivers/lejos_nxj/src/nxtvm/platform/nxt/i2c.c

#### Usage

-	First, include `camduino.h` in your oil and C files. 
-	Initiate the driver library by calling `init_camduino(NXT_PORT_S4)`, 
eventualy, initiate other I2C devices and finally call `i2c_init()`.
-	Then you get data from `Camduino`.
-	Code and think hard. Good luck! 