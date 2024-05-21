
#include "stdint.h"
#include "stdio.h"
#include "Arduino.h"






typedef struct{
    const char *mpn;            // Sensor manufacturer product number.
    void (*request)(void);      // Sensor request data function pointer.
    char *(*response)(void);    // Sensor response data function pointer.
} rs232_sensor_desc_t;          // Sensor rs232 protocol descriptor.



void wf_pwron(void);
void wf_pwroff(void);

void led_on(void);
void led_off(void);
void led_toggle(void);

float batt_voltage(void);

void brd_hw_init(void (*btn_wakeup_handler)(void));

void sensor_pwron(void);
void sensor_pwroff(void);

void rs232_config_sensor(String json_sensor_pn);
char *rs232_reqdata(void);

