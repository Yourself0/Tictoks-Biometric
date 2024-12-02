#include <Arduino.h>
#include <SD.h>
#include <SPIFFS.h>
#include <EEPROM.h>
#include <RTClib.h>
#include <stdlib.h>
#include <Wire.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_Fingerprint.h>

long OtpVerifiy;
bool WifiPage = false;
bool quicksetupCld = false;
bool RfidRegisterPage = false;
bool CompanyPage = false;
bool DeviceidPage = false;
bool RfidRegister = false;
bool wifiScanRequested = false;
bool UpdateEmployee = true;
bool WebsocketConnected = false;
bool RegistrationFinger = true;

int FingerPrintId = 0;

const char *ASSID = "Tictoks RF V2";
const char *APASS = "123456789";

#define Relay 4
#define Buzzer 15
#define REDLED 13
#define SS_1_PIN 2
#define REDLED1 33
#define RST_PIN 27
#define SS_2_PIN 26
#define GREENLED 12
#define ORANGELED 14
#define GREENLED1 25
#define NO_OF_READERS 2
#define WATCHDOG_TIMEOUT_SECONDS 20

String CompanyId = "";
String CompanyName = "";
String DeviceType = "RFID";
String DeviceId = "R1";
String json;
String DeviceList;
String servername = "www.google.com";
String RegEmpId = "";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
// AsyncWebSocket ws1("/Status");

RTC_DS3231 rtc;
WiFiClient client;

HardwareSerial serialPort(2); // use UART2
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&serialPort);

// Adafruit_Fingerprint finger = Adafruit_Fingerprint(&serialPort);

bool matchRfid(int id, int nemo);
uint8_t id;
uint8_t getNextFreeID();
uint8_t getFingerprintEnroll();
int getFingerprintID();
void SensorFingerBegin();
void printHex(int num, int precision);
int ServerSend(String RfId, String companyId);
uint8_t deleteFingerprint(uint8_t id);

uint8_t downloadFingerprintTemplate(uint16_t id)
{
  Serial.println("------------------------------------");
  Serial.print("Attempting to load #");
  Serial.println(id);
  uint8_t p = finger.loadModel(id);
  switch (p)
  {
  case FINGERPRINT_OK:
    Serial.print("Template ");
    Serial.print(id);
    Serial.println(" loaded");
    break;
  case FINGERPRINT_PACKETRECIEVEERR:
    Serial.println("Template Communication error");
    return p;
  default:
    Serial.print("Template error ");
    Serial.println(p);
    return p;
  }

  // OK success!

  Serial.print("Attempting to get #");
  Serial.println(id);
  p = finger.getModel();
  switch (p)
  {
  case FINGERPRINT_OK:
    Serial.print("Template ");
    Serial.print(id);
    Serial.println(" transferring:");
    break;
  default:
    Serial.print("Template Unknown error ");
    Serial.println(p);
    return p;
  }

  // one data packet is 267 bytes. in one data packet, 11 bytes are 'usesless' :D
  uint8_t bytesReceived[534]; // 2 data packets
  memset(bytesReceived, 0xff, 534);

  uint32_t starttime = millis();
  int i = 0;
  while (i < 534 && (millis() - starttime) < 20000)
  {
    if (serialPort.available())
    {
      bytesReceived[i++] = serialPort.read();
    }
  }
  Serial.print(i);
  Serial.println(" bytes read.");
  Serial.println("Decoding packet...");

  uint8_t fingerTemplate[512]; // the real template
  memset(fingerTemplate, 0xff, 512);

  // filtering only the data packets
  int uindx = 9, index = 0;
  memcpy(fingerTemplate + index, bytesReceived + uindx, 256); // first 256 bytes
  uindx += 256;                                               // skip data
  uindx += 2;                                                 // skip checksum
  uindx += 9;                                                 // skip next header
  index += 256;                                               // advance pointer
  memcpy(fingerTemplate + index, bytesReceived + uindx, 256); // second 256 bytes

  for (int i = 0; i < 512; ++i)
  {
    // Serial.print("0x");
    printHex(fingerTemplate[i], 2);
    // Serial.print(", ");
  }
  Serial.println("\ndone.");

  return p;

  /*
    uint8_t templateBuffer[256];
    memset(templateBuffer, 0xff, 256);  //zero out template buffer
    int index=0;
    uint32_t starttime = millis();
    while ((index < 256) && ((millis() - starttime) < 1000))
    {
    if (mySerial.available())
    {
      templateBuffer[index] = mySerial.read();
      index++;
    }
    }

    Serial.print(index); Serial.println(" bytes read");

    //dump entire templateBuffer.  This prints out 16 lines of 16 bytes
    for (int count= 0; count < 16; count++)
    {
    for (int i = 0; i < 16; i++)
    {
      Serial.print("0x");
      Serial.print(templateBuffer[count*16+i], HEX);
      Serial.print(", ");
    }
    Serial.println();
    }*/
}

void printHex(int num, int precision)
{
  char tmp[16];
  char format[128];

  sprintf(format, "%%.%dX", precision);

  sprintf(tmp, format, num);
  Serial.print(tmp);
}

void searchFingerprint()
{
  Serial.println("Place your finger on the sensor...");
  int result = finger.getImage();

  if (result != FINGERPRINT_OK)
  {
    Serial.println("Failed to capture fingerprint image");
    return;
  }

  result = finger.image2Tz();
  if (result != FINGERPRINT_OK)
  {
    Serial.println("Failed to convert image");
    return;
  }

  result = finger.fingerFastSearch();
  if (result == FINGERPRINT_OK)
  {
    Serial.print("Found a match! ID: ");
    Serial.print(finger.fingerID);
    Serial.print(" with confidence: ");
    Serial.println(finger.confidence);
  }
  else if (result == FINGERPRINT_NOTFOUND)
  {
    Serial.println("No match found");
  }
  else
  {
    Serial.println("Error searching for fingerprint");
  }
}
// WebSocket event Handle
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
               AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
  case WS_EVT_CONNECT:
    WebsocketConnected = true;
    ws.textAll("Connected");
    Serial.println("Websocket is true now");
    Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());

    break;
  case WS_EVT_DISCONNECT:
    WebsocketConnected = false;
    ws.textAll("Disconnected");
    Serial.println("Websocket is not False");
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
    break;
  case WS_EVT_DATA:
    Serial.print("handle incomming data");
    // Handle incoming data
    break;
  case WS_EVT_PONG:
    Serial.print("Pong message");
    // Handle a pong message
    break;
  case WS_EVT_ERROR:
    Serial.print("error ");
    // Handle an error
    break;
  }
}

void InitializeRTC()
{
  Wire.begin();
  if (!rtc.begin())
  {
    Serial.println("RTC is NOT RUNNING");
  }
  else
  {
    Serial.println("RTC is RUNNING");
    DateTime now = rtc.now();
    Serial.print(now.year());
    Serial.print(now.month());
    Serial.print(now.day());
  }
}

void testFingerprintSensor()
{
  uint8_t templateCount = finger.getTemplateCount();
  if (templateCount == 0xFF)
  {
    Serial.println("Communication error with sensor.");
  }
  else
  {
    Serial.print("Number of templates stored: ");
    Serial.println(templateCount);
  }
}

void checkAllTemplates()
{
  Serial.println("Checking all stored templates...");
  for (int i = 1; i <= 255; i++)
  { // Assuming IDs from 1 to 255
    if (finger.loadModel(i) == FINGERPRINT_OK)
    {
      Serial.print("Template ID ");
      Serial.print(i);
      Serial.println(" is stored.");
    }
  }
  Serial.println("Finished checking all templates.");
}

void WifiConnectCheck()
{
  EEPROM.begin(512);
  String wssid = "";
  String wpass = "";
  for (int i = 0; i < 32; ++i)
  {
    if (EEPROM.read(i) != 255)
    {
      if (EEPROM.read(i) != 0)
      {
        wssid += char(EEPROM.read(i));
      }
    }
  }
  for (int i = 32; i < 94; ++i)
  {
    if (EEPROM.read(i) != 255)
    {
      if (EEPROM.read(i) != 0)
      {
        wpass += char(EEPROM.read(i));
      }
    }
  }
  if (wssid.length() > 0)
  {
    Serial.print("WiFi : ");
    Serial.println(wssid);
    Serial.print("Pass :");
    Serial.println(wpass);
    WiFi.begin("LOOM SOLAR / BEE Solars Power", "09791181639");
    int count = 0;

    while (WiFi.status() != WL_CONNECTED && count < 10)
    { // Adjust attempts as needed
      vTaskDelay(pdMS_TO_TICKS(500));
      // delay(500); // Reduced delay for faster attempts
      count++;
      Serial.println("Connecting to WiFi..");
      esp_task_wdt_reset();
    }
    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("Connected to WiFi");
      Serial.print("IP Address ");
      Serial.println(WiFi.localIP());
      WiFi.scanDelete();
    }
    else
    {
      WiFi.scanDelete();
      Serial.println("Not Connected");
    }
  }
  else
  {
    Serial.println("WiFi was not registered");
  }
}

void CompanyIdCheck()
{
  EEPROM.begin(512);
  Serial.println("companyid check");
  String cmpid;
  String cmpName;
  for (int i = 96; i < 100; ++i)
  {
    if (EEPROM.read(i) != 255)
    {
      if (EEPROM.read(i) != 0)
      {
        cmpid += char(EEPROM.read(i));
      }
    }
    Serial.println(EEPROM.read(i));
  }
  for (int i = 100; i < 164; ++i)
  {
    if (EEPROM.read(i) != 255)
    {
      if (EEPROM.read(i) != 0)
      {
        cmpName += char(EEPROM.read(i));
      }
    }
  }
  if (cmpid.length() <= 0)
  {
    Serial.println("Company Id is empty");
  }
  else
  {
    Serial.print("Company Id : ");
    Serial.println(cmpid);
    Serial.print("Company Name : ");
    Serial.println(cmpName);
    CompanyId = cmpid;
    CompanyName = cmpName;
    Serial.println("here");
  }
}

void OfflineDataWrite(String empId)
{
  // mountinSD();
  String Date;
  String Time;
  // String csvData;
  Serial.println("Offline Data Write Inside");
  File file = SD.open("/OfflineData.csv", "a"); // Open the file in write mode (overwrite existing content)
  if (!file)
  {
    Serial.println("Failed to open file for writing");
    return;
  }
  Serial.println("Opens CSV file");
  DateTime now = rtc.now();
  String currentDate = String(now.year()) + "-" + String(now.month(), DEC) + "-" + String(now.day());
  String currentTime = String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second());
  String csvData = empId + String(",") + currentDate + String(",") + currentTime + String("\n");
  Serial.println("Data written");
  Serial.print("CSV Data ");
  Serial.println(csvData);
  file.println(csvData); // Write data to a new line
  file.close();
  Serial.println("File closed");
}

void RestartEsp()
{
  ESP.restart();
}

void rtc_wdt_protect_off()
{
}

void mountinSD()
{
  if (!SD.begin(5))
  {
    Serial.println("Card Mount Failed");
  }
  else
  {
    Serial.println("Card Mount Success");
  }
}

void UnauthorizedAccess()
{
  for (int i = 0; i < 2; i++)
  {                    // Repeat the pattern 3 times
    tone(Buzzer, 500); // Play the beep tone
    delay(100);        // Wait for the specified duration
    noTone(Buzzer);    // Stop the tone
    delay(100);
  }
}

void DeviceIdInitialize()
{
  EEPROM.begin(512);
  String deviceId = "";
  for (int i = 166; i <= 170; ++i)
  {
    if (EEPROM.read(i) != 0)
    {
      if (EEPROM.read(i) != 255)
      {
        deviceId += char(EEPROM.read(i));
        Serial.println("Device ID:");
        Serial.print(EEPROM.read(i));
        Serial.println(deviceId);
      }
    }
  }
  if (deviceId.length() > 0)
  {
    DeviceId = deviceId;
    Serial.println("Device Id was there");
  }
  else
  {
    Serial.println("Device id Not there");
  }
}

