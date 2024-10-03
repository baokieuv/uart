#include "main.h"

extern const char *TAG;
extern const char* ap_html;
extern const char* station_html;

extern void station_start(char *ssid, char *password, int8_t turn);
extern void write_storage(char *ssid, char *password);
extern void event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data);

//URI handler function được gọi trong request GET '/'
esp_err_t send_handler(httpd_req_t* req){
    printf("In send function\n");
    wifi_mode_t mode;
    esp_err_t err = esp_wifi_get_mode(&mode); //lấy mode của wifi
    if(err == ESP_OK){
        //thiết lập loại nội dung
        httpd_resp_set_type(req, "text/html");
        if(mode == WIFI_MODE_AP){
            //gửi dữ liệu như 1 HTTP response cho request
            err = httpd_resp_send(req, ap_html, strlen(ap_html));
        }
        else if(mode == WIFI_MODE_STA){
            err = httpd_resp_send(req, station_html, strlen(station_html));
        }
        else{
            //để gửi mã lỗi trong phản hồi cho HTTP request
            err = httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "WIFI MODE ERROR!");
            printf("Error in Wifi Mode\n");
        }
        printf("%s\n", esp_err_to_name(err));
        return ESP_OK;
    }
    return err;
}
//URI handler function được gọi trong request POST '/'
esp_err_t receive_handler(httpd_req_t* req){
    printf("In receive function\n");
    char buf[100];
    //đọc dữ liệu nội dung từ HTTP Request và bộ đệm đã cung cấp. 
    httpd_req_recv(req, buf, req->content_len);
    httpd_resp_send_chunk(req, NULL, 0);
    buf[req->content_len] = '\0';

    //chuỗi được gửi sẽ có dạng ssid&pasword -> tách để lấy ssid và password
    char *ssid = strtok(buf, "&");
    char *password = strtok(NULL, "&");
    printf("%d, %d\n", strlen(ssid), strlen(password));
    printf("ssid: %s, password: %s\n", ssid, password);
    write_storage(ssid, password);
    station_start(ssid, password, 1);
    return ESP_OK;
}
esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    printf("In 404 function\n");
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Some 404 error message");
    return ESP_FAIL;
}
//function for starting the webserver
httpd_handle_t setup_server(void){
    //URI handler structure cho GET '/'
    static httpd_uri_t uri_send = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = send_handler,
        .user_ctx = NULL,
    };

    //URI handler structure cho POST '/'
    static httpd_uri_t uri_receive = {
        .uri = "/",
        .method = HTTP_POST,
        .handler = receive_handler,
        .user_ctx = NULL,
    };

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if(httpd_start(&server, &config) == ESP_OK){
        /* Register URI handlers */
        httpd_register_uri_handler(server, &uri_send);
        httpd_register_uri_handler(server, &uri_receive);
        httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, http_404_error_handler);

        ESP_ERROR_CHECK(esp_event_handler_instance_register(ESP_HTTP_SERVER_EVENT,
                                                            HTTP_SERVER_EVENT_ERROR,
                                                            &event_handler,
                                                            NULL,
                                                            NULL));
    }
    ESP_LOGI(TAG, "HTTP server done!");
    /* If server failed to start, handle will be NULL */
    return server;
}
