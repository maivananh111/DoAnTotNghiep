
#include "hw_analog.h"
#include "config.h"
#include "Arduino_JSON.h"




static double analog_param_a = 0.0, analog_param_b = 0.0;
static char desc[50] = {0};  // Data descriptor string.

void mb_txmode(void);
void mb_rxmode(void);

/**
* Board power control.
*/
void power_high_performance(void){
    digitalWrite(POWER_SAVE_ENABLE, LOW);
}
void power_save(void){
    digitalWrite(POWER_SAVE_ENABLE, HIGH);
}
/**
* Sensor power control.
*/
void sensor_pwron(void){
    pinMode(SENSOR_PWR_PIN, OUTPUT);
    pinMode(SENSOR_CIRCUIT_PWR_PIN, OUTPUT);
    digitalWrite(SENSOR_PWR_PIN, HIGH);
    digitalWrite(SENSOR_CIRCUIT_PWR_PIN, HIGH);
    delay(1000);
}
void sensor_pwroff(void){
    digitalWrite(SENSOR_CIRCUIT_PWR_PIN, LOW);
    digitalWrite(SENSOR_PWR_PIN, LOW);
    pinMode(SENSOR_CIRCUIT_PWR_PIN, INPUT);
    pinMode(SENSOR_PWR_PIN, INPUT);
}
/**
* WiFi module power control.
*/
void wf_pwron(void){
    delay(500);
    pinMode(WF_PWR_PIN, OUTPUT);
    digitalWrite(WF_PWR_PIN, LOW);
    delay(2000);
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

float analog_get(void){
    uint32_t x = 0;
    
    sensor_pwron();

    for(int i=0; i<ANALOG_SAMPLE_NUMBER; i++)
        x += analogRead(SENSOR_ANALOG_PIN);
    x /= ANALOG_SAMPLE_NUMBER;
    
    float ana = (float)x * ANALOG_CALIB_PARAM_A + ANALOG_CALIB_PARAM_B;
    // Serial.printf("ADC = %d, VOL = %.02f\r\n", x, ana);
    float ret = ana * (float)analog_param_a + (float)analog_param_b;

    sensor_pwroff();

    return ret;
}


void brd_hw_init(void (*btn_wakeup_handler)(void)){   
    Serial.begin(115200, RAK_AT_MODE);
    Serial.println("Startup");

    pinMode(LED_ACT_PIN, OUTPUT);
    pinMode(USR_BTN_PIN, INPUT);
    attachInterrupt(USR_BTN_PIN, btn_wakeup_handler, FALLING);

    analogReadResolution(12);
}

void hw_analog_config(String json_calib){
    double max, min;
    double high = ANALOG_VAL_HIGH, low = ANALOG_VAL_LOW;
    JSONVar root = JSON.parse(json_calib);

    if (JSON.typeof(root) == "undefined") {
        Serial.println("Failed to parse JSON");
        return;
    }

    if (!root.hasOwnProperty("max") || !root.hasOwnProperty("min") || !root.hasOwnProperty("desc")
#ifdef NODE_0_10V
         || !root.hasOwnProperty("vl")  || !root.hasOwnProperty("vh")
#endif
    ) return;

    max = (double)root["max"];
    min = (double)root["min"];
#ifdef NODE_0_10V
    high = (double)root["vh"];
    low  = (double)root["vl"];
#endif
    memcpy(desc, (const char*)root["desc"], strlen((const char*)root["desc"]));

    analog_param_a = (max - min) / (high - low);
    analog_param_b = min - low * analog_param_a;
}

char *analog_get_desc(void){
    return desc;
}



















