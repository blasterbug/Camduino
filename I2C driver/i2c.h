#ifndef __I2C_H__
#define __I2C_H__

// Includes:
#include "tpl_os_std_types_generic.h"

// TODO: Generate with Goil:
// Defines:
#define I2C_DATA_N 5
#define I2C_PORT_N 4
#define I2C_FRAME_N 9

// Enums:
enum i2c_conf {
	I2C_CONF_GETDATA = 0,
	I2C_CONF_N
};

/** IRQ handler **/
void i2c_timer_isr_C_function (void);

/** Driver's API: **/
void i2c_init (void);
void i2c_start  (u8 port_id,
				 enum i2c_conf conf[I2C_CONF_N],
				 u8 conf_n);
void i2c_stop (u8 port_id);

////
void i2c_disable(int port);
void i2c_enable(int port);
int i2c_busy(int port);
int i2c_start_transaction(int port, 
						  u32 address, 
						  int internal_address, 
						  int n_internal_address_bytes, 
						  u8 *data, 
						  u32 nbytes,
						  int write);
////

#endif // __I2C_H__
