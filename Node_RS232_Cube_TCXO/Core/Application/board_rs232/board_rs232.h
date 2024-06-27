/*
 * board_rs232.h
 *
 *  Created on: Jun 17, 2024
 *      Author: anh
 */

#ifndef APPLICATION_BOARD_RS232_BOARD_RS232_H_
#define APPLICATION_BOARD_RS232_BOARD_RS232_H_


#include "stdint.h"
#include "stdio.h"
#include "stm32wlxx_hal.h"


#define VBAT_ADC_CHANNEL				ADC_CHANNEL_4
#define ANALOG_SAMPLE_NUMBER   			10

#define RS232_SENSOR_SET_MAX_SIZE 		10


#ifdef __cplusplus
extern "C"{
#endif

typedef struct{
    const char *mpn = NULL;     // Sensor manufacturer product number.
    uint32_t baud = 9600; 		// Sensor UART baudrate.
    void (*request)(void);      // Sensor request data function pointer.
    char *(*response)(void);    // Sensor response data function pointer.
} rs232_sensor_desc_t;          // Sensor rs232 protocol descriptor.



void brd_wf_pwron(bool delay);
void brd_wf_pwroff(void);

void brd_sensor_pwron(void);
void brd_sensor_pwroff(void);

void brd_led_on(void);
void brd_led_off(void);
void brd_led_toggle(void);

float brd_batt_voltage(void);

void board_rs232_init(void);

bool rs232_config_desc(const char *mpn);
char *rs232_reqdata(void);


#ifdef __cplusplus
}
#endif

#endif /* APPLICATION_BOARD_RS232_BOARD_RS232_H_ */
