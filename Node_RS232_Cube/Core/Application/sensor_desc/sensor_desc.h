/*
 * sensor_desc.h
 *
 *  Created on: Jun 21, 2024
 *      Author: anh
 */

#ifndef APPLICATION_SENSOR_DESC_SENSOR_DESC_H_
#define APPLICATION_SENSOR_DESC_SENSOR_DESC_H_


#ifdef __cplusplus
extern "C"{
#endif


void ligo_sp_rs232_request(void);
char *ligo_sp_rs232_response(void);

void incubator_ir_co2_request(void);
char *incubator_ir_co2_response(void);





#ifdef __cplusplus
}
#endif


#endif /* APPLICATION_SENSOR_DESC_SENSOR_DESC_H_ */