void rfidInitialList()
{
  rtc_wdt_protect_off();
  String rfidList = "";
  Serial.print("Rfid Initial Lists ");
  if (WiFi.status() == WL_CONNECTED)
  {
    if (client.connect("www.google.com", 80))
    {
      DeviceIdInitialize();
      HTTPClient http;
      int dataCount = 0;
      int startCount = 0;
      int endCount = 10;
      mountinSD();
      int falseReturn = 0;
      do
      {
        digitalWrite(REDLED, HIGH);
        digitalWrite(GREENLED, HIGH);
        digitalWrite(REDLED1, HIGH);
        digitalWrite(GREENLED1, HIGH);
        Serial.print("inside do while loop");
        DynamicJsonBuffer jsonBuffer;
        JsonObject &JSONEncoder = jsonBuffer.createObject();
        JSONEncoder["companyId"] = CompanyId;
        JSONEncoder["deviceId"] = DeviceId;
        JSONEncoder["startCount"] = startCount;
        JSONEncoder["endCount"] = endCount;
        char JSONmessageBuffer[500];
        JSONEncoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
        Serial.print("RFID Initial list");
        esp_task_wdt_reset();
        yield();
        Serial.println(JSONmessageBuffer);
        http.begin("https://wildfly.tictoks.in:443/EmployeeAttendenceAPI/RFIDAPI/SelectAllEmployeeInfo");
        esp_task_wdt_reset();
        http.addHeader("Content-type", "application/json");
        int response = http.POST(JSONmessageBuffer);
        Serial.print("HTTP code");
        Serial.println(response);
        String payload = http.getString();
        if (response == 200)
        {
          yield();
          esp_task_wdt_reset();
          Serial.print("Payload:");
          Serial.println(payload);
          esp_task_wdt_reset(); // Reset the Watchdog Timer  // Reset the Watchdog Timer
          char *payloadBuffer;  // Adjust the size based on your requirement
          payload.toCharArray(payloadBuffer, sizeof(payloadBuffer));
          esp_task_wdt_reset();
          Serial.println("response:" + payload);
          JsonObject &root = jsonBuffer.parseObject(payload); // Parse the JSON payload dynamically
          yield();
          if (root.success() && root.size() > 0)
          {
            String dataCountStr = root["dataCount"].as<char *>();
            esp_task_wdt_reset();
            dataCount = dataCountStr.toInt();
            startCount += endCount;
            Serial.print("root Size :");
            Serial.print(root.size());
            Serial.print("root:");
            root.printTo(Serial);
            Serial.println("");
            JsonArray &employeeInfoList = root["employeeInfoList"];
            Serial.print("Emp rfid size ");
            root.remove("employeeInfoList");
            Serial.println(employeeInfoList.size());
            int node_length = employeeInfoList.size();
            for (int i = 0; i < node_length; i++)
            {
              File dataFile = SD.open("/Rfid.csv", "a");
              String empId = employeeInfoList[i]["employeeId"].as<const char *>();
              String Name = employeeInfoList[i]["name"].as<const char *>();
              // String DeviceIdM = employeeInfoList[i]["deviceId"].as<const char *>();                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    y"].as<const char *>();
              String Status = employeeInfoList[i]["status"].as<const char *>();
              String Department = employeeInfoList[i]["department"].as<const char *>();
              String rfidNo = employeeInfoList[i]["rfidNo"].as<const char *>();
              char buf[sizeof(Status)];
              if (rfidNo == "" || rfidNo == " ")
              {
                rfidNo = "-";
              }
              int Matched = 0;
              // DeviceIdM.toCharArray(buf, sizeof(buf));
              Serial.print("Device Id: ");
              Serial.println(Status);
              // char *p = const_cast<char *>(DeviceIdM.c_str());
              // ;
              // char *str;
              // while ((str = strtok_r(p, ",", &p)) != NULL)
              // {
              //   Serial.print("str: ");
              //   Serial.println(str);

              //   if (strcmp(str, DeviceId.c_str()) == 0)
              //   {
              //     Matched = 1; // Set Matched to 1 if any match is found
              //   }
              Matched = Status.toInt();
              Serial.print("Matched Value: ");
              Serial.println(Matched);
              Serial.print("Device ID: ");
              // Serial.println(str);
              Serial.println("");

              // while ((str = strtok_r(p, ",", &p)) != NULL)
              // {
              //   Serial.print("str: ");
              //   Serial.println(str);
              //   if (strcmp(str, DeviceId.c_str()) == 0)
              //   {
              //     Matched = 1;
              //   }
              //   else{
              //     Matched = 0;
              //   }
              //   Serial.print("Matched Value : ");
              //   Serial.println(Matched);
              //   Serial.print("Device ID :");
              //   Serial.println(str);
              //   Serial.println("");
              // }
              String empData = rfidNo + String(",") + Matched + String(",") + empId + String(",") + Name + String(",") + Department + String("\n");
              Serial.print("Empid data: ");
              Serial.println(empData);
              dataFile.print(empData);
              dataFile.close();
            }
          }
          else
          {
            Serial.print("root not success");
          }
        }
        else
        {
          Serial.print("HTTP request failed :");
          falseReturn += 1;
          esp_task_wdt_reset();
          Serial.println(response);
        }
        if (falseReturn > 8)
        {
          SD.remove("/Rfid.csv");
          esp_task_wdt_reset();
          Serial.print("Getting Error SD File Rfid removed");
          digitalWrite(REDLED, LOW);
          digitalWrite(GREENLED, HIGH);
          digitalWrite(REDLED1, LOW);
          digitalWrite(GREENLED1, HIGH);
          break;
        }
        http.end();
      } while (dataCount >= startCount - 1);
    }
  }
  else
  {
    Serial.println("Not connected");
  }
  Serial.println("Rfid List");
  digitalWrite(REDLED, LOW);
  digitalWrite(GREENLED, HIGH);
  digitalWrite(REDLED1, LOW);
  digitalWrite(GREENLED1, HIGH);
}

void rfidInitialCheck()
{
  mountinSD();
  if (!SD.exists("/Rfid.csv"))
  {
    Serial.println("Rfid register File Doesn't Exist");
    rfidInitialList();
  }
  else
  {
    File DataFile = SD.open("/Rfid.csv");
    size_t fileSize = DataFile.size();
    Serial.println("Rfid Register File Exists :");
    Serial.println(fileSize);
    if (fileSize == 0)
    {
      DataFile.close();
      SD.remove("/Rfid.csv");
      Serial.println("File Removed");
    }
    else
    {
      Serial.print("File Already There");
      DataFile.close();
    }
  }
}

void DeviceidFetch()
{
  HTTPClient http;
  int httpCode = 0;
  if (WiFi.status() == WL_CONNECTED)
  {
    if (client.connect("www.google.com", 80))
    {
      RegistrationFinger = false;
      DynamicJsonBuffer JsonBuffer;
      Serial.println("Device Id Fetching");
      JsonObject &JsonEncoder = JsonBuffer.createObject();
      char JSONcharMessage[500];
      JsonEncoder["companyId"] = CompanyId;
      JsonEncoder["deviceType"] = DeviceType;
      JsonEncoder.printTo(JSONcharMessage, sizeof(JSONcharMessage));
      http.begin("https://wildfly.tictoks.in:443/EmployeeAttendenceAPI/RFIDAPI/GetRFIDUnMappedDeviceList");
      yield();
      http.addHeader("Content-type", "application/json");
      httpCode = http.POST(JSONcharMessage);
      String payload = http.getString();
      yield();
      Serial.print("Http Response:");
      Serial.println(httpCode);
      Serial.print("Device Id: ");
      Serial.println(payload);
      DeviceList = "";
      if (httpCode == 200)
      {
        JsonObject &root = JsonBuffer.parseObject(payload);
        if (root.success())
        {
          JsonArray &rfidDeviceList = root["rfidDeviceList"];
          rfidDeviceList.printTo(DeviceList);
        }
      }
      http.end();
    }
  }
}

long TokenVerifications()
{
  long randomNumber = random(100000, 1000000);
  return randomNumber;
}

void ResetWebserverPages()
{
  WifiPage = false;
  RfidRegisterPage = false;
  CompanyPage = false;
  DeviceidPage = false;
}

