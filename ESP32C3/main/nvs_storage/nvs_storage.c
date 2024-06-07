/*
 * nvs_storage.c
 *
 *  Created on: May 14, 2024
 *      Author: anh
 */

#include "nvs_storage.h"

#include "nvs_flash.h"
#include "nvs.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_mac.h"

#include "cJSON.h"
#include "string.h"

#include "json.h"

#include "configure.h"

extern nvs_handle_t nvs;
extern nvs_data_t nvs_stored_data;

static nvs_data_t init_data = {
	.class = "A",
	.deveui = "0102030405060708",
	.appeui = "1020304050607080",
	.appkey = "0A0B0C0D0E0F1A1B1C1D1E1F10000001",
	.period = 3000,
#ifdef RS485
    .mb_desc = "{"
               "\"SHT31T\":{"
                 "\"desc\":\"Temperature\","
                 "\"baud\":9600,"
                 "\"addr\":\"0x01\","
                 "\"fcode\":\"0x03\","
                 "\"reg\":\"0x0001\","
                 "\"rqty\":1,"
                 "\"div\":100"
               "},"
               "\"SHT31H\":{"
                 "\"desc\":\"Humidity\","
                 "\"baud\":9600,"
                 "\"addr\":\"0x01\","
                 "\"fcode\":\"0x03\","
                 "\"reg\":\"0x0002\","
                 "\"rqty\":1,"
                 "\"div\":100"
               "}"
             "}"
#elif defined(ANALOG)
    .calib = "{"
             "\"max\":100,"
             "\"min\":0"
             "}"
#elif defined(AMP)
    .calib = "{"
             "\"max\":100,"
             "\"min\":0,"
			 "\"desc\":\"Temp\""
             "}"
#elif defined(VOL)
    .calib = "{"
             "\"max\":100,"
             "\"min\":0,"
			 "\"desc\":\"Temp\","
			 "\"vl\":0,"
			 "\"vh\":10"
             "}"
#elif defined(RS232)
    .mpn = "LIGO SP-RS232"
#endif
};


void nvs_init(void){
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    nvs_init_data(&init_data);
}

