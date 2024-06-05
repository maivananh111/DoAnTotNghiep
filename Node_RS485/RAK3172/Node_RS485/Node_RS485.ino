
#include "Arduino_JSON.h"
#include "config.h"
#include "config_node.h"
#include "hw_modbus.h"





#define SEND_VIEW_DATA    false
#define SEND_LORAWAN_DATA true


bool viewmode = false;    
unsigned long last_send_time = 0; 
unsigned long goto_sleep_time = 0; 
unsigned long sleeped_time = 0; 
uint8_t viewmode_flag = 0;
config_node_t cfg;

void usr_btn_handler(void);
void viewdata_timer_handler(void *param);
void viewled_timer_handler(void *param);

void startup_config(void);
void lorawan_begin(void);

char *create_send_data(void);
void get_sensor_data_and_send(bool lorawan_is_true);



/*
* Setup function.
*/
void setup(void) {
    brd_hw_init(usr_btn_handler);

    startup_config();

    lorawan_begin();

    mb_hw_init();
    mb_config_desc(cfg.mbdesc);

    last_send_time = millis();
}

/*
* Loop program.
*/

void loop(void) {
    if(lorawan_out_of_network()) api.system.reboot();
    
    if(millis() - last_send_time > cfg.lorawan.period){
        last_send_time = millis();
        get_sensor_data_and_send(SEND_LORAWAN_DATA);
    }
    if(viewmode == false){   
        delay(1000);

        goto_sleep_time = millis();
        sleeped_time = 0;

        if((goto_sleep_time - last_send_time) < cfg.lorawan.period){
            uint32_t sleep_period = cfg.lorawan.period - (goto_sleep_time - last_send_time);

            sleep:
            if(sleep_period > sleeped_time){
                api.system.sleep.all(sleep_period - sleeped_time);
            
                sleeped_time += millis() - goto_sleep_time;
                if(sleeped_time < sleep_period) 
                    goto sleep;
                else 
                    sleeped_time = 0;
            }
        }
    }

    if(viewmode == true){ 
        if(viewmode_flag == 1){
            get_sensor_data_and_send(SEND_VIEW_DATA);
            viewmode_flag = 0;
        }
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

            api.system.timer.create(VIEWDATA_TIMER, viewdata_timer_handler, RAK_TIMER_PERIODIC);
            api.system.timer.start(VIEWDATA_TIMER, DATAVIEW_PERIOD, (void *)VIEWDATA_TIMER);
            api.system.timer.create(VIEWLED_TIMER, viewled_timer_handler, RAK_TIMER_PERIODIC);
            api.system.timer.start(VIEWLED_TIMER, 100, (void *)VIEWLED_TIMER);
        }
        else{
            Serial.println("Enter normal mode.");
            wf_pwroff();

            api.system.timer.stop(VIEWDATA_TIMER);
            api.system.timer.stop(VIEWLED_TIMER);
        }
    }
}

void viewdata_timer_handler(void *param){
    if((int)param == VIEWDATA_TIMER)
        viewmode_flag = 1;
}

void viewled_timer_handler(void *param){
    if((int)param == VIEWLED_TIMER) 
        led_toggle();
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
    char *s;
    JSONVar root;
    JSONVar data;

    mb_reqdata();
    for(uint8_t i=0; i<mb_set_size(); i++){
        if(mb_select_slave(i)->res == 1)
            data[(const char *)mb_select_slave(i)->desc] = mb_select_slave(i)->rdata[0];
    }
    mb_release_data();

    data["batt"] = (int)(batt_voltage() * 100);
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

    if(lorawan_is_true)
        lorawan_send((uint8_t *)viewdata, strlen(viewdata)); 
    else
        Serial.write(viewdata);

    if(viewdata) free(viewdata);
}