void SendRegisterData(String Data)
{
  ws.textAll(Data);
}
// AsyncWebServer code for webpage
void WebServerRoutes()
{
  if (SPIFFS.begin(true))
  {
    Serial.println("SPIFFS Beginned");
  }
  // Route for root / web page
  // server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
  //             // if (request->client()->remoteIP().toString().startsWith("192.168.4.")) {
  //             Serial.println("Main page requested");
  //             RfidRegister = false;
  //             OtpVerifiy = TokenVerifications();
  //             ResetWebserverPages();
  //             request->send(200, "text/html", Mainpage);//}
  //             // else{
  //               // request->send(403, "text/html", "Access Forbidden");
  //             // }
  //           });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("Main page requested");
    // Debug Information 
    Serial.print("Rfid Register: ");
    Serial.println(RfidRegister);
    RfidRegister = false;
    RegistrationFinger = true;
    OtpVerifiy = TokenVerifications();
    Serial.print("Otp Verifiy: ");
    Serial.println(OtpVerifiy);
    ResetWebserverPages();
    Serial.println("Reset Webserver pages called");

    // checking if Mainpage is properly intialized
    // if(Mainpage == nullptr || strlen(Mainpage) == 0){
    //   Serial.println("Main page is not initialized properly");
    //   request->send(500,"text/plain","Server error: Maing page is not available");
    //   return;
    // }
    request->send(SPIFFS, "/Mainpage.html", "text/html"); 
    Serial.println("Response sent"); });

  // server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
  //           {
  //   RfidRegister = false;
  //   request->send(200, "text/html", Mainpage); });
  // Images
  server.on("/deleteImg", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/deleteImg.png", "image/png"); });
  server.on("/companyImg", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/companyImg.png", "image/png"); });
  server.on("/back", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/back.png", "image/png"); });
  server.on("/popImg", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/popImg.png", "image/png"); });
  server.on("/resetImg", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/resetImg.png", "image/png"); });
  server.on("/restartImg", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/restartImg.png", "image/png"); });
  server.on("/RFIDImg", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/RFIDImg.png", "image/png"); });
  server.on("/successImg", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/successImg.png", "image/png"); });
  server.on("/TictokLogo", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/TictokLogo.png", "image/png"); });
  server.on("/updateImg", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/updateImg.png", "image/png"); });
  server.on("/wifiImg", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/wifiImg.png", "image/png"); });
  server.on("/QuickSettings", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/QuickSettings.png", "image/png"); });
  server.on("/BiometricImg", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/Biometric.svg", "image/svg+xml"); });

  server.on("/quickSetting.js", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("quick setup called");
    if (SPIFFS.exists("/quickSetting.js")) {
        request->send(SPIFFS, "/quickSetting.js", "application/javascript");
    } else {
        request->send(404, "text/plain", "File not found");
    } });

  // server.on("/quickSetting.js", HTTP_GET, [](AsyncWebServerRequest *request)
  //           {
  //       Serial.print("quick setup called");
  //       if (SPIFFS.exists("/quickSetting.js")) {
  //           request->send(SPIFFS, "/quickSetting.js", "text/plain");
  //       } else {
  //           request->send(404, "text/plain", "File not found");
  //       } });

  // CSS Pages Com.
  server.on("/MainPageCss", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/MainPageCss.css", "text/css"); });
  // Device Id
  server.on("/Deviceid", HTTP_GET, [](AsyncWebServerRequest *request)
            { 
              if(DeviceidPage){
              DeviceidFetch();
             request->send(SPIFFS, "/Deviceid.html", "text/html");}
             else{
              request->send(200,"text/plane","UnAuthorised Access");
             } });
  server.on("/DeviceIDList", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plane", DeviceList); });
  server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/styles.css", "text/css"); });
  server.on("/popupCss", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/popupCss.css", "text/css"); });
  server.on("/OtpVerify", HTTP_GET, [](AsyncWebServerRequest *request)

            { request->send(200, "text/plain", String(OtpVerifiy)); });
  server.on("/OtpVerificationChkQuick", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
            {
    String VerificationsMsg = "Message Verified";
    String OtpVerificationCheck;
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.parseObject(data);
    if (json.success()) {
      OtpVerificationCheck = json["OtpVerify"].as<char*>();
      if (String(OtpVerifiy).equals(OtpVerificationCheck)) {
        Serial.println("Strings are equal quick setup");
        request->send(200,"text/json","Ok");
        quicksetupCld = true;
        if(quicksetupCld){
          WifiPage = true;
          CompanyPage = true;
          DeviceidPage = true;
        }

    } else {
        request->send(400,"text/json","Not_Ok");
        WifiPage = false;
        CompanyPage = false;
        DeviceidPage = false;
        Serial.println("Strings are not equal");
    }
    } });
  server.on("/OtpVerificationChkWifiPage", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
            {
    String VerificationsMsg = "Message Verified";
    String OtpVerificationCheck;
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.parseObject(data);
    Serial.println("Called inside the wifi page ");
    if (json.success()) {
      OtpVerificationCheck = json["OtpVerify"].as<char*>();
      if (String(OtpVerifiy).equals(OtpVerificationCheck)) {
        Serial.println("Strings are equal Wifi page");
        request->send(200,"text/json","Ok");
        WifiPage = true;
    } else {
        request->send(400,"text/json","Not_Ok");
        Serial.println("Strings are not equal");
    }
    } });
  server.on("/OtpVerificationChkCompanyPage", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
            {
    String VerificationsMsg = "Message Verified";
    String OtpVerificationCheck;
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.parseObject(data);
    if (json.success()) {
      OtpVerificationCheck = json["OtpVerify"].as<char*>();
      if (String(OtpVerifiy).equals(OtpVerificationCheck)) {
        Serial.println("Strings are equal Company page");
        request->send(200,"text/json","Ok");
        CompanyPage = true;
    } else {
        request->send(400,"text/json","Not_Ok");
        Serial.println("Strings are not equal");
    }

    } });
  server.on("/OtpVerificationChkDeviceidPage", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)

            {

    String VerificationsMsg = "Message Verified";

    String OtpVerificationCheck;

    DynamicJsonBuffer jsonBuffer;

    JsonObject& json = jsonBuffer.parseObject(data);

    if (json.success()) {

      OtpVerificationCheck = json["OtpVerify"].as<char*>();

      if (String(OtpVerifiy).equals(OtpVerificationCheck)) {

        Serial.println("Strings are equal Device id page");

        request->send(200,"text/json","Ok");

        DeviceidPage = true;

    } else {

        request->send(400,"text/json","Not_Ok");

        Serial.println("Strings are not equal");

    }

    } });
  server.on("/OtpVerificationChkRfidRegisterPage", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)

            {

    String VerificationsMsg = "Message Verified";

    String OtpVerificationCheck;

    DynamicJsonBuffer jsonBuffer;

    JsonObject& json = jsonBuffer.parseObject(data);

    if (json.success()) {

      OtpVerificationCheck = json["OtpVerify"].as<char*>();

      if (String(OtpVerifiy).equals(OtpVerificationCheck)) {

        Serial.println("Strings are equal Rfid register page");

        request->send(200,"text/json","Ok");

        RfidRegisterPage = true;



    } else {

        request->send(400,"text/json","Not_Ok");

        Serial.println("Strings are not equal");

    }

    } });
  // server.on("/Update", HTTP_GET, [](AsyncWebServerRequest *request)
  // Wifi Settings Html and Css

  server.on("/WifiSetting", HTTP_GET, [](AsyncWebServerRequest *request)
            { 
              if(WifiPage){
              request->send(SPIFFS, "/WifiSetting.html", "text/html");
              }
              else{

                request->send(200,"text/plane","Unauthorised access");
              } });
  server.on("/WifiCss.css", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/WifiCss.css", "text/css"); });
  // Company setting page
  server.on("/CompanySetting", HTTP_GET, [](AsyncWebServerRequest *request)

            {

              if(CompanyPage){

              request->send(SPIFFS, "/CompanySetting.html", "text/html"); }

              else{

                request->send(200,"text/plain","Unauthorised Access");

              } });
  server.on("/CompanySettingCss.css", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/CompanySettingCss.css", "text/css"); });
  server.on("/Reset", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/Reset.html", "text/html"); });
  server.on("/quickSetting.js", HTTP_GET, [](AsyncWebServerRequest *request)
            {
        Serial.print("quick setup called");
        if (SPIFFS.exists("/quickSetting.js")) {
            request->send(SPIFFS, "/quickSetting.js", "text/plain");
        } else {
            request->send(404, "text/plain", "File not found");
        } });
  // RFID Page
  server.on("/upload", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/upload.html", "text/html"); });
  server.on("/Biometric", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              RfidRegisterPage = true;
              RegistrationFinger = false;
              if(RfidRegisterPage){
                RfidRegister = true;
                // rfidInitialCheck();
                request->send(SPIFFS, "/Biometric.html", "text/html");}
    else{
      request->send(200,"text/plane","UnAuthorised Access");
    } });
  server.on("/RFID.css", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/RFID.css", "text/css"); });
  // Restart Page ...
  server.on("/Restart", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    RestartEsp();
    request->send(200, "text/json", "Restart"); });
  // Reset page ...
  server.on("/Resets", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              Serial.println("Inside reset page ");
              EEPROM.begin(512);
              mountinSD();
              // Clearing specific sections of EEPROM memory
              for (int i = 0; i <= 95; ++i)
              {
                EEPROM.write(i, 0);
              }
              EEPROM.commit();
              for (int i = 99; i <= 170; ++i)
              {
                EEPROM.write(i, 0);
              }
              EEPROM.commit();
              if (SD.exists("/EmpRfid.csv"))
              {
                if (SD.remove("/EmpRfid.csv"))
                {
                  Serial.println("EmpRfid.csv Deleted Successfully");
                }
                else
                {
                  Serial.println("Failed to Delete EmpRfid.csv");
                }
              }
              else
              {
                Serial.println("EmpRfid.csv does not exist");
              }
              if (SD.exists("/Rfid.csv"))
              {
                if (SD.remove("/Rfid.csv"))
                {
                  Serial.println("Rfid.csv Deleted Successfully");
                }
                else
                {
                  Serial.println("Failed to Delete Rfid.csv");
                }
              }
              else
              {
                Serial.println("Rfid.csv does not exist");
              }
              request->send(200, "text/json", "reset");
              RestartEsp(); });
  server.on("/Reset", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("Inside reset page ");
    EEPROM.begin(512);
    mountinSD();
    // Clearing specific sections of EEPROM memory
    for (int i = 0; i <= 95; ++i) {
      EEPROM.write(i, 0);
    }
    EEPROM.commit();
    for (int i = 99; i <= 170; ++i) {
      EEPROM.write(i, 0);
    }
    EEPROM.commit();
    // Attempting to remove files from SPIFFS
    if (SD.exists("/EmpRfid.csv")) {
      if (SD.remove("/EmpRfid.csv")) {
        Serial.println("EmpRfid.csv Deleted Successfully");
      } else {
        Serial.println("Failed to Delete EmpRfid.csv");
      }
    } else {
      Serial.println("EmpRfid.csv does not exist");
    }
    if (SD.exists("/Rfid.csv")) {
      if (SD.remove("/Rfid.csv")) {
        Serial.println("Rfid.csv Deleted Successfully");
      } else {
        Serial.println("Failed to Delete Rfid.csv");
      }
    } else {
      Serial.println("Rfid.csv does not exist");
    }
    RestartEsp();
    request->send(200, "text/json", "Restart"); });
  // Update the Employee Details ...
  server.on("/Update", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    int statuss = 1;//fetchEmployeeDetails();
    Serial.print("Status");
    Serial.println(statuss);
    if (statuss > 0) {
              SD.remove("/EmpRfid.csv");
              SD.remove("/Rfid.csv");
              RestartEsp();
      request->send(200, "text/json", "Update");
    } });
  // Submit data for companyreg id...
  server.on("/submit-data", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
            {
    EEPROM.begin(512);
    String companyId = "";
    String companyName = "";
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.parseObject(data);
    if (json.success()) {
      String dates = json["date"];
      companyId = json["companyId"].as<char*>();
      companyName = json["companyName"].as<char*>();
      Serial.println("Submit button pressed");
      Serial.println("Received date: " + dates);
      Serial.println("Received companyId: " + companyId);
      Serial.println("Received companyName: " + companyName);
      // Inside your POST handler
      String Time = dates;
      int first = Time.indexOf('-');
      int secon = Time.indexOf('-', first + 1);
      int firstT = Time.indexOf('T');
      int firsCol = Time.indexOf(':');
      int secoCol = Time.indexOf(':', firsCol + 1);
      String year = Time.substring(0, first);
      String month = Time.substring(first + 1, secon);
      String day = Time.substring(secon + 1, firstT);
      String hour = Time.substring(firstT + 1, firsCol);
      String minute = Time.substring(firsCol + 1, secoCol);
      String seconds = Time.substring(secoCol + 1);
      Serial.println(year);
      Serial.println(month);
      Serial.println(day);
      Serial.println(hour);
      Serial.println(minute);
      Serial.println(seconds);
      // Assuming year, month, day, hour, minute, and second are String objects
      int yearInt = year.toInt();
      int monthInt = month.toInt();
      int dayInt = day.toInt();
      int hourInt = hour.toInt();
      int minuteInt = minute.toInt();
      int secondInt = seconds.toInt();
      Wire.begin();
      if (!rtc.begin()) {
        Serial.println("RTC is NOT RUNNING");
      } else {
        Serial.println("RTC is RUNNING");
        DateTime now = rtc.now();
        rtc.adjust(DateTime(yearInt, monthInt, dayInt, hourInt, minuteInt, secondInt));
        Serial.print("RTC Time :");
        //DateTime now =File opened successfully
        rtc.now();
        Serial.println(now.year());
        Serial.println(now.month());
        Serial.println(now.day());
        Serial.println(now.hour());
        Serial.println(now.minute());
        Serial.println(now.second());
      }
      Serial.println("about");
      //      String CmpId="";
      //      String CompanyName;
      bool cmpIdnotEmpty = false;
      Serial.println("Clearing EEPROM");
      for (int i = 96; i < 165; ++i) {
        EEPROM.write(i, 0);
      }
      if (companyId.length() > 0) {
        for (int i = 0; i < 3; ++i) {
          EEPROM.write(i + 96, companyId[i]);
          Serial.print(i);
          Serial.println(companyId[i]);
          //cmpIdnotEmpty = true;
        }
      }
      if (companyName.length() > 0) {
        for (int i = 0; i < companyName.length(); ++i) {
          EEPROM.write(i + 100, companyName[i]);
          Serial.println(companyName[i]);
        }
      }
      EEPROM.commit();
      Serial.println("Reading from EEPROM:");
      for (int i = 96; i < 165; ++i) {
        Serial.print(EEPROM.read(i));
        Serial.print(" ");
      }
      CompanyIdCheck();
      Serial.print("Company Id :");
      Serial.println(CompanyId);
      Serial.print("Company Name :");
      Serial.println(CompanyName);
      request->send(200, "application/json", "{\"message\":\"Data received successfully\"}");
      DeviceidFetch();
    }
    else {
      request->send(400, "application/json", "{\"message\":\"Parsing JSON failed\"}");
    } });
  // Device ID Submit & Fetching employee
  server.on("/Device_id", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
            {
        Serial.println("button pressed");
        HTTPClient http;
        int httpCount = 0;
        DynamicJsonBuffer jsonBuffer;
        JsonObject &json = jsonBuffer.parseObject(data);
        if (json.success())
        {
          String deviceId = json["DeviceId"];
          Serial.println(deviceId);
          Serial.print(deviceId.length());
          EEPROM.begin(512);
          if (deviceId.length() > 0)
          {
            Serial.print("here");
            for (int i = 166; i <= 170; ++i)
            {
              Serial.print("here1");
              EEPROM.write(i, 0);
            }
            EEPROM.commit();
            for (int i = 0; deviceId.length() > i; ++i)
            {
              Serial.print("here2");
              EEPROM.write(i + 166, deviceId[i]);
              Serial.print("Device Id :");
              Serial.println(deviceId[i]);
            }
            EEPROM.commit();
          }
          Serial.print("Calling Function");
          DeviceIdInitialize();
          DynamicJsonBuffer jsonBuffers;
          JsonObject &JsonEncoder = jsonBuffers.createObject();
          JsonEncoder["companyId"] = CompanyId;
          JsonEncoder["deviceId"] = DeviceId;
          char JsonBufferMessage[500];
           JsonEncoder.printTo(JsonBufferMessage, sizeof(JsonBufferMessage));
          Serial.println(JsonBufferMessage);
          http.begin("https://wildfly.tictoks.in:443/EmployeeAttendenceAPI/RFIDAPI/RFIDDoorMapping");
          http.addHeader("Content-type", "application/json");
          httpCount = http.POST(JsonBufferMessage);
          String Payload = http.getString();
          Serial.print("Payload :");
          Serial.println(Payload);
          if (httpCount == 200)
          {
            Serial.println("Successfully sent");
            UpdateEmployee = true;
          }
          else
          {
            Serial.println("Not sent");
          }
        }
        else
        {
          Serial.println("Device id Was not entered");
        }
        http.end();
        request->send(200, "text/plane", "Got the Device id"); });
  // Sends WiFi details ...
  server.on("/get-wifi-data", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("Received /get-wifi-data request");
    if (!wifiScanRequested) //get-wifi-data request
    { 
      wifiScanRequested = true; // Set the request flag
      // xTaskNotify(WiFiscanTaskHandle, 1, eSetValueWithOverwrite); // Notify the task      wifiScantaskDelete();
      request->send(200, "text/plain", json);      
    } });
  // server.on("/EmployeeLists", HTTP_GET, [](AsyncWebServerRequest *request)
  //           {
  //   File csv_File = SD.open("/Rfid.csv", "r");
  //   if (csv_File) {
  //     Serial.print("Opend csv File");
  //     request->send(SD, "/Rfid.csv", "text/csv");
  //     csv_File.close();
  //   }
  //   else {
  //     Serial.print("File not Found");
  //     request->send(404, "text/plain", "File Not Found");
  //   } });

  server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request) {}, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
            {
    if (!index) {
      filename = "Rfid.csv";
      Serial.printf("UploadStart: %s\n", filename.c_str());
      if (SPIFFS.exists("/Rfid.csv")) {
        SPIFFS.remove("/Rfid.csv");
      }
      request->_tempFile = SPIFFS.open("/Rfid.csv", "w");
    }
    if (request->_tempFile) {
      if (len) {
        request->_tempFile.write(data, len);
      }
      if (final) {
        request->_tempFile.close();
        Serial.printf("UploadEnd: %s, %u B\n", filename.c_str(), index + len);
        request->send(200, "text/plain", "File Uploaded Successfully");
      }
    } else {
      request->send(500, "text/plain", "File Upload Failed");
    } });
  server.on("/EmployeeLists", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    File csv_File = SPIFFS.open("/Rfid.csv", "r");
    if (csv_File) {
      Serial.print("Opend csv File");
      request->send(SPIFFS, "/Rfid.csv", "text/csv");
      csv_File.close();
    }
    else {
      Serial.print("File not Found");
      request->send(404, "text/plain", "File Not Found");
    } });
  // server.on("/EmployeeRfidLists", HTTP_GET, [](AsyncWebServerRequest *request)
  //           {
  //   File csv_File = SD.open("/EmpRfid.csv", "r");
  //   if (csv_File) {
  //     Serial.print("Opend csv File");
  //     request->send(SD, "/EmpRfid.csv", "text/csv");
  //     csv_File.close();
  //   }
  //   else {
  //     Serial.print("File not Found");
  //     request->send(404, "text/plain", "FIle Not Found");
  //   } });
  server.on("/EmployeeRfidLists", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    File csv_File = SPIFFS.open("/EmpRfid.csv", "r");
    if (csv_File) {
      Serial.print("Opend csv File");
      request->send(SPIFFS, "/EmpRfid.csv", "text/csv");
      csv_File.close();
    }
    else {
      Serial.print("File not Found");
      request->send(404, "text/plain", "FIle Not Found");
    } });
  server.on("/OfflineEmpLists", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    File csv_File = SD.open("/OfflineData.csv", "r");
    if (csv_File) {
      Serial.print("Opend csv File");
      request->send(SD, "/OfflineData.csv", "text/csv");
      csv_File.close();
    }
    else {
      Serial.print("File not Found");
      request->send(404, "text/plain", "FIle Not Found");
    } });
  server.on("/DeleteSpiffsFiles", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              SPIFFS.remove("/EmpRfid.csv");
              SPIFFS.remove("/Rfid.csv");
              request->send(200, "text/plane", "Deleted"); });
  server.on("/DeleteOfflineFiles", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              SD.remove("/OfflineData.csv");
              // SD.remove("/Rfid.csv");
              request->send(200, "text/plane", "Deleted"); });
  server.on("/Wifi_submit", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
            {
    EEPROM.begin(512);
    String ssid = "";
    String pass = "";
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.parseObject(data);
    if (json.success()) {
      ssid = json["SSID"].as<char*>();
      pass = json["PASSWORD"].as<char*>();
      Serial.println(ssid);
      Serial.println(pass);
      if (ssid.length() > 0) {
        Serial.println("Clearing EEPROM");
        for (int i = 0; i < 94; ++i) {
          EEPROM.write(i, 0);
        }
        Serial.println("Writting SSID and Password to eeprom");
        for (int i = 0; i < ssid.length(); ++i) {
          EEPROM.write(i, ssid[i]);
        }
        for (int i = 0; i < pass.length(); ++i) {
          EEPROM.write(i + 32, pass[i]);
        }
      }
    }
    EEPROM.commit();
    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.print("sent wifi connected");
      request->send(200, "text/plain", "WiFi submited");
    }
    else
    {
    WifiConnectCheck();
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print("inside Loop");
      WifiConnectCheck();
      delay(100);
      if (WiFi.status() == WL_CONNECTED)
      {
        Serial.print("sent wifi connected 1");
        request->send(200, "text/plain", "WiFi submited");
      }
    }
    }
    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.print("sent wifi connected");
      request->send(200, "text/plain", "WiFi submited");
    } });
  // Updated Rfid
  server.on("/RfiduidUpdate", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
            { Serial.println("Rfid Update called"); });

  // Bio metric Scan processes

  /*
  server.on("/start-registration", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Biometric scan Started");
    ws.textAll("Hello");
    yield();
    int p = -1;
    int count = 0;
    Serial.print("Waiting for valid finger to enroll as #");
    Serial.println(id);
    yield();

    // Capture first image
    while (p != FINGERPRINT_OK) {
      p = finger.getImage();
      Serial.print("Count");
      Serial.println(count);
      switch (p) {
        case FINGERPRINT_OK:
          Serial.println("Image taken");
          yield();
          break;
        case FINGERPRINT_NOFINGER:
          Serial.println(".");
          yield();
          break;
        case FINGERPRINT_PACKETRECIEVEERR:
          Serial.println("Communication error");
          yield();
          break;
        case FINGERPRINT_IMAGEFAIL:
          Serial.println("Imaging error");
          yield();
          break;
        default:
          Serial.println("Unknown error");
          yield();
          break;
      }
      delay(50);  // Yield to watchdog
      yield();    // Reset watchdog
    }

    // Convert image to template
    p = finger.image2Tz(1);
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Image converted");
        yield();
        break;
      case FINGERPRINT_IMAGEMESS:
        Serial.println("Image too messy");
        yield();
        request->send(500, "text/plain", "Image too messy");
        return;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        yield();
        request->send(500, "text/plain", "Communication error");
        return;
      case FINGERPRINT_FEATUREFAIL:
        Serial.println("Could not find fingerprint features");
        yield();
        request->send(500, "text/plain", "Could not find fingerprint features");
        return;
      case FINGERPRINT_INVALIDIMAGE:
        Serial.println("Could not find fingerprint features");
        yield();
        request->send(500, "text/plain", "Could not find fingerprint features");
        return;
      default:
        Serial.println("Unknown error");
        yield();
        request->send(500, "text/plain", "Unknown error");
        return;
    }

    Serial.println("Remove finger");
    yield();
    delay(1000);  // Wait for user to remove finger

    // Wait for finger to be removed
    p = 0;
    while (p != FINGERPRINT_NOFINGER) {
      p = finger.getImage();
      delay(500);  // Yield to watchdog
      yield();    // Reset watchdog
    }
    ws.textAll("Remove Finger");
    Serial.print("ID ");
    Serial.println(id);
     });
*/
  // register-fingerprint

  /*
server.on("/register-fingerprint", HTTP_GET, [](AsyncWebServerRequest *request) {
    int retryCount = 0;
    const int maxRetries = 3;

    while (retryCount < maxRetries) {
        Serial.println("Place same finger again");
        yield();
        int p = -1;
        int count = 0;
        // Capture second image
        while (p != FINGERPRINT_OK) {
            p = finger.getImage();
            count += 1;
            Serial.print("Count: ");
            Serial.println(count);
            if (count > 3) {
                break;
            }
            switch (p) {
                case FINGERPRINT_OK:
                    Serial.println("Image taken");
                    yield();
                    break;
                case FINGERPRINT_NOFINGER:
                    Serial.print(".");
                    yield();
                    break;
                case FINGERPRINT_PACKETRECIEVEERR:
                    Serial.println("Communication error");
                    yield();
                    break;
                case FINGERPRINT_IMAGEFAIL:
                    Serial.println("Imaging error");
                    yield();
                    break;
                default:
                    Serial.println("Unknown error");
                    yield();
                    break;
            }
            delay(1000);  // Yield to watchdog
            yield();    // Reset watchdog
        }

        // Convert image to template
        p = finger.image2Tz(2);
        switch (p) {
            case FINGERPRINT_OK:
                Serial.println("Image converted");
                yield();
                break;
            case FINGERPRINT_IMAGEMESS:
                Serial.println("Image too messy");
                yield();
                request->send(500, "text/plain", "Image too messy");
                return;
            case FINGERPRINT_PACKETRECIEVEERR:
                Serial.println("Communication error");
                yield();
                request->send(500, "text/plain", "Communication error");
                return;
            case FINGERPRINT_FEATUREFAIL:
                Serial.println("Could not find fingerprint features");
                yield();
                request->send(500, "text/plain", "Could not find fingerprint features");
                return;
            case FINGERPRINT_INVALIDIMAGE:
                Serial.println("Could not find fingerprint features");
                yield();
                request->send(500, "text/plain", "Could not find fingerprint features");
                return;
            default:
                Serial.println("Unknown error");
                yield();
                request->send(500, "text/plain", "Unknown error");
                return;
        }

        // Create model
        Serial.print("Creating model for #");
        Serial.println(id);
        yield();

        p = finger.createModel();
        if (p == FINGERPRINT_OK) {
            Serial.println("Prints matched!");
            yield();
        } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
            Serial.println("Communication error");
            yield();
            request->send(500, "text/plain", "Communication error");
            return;
        } else if (p == FINGERPRINT_ENROLLMISMATCH) {
            Serial.println("Fingerprints did not match");
            yield();
            request->send(500, "text/plain", "Fingerprints did not match");
            retryCount++;
            continue;  // Retry
        } else {
            Serial.println("Unknown error");
            yield();
            request->send(500, "text/plain", "Unknown error");
            return;
        }

        // Store model
        Serial.print("ID ");
        Serial.println(id);
        yield();
        p = finger.storeModel(id);
        if (p == FINGERPRINT_OK) {
            Serial.println("Stored");
            yield();
            request->send(200, "text/plain", "Fingerprint stored successfully");
            return;
        } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
            Serial.println("Communication error");
            yield();
            request->send(500, "text/plain", "Communication error");
            return;
        } else if (p == FINGERPRINT_BADLOCATION) {
            Serial.println("Could not store in that location");
            yield();
            request->send(500, "text/plain", "Could not store in that location");
            return;
        } else if (p == FINGERPRINT_FLASHERR) {
            Serial.println("Error writing to flash");
            yield();
            request->send(500, "text/plain", "Error writing to flash");
            return;
        } else {
            Serial.println("Unknown error");
            yield();
            request->send(500, "text/plain", "Unknown error");
            return;
        }
    }
    request->send(500, "text/plain", "Failed to register fingerprint after multiple attempts");
});
*/

  /*
  server.on("/start-registration",  HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
              {
      String VerificationsMsg = "Message Verified";
      String OtpVerificationCheck;
      DynamicJsonBuffer jsonBuffer;
      JsonObject& json = jsonBuffer.parseObject(data);
      if (json.success()) {
        String Empid  = json["Empid"].as<char*>();
        Serial.println(Empid);
      }
      Serial.println("Biometric scan Started");
      ws.textAll("Biometric Scan started");

      int p = -1;
      int count = 0;
      Serial.print("Waiting for valid finger to enroll as #");
      Serial.println(id);

      // Capture first image
      while (p != FINGERPRINT_OK) {
          p = finger.getImage();
          Serial.print("Count: ");
          Serial.println(count);
          if (count > 2) { // Increase the count limit to avoid infinite loops
              Serial.println("Timeout waiting for finger");
              ws.textAll("Timeout For Finger");
              delay(100);
              request->send(500, "text/plain", "Timeout waiting for finger");
              break;
          }
          switch (p) {
              case FINGERPRINT_OK:
                  Serial.println("Image taken");
                  break;
              case FINGERPRINT_NOFINGER:
                  Serial.print(".");
                  break;
              case FINGERPRINT_PACKETRECIEVEERR:
                  Serial.println("Communication error");
                  ws.textAll("Communication error");
                  request->send(500, "text/plain", "Communication error");
                  break;
              case FINGERPRINT_IMAGEFAIL:
                  Serial.println("Finger Too Messy");
                  ws.textAll("Image error ");
                  request->send(500, "text/plain", "Imaging error");
                  break;
              default:
                  Serial.println("Unknown error");
                  request->send(500, "text/plain", "Unknown error");
                  break;
          }
          delay(1000);
          count++;
      }

      // Convert image to template
      p = finger.image2Tz(1);
      if (p != FINGERPRINT_OK) {
          String errorMsg = (p == FINGERPRINT_IMAGEMESS) ? "Image too messy" :
                            (p == FINGERPRINT_PACKETRECIEVEERR) ? "Communication error" :
                            (p == FINGERPRINT_FEATUREFAIL) ? "Could not find fingerprint features" :
                            (p == FINGERPRINT_INVALIDIMAGE) ? "Invalid image" : "Unknown error";
          Serial.println(errorMsg);
          request->send(500, "text/plain", errorMsg);
          return;
      }

      Serial.println("Remove finger");
      ws.textAll("Remove Finger");
      request->send(200, "text/plain", "Registration started");
  });
  */

  /*
    server.on("/start-registration", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
              {
                int EmpId;
          DynamicJsonBuffer jsonBuffer;
          JsonObject& json = jsonBuffer.parseObject(data);
          if (json.success()) {
              String empId = json["empId"].as<String>();
              Serial.println("Received empId: " + empId);
              EmpId = empId.toInt();
              Serial.println("Biometric scan started");
              ws.textAll("Biometric Scan started");

              int p = -1;
              int count = 0;
              uint8_t id = getNextFreeID();  // Get the next free ID from the fingerprint sensor
              File datafile = SPIFFS.open("/Emprfid.csv", "a");

              Serial.print("Waiting for valid finger to enroll as #");
              Serial.println(id);
              if(id == -1){
                Serial.println("Id is not available");
                return;
              }
              String data = id+String(",")+EmpId+String("\n");
              datafile.println(data);
              datafile.close();
              Serial.println("Done Writting file");
              Serial.println(id);

              File datafiles = SPIFFS.open("/Emprfid.csv", "r");
              while(datafiles.available()){
                String line = datafiles.readStringUntil('\n');
                Serial.print("Line :");
                Serial.println(line);
              }
              datafiles.close();

              // Capture first image
              while (p != FINGERPRINT_OK) {
                  p = finger.getImage();
                  Serial.print("Count: ");
                  Serial.println(count);
                  if (count > 2) { // Increase the count limit to avoid infinite loops
                      Serial.println("Timeout waiting for finger");
                      ws.textAll("Timeout For Finger");
                      delay(100);
                      request->send(500, "text/plain", "Timeout waiting for finger");
                      break;
                  }
                  switch (p) {
                      case FINGERPRINT_OK:
                          Serial.println("Image taken");
                          break;
                      case FINGERPRINT_NOFINGER:
                          Serial.print(".");
                          break;
                      case FINGERPRINT_PACKETRECIEVEERR:
                          Serial.println("Communication error");
                          ws.textAll("Communication error");
                          request->send(500, "text/plain", "Communication error");
                          break;
                      case FINGERPRINT_IMAGEFAIL:
                          Serial.println("Finger Too Messy");
                          ws.textAll("Image error ");
                          request->send(500, "text/plain", "Imaging error");
                          break;
                      default:
                          Serial.println("Unknown error");
                          request->send(500, "text/plain", "Unknown error");
                          break;
                  }
                  delay(1000);
                  count++;
              }

              // Convert image to template
              p = finger.image2Tz(1);
              if (p != FINGERPRINT_OK) {
                  String errorMsg = (p == FINGERPRINT_IMAGEMESS) ? "Image too messy" :
                                    (p == FINGERPRINT_PACKETRECIEVEERR) ? "Communication error" :
                                    (p == FINGERPRINT_FEATUREFAIL) ? "Could not find fingerprint features" :
                                    (p == FINGERPRINT_INVALIDIMAGE) ? "Invalid image" : "Unknown error";
                  Serial.println(errorMsg);
                  request->send(500, "text/plain", errorMsg);
                  return;
              }

              Serial.println("Remove finger");
              ws.textAll("Remove Finger");
              request->send(200, "text/plain", "Registration started");
          } else {
              request->send(400, "text/plain", "Invalid JSON");
          } });

  */

  /*
    server.on("/re-registering", HTTP_GET, [](AsyncWebServerRequest *request)
              {
      // Assuming re-registering logic is similar to registration
      Serial.println("Re-registering fingerprint");
      ws.textAll("Re-register Finger");
      // Example logic
      int p = -1;
      int count = 0;

      // Capture and process image
      while (p != FINGERPRINT_OK) {
          p = finger.getImage();
          if (count > 2) { // Increase the count limit to avoid infinite loops
              Serial.println("Timeout waiting for finger re-register");
              ws.textAll("Timeout waiting for finger");
              request->send(500, "text/plain", "Timeout waiting for finger");
              break;
              return;
          }
          delay(50);
          count++;
      }

      // Convert image to template
      p = finger.image2Tz(1);
      if (p != FINGERPRINT_OK) {
          String errorMsg = (p == FINGERPRINT_IMAGEMESS) ? "Image too messy" :
                            (p == FINGERPRINT_PACKETRECIEVEERR) ? "Communication error" :
                            (p == FINGERPRINT_FEATUREFAIL) ? "Could not find fingerprint features" :
                            (p == FINGERPRINT_INVALIDIMAGE) ? "Invalid image" : "Unknown error";
          Serial.println(errorMsg);
          request->send(500, "text/plain", errorMsg);
          return;
      }

      Serial.println("Re-registration completed");
      ws.textAll("Re-registration Completed");
      request->send(200, "text/plain", "Re-registration completed"); });
  */


  /*

    server.on("/start-registration", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
              {
          int EmpId;
          RegistrationFinger = false;
          DynamicJsonBuffer jsonBuffer;
          JsonObject& json = jsonBuffer.parseObject(data);
          if (json.success()) {
              String empIdStr = json["empId"].as<String>();
              Serial.println("Received empId: " + empIdStr);
              RegEmpId = empIdStr.toInt();

              // uint8_t id = getNextFreeID();  // Get the next free ID from the fingerprint sensor

              if (id == 0xFF) { // No free ID available
                  Serial.println("No available ID for registration");
                  request->send(500, "text/plain", "No available ID for registration");
                  return;
              }

              id = getNextFreeID();
              int p = -1;
              Serial.print("Waiting for valid finger to enroll as #"); 
              Serial.println(id);
              SendRegisterData("Place Your Finger");

              // Wait for a valid finger to be placed
              int count =0;
              while (count>20) {
                p = finger.getImage();
                count+=1;
                switch (p) {
                  case FINGERPRINT_OK:
                    SendRegisterData("Image taken");
                    Serial.println("Image taken");
                    break;
                  case FINGERPRINT_NOFINGER:
                    Serial.print(".");
                    SendRegisterData("No Finger is Registered");
                    break;
                  case FINGERPRINT_PACKETRECIEVEERR:
                  SendRegisterData("Communication error");
                    Serial.println("Communication error");
                    break;
                  case FINGERPRINT_IMAGEFAIL:
                  SendRegisterData("Imaging error");
                    Serial.println("Imaging error");
                    break;
                  default:
                  SendRegisterData("Unknown error");
                    Serial.println("Unknown error");
                    break;
                }
                delay(100);
              }

              // Convert image to template
              p = finger.image2Tz(1);
              switch (p) {
                case FINGERPRINT_OK:
                SendRegisterData("Finger Ok");
                  Serial.println("Image converted");
                  break;
                case FINGERPRINT_IMAGEMESS:
                SendRegisterData("Image too messy");
                  Serial.println("Image too messy");
                  break;
                case FINGERPRINT_PACKETRECIEVEERR:
                SendRegisterData("Communication error");
                  Serial.println("Communication error");
                  break;
                case FINGERPRINT_FEATUREFAIL:
                SendRegisterData("Could not find fingerprint features");
                  Serial.println("Could not find fingerprint features");
                  break;
                case FINGERPRINT_INVALIDIMAGE:
                  Serial.println("Invalid image");
                  SendRegisterData("Invalid image");
                  break;
                default:
                SendRegisterData("Unknown error");
                  Serial.println("Unknown error");
                  break;
              }

              SendRegisterData("Remove finger");
              Serial.println("Remove finger");


              // Serial.print("Waiting for valid finger to enroll as #");
              // Serial.println(id);

              // // Open file to write the employee ID and fingerprint ID
              // File datafile = SPIFFS.open("/EmpRfid.csv", "a");
              // String data = String(id) + "," + String(EmpId);
              // datafile.println(data);
              // datafile.close();
              // Serial.println("Done Writing file");

              // // Read the file content to verify
              // File datafiles = SPIFFS.open("/EmpRfid.csv", "r");
              // while (datafiles.available()) {
              //     String line = datafiles.readStringUntil('\n');
              //     Serial.print("Line: ");
              //     Serial.println(line);
              // }
              // datafiles.close();
              // getFingerprintEnroll();
              
              request->send(200, "text/plain", "Registration started");
          } else {
              request->send(400, "text/plain", "Invalid JSON");
          } });

    server.on("/re-registering", HTTP_GET, [](AsyncWebServerRequest *request)
              {
              Serial.println("Re-registering fingerprint");
              SendRegisterData("Re-register Finger");
              // getFingerprintEnroll();

              int p = 0;
              while (p != FINGERPRINT_NOFINGER)
              {
                  p = finger.getImage();
              }

              Serial.print("ID ");
              Serial.println(id);
              p = -1;
              Serial.println("Place same finger again");
              SendRegisterData("Place same finger again");
              // Wait for the same finger to be placed again
              int count = 0;
              while (count > 20)
              {
                  p = finger.getImage();
                  count +=1;
                  switch (p)
                  {
                  case FINGERPRINT_OK:
                      Serial.println("Image taken");
                      break;
                  case FINGERPRINT_NOFINGER:
                      Serial.print(".");
                      break;
                  case FINGERPRINT_PACKETRECIEVEERR:
                      Serial.println("Communication error");
                      break;
                  case FINGERPRINT_IMAGEFAIL:
                      Serial.println("Imaging error");
                      break;
                  default:
                      Serial.println("Unknown error");
                      break;
                  }
                  delay(100);
              }

              // Convert image to template
              p = finger.image2Tz(2);
              switch (p)
              {
              case FINGERPRINT_OK:
                  Serial.println("Image converted");
              case FINGERPRINT_IMAGEMESS:
                  Serial.println("Image too messy");
              case FINGERPRINT_PACKETRECIEVEERR:
                  Serial.println("Communication error");
              case FINGERPRINT_FEATUREFAIL:
                  Serial.println("Could not find fingerprint features");
              case FINGERPRINT_INVALIDIMAGE:
                  Serial.println("Invalid image");
              default:
                  Serial.println("Unknown error");
              }
              // Create model
              Serial.print("Creating model for #");
              Serial.println(id);

              p = finger.createModel();
              if (p == FINGERPRINT_OK)
              {
                  Serial.println("Prints matched!");
              }
              else if (p == FINGERPRINT_PACKETRECIEVEERR)
              {
                  Serial.println("Communication error");
              }
              else if (p == FINGERPRINT_ENROLLMISMATCH)
              {
                  Serial.println("Fingerprints did not match");
              }
              else
              {
                  Serial.println("Unknown error");
              }

              // Store model
              Serial.print("ID ");
              Serial.println(id);
              p = finger.storeModel(id);
              if (p == FINGERPRINT_OK)
              {
                SendRegisterData("Validating ...");
                File datafile = SPIFFS.open("/EmpRfid.csv", "a");
                String data = String(id) + "," + String(RegEmpId);
                datafile.println(data);
                RegEmpId = "";
                datafile.close();
                Serial.println("Done Writing file");

                // Read the file content to verify
                File datafiles = SPIFFS.open("/EmpRfid.csv", "r");
                while (datafiles.available()) {
                    String line = datafiles.readStringUntil('\n');
                    Serial.print("Line: ");
                    Serial.println(line);
                  }
                // datafiles.close();
                  Serial.println("Stored!");
                  delay(100);
                  SendRegisterData("SuccessFully Registered !");
                  
                  downloadFingerprintTemplate(id);
              }
              else if (p == FINGERPRINT_PACKETRECIEVEERR)
              {
                  Serial.println("Communication error");
              }
              else if (p == FINGERPRINT_BADLOCATION)
              {
                  Serial.println("Could not store in that location");
              }
              else if (p == FINGERPRINT_FLASHERR)
              {
                  Serial.println("Error writing to flash");
              }
              else
              {
                  Serial.println("Unknown error");
              }
              ws.textAll("Re-registration Completed");
              request->send(200, "text/plain", "Re-registration completed");

              });

  */


  server.on("/start-registration", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      int status=0;
      Serial.println("Start Registration");
      RegistrationFinger = false;
      DynamicJsonBuffer jsonBuffer;
      JsonObject& json = jsonBuffer.parseObject(data);
      if (json.success()) {
          String empIdStr = json["empId"].as<String>();
          Serial.println("Received empId: " + empIdStr);
          RegEmpId = empIdStr.toInt();

          id = getNextFreeID(); // Get the next free ID from the fingerprint sensor
          if (id == 0xFF) { // No free ID available
              Serial.println("No available ID for registration");
              request->send(500, "text/plain", "No available ID for registration");
              return;
          }

          int p = -1;
          Serial.print("Waiting for valid finger to enroll as #"); 
          Serial.println(id);
          SendRegisterData("Place Your Finger");

          // Wait for a valid finger to be placed
          
        int count = 0;
  const int maxAttempts = 10;
  const int delayMs = 200;

  while (count < maxAttempts) {
      p = finger.getImage();
      Serial.println("Image getting");
      count++;
      switch (p) {
          case FINGERPRINT_OK:
              SendRegisterData("Image taken");
              Serial.println("Image taken");
              count = maxAttempts; // Exit the loop
              status =1;
              break;
          case FINGERPRINT_NOFINGER:
              Serial.print(".");
              SendRegisterData("No Finger is Registered");
              break;
          case FINGERPRINT_PACKETRECIEVEERR:
              SendRegisterData("Communication error");
              Serial.println("Communication error");
              break;
          case FINGERPRINT_IMAGEFAIL:
              SendRegisterData("Imaging error");
              Serial.println("Imaging error");
              break;
          default:
              SendRegisterData("Unknown error");
              Serial.println("Unknown error");
              break;
      }
      Serial.print("Status : ");
          Serial.println(status);

      vTaskDelay(pdMS_TO_TICKS(delayMs)); // Yield to other tasks
  }

  Serial.print("Status : ");
          Serial.println(status);

  if (p != FINGERPRINT_OK) {
      Serial.println("Failed to get a valid fingerprint image after 20 attempts");
      status = -1;
      SendRegisterData("Failed to get a valid fingerprint image");
      // Handle the error case here
  } else {
      // Proceed with the next steps as a valid image was obtained
  }

  Serial.print("Status : ");
          Serial.println(status);
          // Convert image to template
          Serial.print("Status : ");
          Serial.println(status);
          if(status > 0){
            p = finger.image2Tz(1);
          switch (p) {
              case FINGERPRINT_OK:
                  SendRegisterData("Finger Ok");
                  Serial.println("Image converted");
                  break;
              case FINGERPRINT_IMAGEMESS:
                  SendRegisterData("Image too messy");
                  Serial.println("Image too messy");
                  break;
              case FINGERPRINT_PACKETRECIEVEERR:
                  SendRegisterData("Communication error");
                  Serial.println("Communication error");
                  break;
              case FINGERPRINT_FEATUREFAIL:
                  SendRegisterData("Could not find fingerprint features");
                  Serial.println("Could not find fingerprint features");
                  break;
              case FINGERPRINT_INVALIDIMAGE:
                  Serial.println("Invalid image");
                  SendRegisterData("Invalid image");
                  break;
              default:
                  SendRegisterData("Unknown error");
                  Serial.println("Unknown error");
                  break;
                  Serial.print("Status : ");
          Serial.println(status);
          }
          SendRegisterData("Remove finger");
          Serial.println("Remove finger");
          request->send(200, "text/plain", "Registration started");
          }
          else{
            request->send(405, "text/plain", "No Finger ");

          }
      } else {
          request->send(400, "text/plain", "Invalid JSON");
      }
  });
  /*
  server.on("/re-registering", HTTP_POST, [](AsyncWebServerRequest *request) {
      Serial.println("Re registering");
      request->send(200, "text/plain", "Endpoint working");
  });
  */

  // Re - registering

  server.on("/re-registering", HTTP_POST, [](AsyncWebServerRequest *request) {
    Serial.println("Re-registering fingerprint");

    int status = 0;
    int p = 0;
    int count = 0;
    const int maxAttempts = 10;
    const int imageSlot = 2; // Slot for the second image

    // Wait until the finger is removed
    while (count < maxAttempts) {
        count++;
        p = finger.getImage();
        if (p == FINGERPRINT_OK) {
            Serial.println("Image taken");
            break;
        }
        delay(100);
        Serial.println("Getting image Re-registering");
    }

    if (p != FINGERPRINT_OK) {
        Serial.println("Failed to capture fingerprint image");
        request->send(500, "text/plain", "Failed to capture fingerprint image");
        return;
    }

    count = 0;
    Serial.println("Place same finger again");
    SendRegisterData("Place same finger again");

    // Wait for the same finger to be placed again
    while (count < maxAttempts) {
        p = finger.getImage();
        Serial.println("Re-register Finger");
        count++;
        if (p == FINGERPRINT_OK) {
            Serial.println("Image taken");
            status = 1;
            break;
        } else if (p == FINGERPRINT_NOFINGER) {
            Serial.print(".");
            delay(1000); // Delay before retrying
        } else {
            Serial.println("Error while getting image");
        }
    }

    if (status > 0) {
        // Convert image to template
        p = finger.image2Tz(imageSlot);
        switch (p) {
            case FINGERPRINT_OK:
                Serial.println("Image converted");
                break;
            case FINGERPRINT_IMAGEMESS:
                Serial.println("Image too messy");
                return;
            case FINGERPRINT_PACKETRECIEVEERR:
                Serial.println("Communication error");
                return;
            case FINGERPRINT_FEATUREFAIL:
                Serial.println("Could not find fingerprint features");
                return;
            case FINGERPRINT_INVALIDIMAGE:
                Serial.println("Invalid image");
                return;
            default:
                Serial.println("Unknown error");
                return;
        }

        // Create model
        p = finger.createModel();
        if (p == FINGERPRINT_OK) {
            Serial.println("Prints matched!");
        } else {
            Serial.println("Failed to create model");
            return;
        }

        // Store model
        p = finger.storeModel(id);
        if (p == FINGERPRINT_OK) {
            Serial.println("Fingerprint model stored successfully");

            // Optionally, add code to write to file or other actions

            request->send(200, "text/plain", "Re-registration completed");
        } else {
            Serial.println("Failed to store model");
            request->send(500, "text/plain", "Failed to store model");
        }
    } else {
        Serial.println("Re-registration failed");
        request->send(500, "text/plain", "Re-registration failed");
    }
  });






  /*
  server.on("/re-registering", HTTP_POST, [](AsyncWebServerRequest *request){
        Serial.println("Re-registering fingerprint");
        SendRegisterData("Re-register Finger");

        int status = 0;
        int p = 0;
        int count = 0;
        const int maxAttempts = 10;
        const int imageSlot = 2; // Slot for the second image

        // Wait until the finger is removed
        while (count < maxAttempts) {
            count++;
            p = finger.getImage();
            if (p == FINGERPRINT_OK) {
                break;
            }
            delay(100);
            Serial.println("Getting image Re-registering");
        }

        if (p != FINGERPRINT_OK) {
            Serial.println("Failed to capture fingerprint image reinfos");
            request->send(500, "text/plain", "Failed to capture fingerprint image");
            Serial.println("Re info Done");
            status = -1;
        }

        count = 0;
        Serial.println("Place same finger again");
        SendRegisterData("Place same finger again");

        // Wait for the same finger to be placed again
        while (count < maxAttempts && status > 0) {
            p = finger.getImage();
            Serial.println("Re-register Finger");
            count++;
            switch (p) {
                case FINGERPRINT_OK:
                    Serial.println("Image taken");
                    status = 1;
                    break;
                case FINGERPRINT_NOFINGER:
                    Serial.print(".");
                    status = -1;
                    continue;
                case FINGERPRINT_PACKETRECIEVEERR:
                    Serial.println("Communication error");
                    status = -1;
                    break;
                case FINGERPRINT_IMAGEFAIL:
                    Serial.println("Imaging error");
                    status = -1;
                    break;
                default:
                    Serial.println("Unknown error");
                    status = -1;
                    break;
            }
            // Exit loop if image taken successfully
            delay(200);
        }

        if (status > 0) {
            // Convert image to template
            p = finger.image2Tz(imageSlot);
            switch (p) {
                case FINGERPRINT_OK:
                    Serial.println("Image converted");
                    break;
                case FINGERPRINT_IMAGEMESS:
                    Serial.println("Image too messy");
                    return;
                case FINGERPRINT_PACKETRECIEVEERR:
                    Serial.println("Communication error");
                    return;
                case FINGERPRINT_FEATUREFAIL:
                    Serial.println("Could not find fingerprint features");
                    return;
                case FINGERPRINT_INVALIDIMAGE:
                    Serial.println("Invalid image");
                    return;
                default:
                    Serial.println("Unknown error");
                    return;
            }

            // Create model
            Serial.print("Creating model for #");
            Serial.println(id);

            p = finger.createModel();
            if (p == FINGERPRINT_OK) {
                Serial.println("Prints matched!");
            } else {
                Serial.println("Failed to create model");
                return;
            }

            // Store model
            Serial.print("ID ");
            Serial.println(id);
            p = finger.storeModel(id);
            if (p == FINGERPRINT_OK) {
                SendRegisterData("Validating ...");
                File datafile = SPIFFS.open("/EmpRfid.csv", "a");
                if (!datafile) {
                    Serial.println("Failed to open file for writing");
                    request->send(500, "text/plain", "File write error");
                    return;
                }
                String data = String(id) + "," + String(RegEmpId);
                datafile.println(data);
                RegEmpId = "";
                datafile.close();
                Serial.println("Done Writing file");

                // Read the file content to verify
                File datafiles = SPIFFS.open("/EmpRfid.csv", "r");
                while (datafiles.available()) {
                    String line = datafiles.readStringUntil('\n');
                    Serial.print("Line: ");
                    Serial.println(line);
                }
                datafiles.close();
                Serial.println("Stored!");
                delay(100);
                SendRegisterData("Successfully Registered!");

                downloadFingerprintTemplate(id);
            } else {
                Serial.println("Failed to store model");
            }
            ws.textAll("Re-registration Completed");
            request->send(200, "text/plain", "Re-registration completed");
        } else {
            Serial.println("Re-registration failed");
            request->send(500, "text/plain", "Re-registration failed");
        }
  });
  server.on("/re-registering", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request) {
      Serial.println("Re-registering fingerprint");
      SendRegisterData("Re-register Finger");

      int status = 0;
      int p = 0;
      int count = 0;
      const int maxAttempts = 10;
      const int imageSlot = 2; // Slot for the second image

      // Wait until the finger is removed
      while (count < maxAttempts) {
          count++;
          p = finger.getImage();
          if (p == FINGERPRINT_OK) {
              break;
          }
          delay(100);
          Serial.println("Getting image Re-registering");
      }

      if (p != FINGERPRINT_OK) {
          Serial.println("Failed to capture fingerprint image reinfos");
          request->send(500, "text/plain", "Failed to capture fingerprint image");
          Serial.println("Re info Done");
          status = -1;
      }

      count = 0;
      Serial.println("Place same finger again");
      SendRegisterData("Place same finger again");

      // Wait for the same finger to be placed again
      while (count < maxAttempts && status > 0) {
          p = finger.getImage();
          Serial.println("Re-register Finger");
          count++;
          switch (p) {
              case FINGERPRINT_OK:
                  Serial.println("Image taken");
                  status = 1;
                  break;
              case FINGERPRINT_NOFINGER:
                  Serial.print(".");
                  status = -1;
                  continue;
              case FINGERPRINT_PACKETRECIEVEERR:
                  Serial.println("Communication error");
                  status = -1;
                  break;
              case FINGERPRINT_IMAGEFAIL:
                  Serial.println("Imaging error");
                  status = -1;
                  break;
              default:
                  Serial.println("Unknown error");
                  status = -1;
                  break;
          }
          // Exit loop if image taken successfully
          delay(200);
      }

      if (status > 0) {
          // Convert image to template
          p = finger.image2Tz(imageSlot);
          switch (p) {
              case FINGERPRINT_OK:
                  Serial.println("Image converted");
                  break;
              case FINGERPRINT_IMAGEMESS:
                  Serial.println("Image too messy");
                  return;
              case FINGERPRINT_PACKETRECIEVEERR:
                  Serial.println("Communication error");
                  return;
              case FINGERPRINT_FEATUREFAIL:
                  Serial.println("Could not find fingerprint features");
                  return;
              case FINGERPRINT_INVALIDIMAGE:
                  Serial.println("Invalid image");
                  return;
              default:
                  Serial.println("Unknown error");
                  return;
          }

          // Create model
          Serial.print("Creating model for #");
          Serial.println(id);

          p = finger.createModel();
          if (p == FINGERPRINT_OK) {
              Serial.println("Prints matched!");
          } else {
              Serial.println("Failed to create model");
              return;
          }

          // Store model
          Serial.print("ID ");
          Serial.println(id);
          p = finger.storeModel(id);
          if (p == FINGERPRINT_OK) {
              SendRegisterData("Validating ...");
              File datafile = SPIFFS.open("/EmpRfid.csv", "a");
              if (!datafile) {
                  Serial.println("Failed to open file for writing");
                  request->send(500, "text/plain", "File write error");
                  return;
              }
              String data = String(id) + "," + String(RegEmpId);
              datafile.println(data);
              RegEmpId = "";
              datafile.close();
              Serial.println("Done Writing file");

              // Read the file content to verify
              File datafiles = SPIFFS.open("/EmpRfid.csv", "r");
              while (datafiles.available()) {
                  String line = datafiles.readStringUntil('\n');
                  Serial.print("Line: ");
                  Serial.println(line);
              }
              datafiles.close();
              Serial.println("Stored!");
              delay(100);
              SendRegisterData("Successfully Registered!");

              downloadFingerprintTemplate(id);
          } else {
              Serial.println("Failed to store model");
          }
          ws.textAll("Re-registration Completed");
          request->send(200, "text/plain", "Re-registration completed");
      } else {
          Serial.println("Re-registration failed");
          request->send(500, "text/plain", "Re-registration failed");
      }
  });
*/


  server.on("/delete-all", HTTP_GET, [](AsyncWebServerRequest *request)
            {
        Serial.println("Deleting all fingerprints");
         uint8_t result = finger.emptyDatabase();
    if (result == FINGERPRINT_OK) {
        Serial.println("All fingerprints deleted successfully.");
    } else {
        Serial.println("Failed to delete fingerprints.");
    } });

  /*

    server.on("/start-registration", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
      {
        if (!finger.begin()) {
          Serial.println("Failed to initialize fingerprint sensor");
          while (1);
      }
          int EmpId;
          DynamicJsonBuffer jsonBuffer;
          JsonObject& json = jsonBuffer.parseObject(data);
          if (json.success()) {
              String empIdStr = json["empId"].as<String>();
              Serial.println("Received empId: " + empIdStr);
              EmpId = empIdStr.toInt();

              uint8_t id = getNextFreeID();  // Get the next free ID from the fingerprint sensor

              if (id == 0xFF) { // No free ID available
                  Serial.println("No available ID for registration");
                  request->send(500, "text/plain", "No available ID for registration");
                  return;
              }

              Serial.print("Waiting for valid finger to enroll as #");
              Serial.println(id);

              // Open file to write the employee ID and fingerprint ID
              File datafile = SPIFFS.open("/Emprfid.csv", "a");
              String data = String(id) + "," + String(EmpId) + "\n";
              datafile.println(data);
              datafile.close();
              Serial.println("Done Writing file");

              // Read the file content to verify
              File datafiles = SPIFFS.open("/Emprfid.csv", "r");
              while (datafiles.available()) {
                  String line = datafiles.readStringUntil('\n');
                  Serial.print("Line: ");
                  Serial.println(line);
              }
              datafiles.close();

              // Capture and process the fingerprint image
              int p = -1;
              int count = 0;
              while (p != FINGERPRINT_OK) {
                  p = finger.getImage();
                  Serial.print("Count: ");
                  Serial.println(count);
                  if (count > 10) { // Increased count limit
                      Serial.println("Timeout waiting for finger");
                      ws.textAll("Timeout For Finger");
                      request->send(500, "text/plain", "Timeout waiting for finger");
                      return;
                  }
                  switch (p) {
                      case FINGERPRINT_OK:
                          Serial.println("Image taken");
                          break;
                      case FINGERPRINT_NOFINGER:
                          Serial.print(".");
                          break;
                      case FINGERPRINT_PACKETRECIEVEERR:
                          Serial.println("Communication error");
                          ws.textAll("Communication error");
                          request->send(500, "text/plain", "Communication error");
                          return;
                      case FINGERPRINT_IMAGEFAIL:
                          Serial.println("Finger Too Messy");
                          ws.textAll("Image error");
                          request->send(500, "text/plain", "Imaging error");
                          return;
                      default:
                          Serial.println("Unknown error");
                          request->send(500, "text/plain", "Unknown error");
                          return;
                  }
                  delay(1000);
                  count++;
              }

              // Convert image to template
              p = finger.image2Tz(1);
              if (p != FINGERPRINT_OK) {
                  String errorMsg = (p == FINGERPRINT_IMAGEMESS) ? "Image too messy" :
                                    (p == FINGERPRINT_PACKETRECIEVEERR) ? "Communication error" :
                                    (p == FINGERPRINT_FEATUREFAIL) ? "Could not find fingerprint features" :
                                    (p == FINGERPRINT_INVALIDIMAGE) ? "Invalid image" : "Unknown error";
                  Serial.println(errorMsg);
                  request->send(500, "text/plain", errorMsg);
                  return;
              }
              Serial.println("Remove finger");
              ws.textAll("Remove Finger");
              request->send(200, "text/plain", "Registration started");
          } else {
              request->send(400, "text/plain", "Invalid JSON");
          }
      });

      server.on("/re-registering", HTTP_GET, [](AsyncWebServerRequest *request)
      {
          Serial.println("Re-registering fingerprint");
          ws.textAll("Re-register Finger");

          int p = -1;
          int count = 0;

          // Capture and process the fingerprint image
          while (p != FINGERPRINT_OK) {
              p = finger.getImage();
              if (count > 10) { // Increased count limit
                  Serial.println("Timeout waiting for finger re-register");
                  ws.textAll("Timeout waiting for finger");
                  request->send(500, "text/plain", "Timeout waiting for finger");
                  return;
              }
              delay(50);
              count++;if (!finger.begin()) {
          Serial.println("Failed to initialize fingerprint sensor");
          while (1);
      }
          }

          // Convert image to template
          p = finger.image2Tz(1);
          if (p != FINGERPRINT_OK) {
              String errorMsg = (p == FINGERPRINT_IMAGEMESS) ? "Image too messy" :
                                (p == FINGERPRINT_PACKETRECIEVEERR) ? "Communication error" :
                                (p == FINGERPRINT_FEATUREFAIL) ? "Could not find fingerprint features" :
                                (p == FINGERPRINT_INVALIDIMAGE) ? "Invalid image" : "Unknown error";
              Serial.println(errorMsg);
              request->send(500, "text/plain", errorMsg);
              return;
          }

          Serial.println("Re-registration completed");
          ws.textAll("Re-registration Completed");
          request->send(200, "text/plain", "Re-registration completed");
      });
      */
  
  server.begin();
}

