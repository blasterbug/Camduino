#include "i2c.h"
#include "interrupts.h"
#include "aic.h"
#include "AT91SAM7.h"

#include "tpl_com_definitions.h"
#include "tpl_com_notification.h"
#include "tpl_com_mo.h"

#include <string.h>

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

// get data over I2C
u8 i2c_data[I2C_PORT_N][I2C_DATA_N];

enum frame_state {
  FRAME_STATE_START0 = 0,
  FRAME_STATE_START1,
  FRAME_STATE_START2,
  FRAME_STATE_START3,

  FRAME_STATE_DATA0,
  FRAME_STATE_DATA1,
  FRAME_STATE_DATA2,
  FRAME_STATE_DATA3,

  FRAME_STATE_ACK0,
  FRAME_STATE_ACK1,
  FRAME_STATE_ACK2,
  FRAME_STATE_ACK3,

  FRAME_STATE_STOP0,
  FRAME_STATE_STOP1,
  FRAME_STATE_STOP2,
  FRAME_STATE_STOP3
};

struct frame {
  u8 write:1;
  u8 stop:1;
  u8 last:1;

  u8 data;
  u8 bit_count;
  u8 byte_count;

  enum frame_state state;
};

struct link {
  const u32 SCL;
  const u32 SDA;
  u8 active:1;

  u8 data_index;
  u8 data[I2C_DATA_N];

  u8 conf_index;
  u8 conf_n;
  enum i2c_conf conf[I2C_CONF_N];

  u8 frame_index;
  u8 frameset_index;
  const struct frame frameset[I2C_CONF_N][I2C_FRAME_N];

  struct frame frame_buffer;
  u8 ack:1;

  const u32 period;
  u32 ticks;

  void (*isr) (u8);
};

static void idle (u8 port_id);
static void busy (u8 port_id);
static void wait (u8 port_id);

static struct link i2c_links[I2C_PORT_N] = {
  // TODO: Generate with Goil.
	{
		/* NXT PORT 1 : 1 << 23, 1 << 18 */
		1 << 23, 1 << 18,FALSE,
		0, {0},
		0, 0, {0},
		0, 0, {},
		{FALSE, FALSE, FALSE, 0x00, 0, 0, FRAME_STATE_START0}, FALSE,
		1337, 0,
		idle
	},
	
	{
		/* NXT PORT 2 : 1 << 28, 1 << 19 */
		1 << 28, 1 << 19,FALSE,
		0, {0},
		0, 0, {0},
		0, 0, {},
		{FALSE, FALSE, FALSE, 0x00, 0, 0, FRAME_STATE_START0}, FALSE,
		1337, 0,
		idle
	},
	{
		/* NXT PORT 3 : 1 << 29, 1 << 20 */
		1 << 29, 1 << 20,FALSE,
		0, {0},
		0, 0, {0},
		0, 0,
		{
			{
				/// frame for IR distance sensor
				{TRUE,  FALSE, FALSE, 0x02, 8, 1, FRAME_STATE_START1},
				{TRUE,  TRUE,  FALSE, 0x42, 8, 1, FRAME_STATE_DATA0 },
				{TRUE,  FALSE, FALSE, 0x03, 8, 1, FRAME_STATE_START0},
				{FALSE, TRUE,  TRUE,  0x00, 8, 2, FRAME_STATE_DATA0 }
			}
		},
		{FALSE, FALSE, FALSE, 0x00, 0, 0, FRAME_STATE_START0}, FALSE,
		1337, 0,
		idle
	},
	
	{
		/* NXT PORT 4 : 1 << 30, 1 << 2*/
		1 << 30, 1 << 2, FALSE,
		0, {0},
		0, 0, {0},
		0, 0,
		{
			{
				/// frame for Arduino board
				{TRUE,  FALSE, FALSE, 0x02, 8, 1, FRAME_STATE_START1},
				{TRUE,  TRUE,  FALSE, 0x42, 8, 1, FRAME_STATE_DATA0 },
				{TRUE,  FALSE, FALSE, 0x03, 8, 1, FRAME_STATE_START0},
				{FALSE, TRUE,  TRUE,  0x00, 8, 5, FRAME_STATE_DATA0 }
			}
		},
		{FALSE, FALSE, FALSE, 0x00, 0, 0, FRAME_STATE_START0}, FALSE,
		1337, 0,
		idle
	}
};

// ===================
//  Utils. functions:
// ===================

