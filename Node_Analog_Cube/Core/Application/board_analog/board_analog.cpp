/*
 * board_modbus.cpp
 *
 *  Created on: Jun 17, 2024
 *      Author: anh
 */


#include <board_analog/board_analog.h>
#include "cJSON/cJSON.h"
#include "main.h"
#include "adc.h"
#include "string.h"
#include "stdlib.h"
#include "log/log.h"




extern ADC_HandleTypeDef hadc;
static double analog_param_a = 0.0, analog_param_b = 0.0;
static char desc[50] = {0};  // Data descriptor string.
static const char *TAG = "ANALOG";

static uint32_t BRD_ADC_ReadChannel(uint32_t channel);


static void _delay_ms(uint32_t ms){
	for(uint64_t i=0; i<4050*ms; i++) __NOP();
}

void brd_sensor_pwron(void){
	HAL_GPIO_WritePin(SENSOR_PWR_GPIO_Port, SENSOR_PWR_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(ANA_PWR_GPIO_Port, ANA_PWR_Pin, GPIO_PIN_SET);
    _delay_ms(1000);
}
void brd_sensor_pwroff(void){
	HAL_GPIO_WritePin(SENSOR_PWR_GPIO_Port, SENSOR_PWR_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(ANA_PWR_GPIO_Port, ANA_PWR_Pin, GPIO_PIN_RESET);
}
/**
* WiFi module.
*/
void brd_wf_pwron(bool delay){
	HAL_GPIO_WritePin(WF_PWR_GPIO_Port, WF_PWR_Pin, GPIO_PIN_RESET);
    if(delay) _delay_ms(1000);
}
void brd_wf_pwroff(void){
	HAL_GPIO_WritePin(WF_PWR_GPIO_Port, WF_PWR_Pin, GPIO_PIN_SET);
}
/**
* LED indicator.
*/
void brd_led_on(void){
	HAL_GPIO_WritePin(LED_ACT_GPIO_Port, LED_ACT_Pin, GPIO_PIN_RESET);
}
void brd_led_off(void){
	HAL_GPIO_WritePin(LED_ACT_GPIO_Port, LED_ACT_Pin, GPIO_PIN_SET);
}
void brd_led_toggle(void){
	HAL_GPIO_WritePin(LED_ACT_GPIO_Port, LED_ACT_Pin, (GPIO_PinState)!HAL_GPIO_ReadPin(LED_ACT_GPIO_Port, LED_ACT_Pin));
}


float brd_batt_voltage(void){
    uint32_t x = 0;

    for(int i=0; i<ANALOG_SAMPLE_NUMBER; i++)
        x += BRD_ADC_ReadChannel(VBAT_ADC_CHANNEL);
    x /= ANALOG_SAMPLE_NUMBER;

    return 0.0054 * (float)x - 0.02;
}

float brd_get_analog(void){
    uint32_t x = 0;

    brd_sensor_pwron();
#if FAKE_DATA
    #ifdef NODE_4_20MA
        float ana = (float)((rand()%1000)+5000)/1000.0;
    #endif
    #ifdef NODE_0_10V
        float ana = (float)((rand()%500)+2500)/1000.0;
    #endif
#else
    for(int i=0; i<ANALOG_SAMPLE_NUMBER; i++)
        x += BRD_ADC_ReadChannel(ANA_ADC_CHANNEL);
    x /= ANALOG_SAMPLE_NUMBER;

    float ana = (float)x * ANALOG_CALIB_PARAM_A + ANALOG_CALIB_PARAM_B;
#endif

    float ret = ana * (float)analog_param_a + (float)analog_param_b;
#if DEBUG_ANALOG
    #ifdef NODE_4_20MA
        LOG_DEBUG(TAG, "ADC = %d, AMP = %.02f, DATA = %.02f", x, ana, ret);
    #endif
    #ifdef NODE_0_10V
        LOG_DEBUG(TAG, "ADC = %d, VOL = %.02f, DATA = %.02f", x, ana, ret);
    #endif
#endif
    brd_sensor_pwroff();

    return ret;
}

void brd_config_analog(const char *json_calib, uint16_t json_len){
    double max, min;
    double high = ANALOG_VAL_HIGH, low = ANALOG_VAL_LOW;
    cJSON *root = cJSON_ParseWithLength(json_calib, json_len);

    if (root->type == cJSON_Invalid) {
    	LOG_ERROR(TAG, "Failed to parse JSON modbus descriptor.");
        return;
    }

    if (!cJSON_HasObjectItem(root, "max") || !cJSON_HasObjectItem(root, "min") || !cJSON_HasObjectItem(root, "desc")
#ifdef NODE_0_10V
         || !cJSON_HasObjectItem(root, "vl")  || !cJSON_HasObjectItem(root, "vh")
#endif
    ) return;

    max = cJSON_GetObjectItem(root, "max")->valuedouble;
    min = cJSON_GetObjectItem(root, "min")->valuedouble;
#ifdef NODE_0_10V
    high = cJSON_GetObjectItem(root, "vh")->valuedouble;
    low  = cJSON_GetObjectItem(root, "vl")->valuedouble;
#endif
    char *data_desc = cJSON_GetObjectItem(root, "desc")->valuestring;
    memcpy(desc, (const char*)data_desc, strlen(data_desc));
    free(data_desc);

    analog_param_a = (max - min) / (high - low);
    analog_param_b = min - low * analog_param_a;
}

char *brd_get_analog_desc(void){
	return desc;
}

static uint32_t BRD_ADC_ReadChannel(uint32_t channel){
	uint32_t ADCxConvertedValues = 0;
	ADC_ChannelConfTypeDef sConfig = {0};

	MX_ADC_Init();
	if (HAL_ADCEx_Calibration_Start(&hadc) != HAL_OK)
		Error_Handler();

	sConfig.Channel = channel;
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;
	if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
		Error_Handler();

	if (HAL_ADC_Start(&hadc) != HAL_OK)
		Error_Handler();
	HAL_ADC_PollForConversion(&hadc, HAL_MAX_DELAY);
	HAL_ADC_Stop(&hadc);
	ADCxConvertedValues = HAL_ADC_GetValue(&hadc);

	HAL_ADC_DeInit(&hadc);

	return ADCxConvertedValues;
}



















