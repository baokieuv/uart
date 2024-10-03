#include "main.h"

extern const char *TAG;

int8_t read_storage(char **ssid, char **password){
    int8_t flag = 1; //cờ đánh dấu đọc thành công (1 = thành công, 0 = không thành công)
    size_t size;
    nvs_handle_t my_handle;

    //mở NVS với namespace đã cho từ phân vùng NVS mặc định (namespace = storage) với chế độ chỉ đọc
    esp_err_t err = nvs_open("storage", NVS_READONLY, &my_handle);
    if(err != ESP_OK){
        ESP_LOGI(TAG, "Error %s opening nvs!", esp_err_to_name(err));
        nvs_close(my_handle);  
        return 0 ;
    }
    
    //lấy giá trị chuỗi cho khóa đã cho. Nếu key không tồn tại hoặc kiểu dữ liệu không 
    //khớp thì error sẽ được trả về. Trong trường hợp có lỗi thì out_value sẽ không được sửa đổi
    err = nvs_get_str(my_handle, "ssid", NULL, &size);
    if(err == ESP_OK){
        if(*ssid != NULL) free(*ssid);
        *ssid = (char*) malloc(size); //cấp phát bộ nhớ cho ssid 
        nvs_get_str(my_handle, "ssid", *ssid, &size);
    } else {
        flag = 0;
        ESP_LOGI(TAG, "Error %s reading ssid from nvs!", esp_err_to_name(err));
    }

    err = nvs_get_str(my_handle, "password", NULL, &size);
    if(err == ESP_OK){
        if(*password != NULL) free(*password);
        *password = (char*) malloc(size); //cấp phát bộ nhớ cho password
        nvs_get_str(my_handle, "password", *password, &size);
    } else {
        flag = 0;
        ESP_LOGI(TAG, "Error %s reading password from nvs!", esp_err_to_name(err));
    }   

    //đóng storage handle và giải phóng toàn bộ tài nguyên đươc phân bổ
    nvs_close(my_handle);

    printf("Read done! ssid: %s, password: %s\n", *ssid, *password);
    return flag;
}
void write_storage(char *ssid, char *password){
    nvs_handle_t my_handle;

    //mở NVS với namespace đã cho từ phân vùng NVS mặc định (namespace = storage) với chế độ đọc+ghi
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK){
        ESP_LOGI(TAG, "Error %s opening nvs!", esp_err_to_name(err));
    } else {
        //set string cho key.  Việc lưu trữ xâu dài có thể bị lỗi do bị phân mảnh bộ nhớ
        //bộ nhớ sẽ không được cập nhật cho đến khi hàm nvs_commit() được gọi
        ESP_ERROR_CHECK(nvs_set_str(my_handle, "ssid", ssid));
        ESP_ERROR_CHECK(nvs_set_str(my_handle, "password", password));

        //ghi bất kỳ thay đổi nào đang chờ xử lý vào NVS
        ESP_ERROR_CHECK(nvs_commit(my_handle));
        printf("Write done! ssid: %s, password: %s\n", ssid, password);
    }

    //đóng storage handle và giải phóng toàn bộ tài nguyên đươc phân bổ
    nvs_close(my_handle); 
}
