#include "main.h"

uint32_t spp_handle = 0;

#define SPP_SERVER_NAME "SPP_SERVER"

extern const char *TAG;

extern void station_start(char *ssid, char *password, int8_t turn);
extern void write_storage(char *ssid, char *password);

void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param){
    switch (event) {
    case ESP_SPP_INIT_EVT:
        ESP_LOGI(TAG, "ESP_SPP_INIT_EVT");
        //hàm sẽ tạo 1 SPP server và bắt đầu lắng nghe SPP connection request từ 1 thiết bị bluetooth khác
        esp_spp_start_srv(ESP_SPP_SEC_AUTHENTICATE, ESP_SPP_ROLE_SLAVE, 0, SPP_SERVER_NAME);
        break;
    case ESP_SPP_OPEN_EVT:
        ESP_LOGI(TAG, "ESP_SPP_OPEN_EVT");
        break;
    case ESP_SPP_CLOSE_EVT:
        ESP_LOGI(TAG, "ESP_SPP_CLOSE_EVT status:%d handle:%"PRIu32" close_by_remote:%d", param->close.status,
                 param->close.handle, param->close.async);
        break;
    case ESP_SPP_START_EVT:
        ESP_LOGI(TAG, "ESP_SPP_START_EVT handle:%"PRIu32" sec_id:%d scn:%d", param->start.handle, param->start.sec_id,
                     param->start.scn);
        //Set device name to the local device.
        esp_bt_gap_set_device_name("esp-idf blue");
        //set discoverability và connectability mode có bluetooth
        esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
        break;
    case ESP_SPP_DATA_IND_EVT:
        char buf[100];
        strncpy(buf, (char*)param->data_ind.data, param->data_ind.len);
        buf[param->data_ind.len-2] = '\0';
        char *ssid = strtok(buf, "&");
        char *password = strtok(NULL, "&");
        printf("%d, %d", strlen(ssid), strlen(password));
        write_storage(ssid, password);
        station_start(ssid, password, 1);
        break;
    case ESP_SPP_SRV_OPEN_EVT:
        spp_handle = param->open.handle;
        ESP_LOGI(TAG, "ESP_SPP_SRV_OPEN_EVT");
        break;
    default:
        break;
    }
}
void esp_bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_BT_GAP_AUTH_CMPL_EVT:{
        if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGI(TAG, "authentication success: %s", param->auth_cmpl.device_name);
            esp_log_buffer_hex(TAG, param->auth_cmpl.bda, ESP_BD_ADDR_LEN);
        }
        else
        {
            ESP_LOGE(TAG, "authentication failed, status:%d", param->auth_cmpl.stat);
        }
        break;
    }
    case ESP_BT_GAP_PIN_REQ_EVT:{
        ESP_LOGI(TAG, "ESP_BT_GAP_PIN_REQ_EVT min_16_digit:%d", param->pin_req.min_16_digit);
        if (param->pin_req.min_16_digit) {
            ESP_LOGI(TAG, "Input pin code: 0000 0000 0000 0000");
            esp_bt_pin_code_t pin_code = {0};
            //trả lời mã pin_code tới thiết bị ngang hàng để ghép nối khi có sk ESP_BT_GAP_PIN_REQ_EVT
            esp_bt_gap_pin_reply(param->pin_req.bda, true, 16, pin_code);
        } else {
            ESP_LOGI(TAG, "Input pin code: 1234");
            esp_bt_pin_code_t pin_code;
            pin_code[0] = '1';
            pin_code[1] = '2';
            pin_code[2] = '3';
            pin_code[3] = '4';
            esp_bt_gap_pin_reply(param->pin_req.bda, true, 4, pin_code);
        }
        break;
    }