void softAp()
{
  WiFi.softAP(ASSID, APASS);
  Serial.println("AP Started");
  Serial.print("IP Address:");
  Serial.println(WiFi.softAPIP());
}

// Define the getNextFreeID function
uint8_t getNextFreeID()
{
  for (uint8_t id = 1; id <= 255; id++)
  { // Assuming a maximum of 255 IDs
    if (finger.loadModel(id) != FINGERPRINT_OK)
    {
      // ID is available
      Serial.print("ID available: ");
      Serial.println(id);
      return id;
    }
    else
    {
      Serial.print("ID in use: ");
      Serial.println(id);
    }
  }
  Serial.println("No free ID available");
  return 0xFF; // Indicate that no free ID was found
}

void WebSocketRegister()
{
  ws.onEvent(onWsEvent);
  // ws1.onEvent(wifiStatusEvent);
  server.addHandler(&ws);
  // server.addHandler(&ws1);
}

uint8_t readnumber(void)
{
  uint8_t num = 0;

  while (num == 0)
  {
    while (!Serial.available())
      ;
    num = Serial.parseInt();
  }
  return num;
}

void getNextFreeIDint()
{
  for (int id = 1; id <= 255; id++)
  { // Assuming a maximum of 255 IDs
    if (finger.loadModel(id) != FINGERPRINT_OK)
    {
      // ID is available
      Serial.print("ID available: ");
      Serial.println(id);
    }
    else
    {
      Serial.print("ID in use: ");
      Serial.println(id);
    }
  }
  Serial.println("No free ID available");
}

