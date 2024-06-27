/*
 * board_modbus.h
 *
 *  Created on: Jun 17, 2024
 *      Author: anh
 */

#ifndef APPLICATION_BOARD_MODBUS_BOARD_MODBUS_H_
#define APPLICATION_BOARD_MODBUS_BOARD_MODBUS_H_


#include "stdint.h"
#include "stdio.h"
#include "stm32wlxx_hal.h"


#define VBAT_ADC_CHANNEL				ADC_CHANNEL_4
#define ANALOG_SAMPLE_NUMBER   			10

#define MB_DESC_SET_MAX_SIZE           	5
#define MB_FCODE_READ_COILS            	0x01
#define MB_FCODE_READ_DISCRTE_INPUTS   	0x02
#define MB_FCODE_READ_HOLDING_REGISTER 	0x03
#define MB_FCODE_READ_INPUT_REGISTER   	0x04
#define MB_FCODE_WRITE_SINGLE_COIL     	0x05
#define MB_FCODE_WRITE_SINGLE_REGISTER 	0x06


#ifdef __cplusplus
extern "C"{
#endif

typedef struct{
    char     *desc    = NULL;  // Slave descriptor string.
    uint32_t baud     = 9600; // Slave baudrate.
    uint8_t  addr     = 0x00; // Slave address.
    uint8_t  fcode    = 0x00; // Function code.
    uint16_t reg      = 0x00; // Read register.
    uint8_t  rqty     = 1;    // Read quanty.
    uint16_t *rdata   = NULL; // Session read data.
    uint16_t wdata    = 0x00; // Session write data.
    uint8_t  res      = 0;    // Session result.
    int      div      = 100;  // Data divide.
} mb_desc_t; // Modbus descriptor.



void brd_wf_pwron(bool delay);
void brd_wf_pwroff(void);

void brd_sensor_pwron(void);
void brd_sensor_pwroff(void);

void brd_led_on(void);
void brd_led_off(void);
void brd_led_toggle(void);

float brd_batt_voltage(void);


void board_modbus_init(void);
void mb_config_desc(const char *json_desc, uint16_t json_len);
void mb_reqdata(void);
void mb_release_data(void);
uint8_t mb_set_size(void);
mb_desc_t *mb_select_slave(uint8_t idx);


#ifdef __cplusplus
}
#endif

#endif /* APPLICATION_BOARD_MODBUS_BOARD_MODBUS_H_ */
