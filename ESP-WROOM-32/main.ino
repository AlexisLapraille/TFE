#include "config.h"
#include "my_wifi.h"
#include "socket.h"
#include "CSS.h"
#include "FS.h"

#include <driver/i2s.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <SD.h> 
#include <SPI.h>
#include <WiFiManager.h> 
#include <MyButton.h>
#include <ESPmDNS.h>
#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebSrv.h>
#include <SPIFFS.h>
#include <MacroDebugger.h>
#include <HTTPClient.h>
#include <UrlEncode.h>


#define ENABLE_DEBUG

int timeout = 120;
String BOT_TOKEN ="xxxxxxx";
String CHAT_ID= "xxxxxxx";
const char* PARAM_GSM = "inputString";
const char* PARAM_INT = "inputString2";
const char* PARAM_API = "inputString3";

WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <meta charset="UTF-8">
  <title>Modification informations</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
    body {
      font-family: Arial, sans-serif;
      margin: 20px;
    }
    form {
      border: 1px solid #ccc;
      padding: 10px;
      margin-bottom: 20px;
    }
    form input[type="text"] {
      width: 100%;
      padding: 8px;
      box-sizing: border-box;
    }
    form input[type="number"] {
      width: 100%;
      padding: 8px;
      box-sizing: border-box;
    }

    form input[type="submit"] {
      background-color: #4CAF50;
      color: white;
      border: none;
      padding: 8px 16px;
      cursor: pointer;
    }
    form input[type="submit"]:hover {
      background-color: #45a049;
    }
    iframe {
      display: none;
    }
  </style>
  <script>
    function submitMessage() {
      alert("Les valeurs ont bien été modifiées.");
      setTimeout(function(){ document.location.reload(false); }, 500);   
    }
  </script></head><body>
  <form action="/get" target="hidden-form">
    Num&eacute;ro (Valeur actuelle : %inputString%): <input type="text" name="inputString" placeholder="+324...">
    <input type="submit" value="Valider" onclick="submitMessage()">
  </form><br>
  <form action="/get" target="hidden-form">
    Message (Valeur actuelle :  %inputString2%): <input type="text" name="inputString2">
    <input type="submit" value="Valider" onclick="submitMessage()">
  </form><br>
  <form action="/get" target="hidden-form">
    Clé API WhatsApp (Valeur actuelle :  %inputString3%): <input type="text" name="inputString3">
    <input type="submit" value="Valider" onclick="submitMessage()">
  </form><br>
  <iframe style="display:none" name="hidden-form"></iframe>
</body></html>)rawliteral";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

void SD_dir()
{
  if (SD_present) 
  {
    if (server.args() > 0 ) /
    { 
      Serial.println(server.arg(0));
  
      String Order = server.arg(0);
      Serial.println(Order);
      
      if (Order.indexOf("download_")>=0)
      {
        Order.remove(0,9);
        SD_file_download(Order);
        Serial.println(Order);
      }
  
      if ((server.arg(0)).indexOf("delete_")>=0)
      {
        Order.remove(0,7);
        SD_file_delete(Order);
        Serial.println(Order);
      }
    }

    File root = SD.open("/");
    if (root) {
      root.rewindDirectory();
      SendHTML_Header();    
      webpage += F("<table align='center'>");
      webpage += F("<tr><th>Nom du fichier</th><th style='width:20%'>Directory ou Fichier</th><th>Taille du fichier</th></tr>");
      printDirectory("/",0);
      webpage += F("</table>");
      SendHTML_Content();
      root.close();
    }
    else 
    {
      SendHTML_Header();
      webpage += F("<h3>No Files Found</h3>");
    }
    append_page_footer();
    SendHTML_Content();
    SendHTML_Stop();
  } else ReportSDNotPresent();
}

