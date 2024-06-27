/*
 * lorawan.cpp
 *
 *  Created on: Jun 18, 2024
 *      Author: anh
 */

#include "lorawan/lorawan.h"
#include "LoRaMac.h"
#include "LmHandler.h"


extern SecureElementNvmData_t seNvmInit;
extern LmHandlerParams_t LmHandlerParams;
extern UTIL_TIMER_Time_t TxPeriodicity;

void lorawan_apply_config(lorawan_config_t *pconf){
	LmHandlerParams.DefaultClass = pconf->dev_class;
	memcpy(seNvmInit.SeNvmDevJoinKey.DevEui, pconf->deveui, 8);
	memcpy(seNvmInit.SeNvmDevJoinKey.JoinEui, pconf->appeui, 8);
	memcpy(seNvmInit.KeyList[APP_KEY].KeyValue, pconf->appkey, 16);
	memcpy(seNvmInit.KeyList[NWK_KEY].KeyValue, pconf->appkey, 16);
	TxPeriodicity = pconf->period;
}

