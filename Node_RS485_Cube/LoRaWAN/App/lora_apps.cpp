

#include <lora_apps.h>
#include "platform.h"
#include "sys_app.h"
#include "cmsis_os.h"
#include "stm32_timer.h"
#include "utilities_def.h"
#include "app_version.h"
#include "lorawan_version.h"
#include "subghz_phy_version.h"
#include "lora_info.h"
#include "LmHandler.h"
#include "adc_if.h"
#include "CayenneLpp.h"
#include "sys_sensors.h"
#include "flash_if.h"


#include "stdlib.h"
#include "string.h"
#include "log/log.h"
#include "board_modbus/board_modbus.h"
#include "cJSON/cJSON.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "event_groups.h"

#include "radio.h"
#include "usart.h"
#include "lptim.h"
#include "dma.h"



/* USER CODE BEGIN PTD */
static const char *TAG = "WAN";
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
#define DATAVIEW_PERIOD							3000
#define E_BIT_CHMOD (1<<0)
/* USER CODE BEGIN PD */
typedef enum{
	MODE_NORMAL = 0,
	MODE_VIEW = 1,
} working_mode_t;

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
extern UART_HandleTypeDef 		huart1;
extern UART_HandleTypeDef 		hlpuart1;
extern TIM_HandleTypeDef   		htim1;
extern LPTIM_HandleTypeDef 		hlptim1;
extern const struct Radio_s 	Radio;

static working_mode_t mode = MODE_NORMAL, lmode = MODE_NORMAL;
static uint8_t joined = 0;
static uint16_t join_fail = 0;
static uint32_t fcnt = 0;
static SemaphoreHandle_t s_dataview;
static SemaphoreHandle_t s_lora_sent;
static EventGroupHandle_t e_chmod;
static UTIL_TIMER_Object_t sleep_wakeup_timer;
UTIL_TIMER_Time_t TxPeriodicity = APP_TX_DUTYCYCLE;
/* USER CODE END PM */