void initializePinMode()
{
  pinMode(Relay, OUTPUT);
  pinMode(Buzzer, OUTPUT);
  pinMode(REDLED, OUTPUT);
  pinMode(GREENLED, OUTPUT);
  pinMode(REDLED1, OUTPUT);
  pinMode(GREENLED1, OUTPUT);
  pinMode(ORANGELED, OUTPUT);
}

void initialavailableWifi()
{
  DynamicJsonBuffer jsonBuffer;
  JsonArray &networks = jsonBuffer.createArray();
  int n = WiFi.scanNetworks();
  if (n == 0)
  {
    Serial.println("No networks found.");
    JsonObject &network = networks.createNestedObject();
    network["ssid"] = "No networks found";
  }
  else
  {
    for (int i = 0; i < n; ++i)
    {
      JsonObject &network = networks.createNestedObject();
      network["ssid"] = WiFi.SSID(i);
      network["rssi"] = WiFi.RSSI(i);
      network["encryption"] = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "Open" : "Encrypted";
    }
  }
  json = "";
  networks.printTo(json);
  Serial.print("Json Data :");
  Serial.println(json);
  WiFi.scanDelete();
}

void mountinSpiffs()
{
  if (!SPIFFS.begin(true))
  {
    Serial.println("Spiffs Not Beginned Properly");
  }
  else
  {
    Serial.println("Spiffs Beginned Correctly");
  }
}

