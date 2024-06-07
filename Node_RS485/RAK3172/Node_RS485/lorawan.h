
#include "stdint.h"
#include "RAKLorawan.h"



typedef struct {
    String name = "";
    uint8_t deveui[8] = {0};
    uint8_t appeui[8] = {0};
    uint8_t appkey[16] = {0};
    RAK_LORA_CLASS dev_class = RAK_LORA_CLASS_A;
    uint32_t period = 30000;
} lorawan_config_t;

typedef enum{
    LoRaWAN_IDLE_SLEEP = 0,
    LoRaWAN_IDLE_NORMAL = 1,
} lorawan_idle_mode_t;

typedef enum{
    LoRaWAN_JOIN_ERROR,
    LoRaWAN_SEND_ERROR,
    LoRaWAN_START,
    LoRaWAN_CONFIG,
    LoRaWAN_JOINING,
    LoRaWAN_JOINED,
    LoRaWAN_DEVSTATUS,
    LoRaWAN_IDLE,
    LoRaWAN_SEND,
    LoRaWAN_SENT
} lorawan_eventid_t;

typedef void (*lorawan_event_handler_fn_t)(lorawan_eventid_t, void *);


bool lorawan_init(lorawan_config_t *pconfig);
bool lorawan_register_event_handler(lorawan_event_handler_fn_t pfn, void *pparam);

void lorawan_set_idle_mode(lorawan_idle_mode_t idlemode);
void lorawan_set_send_param(uint8_t *pdata, uint8_t len, bool req_confirm);
int lorawan_get_join_state(void);

void lorawan_handler(void);

const char *lorawan_eventid_to_str(lorawan_eventid_t evt);
