/**
 * @file camduino.c
 *
 * @section desc File description
 *
 * Librairy to get some data from an Arduino over I2C.
 * CMUcam4 must be connect on it and Arduino must run "CMU_Tracker_I2C" program,
 * distance sensors must be connected to the board too.
 * 
 *
 * @section copyright Copyright
 *
 * Trampoline is copyright (c) IRCCyN 2005-2014
 * Trampoline is protected by the French intellectual property law.
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
 *
 * @date 2014/07/21
 * @author Benjamin Sientzoff
 * @version 0.1
 */

#include "camduino.h"
#include "nxt_avr.h"

/** 
 * Initiate Arduino driver, don't forget to call i2c_init() after.
 * Set up i2c port, tram communication and *power* supply.
 * @param[in] i2c_port NXT port where Arduino is connected
 */
void init_camduino(int i2c_port){
	arduino_port = i2c_port;
	enum i2c_conf conf[] = {I2C_CONF_GETDATA};
	nxt_avr_set_input_power(arduino_port, 2);
	i2c_start(arduino_port, conf, 0);
}

/**
 * Fill a position strucuture to get ball position
 * @param[in] ball A reference to the struct to fill 
 */
void get_ball_position(struct position* ball){
	/* get ball position from I2C */
	ball->x = i2c_data[arduino_port][1] << 8;
	ball->x |= i2c_data[arduino_port][0];
	
	ball->y = i2c_data[arduino_port][3] << 8;
	ball->y |= i2c_data[arduino_port][2];
}

/**
 * Get global presence sensors state 
 * @param[out] int State of the presence sensors, equals to NO_DETECTED_OBJECT if 
 * an object is not detected, else return a different value
 */
int object_detected(){
	pstate = i2c_data[arduino_port][4];
	return pstate;
}

/**
 * Get an given presence sensor state
 * @param[in] sensor Name of the presence sensor to examine
 * @param[out] int state of the sensor, return 0 if no detected object, else 1
 */
int get_pstate(enum psensor sensor){
	return !i2c_data[arduino_port][4] & sensor;
}