/* Private function prototypes -----------------------------------------------*/
static void OnWakeUp(void *context);
static void SendTxData(void);
static void OnJoinRequest(LmHandlerJoinParams_t *joinParams);
static void OnTxData(LmHandlerTxParams_t *params);
static void OnRxData(LmHandlerAppData_t *appData, LmHandlerRxParams_t *params);
static void OnBeaconStatusChange(LmHandlerBeaconParams_t *params);
static void OnSysTimeUpdate(void);
static void OnClassChange(DeviceClass_t deviceClass);
static void OnMacProcessNotify(void);
static void OnTxPeriodicityChanged(uint32_t periodicity);
static void OnTxFrameCtrlChanged(LmHandlerMsgTypes_t isTxConfirmed);
static void OnPingSlotPeriodicityChanged(uint8_t pingSlotPeriodicity);
static void OnSystemReset(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private variables ---------------------------------------------------------*/
static ActivationType_t ActivationType = LORAWAN_DEFAULT_ACTIVATION_TYPE;
static bool ForceRejoin = LORAWAN_FORCE_REJOIN_AT_BOOT;
static LmHandlerCallbacks_t LmHandlerCallbacks ={
	.GetBatteryLevel =              GetBatteryLevel,
	.GetTemperature =               GetTemperatureLevel,
	.GetUniqueId =                  GetUniqueId,
	.GetDevAddr =                   GetDevAddr,
	.OnRestoreContextRequest =      NULL,
	.OnStoreContextRequest =        NULL,
	.OnMacProcess =                 OnMacProcessNotify,
	.OnNvmDataChange =              NULL,
	.OnJoinRequest =                OnJoinRequest,
	.OnTxData =                     OnTxData,
	.OnRxData =                     OnRxData,
	.OnClassChange =                OnClassChange,
	.OnBeaconStatusChange =         OnBeaconStatusChange,
	.OnSysTimeUpdate =              OnSysTimeUpdate,
	.OnTxPeriodicityChanged =       OnTxPeriodicityChanged,
	.OnTxFrameCtrlChanged =         OnTxFrameCtrlChanged,
	.OnPingSlotPeriodicityChanged = OnPingSlotPeriodicityChanged,
	.OnSystemReset =                OnSystemReset,
};
LmHandlerParams_t LmHandlerParams ={
	.ActiveRegion =             ACTIVE_REGION,
	.DefaultClass =             LORAWAN_DEFAULT_CLASS,
	.AdrEnable =                LORAWAN_ADR_STATE,
	.IsTxConfirmed =            LORAWAN_DEFAULT_CONFIRMED_MSG_STATE,
	.TxDatarate =               LORAWAN_DEFAULT_DATA_RATE,
	.TxPower =                  LORAWAN_DEFAULT_TX_POWER,
	.PingSlotPeriodicity =      LORAWAN_DEFAULT_PING_SLOT_PERIODICITY,
	.RxBCTimeout =              LORAWAN_DEFAULT_CLASS_B_C_RESP_TIMEOUT
};

osThreadId_t Thd_LoraSendProcessId;
const osThreadAttr_t Thd_LoraSendProcess_attr ={
	.name = CFG_APP_LORA_PROCESS_NAME,
	.attr_bits = CFG_APP_LORA_PROCESS_ATTR_BITS,
	.cb_mem = CFG_APP_LORA_PROCESS_CB_MEM,
	.cb_size = CFG_APP_LORA_PROCESS_CB_SIZE,
	.stack_mem = CFG_APP_LORA_PROCESS_STACK_MEM,
	.stack_size = CFG_APP_LORA_PROCESS_STACK_SIZE,
	.priority = CFG_APP_LORA_PROCESS_PRIORITY,
};
static void Thd_LoraSendProcess(void *argument);

osThreadId_t Thd_LmHandlerProcessId;
const osThreadAttr_t Thd_LmHandlerProcess_attr ={
	.name = CFG_LM_HANDLER_PROCESS_NAME,
	.attr_bits = CFG_LM_HANDLER_PROCESS_ATTR_BITS,
	.cb_mem = CFG_LM_HANDLER_PROCESS_CB_MEM,
	.cb_size = CFG_LM_HANDLER_PROCESS_CB_SIZE,
	.stack_mem = CFG_LM_HANDLER_PROCESS_STACK_MEM,
	.stack_size = CFG_LM_HANDLER_PROCESS_STACK_SIZE,
	.priority = CFG_LM_HANDLER_PROCESS_PRIORITY,
};
static void Thd_LmHandlerProcess(void *argument);


static void create_send_data(char *str, uint16_t *len, bool inc_stat);
static void Task_DataView(void *param);
static void Task_Change_Mode(void *param);
extern uint32_t dev_get_free_heap_size(void);


void enter_sleep_mode(uint32_t ms){
	UTIL_TIMER_Create(&sleep_wakeup_timer, ms, UTIL_TIMER_ONESHOT, OnWakeUp, NULL);
	UTIL_TIMER_Start(&sleep_wakeup_timer);

    if(Radio.GetStatus() == RF_IDLE)
        Radio.Sleep();

    HAL_UART_DeInit(&huart1);
    HAL_UART_DeInit(&hlpuart1);
    HAL_DMA_DeInit((&hlpuart1)->hdmarx);

	brd_wf_pwroff();
	brd_sensor_pwroff();
	brd_led_off();

  	HAL_SuspendTick();
  	HAL_TIM_Base_DeInit(&htim1);

	LL_PWR_ClearFlag_C1STOP_C1STB();
	HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);

	extern HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority);
	HAL_InitTick(15);
	HAL_ResumeTick();
	extern void SystemClock_Config(void);
	SystemClock_Config();

	MX_DMA_Init();
	MX_LPUART1_UART_Init();
	MX_USART1_UART_Init();
}


void LoRaWAN_Init(void){
	Thd_LmHandlerProcessId = osThreadNew(Thd_LmHandlerProcess, NULL, &Thd_LmHandlerProcess_attr);
	if (Thd_LmHandlerProcessId == NULL)
		Error_Handler();
	Thd_LoraSendProcessId = osThreadNew(Thd_LoraSendProcess, NULL, &Thd_LoraSendProcess_attr);
	if (Thd_LoraSendProcessId == NULL)
		Error_Handler();

	s_dataview = xSemaphoreCreateBinary();
	s_lora_sent = xSemaphoreCreateBinary();
	e_chmod = xEventGroupCreate();
    if (xTaskCreate(Task_DataView, "Task_DataView", 1024, NULL, 3, NULL) != pdPASS) {
    	LOG_ERROR(TAG, "Start view data task fail.");
    }
    if (xTaskCreate(Task_Change_Mode, "Task_Change_Mode", 512, NULL, osPriorityISR, NULL) != pdPASS) {
    	LOG_ERROR(TAG, "Start change mode task fail.");
    }
    board_modbus_init();


	LoraInfo_Init();
	LmHandlerInit(&LmHandlerCallbacks, APP_VERSION);
	LmHandlerConfigure(&LmHandlerParams);

	LmHandlerJoin(ActivationType, ForceRejoin);
	HAL_LPTIM_Counter_Start_IT(&hlptim1, 60000);
	LOG_EVENT(TAG, "JOINING.");
}