void
communicate (struct link *link)
{
  link -> data_index = 0;
  memset (link -> data, 0, sizeof (link -> data));

  link -> frame_index = 0;
  link -> frameset_index = link -> conf[link -> conf_index];
  if (link -> ack &&
      link -> conf_index < link -> conf_n - 1)
    link -> conf_index++;
 
  memcpy (&link -> frame_buffer,
          &link -> frameset[link -> frameset_index]
          [link -> frame_index],
          sizeof (struct frame));

  link -> ticks = 0;
  link -> isr = busy;
}

// ================
//  ISR functions:
// ================

void
i2c_timer_isr_C_function (void)
{
  /* `dummy` is an unused variable but mustn't be removed!
   * Question of life or click of the death.
   * This __attribute__ ((unused)) is used to avoid the GCC
   * unused variable warrning.
   */

  __attribute__ ((unused)) u32 dummy = *AT91C_TC0_SR;

  // For each port: call associated ISR:
  for (u8 port_id = 0; port_id < I2C_PORT_N; port_id++)
    i2c_links[port_id].isr (port_id);
}

void
idle (u8 port_id)
{
  /* Do nothing on this state until link is activated.
   */

  struct link *link = &i2c_links[port_id];
  if (link -> active)
    {
      /* User asked to start I2C communication using API function
       * 'i2c_start'. So link goes into busy state.
       */
  
      communicate (link);
    }
}

void
busy (u8 port_id)
{
  /*
   */

  u32 output = 0;
  u32 input  = 0;
  u32 high   = 0;
  u32 low    = 0;
  u32 lines  = *AT91C_PIOA_PDSR;

  struct link  *link  = &i2c_links[port_id];
  struct frame *frame = &link -> frame_buffer;
  u8 *data = &link -> data[link -> data_index];

  link -> ticks++;
 
  switch (frame -> state)
    {
      // Start bit:
    case FRAME_STATE_START0:
      low |= link -> SCL;
      frame -> state = FRAME_STATE_START1;
      break;

    case FRAME_STATE_START1:
      high |= link -> SDA;
      frame -> state = FRAME_STATE_START2;
      break;

    case FRAME_STATE_START2:
      high |= link -> SCL;
      frame -> state = FRAME_STATE_START3;
      break;

    case FRAME_STATE_START3:
      low |= link -> SDA;
      frame -> state = FRAME_STATE_DATA0;
      break;

      /* Data bit:
       * If it is a write frame, i.e. the NXT send data to the
       * device, the data line (SDA) is set as an output in which
       * we will be able to write data (MSB).
       * Else, it is a read frame, i.e. the device send data to
       * the NXT, the data line (SDA) is set as an input in which
       * we will be able to read data (MSB).
       */
    case FRAME_STATE_DATA0:
      frame -> bit_count--;

      low |= link -> SCL;
      frame -> state = FRAME_STATE_DATA1;
      break;

    case FRAME_STATE_DATA1:
      if (frame -> write)
        {
          output |= link -> SDA;
          if ((frame -> data >> frame -> bit_count) & 0x01)
            high |= link -> SDA;
          else
            low  |= link -> SDA;
        }
      else
        {
          input |= link -> SDA;
        }
      frame -> state = FRAME_STATE_DATA2;
      break;

    case FRAME_STATE_DATA2:
      high |= link -> SCL;
      frame -> state = FRAME_STATE_DATA3;
      break;

    case FRAME_STATE_DATA3:
		if (!frame -> write)
		{
			/* clock stretching */
			if (!(lines & link -> SCL)) break;
			
			*data |= ((lines & link -> SDA ? 1 : 0) << frame -> bit_count);
		}
			
      if (frame -> bit_count > 0)
        frame -> state = FRAME_STATE_DATA0;
      else
        frame -> state = FRAME_STATE_ACK0;
      break;

      /* Ack bit:
       * If it was a write frame, i.e. the NXT sended data to
       * the device, the data line (SDA) is set as an input in
       * which we will be able to read the ACK or NACK of the
       * device.
       * Else, it was a read frame, i.e. the device sended data
       * to the NXT, the data line (SDA) is set as an output in
       * which we will be able to send an ACK to the device.
       */
    case FRAME_STATE_ACK0:
      frame -> byte_count--;

      low |= link -> SCL;
      frame -> state = FRAME_STATE_ACK1;
      break;

    case FRAME_STATE_ACK1:
      if (frame -> write)
        {
          input |= link -> SDA;
        }
      else
        {
          output |= link -> SDA;

          /* We won't acknowledge device for the last sended byte. It
           * avoids strange & unidentified behavior. It have been
           * observed that the official driver does like it.
           */

          if (!frame -> last ||
              frame -> byte_count > 0)
            low  |= link -> SDA;
          else
            high |= link -> SDA;
        }
      frame -> state = FRAME_STATE_ACK2;
      break;

    case FRAME_STATE_ACK2:
      high |= link -> SCL;
      frame -> state = FRAME_STATE_ACK3;
      break;

    case FRAME_STATE_ACK3:
      if (frame -> write)
        {
          /* Receive an ACK or a NACK, i.e. read SDA pin.
           * SDA setted as low means an ACK while setted as
           * high means a NACK.
           */

          link -> ack = (lines & link -> SDA ? FALSE : TRUE);
        }
      else
        {
          link -> ack = TRUE;
        }

      if (link -> ack &&
          frame -> byte_count > 0)
        {
          frame -> state = FRAME_STATE_DATA0;
          frame -> bit_count = 8;

          // On a multiple byte read, handling data buffer:
          if (!frame -> write)
            link -> data_index++;
        }
      else if (!link -> ack ||
               frame -> stop)
        {
          frame -> state = FRAME_STATE_STOP0;
        }
      else
        {
          /* Next frame must have DATA0 setted as state!
           * (Or START if its intended to do a restart bit, that is
           * not an existing behavior while using the NXT standard I2C
           * sensor.)
           */

          link -> frame_index++;
          memcpy (&link -> frame_buffer,
                  &link -> frameset[link -> frameset_index]
                  [link -> frame_index],
                  sizeof (struct frame));
        }
      break;

      // Stop bit:
    case FRAME_STATE_STOP0:
      output |= link -> SDA;
      low |= link -> SCL;
      frame -> state = FRAME_STATE_STOP1;
      break;

    case FRAME_STATE_STOP1:
      low    |= link -> SDA;
      frame -> state = FRAME_STATE_STOP2;
      break;

    case FRAME_STATE_STOP2:
      high |= link -> SCL;
      frame -> state = FRAME_STATE_STOP3;
      break;

    case FRAME_STATE_STOP3:
      if(!link -> ack ||
         frame -> last)
        {
          link -> isr = wait;
        }
      else
        {
          /* Next frame must have START0 setted as state!
           * (No exception.)
           */
          link -> frame_index++;
          memcpy (&link -> frame_buffer,
                  &link -> frameset[link -> frameset_index]
                  [link -> frame_index],
                  sizeof (struct frame));
        }
      
      high |= link -> SDA;
      break;

      // Avoids GCC warnings (sometimes), should not happen.
    default:
      break;
    }

  if (output) *AT91C_PIOA_OER  = output;
  if (input)  *AT91C_PIOA_ODR  = input;
  if (high)   *AT91C_PIOA_SODR = high;
  if (low)    *AT91C_PIOA_CODR = low;
}