void File_Upload()
{
  append_page_header();
  webpage += F("<h3>Select File to Upload</h3>"); 
  webpage += F("<FORM action='/fupload' method='post' enctype='multipart/form-data'>");
  webpage += F("<input class='buttons' style='width:25%' type='file' name='fupload' id = 'fupload' value=''>");
  webpage += F("<button class='buttons' style='width:10%' type='submit'>Upload File</button><br><br>");
  webpage += F("<a href='/'>[Back]</a><br><br>");
  append_page_footer();
  server.send(200, "text/html",webpage);
}

void printDirectory(const char * dirname, uint8_t levels)
{
  
  File root = SD.open(dirname);

  if(!root){
    return;
  }
  if(!root.isDirectory()){
    return;
  }
  File file = root.openNextFile();

  int i = 0;
  while(file){
    if (webpage.length() > 1000) {
      SendHTML_Content();
    }
    if(file.isDirectory()){
      webpage += "<tr><td>"+String(file.isDirectory()?"Dir":"File")+"</td><td>"+String(file.name())+"</td><td></td></tr>";
      printDirectory(file.name(), levels-1);
    }
    else
    {
      webpage += "<tr><td>"+String(file.name())+"</td>";
      webpage += "<td>"+String(file.isDirectory()?"Dir":"File")+"</td>";
      int bytes = file.size();
      String fsize = "";
      if (bytes < 1024)                     fsize = String(bytes)+" B";
      else if(bytes < (1024 * 1024))        fsize = String(bytes/1024.0,3)+" KB";
      else if(bytes < (1024 * 1024 * 1024)) fsize = String(bytes/1024.0/1024.0,3)+" MB";
      else                                  fsize = String(bytes/1024.0/1024.0/1024.0,3)+" GB";
      webpage += "<td>"+fsize+"</td>";
      webpage += "<td>";
      webpage += F("<FORM action='/' method='post'>"); 
      webpage += F("<button type='submit' name='download'"); 
      webpage += F("' value='"); webpage +="download_"+String(file.name()); webpage +=F("'>T&eacute;l&eacute;charger</button>");
      webpage += "</td>";
      webpage += "<td>";
      webpage += F("<FORM action='/' method='post'>"); 
      webpage += F("<button type='submit' name='delete'"); 
      webpage += F("' value='"); webpage +="delete_"+String(file.name()); webpage +=F("'>Supprimer</button>");
      webpage += "</td>";
      webpage += "</tr>";
    }
    file = root.openNextFile();
    i++;
  }
  file.close();
}

void SD_file_download(String filename)
{
  if (SD_present) 
  { 
    File download = SD.open("/"+filename);
    if (download) 
    {
      server.sendHeader("Content-Type", "text/text");
      server.sendHeader("Content-Disposition", "attachment; filename="+filename);
      server.sendHeader("Connection", "close");
      server.streamFile(download, "application/octet-stream");
      download.close();
    } else ReportFileNotPresent("download"); 
  } else ReportSDNotPresent();
}

File UploadFile;

void handleFileUpload()
{ 
  HTTPUpload& uploadfile = server.upload();
  if(uploadfile.status == UPLOAD_FILE_START)
  {
    String filename = uploadfile.filename;
    if(!filename.startsWith("/")) filename = "/"+filename;
    Serial.print("Upload File Name: "); Serial.println(filename);
    SD.remove(filename);
    UploadFile = SD.open(filename, FILE_WRITE);  
    filename = String();
  }
  else if (uploadfile.status == UPLOAD_FILE_WRITE)
  {
    if(UploadFile) UploadFile.write(uploadfile.buf, uploadfile.currentSize);
  } 
  else if (uploadfile.status == UPLOAD_FILE_END)
  {
    if(UploadFile)
    {                                    
      UploadFile.close();
      Serial.print("Upload Size: "); Serial.println(uploadfile.totalSize);
      webpage = "";
      append_page_header();
      webpage += F("<h3>File was successfully uploaded</h3>"); 
      webpage += F("<h2>Uploaded File Name: "); webpage += uploadfile.filename+"</h2>";
      webpage += F("<h2>File Size: "); webpage += file_size(uploadfile.totalSize) + "</h2><br><br>"; 
      webpage += F("<a href='/'>[Back]</a><br><br>");
      append_page_footer();
      server.send(200,"text/html",webpage);
    } 
    else
    {
      ReportCouldNotCreateFile("upload");
    }
  }
}