static void Thd_LmHandlerProcess(void *argument){
	UNUSED(argument);
	while(1){
		osThreadFlagsWait(1, osFlagsWaitAny, osWaitForever);
		LmHandlerProcess();
	}
}

static void Thd_LoraSendProcess(void *argument){
	UNUSED(argument);
	while(1){
		SendTxData();

		if(xSemaphoreTake(s_lora_sent, portMAX_DELAY) == pdPASS){
			if(mode == MODE_NORMAL) {
				LOG_DEBUG(TAG, "SLEEP.");
				enter_sleep_mode((joined)? TxPeriodicity : APP_TX_DUTYCYCLE);
				LOG_DEBUG(TAG, "WAKEUP.");
			}
			else
				vTaskDelay((joined)? TxPeriodicity : APP_TX_DUTYCYCLE);
		}
	}
}

static void Task_DataView(void *param){
	while(1){
		if(xSemaphoreTake(s_dataview, 10) == pdPASS){
			LOG_MEM(TAG, "Free heap = %lu.", dev_get_free_heap_size());

			uint8_t json[300] = {0};
			uint16_t len = 0;
			create_send_data((char *)json, &len, true);
			HAL_UART_Transmit(&hlpuart1, json, len, 1000);
			xSemaphoreGive(s_dataview);
		}
		vTaskDelay(DATAVIEW_PERIOD);
	}
}

void Task_Change_Mode(void *param){
	while(1){
		if(xEventGroupWaitBits(e_chmod, E_BIT_CHMOD, pdTRUE, pdFAIL, portMAX_DELAY) == pdPASS){
			mode = (working_mode_t)!mode;

			if(mode == MODE_VIEW && lmode == MODE_NORMAL){
			    LOG_WARN(TAG, "CHANGE TO VIEW MODE");
				brd_wf_pwron(false);

			    HAL_LPTIM_Counter_Start_IT(&hlptim1, 20000);

			    xSemaphoreGive(s_dataview);
			}
			else if(mode == MODE_NORMAL && lmode == MODE_VIEW){
				LOG_WARN(TAG, "CHANGE TO NORMAL MODE");
				brd_wf_pwroff();

				HAL_LPTIM_Counter_Stop_IT(&hlptim1);
				if(joined) brd_led_off();

				xSemaphoreTake(s_dataview, 10);

				if(joined){
					LOG_DEBUG(TAG, "SLEEP.");
					enter_sleep_mode((joined)? TxPeriodicity : APP_TX_DUTYCYCLE);
					LOG_DEBUG(TAG, "WAKEUP.");
				}
			}

			lmode = mode;
		}
	}
}




static void OnRxData(LmHandlerAppData_t *appData, LmHandlerRxParams_t *params){
	if(appData->BufferSize > 0)
		LOG_WARN(TAG, "OnRxData: %s, RSSI: %d", appData->Buffer, params->Rssi);
}

static void SendTxData(void){
	LmHandlerErrorStatus_t status = LORAMAC_HANDLER_ERROR;
	UTIL_TIMER_Time_t nextTxIn = 0;
	LmHandlerAppData_t appdata;

	if(joined) {
		char json[300] = {0};
		create_send_data(json, (uint16_t *)&appdata.BufferSize, false);

		appdata.Port = LORAWAN_USER_APP_PORT;
		appdata.Buffer = (uint8_t *)json;
	}

	status = LmHandlerSend(&appdata, LmHandlerParams.IsTxConfirmed, true);
//	APP_LOG(TS_ON, VLEVEL_L, "status: %d\r\n", status);
	if (status == LORAMAC_HANDLER_SUCCESS) {
		LOG_RET(TAG, "SEND UPLINK, fcnt = %lu.", fcnt);
		brd_led_on();
		HAL_Delay(200);
		brd_led_off();
	}
	else if (status == LORAMAC_HANDLER_DUTYCYCLE_RESTRICTED) {
       if(nextTxIn > 0)
           nextTxIn = LmHandlerGetDutyCycleWaitTime();
       LOG_EVENT(TAG, "Next Tx in %lums", nextTxIn);
	}
}

static void OnWakeUp(void *context){
	UNUSED(context);
}

