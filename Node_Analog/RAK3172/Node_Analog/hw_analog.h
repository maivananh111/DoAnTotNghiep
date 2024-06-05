
#include "stdint.h"
#include "stdio.h"
#include "Arduino.h"





void wf_pwron(void);
void wf_pwroff(void);

void led_on(void);
void led_off(void);
void led_toggle(void);

void sensor_pwron(void);
void sensor_pwroff(void);

float batt_voltage(void);

void brd_hw_init(void (*btn_wakeup_handler)(void));

void hw_analog_init(String json_calib);
float get_analog(void);

void power_high_performance(void);
void power_save(void);


// {
//     ....., 
//     "calib":{
//         "max": 100.0,
//         "min": 25.5,
//     }
// }