void SD_file_delete(String filename) 
{ 
  if (SD_present) { 
    SendHTML_Header();
    File dataFile = SD.open("/"+filename, FILE_READ);
    if (dataFile)
    {
      if (SD.remove("/"+filename)) {
        Serial.println(F("File deleted successfully"));
        webpage += "<h3>File '"+filename+"' has been erased</h3>"; 
        webpage += F("<a href='/'>[Back]</a><br><br>");
      }
      else
      { 
        webpage += F("<h3>File was not deleted - error</h3>");
        webpage += F("<a href='/'>[Back]</a><br><br>");
      }
    } else ReportFileNotPresent("delete");
    append_page_footer(); 
    SendHTML_Content();
    SendHTML_Stop();
  } else ReportSDNotPresent();
} 

void SendHTML_Header()
{
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate"); 
  server.sendHeader("Pragma", "no-cache"); 
  server.sendHeader("Expires", "-1"); 
  server.setContentLength(CONTENT_LENGTH_UNKNOWN); 
  server.send(200, "text/html", "");
  append_page_header();
  server.sendContent(webpage);
  webpage = "";
}

void SendHTML_Content()
{
  server.sendContent(webpage);
  webpage = "";
}

void SendHTML_Stop()
{
  server.sendContent("");
  server.client().stop(); 
}

void ReportSDNotPresent()
{
  SendHTML_Header();
  webpage += F("<h3>No SD Card present</h3>"); 
  webpage += F("<a href='/'>[Back]</a><br><br>");
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
}

void ReportFileNotPresent(String target)
{
  SendHTML_Header();
  webpage += F("<h3>File does not exist</h3>"); 
  webpage += F("<a href='/"); webpage += target + "'>[Back]</a><br><br>";
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
}

void ReportCouldNotCreateFile(String target)
{
  SendHTML_Header();
  webpage += F("<h3>Could Not Create Uploaded File (write-protected?)</h3>"); 
  webpage += F("<a href='/"); webpage += target + "'>[Back]</a><br><br>";
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
}

String file_size(int bytes)
{
  String fsize = "";
  if (bytes < 1024)                 fsize = String(bytes)+" B";
  else if(bytes < (1024*1024))      fsize = String(bytes/1024.0,3)+" KB";
  else if(bytes < (1024*1024*1024)) fsize = String(bytes/1024.0/1024.0,3)+" MB";
  else                              fsize = String(bytes/1024.0/1024.0/1024.0,3)+" GB";
  return fsize;
}

void i2sInit(){
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = I2S_SAMPLE_RATE,
    .bits_per_sample = i2s_bits_per_sample_t(I2S_SAMPLE_BITS),
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = 0,
    .dma_buf_count = 64,
    .dma_buf_len = 1024,
    .use_apll = 1
  };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);

  const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = -1,
    .data_in_num = I2S_SD
  };

  i2s_set_pin(I2S_PORT, &pin_config);
}


void i2s_adc_data_scale(uint8_t * d_buff, uint8_t* s_buff, uint32_t len)
{
    uint32_t j = 0;
    uint32_t dac_value = 0;
    for (int i = 0; i < len; i += 2) {
        dac_value = ((((uint16_t) (s_buff[i + 1] & 0xf) << 8) | ((s_buff[i + 0]))));
        d_buff[j++] = 0;
        d_buff[j++] = dac_value * 256 / 2048;
    }
}