void setup()
{
  Serial.begin(115200);
  WiFi.mode(WIFI_AP_STA);
  softAp();
  WebServerRoutes();
  delay(100);
  InitializeRTC();
  WifiConnectCheck();
  initializePinMode();
  SensorFingerBegin();
  mountinSpiffs();
  mountinSD();
  SensorFingerBegin();
  testFingerprintSensor();
  WebSocketRegister();
}

void loop()
{
  ws.cleanupClients();
  // RegistrationFinger = false;

  if (RegistrationFinger)
  {
    // Serial.print("Finger print id: ");
    // Serial.println(FingerPrintId);
    // searchFingerprint();

    int id = getFingerprintID();
    if (id == FINGERPRINT_NOFINGER || id == FINGERPRINT_PACKETRECIEVEERR || id == FINGERPRINT_IMAGEFAIL)
    {
      // Serial.println("Failed to get fingerprint ID");
      return;
    }

    // Convert uint8_t ID to String
    // int idString = int(id);

    // Serial.print("Fingerprint ID as string: ");
    // Serial.println(FingerPrintId);

    // If you need to match RFID, uncomment and use this line
    if (FingerPrintId > 0)
    {
      // Serial.print("Inside Match rfid");
      matchRfid(FingerPrintId, 1);
      FingerPrintId = 0;
    }

    // int id = getFingerprintIDez();
    // if (id != -1) {
    //     Serial.print("Fingerprint ID as int: ");
    //     Serial.println(id);
    // }
    delay(1);
  }
}

