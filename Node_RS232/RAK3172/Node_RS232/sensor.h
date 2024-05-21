
#include "stdint.h"
#include "Arduino.h"


uint8_t ligo_sp_rs232_checksum(uint8_t *data, uint8_t size);
void ligo_sp_rs232_request(void);
char *ligo_sp_rs232_response(void);