void i2s_adc(void *arg) { 
  int i2s_read_len = I2S_READ_LEN;
  int flash_wr_size = 0;
  size_t bytes_read;
  char* i2s_read_buff = (char*) calloc(i2s_read_len, sizeof(char));
  uint8_t* flash_write_buff = (uint8_t*) calloc(i2s_read_len, sizeof(char));
  i2s_read(I2S_PORT, (void*) i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);
  i2s_read(I2S_PORT, (void*) i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);
  digitalWrite(LED,HIGH);
  File file = SD.open(filename, FILE_WRITE);
  if(!file) {
    Serial.println("File is not available!");
  }
  Serial.println("Recording Start");
  if(!digitalRead(button)) {
    i2s_read(I2S_PORT, (void*) i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);
    i2s_adc_data_scale(flash_write_buff, (uint8_t*)i2s_read_buff, i2s_read_len);
    file.write((const byte*) flash_write_buff, i2s_read_len);
    flash_wr_size += i2s_read_len;
    ets_printf("Sound recording %u%%\n", flash_wr_size * 100 / FLASH_RECORD_SIZE);
    ets_printf("Never Used Stack Size: %u\n", uxTaskGetStackHighWaterMark(NULL));
  }
  file.close();
  digitalWrite(LED_PIN_GREEN,LOW);
  file = SD.open(filename, FILE_READ);
  int bufSize = 1024;
  uint8_t buffer[bufSize];
  size_t bytesRead = file.read(buffer, bufSize);
  if (bytesRead > 0) {
    String response=bot.sendMultipartFormDataToTelegram("sendDocument", "document", filename, "", CHAT_ID, bytesRead, isMoreDataAvailable, getNextByte, nullptr, nullptr);
    Serial.println("File Sent!");
    digitalWrite(LED_PIN_GREEN,HIGH);
    delay(2000);
    digitalWrite(LED_PIN_GREEN,LOW);
  }
  file.close();
  free(i2s_read_buff);
  i2s_read_buff = NULL;
  free(flash_write_buff);
  flash_write_buff = NULL;
  listSD();
  vTaskDelete(NULL);
}

String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path, "r");
  if(!file || file.isDirectory()){
    Serial.println("- empty file or failed to open file");
    return String();
  }
  Serial.println("- read from file:");
  String fileContent;
  while(file.available()){
    fileContent+=String((char)file.read());
  }
  file.close();
  Serial.println(fileContent);
  return fileContent;
}

void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);
  File file = fs.open(path, "w");
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
  file.close();
}

String processor(const String& var){
  if(var == "inputString"){
    return readFile(SPIFFS, "/inputString.txt");
  }
  else if(var == "inputString2"){
    return readFile(SPIFFS, "/inputString2.txt");
  }
  else if(var == "inputString3"){
    return readFile(SPIFFS, "/inputString3.txt");
  }
  return String();
}

void sendMessage(String message){
  String url = "https://api.callmebot.com/whatsapp.php?phone=" + readFile(SPIFFS, "/inputString.txt") + "&apikey=" + readFile(SPIFFS, "/inputString3.txt") + "&text=" + urlEncode(message);    
  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int httpResponseCode = http.POST(url);
  if (httpResponseCode == 200){
    Serial.print("Message sent successfully");
    digitalWrite(LED_PIN_GREEN, HIGH);
    delay(3000); 
    digitalWrite(LED_PIN_GREEN, LOW);
  }
  else{
    Serial.println("Error sending the message");
    Serial.print("HTTP response code: ");
    Serial.println(httpResponseCode);
    digitalWrite(LED_PIN_RED, HIGH);
    delay(3000); 
    digitalWrite(LED_PIN_RED, LOW);
  }
  http.end();
}

MyButton my_btn_wifi(BTN_PIN_WIFI, NORMAL_UP, 50); //debounce 50
MyButton my_btn_call(BTN_PIN_CALL, NORMAL_UP, 50); //debounce 50
MyButton my_btn_msg(BTN_PIN_MSG, NORMAL_DOWN, 50); //debounce 50
MyButton my_btn_voice(BTN_PIN_VOICE, NORMAL_DOWN, 50); //debounce 50

char incoming_msg[MAX_BUFFER_LEN] = {1};
char response[MAX_BUFFER_LEN] = {0};

#define RESPONSES    2
char *responses[RESPONSES] = {
  "call",
  "call-again"
};

