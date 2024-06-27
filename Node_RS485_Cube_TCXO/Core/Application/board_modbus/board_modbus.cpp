/*
 * board_modbus.cpp
 *
 *  Created on: Jun 17, 2024
 *      Author: anh
 */


#include "board_modbus/board_modbus.h"
#include "ModbusMaster/ModbusMaster.h"
#include "cJSON/cJSON.h"
#include "main.h"
#include "adc.h"
#include "string.h"
#include "stdlib.h"
#include "log/log.h"
#include "FreeRTOS.h"
#include "semphr.h"




extern UART_HandleTypeDef huart1;
extern ADC_HandleTypeDef hadc;

static const char *TAG = "MODBUS_MASTER";
static SemaphoreHandle_t _s_inprocess;
static uint8_t _mb_desc_set_size = 0;
static mb_desc_t _mb_desc_set[MB_DESC_SET_MAX_SIZE];
static ModbusMaster _mb;

static void mb_txmode(void);
static void mb_rxmode(void);
static void BRD_MB_UART_Init(uint32_t baud);
static uint32_t BRD_ADC_ReadChannel(uint32_t channel);


static void _delay_ms(uint32_t ms){
	for(uint64_t i=0; i<4050*ms; i++) __NOP();
}
/**
* RS485 power control.
*/
static void mb_txmode(void){
	HAL_GPIO_WritePin(MB_DE_GPIO_Port, MB_DE_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(MB_RE_GPIO_Port, MB_RE_Pin, GPIO_PIN_SET);
}
static void mb_rxmode(void){
	HAL_GPIO_WritePin(MB_DE_GPIO_Port, MB_DE_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(MB_RE_GPIO_Port, MB_RE_Pin, GPIO_PIN_RESET);
}

void brd_sensor_pwron(void){
	HAL_GPIO_WritePin(SENSOR_PWR_GPIO_Port, SENSOR_PWR_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(MB_PWR_GPIO_Port, MB_PWR_Pin, GPIO_PIN_SET);
    _delay_ms(1000);
}
void brd_sensor_pwroff(void){
	HAL_GPIO_WritePin(SENSOR_PWR_GPIO_Port, SENSOR_PWR_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(MB_PWR_GPIO_Port, MB_PWR_Pin, GPIO_PIN_RESET);
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


void board_modbus_init(void){
    brd_sensor_pwroff();
    mb_rxmode();
    HAL_UART_DeInit(&huart1);

    _mb.preTransmission(mb_txmode);
    _mb.postTransmission(mb_rxmode);

    _s_inprocess = xSemaphoreCreateMutex();
}

void mb_reqdata(void){
    if(xSemaphoreTake(_s_inprocess, 10) == pdPASS){
        brd_sensor_pwron();

        for(uint8_t id=0; id<_mb_desc_set_size; id++){
            mb_desc_t *desc = &_mb_desc_set[id];

            HAL_UART_DeInit(&huart1);
            _delay_ms(10);
            BRD_MB_UART_Init(desc->baud);
            _mb.begin(desc->addr, &huart1);

            switch(desc->fcode){
                case MB_FCODE_READ_COILS:
                    desc->res = _mb.readCoils(desc->reg, desc->rqty);
                break;
                case MB_FCODE_READ_DISCRTE_INPUTS:
                    desc->res = _mb.readDiscreteInputs(desc->reg, desc->rqty);
                break;
                case MB_FCODE_READ_HOLDING_REGISTER:
                    desc->res = _mb.readHoldingRegisters(desc->reg, desc->rqty);
                break;
                case MB_FCODE_READ_INPUT_REGISTER:
                    desc->res = _mb.readInputRegisters(desc->reg, desc->rqty);
                break;
                case MB_FCODE_WRITE_SINGLE_COIL:
                    desc->res = _mb.writeSingleCoil(desc->reg, (uint8_t)desc->wdata);
                break;
                case MB_FCODE_WRITE_SINGLE_REGISTER:
                    desc->res = _mb.writeSingleRegister(desc->reg, desc->wdata);
                break;
            };

            if (desc->res == _mb.ku8MBSuccess
                && desc->fcode != MB_FCODE_WRITE_SINGLE_COIL
                && desc->fcode != MB_FCODE_WRITE_SINGLE_REGISTER) {
            	desc->rdata = NULL;
                desc->rdata = (uint16_t *)calloc(desc->rqty, sizeof(uint16_t));
                if(desc->rdata == NULL){
                	LOG_ERROR(TAG, "Modbus data allocation failed.");
                	continue;
                }
                for(uint8_t i=0; i<desc->rqty; i++)
                    desc->rdata[i] = _mb.getResponseBuffer(i);

                desc->res = 1;
            }
            else{
            	LOG_ERROR(TAG, "Modbus slave 0x%02x error %d.", desc->addr, desc->res);
            }
        }

        brd_sensor_pwroff();
        HAL_UART_DeInit(&huart1);

        xSemaphoreGive(_s_inprocess);
    }
}

void mb_release_data(void){
    for(uint8_t id=0; id<_mb_desc_set_size; id++){
        mb_desc_t *desc = &_mb_desc_set[id];
        if(desc->rdata != NULL) free(desc->rdata);
    }
}

uint8_t mb_set_size(void){
    return _mb_desc_set_size;
}

mb_desc_t *mb_select_slave(uint8_t idx){
    if(idx > MB_DESC_SET_MAX_SIZE) idx = MB_DESC_SET_MAX_SIZE;

    return &_mb_desc_set[idx];
}

void mb_config_desc(const char *json_desc, uint16_t json_len){
    cJSON *root = cJSON_ParseWithLength(json_desc, json_len);

    if (root->type == cJSON_Invalid) {
    	LOG_ERROR(TAG, "Failed to parse JSON modbus descriptor.");
        return;
    }

    _mb_desc_set_size = cJSON_GetArraySize(root);
    if(_mb_desc_set_size > MB_DESC_SET_MAX_SIZE) _mb_desc_set_size = MB_DESC_SET_MAX_SIZE;
    for(uint8_t i=0; i<_mb_desc_set_size; i++){
        cJSON *child = cJSON_GetArrayItem(root, i);

        if (cJSON_HasObjectItem(child, "desc"))  {
        	char *s = cJSON_GetObjectItem(child, "desc")->valuestring;
        	uint8_t len = strlen(s);
        	_mb_desc_set[i].desc = (char *)malloc(len+1);
        	memcpy(_mb_desc_set[i].desc, s, len);
        	_mb_desc_set[i].desc[len] = 0;
        }
        if (cJSON_HasObjectItem(child, "baud"))  _mb_desc_set[i].baud  = cJSON_GetObjectItem(child, "baud")->valueint;
        if (cJSON_HasObjectItem(child, "addr"))  _mb_desc_set[i].addr  = strtol(cJSON_GetObjectItem(child, "addr")->valuestring,  NULL, 16);
        if (cJSON_HasObjectItem(child, "fcode")) _mb_desc_set[i].fcode = strtol(cJSON_GetObjectItem(child, "fcode")->valuestring, NULL, 16);
        if (cJSON_HasObjectItem(child, "reg"))   _mb_desc_set[i].reg   = strtol(cJSON_GetObjectItem(child, "reg")->valuestring,   NULL, 16);
        if (cJSON_HasObjectItem(child, "rqty"))  _mb_desc_set[i].rqty  = cJSON_GetObjectItem(child, "rqty")->valueint;
        if (cJSON_HasObjectItem(child, "div"))   _mb_desc_set[i].div   = cJSON_GetObjectItem(child, "div")->valueint;
    }

    cJSON_Delete(root);
}





static void BRD_MB_UART_Init(uint32_t baud){
	huart1.Instance = USART1;
	huart1.Init.BaudRate = baud;
	huart1.Init.WordLength = UART_WORDLENGTH_8B;
	huart1.Init.StopBits = UART_STOPBITS_1;
	huart1.Init.Parity = UART_PARITY_NONE;
	huart1.Init.Mode = UART_MODE_TX_RX;
	huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart1.Init.OverSampling = UART_OVERSAMPLING_16;
	huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
	huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
	if (HAL_UART_Init(&huart1) != HAL_OK)
		Error_Handler();
	if (HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
		Error_Handler();
	if (HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
		Error_Handler();
	if (HAL_UARTEx_DisableFifoMode(&huart1) != HAL_OK)
		Error_Handler();
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



