static void OnTxData(LmHandlerTxParams_t *params){
	if(params->Status == LORAMAC_EVENT_INFO_STATUS_OK){
		LOG_INFO(TAG, "TX COMPLETE, fcnt = %lu.", (joined)? fcnt++ : fcnt);
		if(joined) {
			for(uint8_t i=0; i<4; i++){
				brd_led_toggle();
				HAL_Delay(100);
			}
		}
	}
	xSemaphoreGive(s_lora_sent);

}


static void OnJoinRequest(LmHandlerJoinParams_t *joinParams){
	if(joinParams->Status == LORAMAC_HANDLER_SUCCESS){
		LOG_EVENT(TAG, "JOINED.");
		joined = 1;
		join_fail = 0;
		fcnt = 1;

		HAL_LPTIM_Counter_Stop_IT(&hlptim1);
		brd_led_off();
		for(uint8_t i=0; i<5; i++){
			brd_led_toggle();
			HAL_Delay(500);
		}

		if(mode == MODE_VIEW)
			HAL_LPTIM_Counter_Start_IT(&hlptim1, 20000);
	}
	else{
		LOG_ERROR(TAG, "JOIN ERROR, TRYING AGAIN.");
		joined = 0;
		join_fail++;
		if(join_fail >= 10 && mode == MODE_NORMAL) __NVIC_SystemReset();
	}
	xSemaphoreGive(s_lora_sent);
}

static void OnMacProcessNotify(void){
	osThreadFlagsSet(Thd_LmHandlerProcessId, 1);
}

static void OnTxPeriodicityChanged(uint32_t periodicity){
	TxPeriodicity = periodicity;

	if (TxPeriodicity == 0){
		TxPeriodicity = APP_TX_DUTYCYCLE;
	}
}

static void OnTxFrameCtrlChanged(LmHandlerMsgTypes_t isTxConfirmed){
	LmHandlerParams.IsTxConfirmed = isTxConfirmed;
}

static void OnPingSlotPeriodicityChanged(uint8_t pingSlotPeriodicity){
	LmHandlerParams.PingSlotPeriodicity = pingSlotPeriodicity;
}

static void OnSystemReset(void){
	if ((LORAMAC_HANDLER_SUCCESS == LmHandlerHalt()) && (LmHandlerJoinStatus() == LORAMAC_HANDLER_SET)){
		__NVIC_SystemReset();
	}
}


static void OnBeaconStatusChange(LmHandlerBeaconParams_t *params){}
static void OnSysTimeUpdate(void){}
static void OnClassChange(DeviceClass_t deviceClass){}





static void create_send_data(char *str, uint16_t *len, bool inc_stat){
	uint16_t idx = 0;
	char tmp[60] = {0};
	uint8_t tmp_len = 0;

	memcpy(str, "{\"data\":{", idx += strlen("{\"data\":{"));

    mb_reqdata();
    for(uint8_t i=0; i<mb_set_size(); i++){
        mb_desc_t *desc = mb_select_slave(i);
        if(desc->res == 1){
        	sprintf(tmp, "\"%s\":%d,", desc->desc, desc->rdata[0] / (desc->div / 100));
        	tmp_len = strlen(tmp);
        	memcpy((char *)(str + idx), tmp, tmp_len);
        	idx += tmp_len;
        }
    }
    mb_release_data();

	sprintf(tmp, "\"batt\":%d,", (int)(brd_batt_voltage() * 100));
	tmp_len = strlen(tmp);
	memcpy((char *)(str + idx), tmp, tmp_len);
	idx += tmp_len;

	sprintf(tmp, "\"stat\":%d,", joined);
	tmp_len = strlen(tmp);
	memcpy((char *)(str + idx), tmp, tmp_len);
	idx += tmp_len;

    memcpy((char *)(str + (--idx)), "}}", 2);
    *len = idx + 2;

    return;
}

extern "C" void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
	if(GPIO_Pin == CHMOD_BTN_Pin){
		for(uint32_t i=0; i<40000; i++) __NOP();

		if(HAL_GPIO_ReadPin(CHMOD_BTN_GPIO_Port, CHMOD_BTN_Pin) == GPIO_PIN_RESET){
			BaseType_t pxHigherPriorityTaskWoken;
			xEventGroupSetBitsFromISR(e_chmod, E_BIT_CHMOD, &pxHigherPriorityTaskWoken);
			portYIELD_FROM_ISR(pxHigherPriorityTaskWoken);
		}
	}
}

extern "C" void HAL_LPTIM_UpdateEventCallback(LPTIM_HandleTypeDef *hlptim){
	brd_led_toggle();
}

