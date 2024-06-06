
#include "lorawan.h"
#include "lorawan_config.h"
#include "Arduino_JSON.h"




static lorawan_state_t _state = LoRaWAN_START;
static lorawan_idle_mode_t _idlemode = LoRaWAN_IDLE_SLEEP;

static lorawan_config_t *_pconfig = NULL;
static lorawan_event_handler_fn_t _event_handler = NULL;
static void *_event_param = NULL;

static uint8_t _rejoin_cnt = 0;
static uint8_t _send_fail_cnt = 0;
static bool _sent = false;
static uint8_t _joined = 0;

static uint32_t _time_start_pre_period = 0, _time_start_pre_join = 0;
static uint32_t _time_to_sleep = 0, _time_to_start_sleep = 0, _time_slept = 0;

static uint8_t _devstatus_len = 0, _data_to_send_len = 0;
static uint8_t *_pdevstatus_to_send = NULL, *_pdata_to_send = NULL;
static bool _confirm = false;


static bool _lorawan_config(void);
static bool _lorawan_out_of_network(void);
static bool _lorawan_join(void);
static bool _lorawan_start(void);
static void _lorawan_send_devstatus(uint8_t *pdata);
static bool _lorawan_send_data(void);




static void joinCallback(int32_t status){
    Serial.printf("Join status: %d\r\n", status);
}

static void sendCallback(int32_t status){
    if (status != RAK_LORAMAC_STATUS_OK){
        Serial.printf("Sending failed, error: %d\r\n", status);
    }
    _sent = true;
}

static void recvCallback(SERVICE_LORA_RECEIVE_T *data){
    if (data->BufferSize > 0) {
        Serial.println("Something received!");
        for (int i = 0; i < data->BufferSize; i++) {
            Serial.printf("%x", data->Buffer[i]);
        }
        Serial.print("\r\n");
    }
}

static bool _lorawan_out_of_network(void){
    if(_send_fail_cnt >= LORAWAN_MAX_RESEND) 
        return true;

    return false;
}





static bool _lorawan_config(void){
    if(api.lorawan.nwm.get() != 1){
        api.lorawan.nwm.set();
        api.system.reboot();
    }

    if (!api.lorawan.appeui.set(_pconfig->appeui, 8)) {
        Serial.printf("LoRaWan - set application EUI is incorrect! \r\n");
        return false;
    }
    if (!api.lorawan.appkey.set(_pconfig->appkey, 16)) {
        Serial.printf("LoRaWan - set application key is incorrect! \r\n");
        return false;
    }
    if (!api.lorawan.deui.set(_pconfig->deveui, 8)) {
        Serial.printf("LoRaWan - set device EUI is incorrect! \r\n");
        return false;
    }
    if (!api.lorawan.band.set(LORAWAN_OTAA_BAND)) {
        Serial.printf("LoRaWan - set band is incorrect! \r\n");
        return false;
    }
    if (!api.lorawan.deviceClass.set(_pconfig->dev_class)) {
        Serial.printf("LoRaWan - set device class is incorrect! \r\n");
        return false;
    }
    if (!api.lorawan.njm.set(RAK_LORA_OTAA)){
        Serial.printf("LoRaWan - set network join mode is incorrect! \r\n");
        return false;
    }
    // if (!api.lorawan.txp.set(22)) {
    //     Serial.printf("LoRaWan - set tx power is incorrect! \r\n");
    //     return false;
    // }
    // if (!api.lorawan.dr.set(2)) {
    //     Serial.printf("LoRaWan - set tx power is incorrect! \r\n");
    //     return false;
    // }

    return true;
}

static bool _lorawan_join(void){
    if(api.lorawan.njs.get() == 0) {
        if(millis() - _time_start_pre_join > 10000){
            _time_start_pre_join = millis();
            Serial.print(".");
            api.lorawan.join();
            _rejoin_cnt++;
        }

        return false;
    }
    else {
        _joined = 1;
        _rejoin_cnt = 0;
    }

    return true;
}

static bool _lorawan_start(void){
    if (!api.lorawan.adr.set(true)) {
        Serial.printf("LoRaWan - set adaptive data rate is incorrect! \r\n");
        return false;
    }
    if (!api.lorawan.rety.set(5)) {
        Serial.printf("LoRaWan - set retry times is incorrect! \r\n");
        return false;
    }
    if (!api.lorawan.cfm.set(1)) {
        Serial.printf("LoRaWan - set confirm mode is incorrect! \r\n");
        return false;
    }
    if(!api.lorawan.registerJoinCallback(joinCallback)){
        Serial.printf("LoRaWan - register join callback failed! \r\n");
        return false;
    }
    if(!api.lorawan.registerSendCallback(sendCallback)){
        Serial.printf("LoRaWan - register send callback failed! \r\n");
        return false;
    }
    if(!api.lorawan.registerRecvCallback(recvCallback)){
        Serial.printf("LoRaWan - register receive callback failed! \r\n");
        return false;
    }

    return true;
}