void nvs_init_data(nvs_data_t *pdata){
    ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &nvs));

    size_t len;

    if(nvs_get_str(nvs, "name", NULL, &len) != ESP_OK){
    	ESP_LOGE("NVS", "Storage data empty, try to set default data.");

    	uint8_t mac_addr[6];
        ESP_ERROR_CHECK(esp_read_mac(mac_addr, ESP_MAC_EFUSE_FACTORY));
#ifdef RS232
        sprintf(pdata->name, "RS232@%02x%02x%02x%02x%02x%02x"
#elif defined(ANALOG)
        sprintf(pdata->name, "ANALOG@%02x%02x%02x%02x%02x%02x"
#elif defined(AMP)
        sprintf(pdata->name, "AMP@%02x%02x%02x%02x%02x%02x"		
#elif defined(VOL)
        sprintf(pdata->name, "VOL@%02x%02x%02x%02x%02x%02x"	
#elif defined(RS485)
        sprintf(pdata->name, "RS485@%02x%02x%02x%02x%02x%02x"
#endif	
    		, mac_addr[0]
    		, mac_addr[1]
    		, mac_addr[2]
    		, mac_addr[3]
    		, mac_addr[4]
    		, mac_addr[5]);

		ESP_ERROR_CHECK(nvs_set_str(nvs, "name", pdata->name));
		ESP_ERROR_CHECK(nvs_commit(nvs));

		ESP_ERROR_CHECK(nvs_set_str(nvs, "class", pdata->class));
		ESP_ERROR_CHECK(nvs_commit(nvs));
		ESP_ERROR_CHECK(nvs_set_u32(nvs, "period", pdata->period));
		ESP_ERROR_CHECK(nvs_commit(nvs));
		ESP_ERROR_CHECK(nvs_set_str(nvs, "deveui", pdata->deveui));
		ESP_ERROR_CHECK(nvs_commit(nvs));
		ESP_ERROR_CHECK(nvs_set_str(nvs, "appeui", pdata->appeui));
		ESP_ERROR_CHECK(nvs_commit(nvs));
		ESP_ERROR_CHECK(nvs_set_str(nvs, "appkey", pdata->appkey));
		ESP_ERROR_CHECK(nvs_commit(nvs));
#ifdef RS485
		ESP_ERROR_CHECK(nvs_set_str(nvs, "mbdesc", pdata->mb_desc));
		ESP_ERROR_CHECK(nvs_commit(nvs));
#elif defined(ANALOG) || defined(AMP) || defined(VOL)
		ESP_ERROR_CHECK(nvs_set_str(nvs, "calib", pdata->calib));
		ESP_ERROR_CHECK(nvs_commit(nvs));
#elif defined(RS232)
		ESP_ERROR_CHECK(nvs_set_str(nvs, "mpn", pdata->mpn));
		ESP_ERROR_CHECK(nvs_commit(nvs));
#endif
    }
    nvs_close(nvs);
}

void nvs_read_stored_data(void){
	size_t len;

    ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &nvs));

    nvs_get_str(nvs, "name", NULL, &len);
	nvs_get_str(nvs, "name", nvs_stored_data.name, &len);
	ESP_LOGE("NAME", "%s", nvs_stored_data.name);

	nvs_get_str(nvs, "class", NULL, &len);
	nvs_get_str(nvs, "class", nvs_stored_data.class, &len);
	ESP_LOGE("CLASS", "%s", nvs_stored_data.class);

	nvs_get_u32(nvs, "period", &nvs_stored_data.period);
	ESP_LOGE("PERIOD", "%lu", nvs_stored_data.period);

	nvs_get_str(nvs, "deveui", NULL, &len);
	nvs_get_str(nvs, "deveui", nvs_stored_data.deveui, &len);
	ESP_LOGE("DEVEUI", "%s", nvs_stored_data.deveui);

	nvs_get_str(nvs, "appeui", NULL, &len);
	nvs_get_str(nvs, "appeui", nvs_stored_data.appeui, &len);
	ESP_LOGE("APPEUI", "%s", nvs_stored_data.appeui);

	nvs_get_str(nvs, "appkey", NULL, &len);
	nvs_get_str(nvs, "appkey", nvs_stored_data.appkey, &len);
	ESP_LOGE("APPKEY", "%s", nvs_stored_data.appkey);

#ifdef RS485
	nvs_get_str(nvs, "mbdesc", NULL, &len);
	nvs_get_str(nvs, "mbdesc", nvs_stored_data.mb_desc, &len);
	ESP_LOGE("DESC", "%s", nvs_stored_data.mb_desc);
#elif defined(ANALOG) || defined(AMP) || defined(VOL)
	nvs_get_str(nvs, "calib", NULL, &len);
	nvs_get_str(nvs, "calib", nvs_stored_data.calib, &len);
	ESP_LOGE("ANALOG", "%s", nvs_stored_data.calib);
#elif defined(RS232)
	nvs_get_str(nvs, "mpn", NULL, &len);
	nvs_get_str(nvs, "mpn", nvs_stored_data.mpn, &len);
	ESP_LOGE("RS232", "%s", nvs_stored_data.mpn);
#endif

	nvs_close(nvs);
}

void nvs_storing_eui(char *deveui){
	memcpy(nvs_stored_data.deveui, deveui, 16);

    ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &nvs));

    ESP_ERROR_CHECK(nvs_set_str(nvs, "deveui", nvs_stored_data.deveui));
    ESP_ERROR_CHECK(nvs_commit(nvs));

    nvs_close(nvs);
}

char *nvs_create_lorawan_data(void){
	char *json_string;
	asprintf(&json_string,"{\"wan\":{\"name\":\"%s\",\"deveui\":\"%s\",\"appeui\":\"%s\",\"appkey\":\"%s\",\"class\":\"%s\",\"period\":%ld}}",
			nvs_stored_data.name,
			nvs_stored_data.deveui,
			nvs_stored_data.appeui,
			nvs_stored_data.appkey,
			nvs_stored_data.class,
			nvs_stored_data.period);
	return json_string;
}

char *nvs_create_rak_data(void) {
    char *json_string = NULL;
    char *json_suffix = NULL;

    asprintf(&json_string,
             "{\"wan\":{\"name\":\"%s\",\"deveui\":\"%s\",\"appeui\":\"%s\",\"appkey\":\"%s\",\"class\":\"%s\",\"period\":%ld},",
             nvs_stored_data.name,
             nvs_stored_data.deveui,
             nvs_stored_data.appeui,
             nvs_stored_data.appkey,
             nvs_stored_data.class,
             nvs_stored_data.period);

#ifdef RS485
    asprintf(&json_suffix, "\"mbdesc\":%s}", nvs_stored_data.mb_desc);
#elif defined(ANALOG) || defined(VOL) || defined(AMP)
    asprintf(&json_suffix, "\"calib\":%s}", nvs_stored_data.calib);
#elif defined(RS232)
    asprintf(&json_suffix, "\"mpn\":\"%s\"}", nvs_stored_data.mpn);
#endif

    char *full_json_string = NULL;
    asprintf(&full_json_string, "%s%s#", json_string, json_suffix);

    free(json_string);
    free(json_suffix);

    return full_json_string;
}
