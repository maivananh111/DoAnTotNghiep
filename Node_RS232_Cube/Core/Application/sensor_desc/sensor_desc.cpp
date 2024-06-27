/*
 * sensor_desc.cpp
 *
 *  Created on: Jun 21, 2024
 *      Author: anh
 */

#include "sensor_desc/sensor_desc.h"
#include "stm32wlxx_hal.h"
#include "stdio.h"
#include "stdint.h"
#include "stdlib.h"
#include "string.h"




extern UART_HandleTypeDef huart1;





static uint8_t ligo_sp_rs232_checksum(uint8_t *data, uint8_t size){
    uint8_t crc = 0;

    for(uint8_t j=0; j<size; j++){
        uint8_t i = data[j] ^ crc;
        crc = 0;

        if(i & 0x01) crc ^= 0x5e;
        if(i & 0x02) crc ^= 0xbc;
        if(i & 0x04) crc ^= 0x61;
        if(i & 0x08) crc ^= 0xc2;
        if(i & 0x10) crc ^= 0x9d;
        if(i & 0x20) crc ^= 0x23;
        if(i & 0x40) crc ^= 0x46;
        if(i & 0x80) crc ^= 0x8c;
    }

    return crc;
}
void ligo_sp_rs232_request(void){
    uint8_t buf[4] = {0};
    buf[0] = 0x31;
    buf[1] = 0x02;
    buf[2] = 0x06;
    buf[3] = ligo_sp_rs232_checksum(buf, 3);

    HAL_UART_Transmit(&huart1, buf, 4, 1000);
}
char *ligo_sp_rs232_response(void){
    uint8_t buf[12];
//    uint8_t i=0;

//    while(i<12){
	HAL_UART_Receive(&huart1, buf, 12, 2000);
//		return NULL;
//    }

    char *resp = (char *)malloc(65);
    memset(resp, 0, 65);
    sprintf(resp, "\"temperature\":%d,\"relative_level\":%d", (int)(buf[3]*100), (int)((buf[4]<<8 | buf[5])));

    return resp;
}





void incubator_ir_co2_request(void){

}
char *incubator_ir_co2_response(void){
    char *resp = (char *)malloc(65);
    memset(resp, 0, 65);
    sprintf(resp, "\"co2\":%d", (int)(rand()%30+10));

    return resp;
}