#if (CONFIG_EXAMPLE_SSP_ENABLED == true)
    case ESP_BT_GAP_CFM_REQ_EVT:
        ESP_LOGI(TAG, "ESP_BT_GAP_CFM_REQ_EVT Please compare the numeric value: %"PRIu32, param->cfm_req.num_val);
        //trả lời giá trị xác nhận tới thiết bị ngang hàng trong giai đoạn kết nối
        esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
        break;
    case ESP_BT_GAP_KEY_NOTIF_EVT:
        ESP_LOGI(TAG, "ESP_BT_GAP_KEY_NOTIF_EVT passkey:%"PRIu32, param->key_notif.passkey);
        break;
    case ESP_BT_GAP_KEY_REQ_EVT:
        ESP_LOGI(TAG, "ESP_BT_GAP_KEY_REQ_EVT Please enter passkey!");
        break;
#endif

    case ESP_BT_GAP_MODE_CHG_EVT:
        ESP_LOGI(TAG, "ESP_BT_GAP_MODE_CHG_EVT mode:%d", param->mode_chg.mode);
        break;

    default: {
        ESP_LOGI(TAG, "event: %d", event);
        break;
    }
    }
    return;
}

void stop_bluetooth(){
    if(spp_handle != 0){
        esp_spp_disconnect(spp_handle);
        spp_handle = 0;
        //ESP_ERROR_CHECK(esp_spp_stop_srv_scn(0));
    }
    //dừng toàn bộ SPP servers. hàm trước tiên sẽ đóng toàn bộ SPP connection, sau đó callback function sẽ được gọi với sự kiện ESP_SPP_CLOSE_EVT, sau đó hàm sẽ được hoàn thành, callback được gọi với sự kiện ESP_SPP_SRV_STOP_EVT. 
    //esp_spp_stop_srv();
    //tắt bluetooth, bắt buộc phải gọi trước esp_bluedroid_deinit(), trước khi gọi hàm đảm bảo mọi hoạt động liên quan đến application phải được đóng
    //esp_err_t err = esp_bluedroid_disable();
    //printf("Error %s in stop_bluetooth func\n", esp_err_to_name(err));
    printf("Stop bluetooth\n");
}
void start_bluetooth(){
    //esp_err_t err = esp_bluedroid_enable();
    //esp_spp_start_srv(ESP_SPP_SEC_NONE, ESP_SPP_ROLE_SLAVE, 0, SPP_SERVER_NAME);
    //printf("Error %s in start_bluetooth func\n", esp_err_to_name(err));
    printf("Start bluetooth\n");
}
void esp_bt_init(){
    //giải phóng controller memory theo chế độ. giải phóng BSS, dữ liệu và các thành phần khác của controller vào heap
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));

    //khởi tạo bluetooth controller để phân bổ tác vụ và các tài nguyên khác
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_bt_controller_init(&bt_cfg);

    //bật bluetooth controller, tham số chuyển vào phải khớp với mode đã được khởi tạo trước đó
    esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT);

    //khời tạo và phân bổ tài nguyên cho bluetooth(phải được thực hiện trước mọi thao tác liên quan tời bluetooth)
    esp_bluedroid_config_t bluedroid_cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
    esp_bluedroid_init_with_cfg(&bluedroid_cfg);

    //bật bluetooth, bắt buộc phải sau hàm esp_bluedroid_init()/sp_bluedroid_init_with_cfg()
    esp_bluedroid_enable();

    //đăng ký callback function
    esp_spp_register_callback(esp_spp_cb);

    //đăng ký callback function
    esp_bt_gap_register_callback(esp_bt_gap_cb);

    esp_spp_cfg_t bt_spp_cfg = {
        .mode = ESP_SPP_MODE_CB,
        .enable_l2cap_ertm = false,
        .tx_buffer_size = 0, /* Only used for ESP_SPP_MODE_VFS mode */
    };

    //khởi tạp SPP module, sau khi hàm được hoàn thành, callback function sẽ được gọi với sự kiện ESP_SPP_INIT_EVT
    esp_spp_enhanced_init(&bt_spp_cfg);
    
    /* Set default parameters for Legacy Pairing
     * Use variable pin, input pin code when pairing
     */
    esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_VARIABLE;
    esp_bt_pin_code_t pin_code;
    //đặt pin type và pin code mặc định cho ghép nối
    esp_bt_gap_set_pin(pin_type, 0, pin_code);

    ESP_LOGI(TAG, "Bluetooth is initialized");

}