extern tpl_data_receiving_mo i2c_data_in_message;

void
wait (u8 port_id)
{
  /* In the aim to offer a real-time compatible I2C driver, once a
   * communication ended (in a proprer way or because of receiving a
   * NACK from device) it is needed to "wait". In that maner each value
   * received from device will be sended back to an userland task
   * depending on an user-specified period.
   *
   * The I2C driver is driven by a NXT hardware clock, when activated,
   * an interrupt is raised periodically calling one of the I2C'
   * ISR. After the "user-specified period" times that an ISR have
   * been called a message will be send to a specified userland
   * task. These messages contain data received from device or an
   * indication on either or not the data send to device have been
   * received by it.
   */
 
  struct link *link = &i2c_links[port_id];

  if (!link -> active)
    {
      /* User asked to stop I2C communication using API function
       * 'i2c_stop'. So link get back to idling state.
       */
  
      link -> isr = idle;
    }
  else
    {
      link -> ticks++;

      /* TODO: Be sure that first time this function is called `link ->
       * ticks` is strictly less than `link -> period`. Must be checked
       * by Goil.
       */

      if (link -> ticks == link -> period)
        {
          /* Defined waitting time is reached. So stop watting, send
           * message & start a new communication.
           */
			memcpy (i2c_data[port_id], link -> data, sizeof (u8) * I2C_DATA_N);

/*   
          // TODO: SendMessage + remove sending message declaration in app .oil
          //////////
          u8 *data = link -> data;
  
          //// From tpl_com_internal_com.c file in tpl_send_static_internal_message function:
          tpl_status result = E_OK;
          tpl_status result_notification = E_OK;
  
          tpl_data_receiving_mo *rmo              = (tpl_data_receiving_mo*) &i2c_data_in_message;
          tpl_base_receiving_mo *rmo_notification = (tpl_base_receiving_mo*) &i2c_data_in_message;
  
          /  iterate through the receiving mo to copy the data to the receivers  /
          while ((result == E_OK || result == E_COM_FILTEREDOUT) && (rmo != NULL))
            {
              result = rmo->receiver(rmo, data);
   
              /
               * Walk along the receiving message object chain and call the notification
               * for each one when the notication exists (and if the message is not 
               * filtered out ).
               /
              if (result == E_OK)
                {
                  tpl_action *notification = rmo_notification -> notification;
                  if (notification != NULL)
                    result_notification |= notification -> action (notification);
                }
              else if (result == E_COM_FILTEREDOUT)
                {
                  result = E_OK;
                }
              rmo_notification = rmo_notification -> next_mo;
   
              rmo = (tpl_data_receiving_mo*) rmo -> base_mo.next_mo;
            }
  
          /  notify the receivers    /
          tpl_notify_receiving_mos(result_notification, FROM_TASK_LEVEL);
          //////////
*/  
          communicate (link);
        }
    }
}