/*
uint8_t getFingerprintID() {
    uint8_t p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
        Serial.println("Image taken");
        break;
    case FINGERPRINT_NOFINGER:
        Serial.println("No finger detected");
        return p;
    case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        return p;
    case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error");
        return p;
    default:
        Serial.println("Unknown error");
        return p;
    }

    // OK success!
    p = finger.image2Tz();
    switch (p) {
    case FINGERPRINT_OK:
        Serial.println("Image converted");
        break;
    case FINGERPRINT_IMAGEMESS:
        Serial.println("Image too messy");
        return p;
    case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        return p;
    case FINGERPRINT_FEATUREFAIL:
        Serial.println("Could not find fingerprint features");
        return p;
    case FINGERPRINT_INVALIDIMAGE:
        Serial.println("Could not find fingerprint features");
        return p;
    default:
        Serial.println("Unknown error");
        return p;
    }

    // OK converted!
    p = finger.fingerSearch();
    if (p == FINGERPRINT_OK) {
        Serial.println("Found a print match!");
    } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
        Serial.println("Communication error");
        return p;
    } else if (p == FINGERPRINT_NOTFOUND) {
        Serial.println("Did not find a match");
        return p;
    } else {
        Serial.println("Unknown error");
        return p;
    }

    // found a match!
    Serial.print("Found ID #");
    Serial.print(finger.fingerID);
    Serial.print(" with confidence of ");
    Serial.println(finger.confidence);

    return finger.fingerID;
}

*/

int getFingerprintID()
{

  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)
  {
    // Serial.println("Failed to take image");
    return -1; // Use -1 to indicate failure
  }

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)
  {
    Serial.println("Failed to convert image");
    return -1; // Use -1 to indicate failure
  }

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)
  {
    Serial.println("Failed to find a match");
    return -1; // Use -1 to indicate failure
  }

  // p = finger.getImage();
  // if (p != FINGERPRINT_OK)
  //   return -1;

  // p = finger.image2Tz();
  // if (p != FINGERPRINT_OK)
  //   return -1;

  // p = finger.fingerFastSearch();
  // if (p != FINGERPRINT_OK)
  //   return -1;

  // found a match!
  FingerPrintId = (int)finger.fingerID; // Convert uint8_t to int
  Serial.print("SSSSS");
  Serial.print(FingerPrintId); // Print as int
  Serial.print(" with confidence of ");
  Serial.println(finger.confidence);
  Serial.println();
  return 1; // Return as int
}

