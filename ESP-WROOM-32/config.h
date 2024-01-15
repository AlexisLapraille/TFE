
#ifndef __CONFG_H___
#define __CONFG_H___

#define LED_PIN_GREEN      27 //video call success - msg success - recording succes
#define LED_PIN_RED        14 //video call failed - msg failed - recording failed
#define BTN_PIN_WIFI       26 //reset wifi to start portal
#define BTN_PIN_CALL       22 //video call start
#define BTN_PIN_MSG        21 //msg sending
#define BTN_PIN_VOICE      13 //record and send voice messages
#define SD_PIN             5 //sd card reader
#define I2S_WS             15 // i2s pins
#define I2S_SD             13 // i2s pins
#define I2S_SCK            2  // i2s pins

#define I2S_PORT I2S_NUM_0
#define I2S_SAMPLE_RATE   (16000)
#define I2S_SAMPLE_BITS   (16)
#define I2S_READ_LEN      (16 * 1024)
#define I2S_CHANNEL_NUM   (1)
#define FLASH_RECORD_SIZE (I2S_CHANNEL_NUM * I2S_SAMPLE_RATE * I2S_SAMPLE_BITS / 8 * RECORD_TIME)

#define ACK                 "A" // acknowledgment packet
#define QUEUE_LEN           5
#define MAX_BUFFER_LEN      128

#define WIFI_SSID           "test" //not real 
#define WIFI_PASSWORD       "test" //not real

#define SERVER_ADDRESS      "192.168.1.x"
#define SERVER_PORT         xxxxx

#endif // __CONFG_H___
