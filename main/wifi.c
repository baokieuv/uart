#include "main.h"

#define ESP_MAXIMUM_RETRY       2           //số lần kết nối wifi lại tối đa

#define ESP_WIFI_AP_SSID        "esp-idf"   //ssid của wifi khi ở AP mode
#define ESP_WIFI_CHANNEL        1           
#define MAX_STA_CONN            5           //số station có thể kết nối cùng lúc


#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_BOTH
#define EXAMPLE_H2E_IDENTIFIER ""
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK

static EventGroupHandle_t s_wifi_event_group;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

extern const char *TAG;
static int s_retry_num = 0;
static bool flag = true;

esp_netif_t *esp_netif_sta;
esp_netif_t *esp_netif_ap;

extern void stop_bluetooth();
extern void start_bluetooth();

extern void disable_uart();
extern void uart_init();

const char* ap_html = "<!DOCTYPE html>\n"
"<html lang=\"en\">\n"
"<head>\n"
"    <meta charset=\"UTF-8\">\n"
"    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
"    <title>ESP32 Webserver</title>\n"
"    <style>\n"
"        body {\n"
"            font-family: Arial, sans-serif;\n"
"            display: flex;\n"
"            justify-content: center;\n"
"            align-items: center;\n"
"            height: 100vh;\n"
"            background-color: #f0f0f0;\n"
"            margin: 0;\n"
"        }\n"
"        .container {\n"
"            background-color: white;\n"
"            padding: 20px;\n"
"            border-radius: 8px;\n"
"            box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);\n"
"            width: 300px;\n"
"        }\n"
"        h1 {\n"
"            margin: 0;\n"
"            font-size: 24px;\n"
"            text-align: center;\n"
"        }\n"
"        h2 {\n"
"            margin: 10px 0;\n"
"            font-size: 18px;\n"
"            color: #555;\n"
"            text-align: center;\n"
"        }\n"
"        form {\n"
"            display: flex;\n"
"            flex-direction: column;\n"
"        }\n"
"        label {\n"
"            margin-bottom: 5px;\n"
"            text-align: left;\n"
"            display: block;\n"
"        }\n"
"        input[type=\"text\"], input[type=\"password\"] {\n"
"            width: 100%%;\n"
"            padding: 8px;\n"
"            margin: 8px 0 15px 0;\n"
"            box-sizing: border-box;\n"
"            border: 1px solid #ccc;\n"
"            border-radius: 4px;\n"
"            display: block;\n"
"        }\n"
"        input[type=\"submit\"] {\n"
"            background-color: #4CAF50;\n"
"            color: white;\n"
"            padding: 10px 15px;\n"
"            border: none;\n"
"            border-radius: 4px;\n"
"            cursor: pointer;\n"
"            margin-top: 10px;\n"
"        }\n"
"        input[type=\"submit\"]:hover {\n"
"            background-color: #45a049;\n"
"        }\n"
"    </style>\n"
"</head>\n"
"<body>\n"
"\n"
"    <div class=\"container\">\n"
"        <h1>ESP32</h1>\n"
"        <h1>Webserver</h1>\n"
"        <h2>AiThings Lab IoT-07</h2>\n"
"\n"
"        <form>\n"
"            <label for=\"ssid\">SSID</label>\n"
"            <input type=\"text\" id=\"ssid\" name=\"ssid\" placeholder=\"Enter ssid\">\n"
"\n"
"            <label for=\"password\">Password</label>\n"
"            <input type=\"password\" id=\"password\" name=\"password\" placeholder=\"Enter password\">\n"
"\n"
"            <input type=\"submit\" id=\"button1\" value=\"Submit\" onclick=\"myFunction1()\">\n"
"        </form>\n"
"    </div>\n"
"\n"
"    <script>\n"
"        var xhttp1 = new XMLHttpRequest();\n"
"\n"
"        function myFunction1(){\n"
"            var element1 = document.getElementById(\"ssid\");\n"
"            var element2 = document.getElementById(\"password\");\n"
"\n"
"            if(element1.value != \"\" && element2.value != \"\") {\n"
"                xhttp1.open(\"POST\", \"/\", true);\n"
"                xhttp1.send(element1.value + \"&\" + element2.value);\n"
"            }\n"
"        }\n"
"    </script>\n"
"</body>\n"
"</html>\n";


