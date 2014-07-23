/**
 * Track a red ball with CMUcam and send his postion over I2C when requested
 * the program is also able to save frames (what cam see) on a micro SD card on 
 * board, just define "CAPTIRE"
 *
 * @section copyright Copyright
 * (c) IRCCyN 2014
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * @section infos File informations
 * @date 2014/07/22
 * @version 0.3
 * @author Benjamin Sientzoff
*/

#include <CMUcam4.h>
#include <Wire.h>

#undef CAPTURE
///#define CAPTURE

/* macros */
/*  blink led at 2Hz when set it up */
#define CAM_LED_SETUP_BLINK 3
/* blink led at 1Hz when errors occur */
#define CAM_LED_ERROR_BLINK 1
/* blink led at 5Hz when red is detected */
#define CAM_LED_DETECTED_BLINK 5
/* among of time to init properly camera */
#define CAM_SETTING_TIME 5000 /* ms */
/* pixel tolerence (min pixels to track) */
#define NOISE_THRESHOLD 1
/* color filter setting to detect red */
#define RED_MIN 120
#define RED_MAX 255
#define GREEN_MIN 0
#define GREEN_MAX 90
#define BLUE_MIN 0
#define BLUE_MAX 90

/* Arduino I2C adress */
#define I2C_ADDRESS 1

/* create CMUcam object */
CMUcam4 cam(CMUCOM4_SERIAL);
/* data send by camera in stream mode */
CMUcam4_tracking_data_t data_t;

/* store previous ball coord */
uint8_t ballposition[5] = {0, 0, 0, 0, 0};

void setup(){
  /* set up sensor pin mode as input pins */
  for(int i=3; i<8; ++i)
  {
    pinMode(i, INPUT);
  }
  /* activate CMUcam4 interface */
  cam.begin();
  cam.LEDOn(CAM_LED_SETUP_BLINK);
  cam.monitorOff();
  #ifdef CAPTURE
  /* sweep off files on microSD */
  cam.formatDisk();
  #endif
  /* wait for auto gain and auto white balance to run */
  delay(CAM_SETTING_TIME);
  /* turn off auto gain and auto white balance */
  cam.autoGainControl(false);
  cam.autoWhiteBalance(false);	

  /* turn off aux LED off */
  cam.LEDOn(CMUCAM4_LED_OFF);
  /* set tracking color */
  cam.setTrackingParameters(RED_MIN, RED_MAX, GREEN_MIN, GREEN_MAX, 
    BLUE_MIN, BLUE_MAX);
  //cam.noiseFilter(NOISE_THRESHOLD);
  
  /* function called when master request data */
  Wire.onRequest(requestI2C);
  /* function call when receiving data over I2C */
  Wire.onReceive(receiveI2C);
  /* join I2C bus */
  Wire.begin(I2C_ADDRESS);
  if( 0 != cam.trackColor()) cam.LEDOn(CAM_LED_ERROR_BLINK);
}

void loop(){
  /* detect red ball */
  
  /* if cam in stream mode get T type packet */
  if(0==cam.getTypeTDataPacket(&data_t))
  {
    ///cam.idleCamera();
    ///if(ballposition[0] != data_t.mx || ballposition[1] != data_t.my)
    {
      /* disable iteruptions to insure data consistency */
      noInterrupts();
      /* get the center of the detected object */
      ballposition[0] = data_t.mx;
      ballposition[1] = data_t.mx >> 8;
      ballposition[2] = data_t.my;
      ballposition[3] = data_t.my >> 8;
      /* re-enable interruptions */
      interrupts();
     #ifdef CAPTURE
      cam.idleCamera();
      /* save frame and detected point pictures on SD */
      cam.dumpBitmap();
      //cam.dumpFrame(6,6); /* take a long time ! */
      cam.LEDOn(CAM_LED_DETECTED_BLINK);
      if( 0 != cam.trackColor()) cam.LEDOn(CAM_LED_ERROR_BLINK);
     #endif
    }
  }
}

/* function called when slave receive data */
void receiveI2C(int howMany){
  /* useless */
  int reg = Wire.read();
}

/* function called when master request data */
void requestI2C(){
  /* poll presence sensors */
  ballposition[4] = 0;
  for(int i=3; i<8; ++i){
    ballposition[4] |= digitalRead(i) << (i-3);
  }
  /* send requested data */
  Wire.write(ballposition, 5);
}