uint8_t getFingerprintEnroll()
{
  id = getNextFreeID();
  int p = -1;
  Serial.print("Waiting for valid finger to enroll as #");
  Serial.println(id);

  // Wait for a valid finger to be placed
  while (p != FINGERPRINT_OK)
  {
    p = finger.getImage();
    switch (p)
    {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.print(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
    delay(100);
  }

  // Convert image to template
  p = finger.image2Tz(1);
  switch (p)
  {
  case FINGERPRINT_OK:
    Serial.println("Image converted");
    break;
  case FINGERPRINT_IMAGEMESS:
    Serial.println("Image too messy");
    return p;
  case FINGERPRINT_PACKETRECIEVEERR:
    Serial.println("Communication error");
    return p;
  case FINGERPRINT_FEATUREFAIL:
    Serial.println("Could not find fingerprint features");
    return p;
  case FINGERPRINT_INVALIDIMAGE:
    Serial.println("Invalid image");
    return p;
  default:
    Serial.println("Unknown error");
    return p;
  }

  Serial.println("Remove finger");
  // Seperation code
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER)
  {
    p = finger.getImage();
  }

  Serial.print("ID ");
  Serial.println(id);
  p = -1;
  Serial.println("Place same finger again");

  // Wait for the same finger to be placed again
  while (p != FINGERPRINT_OK)
  {
    p = finger.getImage();
    switch (p)
    {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.print(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
    delay(100);
  }

  // Convert image to template
  p = finger.image2Tz(2);
  switch (p)
  {
  case FINGERPRINT_OK:
    Serial.println("Image converted");
    break;
  case FINGERPRINT_IMAGEMESS:
    Serial.println("Image too messy");
    return p;
  case FINGERPRINT_PACKETRECIEVEERR:
    Serial.println("Communication error");
    return p;
  case FINGERPRINT_FEATUREFAIL:
    Serial.println("Could not find fingerprint features");
    return p;
  case FINGERPRINT_INVALIDIMAGE:
    Serial.println("Invalid image");
    return p;
  default:
    Serial.println("Unknown error");
    return p;
  }
  // Create model
  Serial.print("Creating model for #");
  Serial.println(id);

  p = finger.createModel();
  if (p == FINGERPRINT_OK)
  {
    Serial.println("Prints matched!");
  }
  else if (p == FINGERPRINT_PACKETRECIEVEERR)
  {
    Serial.println("Communication error");
    return p;
  }
  else if (p == FINGERPRINT_ENROLLMISMATCH)
  {
    Serial.println("Fingerprints did not match");
    return p;
  }
  else
  {
    Serial.println("Unknown error");
    return p;
  }

  // Store model
  Serial.print("ID ");
  Serial.println(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK)
  {
    Serial.println("Stored!");
    downloadFingerprintTemplate(id);
  }
  else if (p == FINGERPRINT_PACKETRECIEVEERR)
  {
    Serial.println("Communication error");
    return p;
  }
  else if (p == FINGERPRINT_BADLOCATION)
  {
    Serial.println("Could not store in that location");
    return p;
  }
  else if (p == FINGERPRINT_FLASHERR)
  {
    Serial.println("Error writing to flash");
    return p;
  }
  else
  {
    Serial.println("Unknown error");
    return p;
  }

  return true;
}

/*

uint8_t getFingerprintEnroll()
{
  id = getNextFreeID();
  int p = -1;
  Serial.print("Waiting for valid finger to enroll as #");
  Serial.println(id);
  int countMatch = 0;
  do {
    p = finger.getImage();
    Serial.print("p Status: ");
    Serial.println(String(p));
    countMatch++;
    switch (p)
    {
      case FINGERPRINT_OK:
        Serial.println("Image taken");
        countMatch = 100;
        break;
      case FINGERPRINT_NOFINGER:
        Serial.println(".");
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        countMatch = 100;
        break;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error");
        countMatch = 100;
        break;
      default:
        Serial.println("Unknown error");
        break;
    }
    delay(50); // Increase delay to yield control
  } while (countMatch < 100);

  if (p == FINGERPRINT_OK) {
    p = finger.image2Tz(1);
    Serial.print("p Status 2nd: ");
    Serial.println(String(p));
    switch (p)
    {
      case FINGERPRINT_OK:
        Serial.println("Image converted");
        break;
      case FINGERPRINT_IMAGEMESS:
        Serial.println("Image too messy");
        return p;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        return p;
      case FINGERPRINT_FEATUREFAIL:
      case FINGERPRINT_INVALIDIMAGE:
        Serial.println("Could not find fingerprint features");
        return p;
      default:
        Serial.println("Unknown error");
        return p;
    }

    Serial.println("Remove finger");
    delay(1000);
    while (finger.getImage() != FINGERPRINT_NOFINGER) {
      delay(1); // Yield control
    }

    Serial.println("Place same finger again");
    countMatch = 0;
    do {
      p = finger.getImage();
      countMatch++;
      switch (p)
      {
        case FINGERPRINT_OK:
          Serial.println("Image taken");
          countMatch = 100;
          break;
        case FINGERPRINT_NOFINGER:
          Serial.print(".");
          break;
        case FINGERPRINT_PACKETRECIEVEERR:
          Serial.println("Communication error");
          countMatch = 100;
          break;
        case FINGERPRINT_IMAGEFAIL:
          Serial.println("Imaging error");
          countMatch = 100;
          break;
        default:
          Serial.println("Unknown error");
          break;
      }
      delay(50); // Increase delay to yield control
    } while (countMatch < 100);

    if (p == FINGERPRINT_OK) {
      p = finger.image2Tz(2);
      switch (p)
      {
        case FINGERPRINT_OK:
          Serial.println("Image converted");
          break;
        case FINGERPRINT_IMAGEMESS:
          Serial.println("Image too messy");
          return p;
        case FINGERPRINT_PACKETRECIEVEERR:
          Serial.println("Communication error");
          return p;
        case FINGERPRINT_FEATUREFAIL:
        case FINGERPRINT_INVALIDIMAGE:
          Serial.println("Could not find fingerprint features");
          return p;
        default:
          Serial.println("Unknown error");
          return p;
      }

      Serial.print("Creating model for #");
      Serial.println(id);

      p = finger.createModel();
      if (p == FINGERPRINT_OK)
      {
        Serial.println("Prints matched!");
      }
      else if (p == FINGERPRINT_PACKETRECIEVEERR)
      {
        Serial.println("Communication error");
        return p;
      }
      else if (p == FINGERPRINT_ENROLLMISMATCH)
      {
        Serial.println("Fingerprints did not match");
        return p;
      }
      else
      {
        Serial.println("Unknown error");
        return p;
      }

      Serial.print("ID ");
      Serial.println(id);
      p = finger.storeModel(id);
      if (p == FINGERPRINT_OK)
      {
        Serial.println("Stored!");
      }
      else if (p == FINGERPRINT_PACKETRECIEVEERR)
      {
        Serial.println("Communication error");
        return p;
      }
      else if (p == FINGERPRINT_BADLOCATION)
      {
        Serial.println("Could not store in that location");
        return p;
      }
      else if (p == FINGERPRINT_FLASHERR)
      {
        Serial.println("Error writing to flash");
        return p;
      }
      else
      {
        Serial.println("Unknown error");
        return p;
      }

      // Add retry mechanism for loading and transferring templates
      for (int attempt = 0; attempt < 3; attempt++) {
        p = finger.loadModel(id);
        if (p == FINGERPRINT_OK) break;
        Serial.print("Failed to load template: ");
        Serial.println(p);
        delay(1000); // Retry delay
      }
      if (p != FINGERPRINT_OK) return p;

      Serial.print("Template AFEER ");
      Serial.print(id);
      Serial.println(" loaded");

      for (int attempt = 0; attempt < 3; attempt++) {
        p = finger.getModel();
        if (p == FINGERPRINT_OK) break;
        Serial.print("Failed to get template: ");
        Serial.println(p);
        delay(1000); // Retry delay
      }
      if (p != FINGERPRINT_OK) return p;

      Serial.print("Template ");
      Serial.print(id);
      Serial.println(" transferring:");

      uint8_t bytesReceived[534] = {0};
      uint32_t starttime = millis();
      int i = 0;
      while (i < 534 && (millis() - starttime) < 20000)
      {
        if (serialPort.available())
        {
          bytesReceived[i++] = serialPort.read();
        }
        delay(1); // Yield control
      }
      Serial.print(i);
      Serial.println(" bytes read.");
      Serial.println("Decoding packet...");

      uint8_t fingerTemplate[512] = {0};

      int uindx = 9, index = 0;
      while (index < 534)
      {
        while (index < uindx)
          ++index;
        uindx += 256;
        while (index < uindx)
        {
          fingerTemplate[index++] = bytesReceived[index];
        }
        uindx += 2;
        while (index < uindx)
          ++index;
        uindx = index + 9;
        delay(1); // Yield control
      }
      for (int i = 0; i < 512; ++i)
      {
        printHex(fingerTemplate[i], 2);
      }
      Serial.println("\ndone.");
    } else {
      Serial.println("Fingerprints do not match");
    }
  } else {
    Serial.println("Failed to take first fingerprint image");
  }
  delay(1000);
  Serial.println("Welcome");
  Serial.println("End");
  // Function for sending data to backend
  // SendData();
}


*/

/*


uint8_t getFingerprintID() {
    uint8_t p = finger.getImage();
    switch (p) {
        case FINGERPRINT_OK:
            Serial.println("Image taken");
            break;
        case FINGERPRINT_NOFINGER:
            Serial.println("No finger detected");
            return p;
        case FINGERPRINT_PACKETRECIEVEERR:
            Serial.println("Communication error");
            return p;
        case FINGERPRINT_IMAGEFAIL:
            Serial.println("Imaging error");
            return p;
        default:
            Serial.println("Unknown error");
            return p;
    }

    // OK success!

    p = finger.image2Tz();
    switch (p) {
        case FINGERPRINT_OK:
            Serial.println("Image converted");
            break;
        case FINGERPRINT_IMAGEMESS:
            Serial.println("Image too messy");
            return p;
        case FINGERPRINT_PACKETRECIEVEERR:
            Serial.println("Communication error");
            return p;
        case FINGERPRINT_FEATUREFAIL:
            Serial.println("Could not find fingerprint features");
            return p;
        case FINGERPRINT_INVALIDIMAGE:
            Serial.println("Could not find fingerprint features");
            return p;
        default:
            Serial.println("Unknown error");
            return p;
    }

    // OK converted!
    p = finger.fingerSearch();
    if (p == FINGERPRINT_OK) {
        Serial.println("Found a print match!");
    } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
        Serial.println("Communication error");
        return p;
    } else if (p == FINGERPRINT_NOTFOUND) {
        Serial.println("Did not find a match");
        return p;
    } else {
        Serial.println("Unknown error");
        return p;
    }

    // found a match!
    Serial.print("Found ID #");
    Serial.print(finger.fingerID);
    Serial.print(" with confidence of ");
    Serial.println(finger.confidence);

    int FingerPrintId = (int)finger.fingerID; // Convert uint8_t to int
    Serial.print("Found ID #");
    Serial.print(FingerPrintId);
    Serial.print(" with confidence of ");
    Serial.println(finger.confidence);

    return finger.fingerID;
}
*/
// returns -1 if failed, otherwise returns ID #
bool matchRfid(int fingerid, int nemo)
{
    bool rfidFound = false;
    String Empid;

    Serial.println("inside else part Match Rfid");
    Serial.println(fingerid);
    File datafiles = SPIFFS.open("/EmpRfid.csv", "r");
    if (!datafiles) {
        Serial.println("Failed to open file for reading");
        return false;
    }

    while (datafiles.available()) {
        String line = datafiles.readStringUntil('\n');
        Serial.print("Line: ");
        Serial.println(line);

        int commaIndex = line.indexOf(',');
        if (commaIndex == -1) {
            continue; // Skip lines that don't have a comma
        }

        String idfile = line.substring(0, commaIndex);
        Empid = line.substring(commaIndex + 1); // Assuming empId is the second value

        Serial.println("Empid check: " + Empid);
        int idmatchCheck = idfile.toInt();

        if (fingerid == idmatchCheck) {
            Serial.println("ID Found");
            ServerSend(Empid, CompanyId);
            rfidFound = true;
            break;
        }
    }
    datafiles.close();

    if (rfidFound) {
        Serial.print("ID: ");
        Serial.println(fingerid);
        Serial.print("EmployeeID: ");
        Serial.println(Empid);
        Serial.println("OFFLINE Preparation");
        Serial.println("DOOR OPEN");
        // OpenDoors = true;
        OfflineDataWrite(Empid);
    } else {
        Serial.println("RFID not found in CSV");
        digitalWrite(REDLED, HIGH);
        digitalWrite(REDLED1, HIGH);
        // UnauthorizedAccess();
        vTaskDelay(pdMS_TO_TICKS(300));
        digitalWrite(REDLED, LOW);
        digitalWrite(REDLED1, LOW);
    }

    vTaskDelay(pdMS_TO_TICKS(2000));

    return rfidFound;
}

void SensorFingerBegin(){
  finger.begin(57600);
  if (finger.verifyPassword())
  {
    Serial.println("Found fingerprint sensor!");
  }
  else
  {
    Serial.println("Did not find fingerprint sensor :(");
  }
  Serial.println(F("Reading sensor parameters"));
  finger.getParameters();
  Serial.print(F("Status: 0x"));
  Serial.println(finger.status_reg, HEX);
  Serial.print(F("Sys ID: 0x"));
  Serial.println(finger.system_id, HEX);
  Serial.print(F("Capacity: "));
  Serial.println(finger.capacity);
  Serial.print(F("Security level: "));
  Serial.println(finger.security_level);
  Serial.print(F("Device address: "));
  Serial.println(finger.device_addr, HEX);
  Serial.print(F("Packet len: "));
  Serial.println(finger.packet_len);
  Serial.print(F("Baud rate: "));
  Serial.println(finger.baud_rate);
}

int ServerSend(String Empid, String companyId)
{
  Serial.print("Empid ");
  Serial.println(Empid);
  Serial.print("COMPANY ID ");
  Serial.println(companyId);
  DynamicJsonBuffer JSONbuffer;
  JsonObject &JSONencoder = JSONbuffer.createObject();
  DateTime now = rtc.now();
  Serial.print("Current time: ");
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(" ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
  Serial.println("Sending data TO Backend");
  HTTPClient http; // Declare object of class HTTPClient
  JSONencoder["employeeId"] = Empid;
  JSONencoder["rfid"] = "-";
  JSONencoder["deviceId"] = DeviceId;
  JSONencoder["companyId"] = companyId;
  JSONencoder["deviceType"] = "Biometric";
  JSONencoder["date"] = String(now.year()) + String("-") + String(now.month(), DEC) + String("-") + String(now.day());
  JSONencoder["time"] = String(now.hour()) + String(":") + String(now.minute()) + String(":") + String(now.second());
  Serial.println(String(now.hour()) + String(":") + String(now.minute()) + String(":") + String(now.second()));
  char JSONmessageBuffer[300];
  JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  Serial.print("JSON MESSAGE server send");
  Serial.println(JSONmessageBuffer);
  http.begin("http://13.126.195.214:8080/EmployeeAttendenceAPI/employee/EmployeeCheckInOut"); // Specify request destination
  http.addHeader("Content-Type", "application/json");                                         // Specify content-type header
  Serial.println("Here after content type");
  int httpCode = http.POST(JSONmessageBuffer); // Send the request
  Serial.print("HttpCode:");
  Serial.println(httpCode);
  if (httpCode == 200)
  {
    Serial.println("inside http code ");
    DynamicJsonBuffer jsonBuffer(300);
    // Parse JSON object
    Serial.println("parse json");
    JsonObject &root = jsonBuffer.parseObject(http.getString());
    root.printTo(Serial);
    Serial.println("json object");
    const char *code = root["employeeId"];
    const char *department = root["department"];
    const char *retStatus = root["status"];
    const char *printStatus = root["status"];
    const char *userName = root["employeeName"];
    Serial.print("Code return element = ");
    Serial.println("EmployeeId ");
    Serial.println(code);
    Serial.println("Employee name ");
    Serial.println(userName);
    Serial.println("Status name ");
    Serial.println(printStatus);
    if ((strcmp(retStatus, "CHECKIN") == 0))
    {
      Serial.println("Inside Open Door send Server");
      // OpenDoors = true;
      // Fetching = true;
    }
    else if ((strcmp(retStatus, "CHECKOUT") == 0))
    {
      // OpenDoor();
      // OpenDoors = true;
      // Fetching = true;
    }
    else if (strcmp(retStatus, "SAME_TIME") == 0)
    {
      // CloseDoors = true;
      // Fetching = true;
      return -1;
    }
    else if ((strcmp(retStatus, "BLOCKED") == 0))
    {
      UnauthorizedAccess();
      // CloseDoors = true;
      digitalWrite(REDLED, HIGH); // turn the LED off.
      digitalWrite(REDLED1, HIGH);
      return -1;
      // Fetching = true;
    }
    else if ((strcmp(retStatus, "NOT_VAILD") == 0))
    {
      UnauthorizedAccess();
      digitalWrite(REDLED, HIGH); // turn the LED off.
      digitalWrite(REDLED1, HIGH);
      return -1;
      // CloseDoors = true;
      // Fetching = true;
    }
    else if ((strcmp(retStatus, "Employee_Not_Assigned_To_The_Device") == 0))
    {
      UnauthorizedAccess();
      // CloseDoors = true;
      digitalWrite(REDLED, HIGH); // turn the LED off.
      digitalWrite(REDLED1, HIGH);
      // Fetching = true;
      return -1;
    }
    else if ((strcmp(retStatus, "RFID_NO_Is_Not_Mapped_To_Any_Employee") == 0))
    {
      UnauthorizedAccess();
      // CloseDoors = true;
      digitalWrite(REDLED, HIGH); // turn the LED off.
      digitalWrite(REDLED1, HIGH);
      // Fetching = true;
      return -1;
    }

    else
    {
      Serial.print("Wrong method followed");
      // Fetching = true;
      return -1;
    }
    Serial.println(httpCode); // Print HTTP return code
  }
  // NOT_VAILD
  else
  {
    Serial.println("could not send back to server ");
    matchRfid(FingerPrintId, 1);
    // Fetching = true;
    // OfflineDataWrite(RfId, companyId);
  }
  
  http.end(); // Close connection
  // Serial.println("Succesfully Send Data To BackEnd");
  // i = 0;
  digitalWrite(GREENLED, LOW);
  digitalWrite(GREENLED1, LOW);
  digitalWrite(REDLED, LOW);
  digitalWrite(REDLED1, LOW);
  // added green led
  digitalWrite(GREENLED, HIGH); // turn the LED off.
  digitalWrite(GREENLED1, HIGH);
  delay(1000);
}

uint8_t deleteFingerprint(uint8_t id)
{
  uint8_t p = -1;

  p = finger.deleteModel(id);

  if (p == FINGERPRINT_OK)
  {
    Serial.println("Deleted!");
  }
  else if (p == FINGERPRINT_PACKETRECIEVEERR)
  {
    Serial.println("Communication error");
  }
  else if (p == FINGERPRINT_BADLOCATION)
  {
    Serial.println("Could not delete in that location");
  }
  else if (p == FINGERPRINT_FLASHERR)
  {
    Serial.println("Error writing to flash");
  }
  else
  {
    Serial.print("Unknown error: 0x");
    Serial.println(p, HEX);
  }

  return p;
}

/*
uint8_t getFingerprintID()
{
    uint8_t p = finger.getImage();
    switch (p)
    {
    case FINGERPRINT_OK:
        Serial.println("Image taken");
        break;
    case FINGERPRINT_NOFINGER:
        Serial.println("No finger detected");
        return p;
    case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        return p;
    case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error");
        return p;
    default:
        Serial.println("Unknown error");
        return p;
    }

    // OK success!

    p = finger.image2Tz();
    switch (p)
    {
    case FINGERPRINT_OK:
        Serial.println("Image converted");
        break;
    case FINGERPRINT_IMAGEMESS:
        Serial.println("Image too messy");
        return p;
    case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        return p;
    case FINGERPRINT_FEATUREFAIL:
        Serial.println("Could not find fingerprint features");
        return p;
    case FINGERPRINT_INVALIDIMAGE:
        Serial.println("Could not find fingerprint features");
        return p;
    default:
        Serial.println("Unknown error");
        return p;
    }

    // OK converted!
    p = finger.fingerSearch();
    if (p == FINGERPRINT_OK)
    {
        Serial.println("Found a print match!");
    }
    else if (p == FINGERPRINT_PACKETRECIEVEERR)
    {
        Serial.println("Communication error");
        return p;
    }
    else if (p == FINGERPRINT_NOTFOUND)
    {
        Serial.println("Did not find a match");
        return p;
    }
    else
    {
        Serial.println("Unknown error");
        return p;
    }

    // found a match!
    Serial.print("Found ID #");
    Serial.print(finger.fingerID);
    Serial.print(" with confidence of ");
    Serial.println(finger.confidence);

    return finger.fingerID;
}
*/

