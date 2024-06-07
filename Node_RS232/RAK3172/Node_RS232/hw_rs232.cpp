
#include "hw_rs232.h"
#include "config.h"
#include "Arduino_JSON.h"
#include "sensor.h"





rs232_sensor_desc_t ligo_sp_rs232 = {
    .mpn      = "LIGO SP-RS232",
    .request  = ligo_sp_rs232_request,
    .response = ligo_sp_rs232_response
};
rs232_sensor_desc_t incubator_ir_co2 = {
    .mpn      = "INCUBATOR IR-CO2",
    .request  = incubator_ir_co2_request,
    .response = incubator_ir_co2_response
};


uint8_t sensor_desc_set_size = 0;
rs232_sensor_desc_t *sensor_desc_set[RS232_SENSOR_SET_MAX_SIZE] = {0};
rs232_sensor_desc_t *rs232_desc = NULL;


/**
* Board power control.
*/
void power_high_performance(void){
    pinMode(POWER_SAVE_ENABLE, OUTPUT);
    digitalWrite(POWER_SAVE_ENABLE, HIGH);
}
void power_save(void){
    digitalWrite(POWER_SAVE_ENABLE, LOW);
}
/**
* RS232 power control.
*/
void sensor_pwron(void){
    pinMode(SENSOR_PWR_PIN, OUTPUT);
    pinMode(SENSOR_CIRCUIT_PWR_PIN, OUTPUT);
    digitalWrite(SENSOR_PWR_PIN, HIGH);
    digitalWrite(SENSOR_CIRCUIT_PWR_PIN, HIGH);
    delay(1000);
}
void sensor_pwroff(void){
    digitalWrite(SENSOR_PWR_PIN, LOW);
    digitalWrite(SENSOR_CIRCUIT_PWR_PIN, LOW);
    pinMode(SENSOR_PWR_PIN, INPUT);
    pinMode(SENSOR_CIRCUIT_PWR_PIN, INPUT);
}
/**
* WiFi module.
*/
void wf_pwron(void){
    pinMode(WF_PWR_PIN, OUTPUT);
    digitalWrite(WF_PWR_PIN, LOW);
    delay(1000);
}
void wf_pwroff(void){
    digitalWrite(WF_PWR_PIN, HIGH);
    pinMode(WF_PWR_PIN, INPUT);
}
/**
* LED indicator.
*/
void led_on(void){
    digitalWrite(LED_ACT_PIN, LOW);
}
void led_off(void){
    digitalWrite(LED_ACT_PIN, HIGH);
}
void led_toggle(void){
    digitalWrite(LED_ACT_PIN, !digitalRead(LED_ACT_PIN));
}


float batt_voltage(void){
    uint32_t x = 0;
    
    for(int i=0; i<ANALOG_SAMPLE_NUMBER; i++)
        x += analogRead(BAT_SENSE_PIN);
    x /= ANALOG_SAMPLE_NUMBER;

    return 0.0054 * (float)x - 0.02;
}


void brd_hw_init(void (*btn_wakeup_handler)(void)){   
    Serial.begin(115200, RAK_AT_MODE);
    Serial.println("Startup");

    pinMode(LED_ACT_PIN, OUTPUT);
    pinMode(USR_BTN_PIN, INPUT);

    attachInterrupt(USR_BTN_PIN, btn_wakeup_handler, FALLING);
    analogReadResolution(12);

    Serial1.begin(9600, RAK_CUSTOM_MODE);
    sensor_desc_set[0] = &ligo_sp_rs232;
    sensor_desc_set[1] = &incubator_ir_co2;
}

char *rs232_reqdata(void){
    sensor_pwron();
    rs232_desc->request();
    char *s = rs232_desc->response();
    if(s == NULL) Serial.println("Sensor error.");
    sensor_pwroff();

    return s;
}

void rs232_config_sensor(String json_sensor_pn){
    for(uint8_t i=0; i<RS232_SENSOR_SET_MAX_SIZE; i++){
        rs232_sensor_desc_t *desc = sensor_desc_set[i];
        if(desc == NULL) continue;

        if(String(desc->mpn) == json_sensor_pn){
            Serial.printf("Selected sensor product number %s\r\n", desc->mpn);
            rs232_desc = desc;
            break;
        }
    }
}





















