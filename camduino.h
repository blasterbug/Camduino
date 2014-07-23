/**
 * @file camduino.h
 *
 * @section desc File description
 *
 * Librairy to get some data from an Arduino over I2C.
 * CMUcam4 must be connected on it and Arduino must run "CMU_Tracker_I2C" program,
 * distance sensors must be connected to the board too.
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
 * @date 2014/07/18
 * @author Benjamin Sientzoff
 * @version 0.1
 */

#ifndef __CAMDUINO_H__
#define __CAMDUINO_H__

#include "i2c.h"

/** list of presence sensors connected to the Arduino board */
enum psensor {
	/** the first presence sensor on the left */
	PSENSOR_A = 0x1,
	/** the second presence sensor on the left */
	PSENSOR_B = 0x2,
	/** presence sensor on the middle */
	PSENSOR_C = 0x4,
	/** the second sensor of the right */
	PSENSOR_D = 0x8,
	/** the last sensor (on the right) */
	PSENSOR_E = 0x10
};

/** state of (all) presence sensors when there is no object */
#define NO_DETECTED_OBJECT 31

/** store ball position */
struct position {
	/** x coordinate of the ball */
	int x;
	/** y coordinate of the ball */
	int y; /* y coordinate of the ball */
};


/** buffers storing I2C data */
extern u8 i2c_data[I2C_PORT_N][I2C_DATA_N];

/** NXT port connected to Arduino board */
int arduino_port;

/** global state of presence sensors */
int pstate = NO_DETECTED_OBJECT;

/** Initiate Arduino driver */
void init_camduino(int);

/** Fill a position strucuture to get ball position */
void get_ball_position(struct position*);

/** Get global presence sensors state, consider the five sensors as one */
int object_detected();

/** get an given presence sensor state */
int get_pstate(enum psensor);

#endif // __CAMDUINO_H__
