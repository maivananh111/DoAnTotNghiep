/*
 * board_config.cpp
 *
 *  Created on: Jun 17, 2024
 *      Author: anh
 */


#include "board_config/board_config.h"
#include "stdio.h"
#include "string.h"
#include "log/log.h"
#include "cJSON/cJSON.h"


static const char *TAG = "CFG";
static UART_HandleTypeDef *_phuart;


void board_read_config(UART_HandleTypeDef *phuart){
	_phuart = phuart;

    uint32_t UID_L = *(__IO uint32_t *)UID64_BASE;
    uint32_t UID_H = *(__IO uint32_t *)(UID64_BASE + 4UL);

    char buf[30] = {0};
    sprintf(buf, "{\"deveui\":\"%04x%04x%04x%04x\"}",
    		(uint16_t)((UID_H>>16)&0xFFFF), (uint16_t)(UID_H&0xFFFF), (uint16_t)((UID_L>>16)&0xFFFF), (uint16_t)(UID_L&0xFFFF));
    HAL_UART_Transmit(phuart, (uint8_t *)buf, strlen(buf), 1000);
}


bool board_config_parse(board_config_t *pconfig, const char *json_str, uint16_t json_len){
    cJSON *root = cJSON_ParseWithLength(json_str, json_len);

    if (root->type == cJSON_Invalid) {
    	LOG_ERROR(TAG, "Failed to parse JSON board config.");
        return false;
    }

    if(!cJSON_HasObjectItem(root, "wan") || !cJSON_HasObjectItem(root, "mbdesc")) return false;
    cJSON *wan = cJSON_GetObjectItem(root, "wan");
    cJSON *mbdesc = cJSON_GetObjectItem(root, "mbdesc");

    char *mb_desc_str = cJSON_PrintUnformatted(mbdesc);
    char *wan_str = cJSON_PrintUnformatted(wan);
    uint16_t mb_desc_len = strlen(mb_desc_str);
    pconfig->mbdesc = (char *)malloc(mb_desc_len + 1);
    memcpy(pconfig->mbdesc, mb_desc_str, mb_desc_len);
    pconfig->mbdesc[mb_desc_len] = 0;

    LOG_INFO(TAG, "LoRaWAN configuration: %s", wan_str);
    LOG_INFO(TAG, "Modbus descriptor: %s", pconfig->mbdesc);

    free(wan_str);
    free(mb_desc_str);

    if (cJSON_HasObjectItem(wan, "deveui")) {
        uint64_t deveui = (uint64_t)strtoull(cJSON_GetObjectItem(wan, "deveui")->valuestring, NULL, 16);
        for (uint8_t i=0; i<8; i++){
        	pconfig->lorawan.deveui[i] = (uint8_t)(deveui >> (56-8*i));
        }
    }
    if (cJSON_HasObjectItem(wan, "appeui")) {
        uint64_t appeui = (uint64_t)strtoull(cJSON_GetObjectItem(wan, "appeui")->valuestring, NULL, 16);
        for (uint8_t i=0; i<8; i++){
        	pconfig->lorawan.appeui[i] = (uint8_t)(appeui >> (56-8*i));
        }
    }
    if (cJSON_HasObjectItem(wan, "appkey")) {
        const char *appkey = cJSON_GetObjectItem(wan, "appkey")->valuestring;
        char appkey_l_buf[16];
        memcpy(appkey_l_buf, appkey, 16);

        uint64_t appkey_l = strtoull((const char *)appkey_l_buf, NULL, 16);
        uint64_t appkey_h = strtoull((const char *)(appkey + 16), NULL, 16);

        for (uint8_t i=0; i<8; i++){
        	pconfig->lorawan.appkey[i] = (uint8_t)(appkey_l >> (56-8*i));
        	pconfig->lorawan.appkey[i+8] = (uint8_t)(appkey_h >> (56-8*i));
        }
    }
    if (cJSON_HasObjectItem(wan, "class")) {
        char *dev_class = cJSON_GetObjectItem(wan, "class")->valuestring;
        if(!strcmp(dev_class, "B")) pconfig->lorawan.dev_class = CLASS_B;
        if(!strcmp(dev_class, "C")) pconfig->lorawan.dev_class = CLASS_C;
        else                 		pconfig->lorawan.dev_class = CLASS_A;
    }
    if (cJSON_HasObjectItem(wan, "period")) pconfig->lorawan.period = cJSON_GetObjectItem(wan, "period")->valueint;

    cJSON_Delete(root);

    return true;
}




