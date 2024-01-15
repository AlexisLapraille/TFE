
#ifndef __SOCKET_H__
#define __SOCKET_H__

#include "my_wifi.h"
#include "config.h"

static SemaphoreHandle_t _send_tsk_mutex;
static SemaphoreHandle_t _recv_tsk_mutex;
static QueueHandle_t _send_q;
static QueueHandle_t _recv_q;
static TaskHandle_t _socket_reporter_task_h = NULL;

static WiFiClient _client;

void connect_client(){
  while(!_client.connect(SERVER_ADDRESS, SERVER_PORT)){ delay(1000); }
}

bool is_client_connected(){
  return _client.connected();
}

void send_message(char *msg){
  xQueueSend(_send_q, (void*)msg, 5);
}

bool get_message(char *msg){
  return xQueueReceive(_recv_q, (void*)msg, 5) == pdTRUE;
}

static void sender_task(void*){
  xSemaphoreTake(_send_tsk_mutex, portMAX_DELAY);
  char buff[MAX_BUFFER_LEN] = {0};
  while(1){
    if(xQueueReceive(_send_q, (void*)&buff, 5) == pdTRUE){
      _client.print(buff);
    }
  }
}

static void receiver_task(void*){
  xSemaphoreTake(_recv_tsk_mutex, portMAX_DELAY);
  char buf[MAX_BUFFER_LEN] = {0};
  while(1){
    while(_client.available() <= 0){ delay(100); };
    for(uint8_t i = 0; i < _client.available(); i++){ buf[i] = (char)_client.read(); };
    xQueueSend(_recv_q, (void*)&buf, 5);
  }
}

void setup_socket(){
  _send_q = xQueueCreate(QUEUE_LEN, MAX_BUFFER_LEN);
  _recv_q = xQueueCreate(QUEUE_LEN, MAX_BUFFER_LEN);
  _recv_tsk_mutex = xSemaphoreCreateMutex();
  _send_tsk_mutex = xSemaphoreCreateMutex();
    
  xSemaphoreTake(_send_tsk_mutex, portMAX_DELAY);
  xSemaphoreTake(_recv_tsk_mutex, portMAX_DELAY);
  xTaskCreatePinnedToCore(sender_task, "sendTask", 4096, NULL, 3, NULL, 1);
  xTaskCreatePinnedToCore(receiver_task, "receiveTask", 4096, NULL, 3, NULL, 1);

  connect_client();

  xSemaphoreGive(_send_tsk_mutex);
  xSemaphoreGive(_recv_tsk_mutex);
}

#endif // __SOCKET_H__
