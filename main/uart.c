#include "main.h"

extern const char *TAG;

#define EX_UART_NUM UART_NUM_0
#define PATTERN_CHR_NUM    (3)         /*!< Set the number of consecutive and identical characters received by receiver which defines a UART pattern*/

#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)
static QueueHandle_t uart0_queue;

uint8_t line_buffer[BUF_SIZE];
static int line_pos = 0;
static bool uart_installed = false;

extern void write_storage(char *ssid, char *password);
extern void station_start(char *ssid, char *password, int8_t turn);

void uart_event_task(void *pvParameters)
{
    uart_event_t event;
    uint8_t* dtmp = (uint8_t*) malloc(RD_BUF_SIZE);
    for (;;) {
        //Waiting for UART event.
        if (xQueueReceive(uart0_queue, (void *)&event, (TickType_t)portMAX_DELAY)) {
            bzero(dtmp, RD_BUF_SIZE);
            switch (event.type) {
            case UART_DATA:
                int len = uart_read_bytes(EX_UART_NUM, dtmp, event.size, portMAX_DELAY);
                printf("%s", dtmp);
                fflush(stdout);
                for(int i = 0; i < len; i++){
                    if(dtmp[i] == 13){
                        line_buffer[line_pos] = '\0';
                        printf("\ndata: %s\n", line_buffer);
                        char* ssid = strtok((char*)line_buffer, "&");
                        char* password = strtok(NULL, "&");
                        line_pos = 0;
                        write_storage(ssid, password);
                        station_start(ssid, password, 1);
                    }else if(dtmp[i] == 8){
                        line_pos--;
                        if(line_pos < 0) line_pos = 0;
                    }
                    else{
                        line_buffer[line_pos++] = dtmp[i];
                    }
                }
                break;

            //Event of HW FIFO overflow detected
            case UART_FIFO_OVF:
                ESP_LOGI(TAG, "hw fifo overflow");
                uart_flush_input(EX_UART_NUM);
                xQueueReset(uart0_queue);
                break;
            //Event of UART ring buffer full
            case UART_BUFFER_FULL:
                ESP_LOGI(TAG, "ring buffer full");
                uart_flush_input(EX_UART_NUM);
                xQueueReset(uart0_queue);
                break;
            //Others
            default:
                ESP_LOGI(TAG, "uart event type: %d", event.type);
                break;
            }
        }
    }
    free(dtmp);
    dtmp = NULL;
    vTaskDelete(NULL);
}

void uart_init(){
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    if(!uart_installed){
    uart_config_t uart_config = {
            .baud_rate = 115200,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .source_clk = UART_SCLK_DEFAULT,
        };
        //Install UART driver, and get the queue.
        uart_driver_install(EX_UART_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 20, &uart0_queue, 0);
        uart_param_config(EX_UART_NUM, &uart_config);

        //Create a task to handler UART event from ISR
        xTaskCreate(uart_event_task, "uart_event_task", 2048*2, NULL, 12, NULL);
        uart_installed = true;
        printf("\nEnable UART\n");
    }
}

void disable_uart(){
    if(uart_installed){
        //printf("1\n");
        //vTaskDelete(NULL); 
        // printf("1");
        // uart_driver_delete(EX_UART_NUM);
        // uart_installed = false;
        printf("\nDisable UART\n");
    }
}