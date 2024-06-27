/*
 * board_modbus.h
 *
 *  Created on: Jun 17, 2024
 *      Author: anh
 */

#ifndef APPLICATION_BOARD_ANALOG_BOARD_ANALOG_H_
#define APPLICATION_BOARD_ANALOG_BOARD_ANALOG_H_


#include "stdint.h"
#include "stdio.h"
#include "stm32wlxx_hal.h"




//#define NODE_4_20MA
#define NODE_0_10V


#define DEBUG_ANALOG 					0
#define FAKE_DATA 						0

#define ANA_ADC_CHANNEL					ADC_CHANNEL_3
#define VBAT_ADC_CHANNEL				ADC_CHANNEL_4
#define ANALOG_SAMPLE_NUMBER   			10

#ifdef NODE_4_20MA
#define ANALOG_CALIB_PARAM_A (0.0055890411)
#define ANALOG_CALIB_PARAM_B (1.01465753425)
#define ANALOG_VAL_LOW       (4.0)
#define ANALOG_VAL_HIGH      (20.0)
#endif
#ifdef NODE_0_10V
#define ANALOG_CALIB_PARAM_A (0.0031)
#define ANALOG_CALIB_PARAM_B (0.0118)
#define ANALOG_VAL_LOW       (0.0)
#define ANALOG_VAL_HIGH      (10.0)
#endif


#ifdef __cplusplus
extern "C"{
#endif

void brd_wf_pwron(bool delay);
void brd_wf_pwroff(void);

void brd_sensor_pwron(void);
void brd_sensor_pwroff(void);

void brd_led_on(void);
void brd_led_off(void);
void brd_led_toggle(void);

float brd_batt_voltage(void);

void brd_config_analog(const char *json_calib, uint16_t json_len);
float brd_get_analog(void);
char *brd_get_analog_desc(void);


#ifdef __cplusplus
}
#endif

#endif /* APPLICATION_BOARD_ANALOG_BOARD_ANALOG_H_ */