const char* station_html = 
"<!DOCTYPE html>"
"<html lang=\"en\">"
"<head>"
    "<meta charset=\"UTF-8\">"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
    "<title>ESP32 Webserver</title>"
    "<style>"
        "body {"
            "font-family: Arial, sans-serif;"
            "display: flex;"
            "justify-content: center;"
            "align-items: center;"
            "height: 100vh;"
            "background-color: #f0f0f0;"
            "margin: 0;"
        "}"
        ".container {"
            "background-color: white;"
            "padding: 20px;"
            "border-radius: 8px;"
            "box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);"
            "width: 300px;"
        "}"
        "h1 {"
            "margin: 0;"
            "font-size: 24px;"
            "text-align: center;"
        "}"
        "h2 {"
            "margin: 10px 0;"
            "font-size: 18px;"
            "color: #555;"
            "text-align: center;"
        "}"
        "form {"
            "display: flex;"
            "flex-direction: column;"
        "}"
        "label {"
            "margin-bottom: 5px;"
            "text-align: left;"
        "}"
    "</style>"
"</head>"
"<body>"

    "<div class=\"container\">"
        "<h1>ESP32</h1>"
        "<h1>Webserver</h1>"
        "<h2>AiThings Lab IoT-07</h2>"
        "<h2>Station Mode</h2>"
    "</div>"

    "<script>"
    "</script>"
"</body>"
"</html>";

void ap_start(void){
    //stop Wifi driver
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_LOGI(TAG, "AP mode start!");

    esp_netif_set_default_netif(esp_netif_ap);

    //Wi-Fi configuration
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = ESP_WIFI_AP_SSID,
            .ssid_len = strlen(ESP_WIFI_AP_SSID),
            .channel = ESP_WIFI_CHANNEL,
            .max_connection = MAX_STA_CONN,
            .authmode = WIFI_AUTH_OPEN,
            .pmf_cfg = {
                .required = false,
            },
        },
    };
 
    //configure mode (AP mode)-> start
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start()); 

    start_bluetooth();   
    uart_init();
}

//hàm xử lý các event khi được phát sinh
void event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    //WIFI_EVENT_AP_STACONNECTED phát sinh -> log ra thông báo
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *) event_data;
        ESP_LOGI(TAG, "Station "MACSTR" joined, AID = %d",
                 MAC2STR(event->mac), event->aid);
    } 
    //WIFI_EVENT_AP_STADISCONNECTED phát sinh -> log ra thông báo+viết ssid và pass vào flash + restart esp
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *) event_data;
        ESP_LOGI(TAG, "Station "MACSTR" left, AID = %d, reason: %d",
                 MAC2STR(event->mac), event->aid, event->reason);
    } 
    //WIFI_EVENT_STA_START phát sinh -> bắt đầu thực hiện các phase connect
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } 
    //WIFI_EVENT_STA_DISCONNECTED phát sinh -> kiểm tra s_retry_num
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        flag = false;
        //nếu s_retry_num < ESP_MAXIMUM_RETRY -> kết nối lại + log thông báo
        if (s_retry_num < ESP_MAXIMUM_RETRY) { 
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retry to connect to the AP");
        } 
        //set WIFI_FAIL_BIT cho s_wifi_event_group + log thông báo + chuyển wifi sang AP mode
        else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGI(TAG,"Connect to the AP fail");
            s_retry_num = 0;
            ap_start();
        }
    }
    //IP_EVENT_STA_GOT_IP phát sinh -> log thông báo + set WIFI_CONNECTED_BIT cho s_wifi_event_group
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        flag = true;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        s_retry_num = 0;
    }
    else if(event_base == ESP_HTTP_SERVER_EVENT && event_id == HTTP_SERVER_EVENT_ERROR){
        printf("Something wrong!\n");
    }
}
void wifi_init(void){
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_sta = esp_netif_create_default_wifi_sta();
    esp_netif_ap = esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL));
}
void station_start(char *ssid, char *password, int8_t turn){
    stop_bluetooth();
    disable_uart();
    ESP_LOGI(TAG, "Station mode start!");
    ESP_ERROR_CHECK(esp_wifi_stop());
    if(s_wifi_event_group == NULL) s_wifi_event_group = xEventGroupCreate();

    esp_netif_set_default_netif(esp_netif_sta);

    //Wi-Fi configuration
    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
        },
    };

    strcpy((char*)wifi_config.sta.ssid, ssid);
    strcpy((char*)wifi_config.sta.password, password);

    //configure mode (Station mode) -> start
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    if(turn){
        esp_wifi_connect();
    }
    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */

    //thông báo kết nối
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 ssid, password);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 ssid, password);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
    xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);

    while(flag){
        vTaskDelay(10000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 ssid, password);
    }
}

