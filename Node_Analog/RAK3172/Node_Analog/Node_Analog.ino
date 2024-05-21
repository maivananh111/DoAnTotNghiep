
#include "Arduino_JSON.h"
#include "config.h"
#include "config_node.h"
#include "hw_analog.h"






bool viewmode = false;    
unsigned long last_action_tick = 0; 
uint8_t viewmode_flag = 0;
config_node_t cfg;


void usr_btn_handler(void);
void viewdata_timer_handler(void *param);
void viewled_timer_handler(void *param);

void startup_config(void);
void lorawan_begin(void);

char *create_send_data(void);
void get_sensor_data_and_send(bool lorawan_is_true);





void setup() {
    brd_hw_init(usr_btn_handler);

    startup_config();

    lorawan_begin();

    hw_analog_init(cfg.calib);

    last_action_tick = millis();
}

void loop() {
    if(millis() - last_action_tick > cfg.lorawan.period){
        last_action_tick = millis();
        get_sensor_data_and_send(true);
    }

    if(viewmode == true){ 
        if(viewmode_flag == 1){
            get_sensor_data_and_send(false);
            viewmode_flag = 0;
        }
    }
    else{
        delay(1000);
        api.system.sleep.all(cfg.lorawan.period-2000);
    }
}






void usr_btn_handler(void){
    delay(50);

    if(digitalRead(USR_BTN_PIN) == LOW){
        delay(10);
        viewmode = !viewmode;
        
        if(viewmode == true){
            Serial.println("Enter view mode.");
            wf_pwron(); 

            if (api.system.timer.create(VIEWDATA_TIMER, viewdata_timer_handler, RAK_TIMER_PERIODIC) != true)
                Serial.println("Creating timer VIEWDATA_TIMER failed.");
            if (api.system.timer.start(VIEWDATA_TIMER, DATAVIEW_PERIOD, (void *)VIEWDATA_TIMER) != true)
                Serial.println("Starting timer VIEWDATA_TIMER failed.");

            if (api.system.timer.create(VIEWLED_TIMER, viewled_timer_handler, RAK_TIMER_PERIODIC) != true)
                Serial.println("Creating timer VIEWLED_TIMER failed.");
            if (api.system.timer.start(VIEWLED_TIMER, 100, (void *)VIEWLED_TIMER) != true)
                Serial.println("Starting timer VIEWLED_TIMER failed.");
        }
        else{
            Serial.println("Enter normal mode.");
            wf_pwroff();

            if (api.system.timer.stop(VIEWDATA_TIMER) != true)
                Serial.println("Stopping timer VIEWDATA_TIMER failed.");
            if (api.system.timer.stop(VIEWLED_TIMER) != true)
                Serial.println("Stopping timer VIEWLED_TIMER failed.");
        }
    }
}

void viewdata_timer_handler(void *param){
    if((int)param == VIEWDATA_TIMER) {
        viewmode_flag = 1;
    }
}

void viewled_timer_handler(void *param){
    if((int)param == VIEWLED_TIMER) {
        led_toggle();
    }
}











void startup_config(void){
    led_on();

    wf_pwron();
    bool config_success = config_node(&cfg);
    delay(1000);
    wf_pwroff();
    if(config_success == false) api.system.reboot();
    led_off();

    delay(1000);
}

void lorawan_begin(void){
    lorawan_init(&cfg.lorawan);
    lorawan_join();
    lorawan_start();

    led_on();
    delay(500);
    led_off();
}

char *create_send_data(void){
    char *s = NULL;
    JSONVar root;
    JSONVar data;

    data["batt"] = (int)(batt_voltage() * 100.0);
    data["data"] = (int)(get_analog() * 100.0);
    root["data"] = data;

    String json_str = JSON.stringify(root);
    size_t len = json_str.length();
    s = (char *)malloc(len * sizeof(char) + 1);
    memset(s, 0, len+1);
    json_str.toCharArray(s, len+1);

    return s;
}

void get_sensor_data_and_send(bool lorawan_is_true){
    char *viewdata;

    led_on();
    viewdata = create_send_data();
    led_off();

    if(lorawan_is_true){
        lorawan_send((uint8_t *)viewdata, strlen(viewdata));
    }
    else{
        Serial.write(viewdata);
    }

    if(viewdata) free(viewdata);
}