static void _lorawan_send_devstatus(uint8_t *pdata){

}

static bool _lorawan_send_data(void){
    if(_pdata_to_send == NULL || _data_to_send_len == 0) return false;

    if(!api.lorawan.send(_data_to_send_len, _pdata_to_send, LORAWAN_DEFAULT_FPORT, _confirm, LORAWAN_MAX_RESEND)) {
        Serial.println("Send failed...");
        _send_fail_cnt++;
        _sent = false;

        return false;
    }
    else{
        do delay(1000);
        while(!_sent);
        
        _send_fail_cnt = 0;
    }

    return true; 
}





bool lorawan_init(lorawan_config_t *pconfig){
    if(pconfig) 
        _pconfig = pconfig;
    else{
        Serial.println("Invalid config parameter.");

        return false;
    }

    return true;
}

bool lorawan_register_event_handler(lorawan_event_handler_fn_t pfn, void *pparam){
    if(pfn) {
        _event_handler = pfn;
        _event_param = pparam;
    
        return true;
    }
    else
        Serial.println("Invalid event handler function.");
    
    return false;
}

void lorawan_set_idle_mode(lorawan_idle_mode_t idlemode){
    _idlemode = idlemode;
}

void lorawan_set_send_param(uint8_t *pdata, uint8_t len, bool req_confirm){
    if(pdata == NULL || len == NULL){
        Serial.println("Invalid data to send.");
        return;
    }
    _pdata_to_send = pdata;
    _data_to_send_len = len;
    _confirm = req_confirm;
}

int lorawan_get_join_state(void){
    return _joined;
}


void lorawan_handler(void){
    switch (_state){
        case LoRaWAN_START:
            if(_event_handler) _event_handler(_state, _event_param);
            _state = LoRaWAN_CONFIG;
            if(_event_handler) _event_handler(_state, _event_param);
        break;
        case LoRaWAN_CONFIG:
            if(_lorawan_config()) { 
                _state = LoRaWAN_JOINING;
                if(_event_handler) _event_handler(_state, _event_param);
                _time_start_pre_join = millis() + 9000;
            }
            else
                delay(1000);
        break;
        case LoRaWAN_JOINING:
            if(_lorawan_join()) { 
                _state = LoRaWAN_JOINED;
                if(_event_handler) _event_handler(_state, _event_param);
            }
            else{
                delay(1000);
                if(_rejoin_cnt == LORAWAN_MAX_REJOIN + 1){
                    _state = LoRaWAN_JOIN_ERROR;
                    _joined = 0;
                    if(_event_handler) 
                        _event_handler(_state, _event_param);
                    else
                        Serial.println("Join error event was unnhandler.");
                }
            }
        break;
        case LoRaWAN_JOINED:
            if(_lorawan_start()) { 
                _state = LoRaWAN_IDLE;
                if(_event_handler) _event_handler(_state, _event_param);
                _time_start_pre_period = millis();
            }
        break;
        case LoRaWAN_IDLE:{
            if(_idlemode == LoRaWAN_IDLE_SLEEP){
                delay(1000);
                _time_to_start_sleep = millis();
                _time_to_sleep = 0;

                if((_time_to_start_sleep - _time_start_pre_period) < _pconfig->period){
                    _time_to_sleep = _pconfig->period - (_time_to_start_sleep - _time_start_pre_period);

                    sleep:
                    if(_time_slept < _time_to_sleep){
                        api.system.sleep.all(_time_to_sleep - _time_slept);
                    
                        _time_slept += millis() - _time_to_start_sleep;
                        if(_time_slept < _time_to_sleep) 
                            goto sleep;
                        else {
                            _time_slept = 0;
                            _state = LoRaWAN_SEND;
                            if(_event_handler) _event_handler(_state, _event_param);
                        }
                    }
                }
            }
        }
        break;
        case LoRaWAN_SEND:
            if(millis() - _time_start_pre_period > _pconfig->period){
                _time_start_pre_period = millis();

                _lorawan_send_data();
                
                if(_lorawan_out_of_network()){
                    _state = LoRaWAN_SEND_ERROR;
                    _joined = 0;
                    if(!_event_handler){
                        Serial.println("Send error event was unnhandler.");
                        delay(1000);
                    }
                }
                else
                    _state = LoRaWAN_SENT;

                if(_event_handler) _event_handler(_state, _event_param);
            }
        break;
        case LoRaWAN_SENT:
            if(_pdata_to_send) free(_pdata_to_send);
            _state = LoRaWAN_IDLE;
            if(_event_handler) _event_handler(_state, _event_param);
        break;
        default:
        break;
    }
}











