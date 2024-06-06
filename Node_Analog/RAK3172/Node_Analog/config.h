

#include "stdint.h"
#include "Arduino.h"


#define NODE_4_20MA
// #define NODE_0_10V



#define POWER_SAVE_ENABLE      PA11
#define SENSOR_PWR_PIN         PA7
#define SENSOR_CIRCUIT_PWR_PIN PA6
#define SENSOR_ANALOG_PIN      PB4
#define WF_PWR_PIN             PA15
#define LED_ACT_PIN            PA4
#define USR_BTN_PIN            PA9
#define BAT_SENSE_PIN          PB2

#define VIEWDATA_PERIOD        3000
#define VIEWDATA_TIMER         RAK_TIMER_1
#define VIEWLED_TIMER          RAK_TIMER_2


#define ANALOG_SAMPLE_NUMBER 10
#ifdef NODE_4_20MA
#define ANALOG_CALIB_PARAM_A (0.00610282)
#define ANALOG_CALIB_PARAM_B (3.454295191)
#define ANALOG_VAL_LOW       (4.0)
#define ANALOG_VAL_HIGH      (20.0)
#endif
#ifdef NODE_0_10V
#define ANALOG_CALIB_PARAM_A (0.0031) 
#define ANALOG_CALIB_PARAM_B (0.0118)
#define ANALOG_VAL_LOW       (0.0)
#define ANALOG_VAL_HIGH      (10.0)
#endif






































