#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include <lwip/netdb.h>

#define PORT 8080
#define CLIENT_PORT 8081
#define HOST_IP_ADDR "192.168.0.180"
#define RX_TIMEOUT 5
#define TX_INTERVAL_MS 2000

static const char *TAG = "udp";
static const char *payload = "Message from ESP32";
static int sock = -1;

static void udp_send_task(void *pvParameters) 
{
    struct sockaddr_in dest_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr.s_addr = inet_addr(HOST_IP_ADDR)
    };

    while (1) {
        int sent = sendto(sock, payload, strlen(payload), 0,
                        (struct sockaddr*)&dest_addr, sizeof(dest_addr));
        if (sent < 0) {
            ESP_LOGE(TAG, "Send failed: %s", strerror(errno));
            close(sock);
            sock = -1;
            break;
        }
        ESP_LOGI(TAG, "Sent %d bytes to %s:%d",
                sent, HOST_IP_ADDR, PORT);
        vTaskDelay(pdMS_TO_TICKS(TX_INTERVAL_MS));
    }
    vTaskDelete(NULL);
}

static void udp_recv_task(void *pvParameters) 
{
    char rx_buffer[512];
    struct sockaddr_in source_addr;
    socklen_t addr_len = sizeof(source_addr);

    while (1) {
        if (sock < 0) break;  // Exit if socket closed

        int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer)-1, 0,
                         (struct sockaddr*)&source_addr, &addr_len);
        if (len < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                ESP_LOGE(TAG, "Recv failed: %s", strerror(errno));
                close(sock);
                sock = -1;
                break;
            }
        } 
        else if (len == 0) {
            ESP_LOGW(TAG, "Received empty packet");
        } 
        else {
            rx_buffer[len] = '\0';
            ESP_LOGI(TAG, "Received %d bytes from %s:%d:\n%s",
                    len, inet_ntoa(source_addr.sin_addr),
                    ntohs(source_addr.sin_port), rx_buffer);
        }
    }
    vTaskDelete(NULL);
}

void udp_client_init(void) 
{
    struct sockaddr_in local_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(CLIENT_PORT),
        .sin_addr.s_addr = INADDR_ANY
    };

    // Create UDP socket
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE(TAG, "Socket create failed: %s", strerror(errno));
        return;
    }

    // Bind socket
    if (bind(sock, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
        ESP_LOGE(TAG, "Socket bind failed: %s", strerror(errno));
        close(sock);
        return;
    }

    // Set receive timeout
    struct timeval timeout = {.tv_sec = RX_TIMEOUT};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    // Create tasks
    //xTaskCreate(udp_send_task, "udp_tx", 4096, NULL, 5, NULL);
    xTaskCreate(udp_recv_task, "udp_rx", 4096, NULL, 5, NULL);
}