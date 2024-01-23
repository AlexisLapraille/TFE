#include "twilio.hpp"
#include <ESPmDNS.h>
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebSrv.h>
#include <SPIFFS.h>

#define BUTTON_PIN 21

const char* ssid = "ZI-AK47";  //devolo-f4068d610801 //ESP32 
const char* password = "TonEnormeDaronneEnY"; // HSJFLMSEZOSPYLOW
static const char *account_sid = "AC3cb491ffe40a7bff6c67b028cdaa4547";
static const char *auth_token = "d77a0a9a109366784b2927ffbaba3c83";
static const char *from_number = "+12057758537";
static const char *message = "Alerte !";
static String to_number = "+32470257007";

Twilio *twilio;

const char* PARAM_STRING = "inputString";
const char* PARAM_INT = "inputString2";

AsyncWebServer server(80);

// HTML web page to handle 3 input fields (input1, input2, input3)
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
  <iframe style="display:none" name="hidden-form"></iframe>
</body></html>)rawliteral";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
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

// Replaces placeholder with stored values
String processor(const String& var){
  //Serial.println(var);
  if(var == "inputString"){
    return readFile(SPIFFS, "/inputString.txt");
  }
  else if(var == "inputString2"){
    return readFile(SPIFFS, "/inputString2.txt");
  }
  return String();
}


void setup() {
  Serial.begin(115200);

  // Initialize SPIFFS
  #ifdef ESP32
    if(!SPIFFS.begin(true)){
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
    }
  #else
    if(!SPIFFS.begin()){
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
    }
  #endif

  delay(100);
  WiFi.mode(WIFI_STA);
  Serial.println("Connecting to ");
  Serial.println(ssid);

  //connect to your local wi-fi network
  WiFi.begin(ssid, password);

  //check wi-fi is connected to wi-fi network
  while (WiFi.status() != WL_CONNECTED) {
  delay(1000);
  Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected..!");

  if(!MDNS.begin("alertbox")) {
     Serial.println("Error starting mDNS");
     return;
  }
  
  Serial.print("Got IP: ");

  Serial.println(WiFi.localIP());

  // Send web page with input fields to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Send a GET request to <ESP_IP>/get?inputString=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    // GET inputString value on <ESP_IP>/get?inputString=<inputMessage>
    if (request->hasParam(PARAM_STRING)) {
      inputMessage = request->getParam(PARAM_STRING)->value();
      writeFile(SPIFFS, "/inputString.txt", inputMessage.c_str());
    }
    // GET inputString2 value on <ESP_IP>/get?inputString2=<inputMessage>
    else if (request->hasParam(PARAM_INT)) {
      inputMessage = request->getParam(PARAM_INT)->value();
      writeFile(SPIFFS, "/inputString2.txt", inputMessage.c_str());
    }
    else {
      inputMessage = "No message sent";
    }
    Serial.println(inputMessage);
    request->send(200, "text/text", inputMessage);
  });

  server.onNotFound(notFound);
  server.begin();
  Serial.println("HTTP server started");

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  twilio = new Twilio(account_sid, auth_token);
}

void loop() {

  Serial.print("Got IP: "); 
  Serial.println(WiFi.localIP());
  // To access your stored values on inputString, inputString2
  String yourInputString = readFile(SPIFFS, "/inputString.txt");
  Serial.print("*** Your inputString: ");
  Serial.println(yourInputString);
  
  String yourinputString2 = readFile(SPIFFS, "/inputString2.txt");
  Serial.print("*** Your inputString2: ");
  Serial.println(yourinputString2);

  int buttonState = digitalRead(BUTTON_PIN);
  // Serial.println(buttonState);
  String response;
  if (buttonState == LOW) {
    bool success = twilio->send_message(yourInputString, from_number, yourinputString2, response);
    Serial.println(yourInputString);
    if (success) {
      Serial.println("Message sent");
    } 
    else {
      Serial.println("Error sending message");
    }
    delay(1000); 
  }
}