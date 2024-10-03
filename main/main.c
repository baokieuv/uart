#include "main.h"


const char *TAG = "MAIN";

extern void wifi_init(void);
extern void station_start(char *ssid, char *password, int8_t turn);
extern void ap_start(void);
extern httpd_handle_t setup_server(void);
extern int8_t read_storage(char **ssid, char **password);
extern void esp_bt_init();
extern void uart_init();


void app_main(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    char *ssid = NULL, *password = NULL;
    wifi_init();
    esp_bt_init();
    if(read_storage(&ssid, &password) == 0){
        ap_start();
    } else {
        station_start(ssid, password, 0);
    }
    setup_server();
    uart_init();
}
