/*
 * board_config.h
 *
 *  Created on: Jun 17, 2024
 *      Author: anh
 */

#ifndef APPLICATION_BOARD_CONFIG_BOARD_CONFIG_H_
#define APPLICATION_BOARD_CONFIG_BOARD_CONFIG_H_


#include "stdlib.h"
#include "lorawan/lorawan.h"
#include "stm32wlxx_hal.h"

#ifdef __cplusplus
extern "C"{
#endif


typedef struct {
    lorawan_config_t lorawan;
    char *mbdesc = NULL;
} board_config_t;



void board_read_config(UART_HandleTypeDef *phuart);
bool board_config_parse(board_config_t *pconfig, const char *json_str, uint16_t json_len);



#ifdef __cplusplus
}
#endif

#endif /* APPLICATION_BOARD_CONFIG_BOARD_CONFIG_H_ */