// ================
//  API functions:
// ================

extern void tpl_primary_irq_handler (void);

void
i2c_init (void)
{
  for (u8 port_id = 0; port_id < I2C_PORT_N; port_id++)
    {
      struct link *link = &i2c_links[port_id];
  
      *AT91C_PIOA_MDER  = link -> SCL;
      *AT91C_PIOA_PPUDR = link -> SDA;
      *AT91C_PIOA_PPUER = link -> SCL;
  
      *AT91C_PIOA_OER  = link -> SCL | link -> SDA;
      *AT91C_PIOA_SODR = link -> SCL | link -> SDA;
    }
 
  // Mask interrupts on NXT:
  int interrupts = interrupts_get_and_disable ();
 
  // Initialize clock:
  *AT91C_PMC_PCER = (1 << AT91C_ID_TC0);  // Enable time counter 0 (TC0)
  *AT91C_TC0_CCR  = AT91C_TC_CLKDIS;      // Disable clock on TC0
  *AT91C_TC0_IDR  = ~0x00;                // Disable interrupt on TC0
  *AT91C_TC0_CMR  = 0x4000;               // Configure TC0
  *AT91C_TC0_RC   =                       // Set the register C value:
    (CLOCK_FREQUENCY / 2) / (4 * 9600);   //   NXT needs 4 IRQ to send a bit, I2C bus speed is 9600 bit/s
  *AT91C_TC0_IER  = AT91C_TC_CPCS;        // Enable register C compare interrupt
  *AT91C_TC0_CCR  = AT91C_TC_CLKEN;       // Enable clock on TC0
 
  aic_mask_off   (AT91C_ID_TC0);          // Mask interrupts on TC0
  aic_set_vector (AT91C_ID_TC0,           // Define IRQ handler
                  AIC_INT_LEVEL_NORMAL,
                  (u32) tpl_primary_irq_handler);
  aic_mask_on    (AT91C_ID_TC0);          // Unmask interrupts on TC0
 
  *AT91C_TC0_CCR = AT91C_TC_SWTRG;        // Reset TC0
 
  // Unmask interrupts on NXT:
  if (interrupts)
    interrupts_enable ();
}

void
i2c_start (u8 port_id,
           enum i2c_conf conf[I2C_CONF_N],
           u8 conf_n)
{ 
  // Verifying `port_id`:
  if (port_id >= I2C_PORT_N)
    return;

  // Verifying if already started:
  struct link *link = &i2c_links[port_id];
  if (link -> isr != idle)
    return;
 
  link -> conf_index = 0;
  link -> conf_n = conf_n;
  for (u8 conf_index = 0; conf_index < conf_n; conf_index++)
    link -> conf[conf_index] = conf[conf_index];
 
  // Asynchrone way to start a communication:
  link -> active = TRUE;
}

void
i2c_stop (u8 port_id)
{
  // Verifying `port_id`:
  if (port_id >= I2C_PORT_N)
    return;

  // Verifying if already stopped:
  struct link *link = &i2c_links[port_id];
  if (link -> isr == idle)
    return;

  // Asynchronous way to stop the communication:
  link -> active = FALSE;
}

//// avoid ecrobot errors
void i2c_disable(int port){}
void i2c_enable(int port){}
int i2c_busy(int port){return 0;}
int i2c_start_transaction(int port, 
						  U32 address, 
						  int internal_address, 
						  int n_internal_address_bytes, 
						  U8 *data, 
						  U32 nbytes,
						  int write){return 0;}
////
