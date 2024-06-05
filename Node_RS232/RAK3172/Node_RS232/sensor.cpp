
#include "sensor.h"



uint8_t ligo_sp_rs232_checksum(uint8_t *data, uint8_t size){
    uint8_t crc = 0;

    for(uint8_t j=0; j<size; j++){
        uint8_t i = data[j] ^ crc;
        crc = 0;
        
        if(i & 0x01) crc ^= 0x5e;
        if(i & 0x02) crc ^= 0xbc;
        if(i & 0x04) crc ^= 0x61;
        if(i & 0x08) crc ^= 0xc2;
        if(i & 0x10) crc ^= 0x9d;
        if(i & 0x20) crc ^= 0x23;
        if(i & 0x40) crc ^= 0x46;
        if(i & 0x80) crc ^= 0x8c;
    }

    return crc;
}

void ligo_sp_rs232_request(void){
    uint8_t buf[4] = {0};
    buf[0] = 0x31;
    buf[1] = 0x02;
    buf[2] = 0x06;
    buf[3] = ligo_sp_rs232_checksum(buf, 3);

    Serial1.write(buf, 4); 
}

char *ligo_sp_rs232_response(void){
    uint32_t tick = millis();
    while(!Serial1.available()){
        if(millis() - tick > 1000) return NULL;
        delay(100);
    }

    uint8_t buf[12];
    uint8_t i = 0;
    while(Serial1.available())
        buf[i++] = (uint8_t)Serial1.read();

    char *resp = (char *)malloc(65);
    memset(resp, 0, 65);
    sprintf(resp, "{\"temperature\":%d,\"relative_level\":%d}", (int)(buf[3]*100), (int)((buf[4]<<8 | buf[5])*100));

    return resp;
}