/*
 * lorawan.h
 *
 *  Created on: Jun 17, 2024
 *      Author: anh
 */

#ifndef APPLICATION_LORAWAN_LORAWAN_H_
#define APPLICATION_LORAWAN_LORAWAN_H_


#include "stdint.h"
#include "LoRaMacTypes.h"


#ifdef __cplusplus
extern "C"{
#endif


typedef struct {
    uint8_t deveui[8] = {0};
    uint8_t appeui[8] = {0};
    uint8_t appkey[16] = {0};
    DeviceClass_t dev_class = CLASS_A;
    uint32_t period = 30000;
} lorawan_config_t;


void lorawan_apply_config(lorawan_config_t *pconf);


#ifdef __cplusplus
}
#endif

#endif /* APPLICATION_LORAWAN_LORAWAN_H_ */