void setup(){
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  
  #ifdef ESP32
    if(!SPIFFS.begin(true)){
      Serial.println("Error while mounting SPIFFS");
      return;
    }
  #else
    if(!SPIFFS.begin()){
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
    }
  #endif
  DEBUG_BEGIN();
  setup_wifi();
  setup_socket();
  pinMode(LED_PIN_GREEN, OUTPUT);
  pinMode(LED_PIN_RED, OUTPUT);
  if(!MDNS.begin("alertbox")) {
     Serial.println("Error starting mDNS");
     return;
  }
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    if (request->hasParam(PARAM_GSM)) {
      inputMessage = request->getParam(PARAM_GSM)->value();
      writeFile(SPIFFS, "/inputString.txt", inputMessage.c_str());
    }
    else if (request->hasParam(PARAM_INT)) {
      inputMessage = request->getParam(PARAM_INT)->value();
      writeFile(SPIFFS, "/inputString2.txt", inputMessage.c_str());
    }
    else if (request->hasParam(PARAM_API)) {
      inputMessage = request->getParam(PARAM_API)->value();
      writeFile(SPIFFS, "/inputString3.txt", inputMessage.c_str());
    }
    else {
      inputMessage = "No message sent";
    }
    Serial.println(inputMessage);
    request->send(200, "text/text", inputMessage);
  });
  server.on("/carteSD",  SD_dir);
  server.on("/upload",   File_Upload);
  server.on("/fupload",  HTTP_POST,[](){ server.send(200);}, handleFileUpload);
  server.onNotFound(notFound);
  server.begin();
  Serial.println("HTTP server started");
  pinMode(BTN_PIN_MSG, INPUT_PULLUP);
  DEBUG_I("Done setting up!");
  i2sInit();
  secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
}

void loop(){
  Serial.print("Got IP: "); 
  Serial.println(WiFi.localIP());
  String yourInputString = readFile(SPIFFS, "/inputString.txt");
  Serial.print("*** Your inputString: ");
  Serial.println(yourInputString);
  String yourinputString2 = readFile(SPIFFS, "/inputString2.txt");
  Serial.print("*** Your inputString2: ");
  Serial.println(yourinputString2);

  if(!is_client_connected()){ connect_client(); }
  
  bool received = get_message(incoming_msg);

  if(received){
    DEBUG_I("Received: %s", incoming_msg);
    uint8_t start = 0;

    if(incoming_msg[0] == 'A'){
      sprintf(response, "%s", ACK);
      start++;
    }

    switch(incoming_msg[start]){
      case 'n':
        digitalWrite(LED_PIN_GREEN, HIGH);
        break;
      case 'h':
        digitalWrite(LED_PIN_RED, HIGH);
        break;
      case 'c':
        digitalWrite(LED_PIN_RED, LOW);
        break;
      default:
      case 'f':
        digitalWrite(LED_PIN_GREEN, LOW);
        break;
    }
    if(start > 0){
      send_message(response);
      memset(response, 0, MAX_BUFFER_LEN);
    }
  }
  
   if (my_btn_wifi.readRisingClick()) {
    WiFiManager wm;    
    wm.setConfigPortalTimeout(timeout);
    if (!wm.startConfigPortal(" Connexion – Alertbox ")) {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      ESP.restart();
      delay(5000);
    }
    Serial.println("Connected to the local network)");
  }
  
  if(my_btn_call.readRisingClick()){
    uint8_t idx = random(RESPONSES);
    strncpy(response, responses[idx], MAX_BUFFER_LEN);
    send_message(response);
    memset(response, 0, MAX_BUFFER_LEN);
    DEBUG_I("Sent: %s", responses[idx]);
  }

  if(my_btn_msg.readRisingClick()){
    String yourInputString2 = readFile(SPIFFS, "/inputString2.txt");
    sendMessage(yourInputString2);
    Serial.println("Le message a été envoyé");
    Serial.println(yourInputString);
    delay(1000); 
  }

  if(my_btn_voice.readRisingClick()){
    xTaskCreate(i2s_adc, "i2s_adc", 1024 * 2, NULL, 1, NULL);
  }
}
