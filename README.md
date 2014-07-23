Camduino
===========

Camduino is an interface to speak with Arduino over 
[I2C](https://en.wikipedia.org/wiki/I2c "I2C - Wikipedia"). You can get a red 
ball position and state of proximity sensors.
The main goal of this project is to supply an easy-to-use interface to 
communicate with Arduino as part of 
[ImpRo](http://anr-impro.irccyn.ec-nantes.fr/#description "ANR ImpRo") project 
demonstrator.

I2C bus is also called Two Wire Interface (TWI) as in the Arduino 
reference/librairy.

[Online doc](http://blasterbug.github.io/Camduino "Camduino doc online")

[Download librairy](https://github.com/blasterbug/Camduino "Camduino on GitHub")

## Arduino setup

Arduino board must be connected to CMUcam4 and five presence sensors on pins 3
to 7 and run [CMU_Tracker_I2C.ino](https://github.com/blasterbug/Camduino/blob/master/Arduino%20Program/CMU_Tracker_I2C.ino "Camduino program").
The Arduino program is supplied with the librairy in *Arduino program* folder.

You can compile and upload the program to the board using Arduino IDE. You must intall 
[CMUcam4 Arduino library](http://cmucam.org/docs/cmucam4/arduino_api/ "CMUcam4 Arduino Interface Library") first.
If you can not reprogram your Arduino when the CMUcam4 is connected to your 
Arduino you can either disconnect the CMUcam4 from your Arduino or you can put 
the CMUcam4 into halt mode : 
-	Press and hold the reset button on the CMUcam4
-	Press and hold the user button on the CMUcam4
-	Release the reset button (do not release the user button)
-	Wait until the red auxiliary LED turns on (2 seconds)
-	Release the user button
-	The CMUcam4 is now halted indefinitely

Press the reset button to exit halt mode.
 

#### Connect NXT and Arduino

The communication is based on I2C bus, so you have to connect them with a wire but 
such wires does not exist. Make your own RJ12 *NXT to Arduino* wire, it's easy.
Here there is some help to do so.

![NXT cutted off wire](http://blasterbug.github.io/Camduino/NXT_black_wire.png "NXT cutted off wire")

NXT uses RJ12 wires made like this : 
-	white : analog
-	black : ground
-	red : ground
-	green : 4.3V
-	yellow : I2C clock line
-	blue : I2C data line

Connect the red or the black wire to Arduino ground pin, the yellow to A5 pin 
and the blue one to A4.

**Be carefull**, NXT can not supply enought power to Arduino, CMUcam4 and 
five presence sensors, so use an external source for the board.

## Trampoline settings

The Trampoline revision running on the NXT must use the custom I2C driver "from
Armel". This driver is supplied with the Camduino library in *I2C driver* folder.

Modifying the driver at your own risk.


#### Arduino port and NXT

When calling `init_camduino(NXT_PORT_S#)` if you do **NOT** use **NXT_PORT_S4**,
you should change it on the driver file i2c.c in 
*Trampolie_folder*/machines/arm/nxt/drivers/lejos_nxj/src/nxtvm/platform/nxt/i2c.c

#### Usage

-	First, include `camduino.h` in your oil and C files. 
-	Initiate the driver library by calling `init_camduino(NXT_PORT_S4)`, 
eventualy, initiate other I2C devices and finally call `i2c_init()`.
-	Then you get data from `Camduino`.
-	Code and think hard. Good luck! 