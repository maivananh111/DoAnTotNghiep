/*
 * board_rs232.cpp
 *
 *  Created on: Jun 17, 2024
 *      Author: anh
 */


#include "board_rs232/board_rs232.h"
#include "sensor_desc/sensor_desc.h"
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



static rs232_sensor_desc_t ligo_sp_rs232 = {
    .mpn      = "LIGO SP-RS232",
	.baud     = 9600,
    .request  = ligo_sp_rs232_request,
    .response = ligo_sp_rs232_response
};
static rs232_sensor_desc_t incubator_ir_co2 = {
    .mpn      = "INCUBATOR IR-CO2",
	.baud     = 9600,
    .request  = incubator_ir_co2_request,
    .response = incubator_ir_co2_response
};



static const char *TAG = "RS232";
static SemaphoreHandle_t _s_inprocess;
static rs232_sensor_desc_t *_sensor_desc_set[RS232_SENSOR_SET_MAX_SIZE] = {0};
static rs232_sensor_desc_t *_rs232_desc = NULL;


static void BRD_RS232_UART_Init(uint32_t baud);
static uint32_t BRD_ADC_ReadChannel(uint32_t channel);


static void _delay_ms(uint32_t ms){
	for(uint64_t i=0; i<4050*ms; i++) __NOP();
}
/**
* RS232 power control.
*/

void brd_sensor_pwron(void){
	HAL_GPIO_WritePin(SENSOR_PWR_GPIO_Port, SENSOR_PWR_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(RS232_PWR_GPIO_Port, RS232_PWR_Pin, GPIO_PIN_SET);
    _delay_ms(2000);
}
void brd_sensor_pwroff(void){
	HAL_GPIO_WritePin(SENSOR_PWR_GPIO_Port, SENSOR_PWR_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(RS232_PWR_GPIO_Port, RS232_PWR_Pin, GPIO_PIN_RESET);
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


void board_rs232_init(void){
    brd_sensor_pwroff();
    HAL_UART_DeInit(&huart1);

//	brd_sensor_pwron();

    _s_inprocess = xSemaphoreCreateMutex();
}

char *rs232_reqdata(void){
	char *s = NULL;

	if(xSemaphoreTake(_s_inprocess, 10)){
        HAL_UART_DeInit(&huart1);
        _delay_ms(10);
		BRD_RS232_UART_Init(_rs232_desc->baud);
		brd_sensor_pwron();

		_rs232_desc->request();
		s = _rs232_desc->response();

		if(s == NULL) LOG_ERROR(TAG, "RS232 Sensor error.");
		HAL_UART_DeInit(&huart1);
		brd_sensor_pwroff();
		xSemaphoreGive(_s_inprocess);
	}

    return s;
}

bool rs232_config_desc(const char *mpn){
    _sensor_desc_set[0] = &ligo_sp_rs232;
    _sensor_desc_set[1] = &incubator_ir_co2;

    for(uint8_t i=0; i<RS232_SENSOR_SET_MAX_SIZE; i++){
        rs232_sensor_desc_t *desc = _sensor_desc_set[i];
        if(desc == NULL) continue;

        if(strcmp(desc->mpn, mpn) == 0){
            LOG_DEBUG(TAG, "Selected sensor product number %s", desc->mpn);
            _rs232_desc = desc;
            return true;
        }
    }

    return false;
}




static void BRD_RS232_UART_Init(uint32_t baud){
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



















