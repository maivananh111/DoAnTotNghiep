

#include "Arduino_JSON.h"
#include "config.h"
#include "config_node.h"
#include "hw_analog.h"





bool viewmode = false;    
uint8_t viewmode_flag = 0;
config_node_t cfg;


void usr_btn_handler(void);
void viewdata_timer_handler(void *param);
void viewled_timer_handler(void *param);
void lorawan_event_handler(lorawan_eventid_t, void *);

void read_nodeconfig(void);

char *create_send_data(bool inc_stat);



/*
* Setup function.
*/
void setup(void) {
    brd_hw_init(usr_btn_handler);
    read_nodeconfig();

    hw_analog_config(cfg.calib);

    lorawan_register_event_handler(lorawan_event_handler, NULL);
}

/*
* Loop program.
*/

void loop(void) {
    lorawan_handler();

    if(viewmode == true && viewmode_flag == 1){ 
        char *viewdata = create_send_data(true);

        Serial.write(viewdata);
        if(viewdata) free(viewdata);

        viewmode_flag = 0;
    }
}


void lorawan_event_handler(lorawan_eventid_t event, void *param){
    char *senddata;

    Serial.printf("\r\nLORAWANEVENT - %s", lorawan_eventid_to_str(event));
    switch(event){
        case LoRaWAN_JOIN_ERROR:
        case LoRaWAN_SEND_ERROR:
            if(viewmode == false) api.system.reboot();
        break;

        case LoRaWAN_START:
            lorawan_init(&cfg.lorawan);
            lorawan_set_idle_mode(LoRaWAN_IDLE_SLEEP);
        break;
        case LoRaWAN_JOINED:
            led_on();
            delay(1000);
            led_off();
        break;
        case LoRaWAN_SEND:{
            led_on();
            senddata = create_send_data(false);
            lorawan_set_send_param((uint8_t *)senddata, strlen(senddata), true);
            led_off();
        }
        break;
        default:
        break;
    }
}





void usr_btn_handler(void){
    delay(50);

    if(digitalRead(USR_BTN_PIN) == LOW){
        delay(10);
        viewmode = !viewmode;
        lorawan_set_idle_mode((lorawan_idle_mode_t)viewmode);
        
        if(viewmode == true){
            Serial.println("WORKINGMODE - LORAWAN AND VIEW/CONFIGURE.");
            wf_pwron(); 

            api.system.timer.create(VIEWDATA_TIMER, viewdata_timer_handler, RAK_TIMER_PERIODIC);
            api.system.timer.start(VIEWDATA_TIMER, VIEWDATA_PERIOD, (void *)VIEWDATA_TIMER);
            api.system.timer.create(VIEWLED_TIMER, viewled_timer_handler, RAK_TIMER_PERIODIC);
            if(!api.system.timer.start(VIEWLED_TIMER, 100, (void *)VIEWLED_TIMER)){
                Serial.println("Fail to start timer VIEWLED_TIMER.");
            }
        }
        else{
            Serial.println("WORKINGMODE - LORAWAN ONLY.");
            wf_pwroff();
            led_off();

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

void read_nodeconfig(void){
    led_on();

    wf_pwron();
    bool config_success = config_node(&cfg);
    wf_pwroff();

    if(config_success == false) api.system.reboot();
    led_off();
    delay(500);
}


char *create_send_data(bool inc_stat){
    char *s = NULL;
    JSONVar root;
    JSONVar data;

    data["batt"] = (int)(batt_voltage() * 100.0);
    data[analog_get_desc()] = (int)(analog_get() * 100.0);
    if(inc_stat) 
        data["stat"] = lorawan_get_join_state();
    root["data"] = data;

    String json_str = JSON.stringify(root);
    size_t len = json_str.length();
    s = (char *)malloc(len * sizeof(char) + 1);
    memset(s, 0, len+1);
    json_str.toCharArray(s, len+1);

    return s;
}












