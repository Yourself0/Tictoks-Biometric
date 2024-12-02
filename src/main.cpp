/*
IT'S BORING I KNEW IT
*/

#define Build_Version "TICK-BIOM_V2_Aug_21_2024"
#include <Arduino.h>
#include <SD.h>
#include <SPIFFS.h>
#include <EEPROM.h>
#include <RTClib.h>
#include <stdlib.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#include <Wire.h>
#include <stdio.h>
#include <vector>
#include "mbedtls/base64.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_Fingerprint.h>

#define Relay 4
#define Buzzer 15
#define RST_PIN 27
#define REDLED 12
#define SS_1_PIN 2
#define REDLED1 25
#define GREENLED 13
#define ORANGELED 14
#define GREENLED1 33
#define SS_2_PIN 26
#define NO_OF_READERS 2
#define WATCHDOG_TIMEOUT_SECONDS 20

/*
#define REDLED 13
#define SS_1_PIN 2
#define REDLED1 33
#define RST_PIN 27
#define SS_2_PIN 26
#define GREENLED 12
#define ORANGELED 14
#define GREENLED1 25
*/
long OtpVerifiy;
bool WifiPage = false;
bool quicksetupCld = false;
bool FingerAlreadyExist = false;
bool RfidRegisterPage = false;
bool CompanyPage = false;
bool DeviceidPage = false;
bool RfidRegister = false;
bool wifiScanRequested = false;
bool UpdateEmployee = true;
bool WebsocketConnected = false;
bool RegistrationFinger = true;
bool Network_status = true;
bool SpiffsTimerStart = false;
bool Fetching = true;
bool OpenDoors = false;
bool CloseDoors = false;
bool OrganizationStatuss;

int FingerPrintId = 0;

const char *ASSID = "Tictoks Biometric V1";
const char *APASS = "123456789";

String CompanyId = "";
String CompanyName = "";
String DeviceType = "Biometric";
String DeviceId;
String json;
String DeviceList;
String servername = "www.google.com";
String RegEmpId = "";
String hostName = "tictoksbiometric";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncWebSocket ws1("/Status");

TaskHandle_t WiFiscanTaskHandleC;
TaskHandle_t WifiStatusDataC = NULL;
TaskHandle_t UpdateEmployeeDetailC = NULL;
TaskHandle_t OpenDoorC = NULL;
TaskHandle_t CloseDoorC = NULL;
TaskHandle_t SdOfflineDataC = NULL;
TaskHandle_t printTaskHandleC = NULL;
TaskHandle_t OrganisationStatusC = NULL;

RTC_DS3231 rtc;
WiFiClient client;

HardwareSerial serialPort(2); // use UART2
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&serialPort);

bool matchRfid(int id);
uint8_t id;
uint8_t getNextFreeID();
uint8_t getFingerprintEnroll();
void deleteFingerprint(uint8_t id);
int getFingerprintID();
int ServerSend(String RfId, String companyId);
// void mountinSD();
void RestartEsp();
void MDNSServer();
void UpdateActivity();
void WifiConnectCheck();
void fileReadAndWrite();
void SensorFingerBegin();
void DeviceIdInitialize();
void WifiStatusConnected();
void UnauthorizedAccess();
void registerSendtoBios();
void ledFetchingon();
void ledFetchingoff();
void delete_activity(String rfid, int Matched, String empId, String Name, String Department);
void updated_Activity(String rfid, int Matched, String empId, String Name, String Department);
// void updated_ActivityBio(String rfid, int Matched, String empId, String Name, String Department, int BioRegster);
void updated_ActivityBio(String rfid, int Matched, String empId, String Name, String Department, int TemplateId, int BioRegster);
void printHex(int num, int precision);
bool CheckNullonServerRfidInitialFetch();

/*

  // https://wildfly.tictoks.in/EmployeeAttendenceAPI/OrganizationDetails/GetOrganizationDetails

  if(WiFi.begin()!= WL_CONNECTED){
    if(client.connect("www.google.com",80)){
      HTTPClient http;
      if(CompanyId.length()>0){
      DynamicJsonBuffer jsonBuffer;
      JsonObject &JSONEncoder = jsonBuffer.createObject();
      JSONEncoder["companyId"] = CompanyId;
      char JSONmessageBuffer[500];
      JSONEncoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
      esp_task_wdt_reset();
      yield();
      Serial.println(JSONmessageBuffer);
      http.begin("https://wildfly.tictoks.in/EmployeeAttendenceAPI/OrganizationDetails/GetOrganizationDetails");
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
          char cStatus = root["activityStatus"];
          Serial.print("Emp rfid size ");
          EEPROM.begin(512);
          int status = int(cStatus);
          EEPROM.write(511,status);

        }  // https://wildfly.tictoks.in/EmployeeAttendenceAPI/OrganizationDetails/GetOrganizationDetails

  if(WiFi.begin()!= WL_CONNECTED){
    if(client.connect("www.google.com",80)){
      HTTPClient http;
      if(CompanyId.length()>0){
      DynamicJsonBuffer jsonBuffer;
      JsonObject &JSONEncoder = jsonBuffer.createObject();
      JSONEncoder["companyId"] = CompanyId;
      char JSONmessageBuffer[500];
      JSONEncoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
      esp_task_wdt_reset();
      yield();
      Serial.println(JSONmessageBuffer);
      http.begin("https://wildfly.tictoks.in/EmployeeAttendenceAPI/OrganizationDetails/GetOrganizationDetails");
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
          char cStatus = root["activityStatus"];
          Serial.print("Emp rfid size ");
          EEPROM.begin(512);
          int status = int(cStatus);
          EEPROM.write(511,status);

        }
      }
    }
    }

  }
  }
    }
    }

  }

*/

bool OrganizationStatus()
{

  EEPROM.begin(512);
  char cStatus = EEPROM.read(511);
  int status = int(cStatus);
  if (status > 0)
  {
    Serial.println("Retrun Turn");
    return true;
  }
  else
  {
    Serial.println("Return False");
    return false;
  }
}

void WifiStatusNotConnected()
{
  if (WebsocketConnected)
  {
    Serial.print("Sent not connected");
    ws1.textAll("Not Connected");
    digitalWrite(ORANGELED, HIGH);
    digitalWrite(ORANGELED, HIGH);
  }
}

void WifiStatusConnected()
{
  if (WebsocketConnected)
  {
    ws1.textAll("Connected");
    Serial.println("Sent Connected");
    digitalWrite(ORANGELED, LOW);
  }
  else
  {
    Serial.println("it was false man ");
  }
}

void WiFiscanTask(void *pvParameters)
{
  while (true)
  {
    // Wait for notification to start scan
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    Serial.print("inside wifi scan task ");
    Serial.println("Insider wifi scan request");
    DynamicJsonBuffer jsonBuffer;
    JsonArray &networks = jsonBuffer.createArray();
    wifiScanRequested = false;
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
    if (networks.size() > 0)
    {
      json = "";
      networks.printTo(json);
      Serial.print("Json Data :");
      Serial.println(json);
    }
    // Delay before next scan
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void SdOfflineData(void *parameter)
{
  while (true)
  {
    SpiffsTimerStart = true;
    Serial.println("Spiffs Start :" + String(SpiffsTimerStart));
    if (SpiffsTimerStart)
    {
      Serial.println("Sending offline data to server");
      Serial.println("Wifi status connected :" + String(WebsocketConnected));

      if (WiFi.status() == WL_CONNECTED)
      {
        digitalWrite(ORANGELED, LOW);
        Network_status = false;
        if (client.connect("www.google.com", 80))
        {
          HTTPClient http; // Declare object of class HTTPClient

          Network_status = true;
          if (SPIFFS.exists("/OfflineData.csv"))
          {
            File csv = SPIFFS.open("/OfflineData.csv", "r");
            if (!csv)
            {
              Serial.println("Failed to open file");
              continue;
            }

            Serial.println("Connected to Internet ./");
            Serial.println("Wifi status connected " + String(WebsocketConnected));
            if (csv.available())
            {
              Serial.println("Sending Offline data to server");
              const byte BUFFER_SIZE = 200;
              char buffer[BUFFER_SIZE + 1];
              buffer[BUFFER_SIZE] = '\0';
              int file_count = 0;
              int success_count = 0;

              while (csv.available())
              {
                int count = 0;
                file_count += 1;
                Serial.println("file read fun");
                String line = csv.readStringUntil('\n');
                Serial.println("line");
                Serial.print(line);

                strncpy(buffer, line.c_str(), sizeof(buffer));
                char *ptr = strtok(buffer, ",");
                int j = 0;
                String EmpId, Date, Temp_Time;
                while (ptr != NULL)
                {
                  if (j == 0)
                  {
                    EmpId = ptr;
                  }
                  else if (j == 1)
                  {
                    Date = ptr;
                  }
                  else if (j == 2)
                  {
                    Temp_Time = ptr;
                  }
                  j++;
                  ptr = strtok(NULL, ",");
                }

                Serial.println("");
                Serial.print("Company id: ");
                Serial.println(CompanyId);
                Serial.print("EmployeeID: ");
                Serial.println(EmpId);
                Serial.print("Date: ");
                Serial.println(Date);
                Serial.print("Time: ");
                Serial.println(Temp_Time);

                if (EmpId != "")
                {
                  String Time = "";
                  for (int i = 0; i < Temp_Time.length(); i++)
                  {
                    char currentChar = Temp_Time.charAt(i);
                    if (currentChar != '\r')
                    {
                      Time += currentChar;
                    }
                  }

                  DynamicJsonBuffer JSONbuffer;
                  JsonObject &JSONencoder = JSONbuffer.createObject();
                  JSONencoder["employeeId"] = EmpId;
                  JSONencoder["companyId"] = CompanyId;
                  JSONencoder["deviceType"] = DeviceType;
                  JSONencoder["date"] = Date;
                  JSONencoder["time"] = Time;
                  char JSONmessageBuffer[300];
                  JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
                  Serial.print("JSON MESSAGE server send");
                  Serial.println(JSONmessageBuffer);

                  http.begin("http://letzcheckin.com:8080/EmployeeAttendenceAPI/employee/EmployeeCheckInOut");
                  http.addHeader("Content-Type", "application/json");
                  Serial.println("Here after content type");
                  int httpCode = http.POST(JSONmessageBuffer);
                  Serial.print("HttpCode:");
                  Serial.println(httpCode);

                  if (httpCode == 200)
                  {
                    Serial.println("Data Send Successfully");
                    success_count += 1;
                  }
                  else
                  {
                    count++;
                  }
                  http.end();
                  if (count > 3)
                  {
                    break;
                  }
                }
              }
              csv.close();
              SPIFFS.remove("/OfflineData.csv");
            }
          }
          else
          {
            Serial.println("File Does Not Exist");
          }
        }
        else
        {
          Serial.println("Internet is Not Available ........ ... ... ..");
          Network_status = false;
        }
      }
      else
      {
        Serial.println("WiFi was not connected offline data ");
        Serial.println("Websocket status: " + String(WebsocketConnected));
        Network_status = false;
        digitalWrite(ORANGELED, HIGH);
        int stationCount = WiFi.softAPgetStationNum();
        Serial.print("Station Count");
        Serial.println(stationCount);
        if (stationCount == 0)
        {
          WifiConnectCheck();
        }
      }
      Serial.println("End of send server ");
    }
    vTaskDelay(pdMS_TO_TICKS(60000));
  }
}

/*
void updateEmployeeDetails(void *pvParameters)
{
  (void)pvParameters; // Unused parameter
  while (true)
  {
    Serial.println("Update Function Updates 1");
    if (WiFi.status() == WL_CONNECTED)
    {
      vTaskDelay(pdMS_TO_TICKS(1800000000));
      Serial.println("Update Function 3");
      // UpdateActivity();
      if(WiFi.status()== WL_CONNECTED){
        if(client.connect("www.google.com",80)){
          SPIFFS.remove("/EmpRfid.csv");
          SPIFFS.remove("/Rfid.csv");
        }
      }
      else{
        if(SPIFFS.exists("/BioRegs.csv")){
          while(true){
            ledFetchingon();
            vTaskDelay(pdMS_TO_TICKS(1000));
            ledFetchingoff();
            if(client.connect("www.google.com",80)){
              break;
            }
          }
        }
      }
      RestartEsp();
      fileReadAndWrite();
    }
    else
    {
      Serial.println("Wi-Fi not connected");
    }
    vTaskDelay(pdMS_TO_TICKS(1800000000));
  }
}
  */

void updateEmployeeDetails(void *pvParameters)
{
  (void)pvParameters; // Unused parameter

  while (true)
  {
    // Wait for 5 hours
    Serial.println("first delay started");
    // vTaskDelay(pdMS_TO_TICKS(60000)); // Delay for 10 minutes
    
    vTaskDelay(pdMS_TO_TICKS(600000)); // Delay for 10 minutes
    Serial.println("first delay Ended");
    Serial.println("second Delay");
    vTaskDelay(pdMS_TO_TICKS(600000)); // Delay for 10 minutes
    Serial.println("Second delay Ended");

    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("Connected to Wi-Fi. Checking server...");

      if (client.connect("www.google.com", 80))
      {
        // Call CheckNullonServerRfidInitialFetch and check if data exists
        bool hasData = CheckNullonServerRfidInitialFetch();
        if (hasData)
        {
          Serial.println("Data fetched from server. Removing files...");
          SPIFFS.remove("/EmpRfid.csv");
          SPIFFS.remove("/Rfid.csv");
          Serial.println("Files removed.");
        }
        else
        {
          Serial.println("No data to update. Skipping file removal.");
        }
      }
      else
      {
        Serial.println("Failed to connect to the server. Skipping file removal.");
      }
    }
    else
    {
      Serial.println("Wi-Fi not connected. Skipping file removal.");
    }

    // Restart the ESP device after completing the task
    RestartEsp();
  }
}


// Update task multiple ...
// remove the finger checker condition...

// Open Door
void OpenDoor(void *pvParameters)
{
  (void)pvParameters; // Unused parameter

  pinMode(GREENLED, OUTPUT);
  pinMode(GREENLED1, OUTPUT);
  pinMode(Relay, OUTPUT);
  pinMode(Buzzer, OUTPUT);

  while (true)
  {
    if (OpenDoors)
    {
      Serial.println("Inside Open Door ");
      OpenDoors = false;
      digitalWrite(GREENLED, HIGH);
      digitalWrite(GREENLED1, HIGH);
      digitalWrite(Relay, HIGH);
      digitalWrite(Buzzer, HIGH);
      vTaskDelay(pdMS_TO_TICKS(300));
      vTaskDelay(pdMS_TO_TICKS(200));
      digitalWrite(Buzzer, LOW);
      vTaskDelay(pdMS_TO_TICKS(4000));
      digitalWrite(GREENLED, LOW);
      digitalWrite(GREENLED1, LOW);
      digitalWrite(Relay, LOW);
    }
    else
    {
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }
}

// Close Door
void CloseDoor(void *pvParameters)
{
  (void)pvParameters; // Unused parameter

  pinMode(REDLED, OUTPUT);
  pinMode(REDLED1, OUTPUT);
  pinMode(Relay, OUTPUT);
  pinMode(Buzzer, OUTPUT);

  while (true)
  {
    if (CloseDoors)
    {
      Serial.println("Inside Close Door");
      digitalWrite(REDLED, HIGH);
      digitalWrite(REDLED1, HIGH);
      digitalWrite(GREENLED1, LOW);
      digitalWrite(GREENLED, LOW);
      CloseDoors = false;
      for (int i = 0; i < 2; i++)
      {                    // Repeat the pattern 3 times
        tone(Buzzer, 800); // Play the beep tone
        delay(500);        // Wait for the specified duration
        noTone(Buzzer);    // Stop the tone
        delay(500);
      }
      vTaskDelay(pdMS_TO_TICKS(2000));
      digitalWrite(REDLED, LOW);
      digitalWrite(REDLED1, LOW);
      // UnauthorizedAccess();
    }
    else
    {
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }
}

void wifiStatusData(void *pvParameters)
{
  (void)pvParameters;
  while (true)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      if (client.connect("www.google.com", 80))
      {
        WifiStatusConnected();
        client.stop(); // Close the connection
      }
      else
      {
        ws1.textAll("Internet Not Available");
      }
    }
    else
    {
      WifiStatusNotConnected();
    }

    vTaskDelay(pdMS_TO_TICKS(10000)); // Delay for 10 seconds
  }
}

/*
void initializeCoreWork()
{
  xTaskCreate(
      wifiStatusData,  // Task function
      "OfflineData",   // Task name
      8192,            // Stack size (bytes)
      NULL,            // Task input parameter
      5,               // Priority
      &WifiStatusDataC // Task handle
  );



  xTaskCreate(
      updateEmployeeDetails, // Task function
      "OfflineData",         // Task name
      8192,                  // Stack size (bytes)
      NULL,                  // Task input parameter
      2,                     // Priority
      &UpdateEmployeeDetailC // Task handle
  );


  // Offline Data Sync
  xTaskCreate(
      SdOfflineData,  // Task function
      "OfflineData",  // Task name
      8192,           // Stack size (bytes)
      NULL,           // Task input parameter
      5,              // Priority
      &SdOfflineDataC // Task handle
  );

  // Open Door
  xTaskCreate(
      OpenDoor,       // Task function
      "Open Door s ", // Task name
      2048,           // Stack size (bytes)
      NULL,           // Task input parameter
      4,              // Priority
      &OpenDoorC      // Task handle
  );

  // Closed Door
  xTaskCreate(
      CloseDoor,      // Task function
      "Close Door s", // Task name
      2048,           // Stack size (bytes)
      NULL,           // Task input parameter
      3,              // Priority
      &CloseDoorC     // Task handle
  );

  xTaskCreate(
      OrganisationStatus,  // Task function
      "Organization Status",  // Task name
      8192,           // Stack size (bytes)
      NULL,           // Task input parameter
      5,              // Priority
      &SdOfflineDataC // Task handle
  );
}
*/

void initializeCoreWork()
{
  // Check WiFi status data task, assigned highest priority
  xTaskCreate(
      wifiStatusData,   // Task function
      "WifiStatusData", // Task name
      8192,             // Stack size (bytes)
      NULL,             // Task input parameter
      2,                // Lower Priority
      &WifiStatusDataC  // Task handle
  );

  // Offline Data Sync, lower priority now
  xTaskCreate(
      SdOfflineData,   // Task function
      "SdOfflineData", // Task name
      8192,            // Stack size (bytes)
      NULL,            // Task input parameter
      2,
      &SdOfflineDataC // Task handle
  );

  // Open Door task, high priority since it deals with physical security
  xTaskCreate(
      OpenDoor,   // Task function
      "OpenDoor", // Task name
      2048,       // Stack size (bytes)
      NULL,       // Task input parameter
      4,          // Priority (High)
      &OpenDoorC  // Task handle
  );

  // Close Door task, slightly lower priority than open door
  xTaskCreate(
      CloseDoor,   // Task function
      "CloseDoor", // Task name
      2048,        // Stack size (bytes)
      NULL,        // Task input parameter
      4,           // Priority (Mid-High)
      &CloseDoorC  // Task handle
  );

  // Organization status task, standard priority

  // Uncomment this if needed, with appropriate priority
  xTaskCreate(
      updateEmployeeDetails,  // Task function
      "UpdateEmployee",       // Task name
      8192,                   // Stack size (bytes)
      NULL,                   // Task input parameter
      2,                      // Priority (Lower)
      &UpdateEmployeeDetailC  // Task handle
  );
  
}

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
    Serial.println("Communication error");
    return p;
  default:
    Serial.print("Unknown error ");
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
    Serial.print("Unknown error ");
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

// WebSocket event Handle
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
  case WS_EVT_CONNECT:
    WebsocketConnected = true;
    RegistrationFinger = false;
    ws.textAll("Connected");
    Serial.println("Websocket is true now");
    Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());

    break;
  case WS_EVT_DISCONNECT:
    RegistrationFinger = true;
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

void wifiStatusEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                     AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
  case WS_EVT_CONNECT:

    WebsocketConnected = true;
    Serial.println("Websocket is true now");
    Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    break;
  case WS_EVT_DISCONNECT:
    WebsocketConnected = false;
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

/*
void InitializeRTC()
{
  Wire.begin();
  if (rtc.begin())
  {
    Serial.println("RTC is RUNNING");
    DateTime now = rtc.now();
    Serial.print(now.year());
    Serial.print(now.month());
    Serial.print(now.day());
  }
  else
  {
    Serial.println("RTC is NOT RUNNING");
  }
}

*/

void InitializeRTC()
{
  // Start I2C communication
  Wire.begin();

  // Initialize the RTC
  if (rtc.begin())
  {
    Serial.println("RTC is RUNNING");

    // Get the current date and time from the RTC
    DateTime now = rtc.now();

    // Print the current date
    Serial.print("Current Date: ");
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.println(now.day(), DEC);

    // Optionally, print the current time as well
    Serial.print("Current Time: ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.println(now.second(), DEC);
  }
  else
  {
    Serial.println("RTC is NOT RUNNING");
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

/*
void checkAllTemplates()
{
  int fingerBufLength = 500;
  int array [fingerBufLength];
  Serial.println("Checking all stored templates...");
  for (int i = 1; i <= fingerBufLength; i++)
  { // Assuming IDs from 1 to 255
    if (finger.loadModel(i) == FINGERPRINT_OK)
    {
      Serial.print("Template ID ");
      Serial.print(i);
      array [i] = i;
      Serial.println(" is stored.");
    }
    else{
      array[i] = 0;
    }
  }
  for (int i = 0 ; i <= fingerBufLength; i++){
    Serial.println(array[i]);
  }
  Serial.println("Finished checking all templates.");


  // Open the file from SPIFFS
  File datafilesBio = SPIFFS.open("/BioRegs.csv", "r");
  if (!datafilesBio)
  {
    Serial.println("Failed to open file for reading");
    return;
  }

  // Read the file line by line
  while (datafilesBio.available())
  {
    String line = datafilesBio.readStringUntil('\n');
    // Serial.print("Line: ");
    // Serial.println(line);

    // Find the comma separating ID and Empid
    int commaIndex = line.indexOf(',');
    if (commaIndex == -1)
    {
      continue; // Skip lines that don't have a comma
    }

    // Extract ID and Empid from the line
    String idfile = line.substring(0, commaIndex);
    String Empid = line.substring(commaIndex + 1); // Assuming Empid is the second value
    // Serial.println("Empid check: " + Empid);
    int idmatchCheck = idfile.toInt();
    bool got_Finger_est = false;
    // Check if the fingerprint ID matches
    for(int i = 1; i <= fingerBufLength; i++){
      if(array[i]!=0){
        if(array[i] == idmatchCheck){
          Serial.print("Finger id is there: ");
          got_Finger_est = true;
          Serial.println(array[i]);
          array[i] = 0;
          break;
        }
      }
    }
  }
  for(int i = 1; i<= fingerBufLength; i ++){
    if(array[i] != 0){
      Serial.print("Found Culprit");
      Serial.println(i);
      esp_task_wdt_reset();
      deleteFingerprint(i);
      Serial.print("Deleted the Fake Finger Check it ");
      Serial.println(i);
    }
  }
  // Close the file
  datafilesBio.close();

}

*/

void checkAllTemplates()
{
  int fingerBufLength = 500;
  int array[fingerBufLength] = {0}; // Initialize the array with zeros
  Serial.println("Checking all stored templates...");

  // Check all stored templates and store the IDs in the array
  for (int i = 1; i <= fingerBufLength; i++)
  {
    if (finger.loadModel(i) == FINGERPRINT_OK)
    {
      Serial.print("Template ID ");
      Serial.print(i);
      array[i] = i;
      Serial.println(" is stored.");
    }
    else
    {
      array[i] = 0;
    }
  }

  // Print the array values
  for (int i = 1; i <= fingerBufLength; i++)
  {
    Serial.println(array[i]);
  }
  Serial.println("Finished checking all templates.");

  // Open the file from SPIFFS
  File datafilesBio = SPIFFS.open("/BioRegs.csv", "r");
  if (!datafilesBio)
  {
    Serial.println("Failed to open file for reading");
    return;
  }
  Serial.println("Serial print open file");
  // Check if the file is empty
  if (datafilesBio.size() == 0)
  {
    Serial.println("The file is empty. Exiting function.");
    datafilesBio.close();
    return;
  }

  // Read the file line by line
  while (datafilesBio.available())
  {
    String line = datafilesBio.readStringUntil('\n');

    // Find the comma separating ID and Empid
    int commaIndex = line.indexOf(',');
    if (commaIndex == -1)
    {
      continue; // Skip lines that don't have a comma
    }

    // Extract ID and Empid from the line
    String idfile = line.substring(0, commaIndex);
    String Empid = line.substring(commaIndex + 1); // Assuming Empid is the second value
    int idmatchCheck = idfile.toInt();
    bool got_Finger_est = false;

    // Check if the fingerprint ID matches
    for (int i = 1; i <= fingerBufLength; i++)
    {
      if (array[i] != 0 && array[i] == idmatchCheck)
      {
        Serial.print("Finger id is there: ");
        got_Finger_est = true;
        Serial.println(array[i]);
        array[i] = 0; // Clear the entry once matched
        break;
      }
    }
  }

  // Check for any fingerprints that were not matched
  for (int i = 1; i <= fingerBufLength; i++)
  {
    if (array[i] != 0)
    {
      Serial.print("Found Culprit: ");
      Serial.println(i);
      esp_task_wdt_reset();
      deleteFingerprint(i);
      Serial.print("Deleted the Fake Finger Check it: ");
      Serial.println(i);
    }
  }

  // Close the file
  datafilesBio.close();
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
    WiFi.begin(wssid, wpass);
    int count = 0;
    esp_task_wdt_reset();
    while (WiFi.status() != WL_CONNECTED && count <= 5)
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
      // turn off led if
      digitalWrite(ORANGELED, LOW);
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
    digitalWrite(ORANGELED, HIGH);
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

void ledFetchingon()
{
  digitalWrite(REDLED, HIGH);
  digitalWrite(GREENLED, HIGH);
  digitalWrite(REDLED1, HIGH);
  digitalWrite(GREENLED1, HIGH);
}

void ledFetchingoff()
{
  digitalWrite(REDLED, LOW);
  digitalWrite(GREENLED, LOW);
  digitalWrite(REDLED1, LOW);
  digitalWrite(GREENLED1, LOW);
}

void delete_activity(String rfid, int Matched, String empId, String Name, String Department)
{
  File readFile = SPIFFS.open("/Rfid.csv", FILE_READ);
  if (!readFile)
  {
    Serial.println("Failed to open file for reading");
    return;
  }
  File writeFile = SPIFFS.open("/tempfile.csv", FILE_WRITE);
  if (!writeFile)
  {
    Serial.println("Failed to open file for writing");
    readFile.close();
    return;
  }
  bool lineFound = false;
  while (readFile.available())
  {
    String line = readFile.readStringUntil('\n');
    int commaIndex = line.indexOf(',');
    int secondCommaIndex = line.indexOf(',', commaIndex + 1);
    int thirdCommaIndex = line.indexOf(',', secondCommaIndex + 1);
    String currentEmpId = line.substring(secondCommaIndex + 1, thirdCommaIndex);
    int currentEmpidint = currentEmpId.toInt();
    int empIdint = empId.toInt();
    if (currentEmpidint == empIdint)
    {
      Serial.print("Current Empid: ");
      Serial.println(empId);
      lineFound = true;
      continue;
    }
    else
    {
      writeFile.print(line + "\n");
    }
  }
  if (!lineFound)
  {
    Serial.print("Line Found Delete:");
    Serial.print(empId);
    Serial.println(lineFound);
  }
  readFile.close();
  writeFile.close();

  if (lineFound)
  {
    // Remove the original file
    if (SPIFFS.remove("/Rfid.csv"))
    {
      Serial.println("Original file removed");
    }
    else
    {
      Serial.println("Failed to remove the original file");
    }

    // Rename the new file to the original file name
    if (SPIFFS.rename("/EmpRfid.csv", "/Rfid.csv"))
    {
      Serial.println("File renamed successfully");
    }
    else
    {
      Serial.println("Failed to rename the file");
    }
    Serial.println("Line updated successfully");
  }
  else
  {
    Serial.println("Line not found; new line added");
  }
}

void OfflineDataWrite(String empId)
{
  // mountinSD();
  String Date;
  String Time;
  // String csvData;
  Serial.println("Offline Data Write Inside");
  File file = SPIFFS.open("/OfflineData.csv", "a"); // Open the file in write mode (overwrite existing content)
  if (!file)
  {
    Serial.println("Failed to open file for writing");
    return;
  }
  Serial.println("Opens CSV file");
  DateTime now = rtc.now();
  String currentDate = String(now.year()) + "-" + String(now.month(), DEC) + "-" + String(now.day());
  String currentTime = String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second());
  String csvData = empId + String(",") + currentDate + String(",") + currentTime;
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

void UnauthorizedAccess()
{
  digitalWrite(REDLED, HIGH);
  digitalWrite(REDLED1, HIGH);
  for (int i = 0; i < 2; i++)
  {                    // Repeat the pattern 3 times
    tone(Buzzer, 500); // Play the beep tone
    delay(100);        // Wait for the specified duration
    noTone(Buzzer);    // Stop the tone
    delay(100);
  }
  vTaskDelay(pdMS_TO_TICKS(1000));
  digitalWrite(REDLED, LOW);
  digitalWrite(REDLED1, LOW);
}

void fileReadAndWrite()
{
  File data = SPIFFS.open("/BioRegs.csv", "w");
  File datafile = SPIFFS.open("/Rfid.csv", FILE_READ);
  if (!datafile)
  {
    Serial.println("Failed to open file for reading");
    return;
  }
  String line;
  bool rfidFound = false;
  while (datafile.available())
  {
    line = datafile.readStringUntil('\n');
    int commaIndex = line.indexOf(',');
    int secondCommaIndex = line.indexOf(',', commaIndex + 1);
    int thirdCommaIndex = line.indexOf(',', secondCommaIndex + 1);
    int fourthCommaIndex = line.indexOf(',', thirdCommaIndex + 1);
    int fifthCommaIndex = line.indexOf(',', fourthCommaIndex + 1);
    int sixthCommaIndex = line.indexOf(',', fifthCommaIndex + 1);
    if (commaIndex == -1)
    {
      continue;
    }
    // String Rfid = line.substring(0, commaIndex);
    // String DeviceIds = line.substring(commaIndex + 1, secondCommaIndex);
    String Empid = line.substring(secondCommaIndex + 1, thirdCommaIndex);
    String BioTemplateId = line.substring(fifthCommaIndex + 1, sixthCommaIndex);
    String BioRegsStatus = line.substring(sixthCommaIndex + 1);

    Serial.println("BioRegsStatus: " + BioRegsStatus);
    Serial.println("Bio Temp" + BioTemplateId);
    int BioTemplateIntId = BioTemplateId.toInt();
    int BioRegsIntStatus = BioRegsStatus.toInt();
    Serial.print("bio Int id ");
    Serial.println(BioTemplateIntId);
    esp_task_wdt_reset();
    if (BioTemplateIntId > 0)
    {
      if (BioRegsIntStatus == 1)
      {
        esp_task_wdt_reset();
        String content = BioTemplateId + String(",") + Empid + "\n";
        Serial.print("Inside Content : ");
        Serial.println(content);
        data.print(content);
      }
    }
    // Serial.println("Rfid and Deviceids "+Rfid+" "+DeviceIds);
    // Process the first two fields (e.g., print them)
  }
  data.close();
  datafile.close();
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
      // mountinSD();
      int falseReturn = 0;
      if (DeviceId.length() > 0 && CompanyId.length() > 0)
      {
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
          // http.begin("https://wildfly.tictoks.in:443/EmployeeAttendenceAPI/RFIDAPI/SelectAllEmployeeInfo");
          http.begin("https://wildfly.letzcheckin.com:443/EmployeeAttendenceAPI/BiometricAPI/SelectAllEmployeeInfo");
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
                File dataFile = SPIFFS.open("/Rfid.csv", "a");
                String empId = employeeInfoList[i]["employeeId"].as<const char *>();
                String Name = employeeInfoList[i]["name"].as<const char *>();
                // String DeviceIdM = employeeInfoList[i]["deviceId"].as<const char *>();                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    y"].as<const char *>();
                String Status = employeeInfoList[i]["status"].as<const char *>();
                String Department = employeeInfoList[i]["department"].as<const char *>();
                String rfidNo = employeeInfoList[i]["rfidNo"].as<const char *>();
                String BioTempD = employeeInfoList[i]["biometricTemplateId1"].as<const char *>();
                String BioRegs = employeeInfoList[i]["biometricFingerPrintStatus"].as<const char *>();
                char buf[sizeof(Status)];
                if (rfidNo == "" || rfidNo == " ")
                {
                  rfidNo = "-";
                }
                int Matched = 0;
                Serial.print("Device Id: ");
                Serial.println(Status);
                Matched = Status.toInt();
                Serial.print("Matched Value: ");
                Serial.println(Matched);
                Serial.print("Device ID: ");
                Serial.println("");
                int Bioregis = BioRegs.toInt();
                int BioTemps = BioTempD.toInt();
                if (BioTempD == "")
                {
                  BioTemps = 0;
                }
                if (Bioregis == 2)
                {
                  Serial.print("Inside delete Finger");
                  delay(1000);
                  Serial.println(BioTemps);
                  deleteFingerprint(BioTemps);

                  esp_task_wdt_reset();
                }

                if (BioRegs == "")
                {
                  Bioregis = 0;
                }
                String empData = rfidNo + String(",") + Matched + String(",") + empId + String(",") + Name + String(",") + Department + String(",") + String(BioTemps) + String(",") + String(Bioregis) + String("\n");
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
            // SPIFFS.remove("/Rfid.csv");
            esp_task_wdt_reset();
            Serial.print("Getting Error SPIFFS File Rfid removed");
            digitalWrite(REDLED, LOW);
            digitalWrite(GREENLED, LOW);
            digitalWrite(REDLED1, LOW);
            digitalWrite(GREENLED1, LOW);
            break;
          }
          http.end();
        } while (dataCount >= startCount - 1);
      }
    }
  }
  else
  {
    Serial.println("Not connected");
  }
  fileReadAndWrite();
  Serial.println("Rfid List");
  digitalWrite(REDLED, LOW);
  digitalWrite(GREENLED, LOW);
  digitalWrite(REDLED1, LOW);
  digitalWrite(GREENLED1, LOW);
}

void rfidInitialCheck()
{
  // mountinSD();
  if (SPIFFS.begin(true))
  {
    if (!SPIFFS.exists("/Rfid.csv"))
    {
      Serial.println("Rfid register File Doesn't Exist");
      rfidInitialList();
    }
    else
    {
      File DataFile = SPIFFS.open("/Rfid.csv");
      size_t fileSize = DataFile.size();
      Serial.println("Rfid Register File Exists :");
      Serial.println(fileSize);
      if (fileSize == 0)
      {
        DataFile.close();
        SPIFFS.remove("/Rfid.csv");
        Serial.println("File Removed");
      }
      else
      {
        Serial.print("File Already There");
        DataFile.close();
      }
    }
  }
  else
  {
    Serial.println("Sd is not There");
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
      esp_task_wdt_reset();
      JsonObject &JsonEncoder = JsonBuffer.createObject();
      char JSONcharMessage[500];
      JsonEncoder["companyId"] = CompanyId;
      JsonEncoder["deviceType"] = DeviceType;
      JsonEncoder.printTo(JSONcharMessage, sizeof(JSONcharMessage));
      http.begin("https://wildfly.letzcheckin.com:443/EmployeeAttendenceAPI/BiometricAPI/GetBiometricUnMappedDeviceList");
      esp_task_wdt_reset();
      http.addHeader("Content-type", "application/json");
      httpCode = http.POST(JSONcharMessage);
      String payload = http.getString();
      esp_task_wdt_reset();
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
          JsonArray &rfidDeviceList = root["biometricDeviceList"];
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

void MDNSServer()
{
  if (!MDNS.begin(hostName))
  {
    Serial.println("Error setting up MDNS responder!");
    esp_restart();
  }
  Serial.println("mDNS responder started");
}

void digitalRegister()
{
  digitalWrite(GREENLED1, HIGH);
  digitalWrite(GREENLED, HIGH);
  digitalWrite(REDLED1, LOW);
  digitalWrite(REDLED, LOW);
}

void hexStringToByteArray(const char *hexStr, uint8_t *byteArray, size_t length)
{
  for (size_t i = 0; i < length; ++i)
  {
    sscanf(hexStr + 2 * i, "%2hhx", &byteArray[i]);
  }
}

/*
void downloadFingerprintTemplate(uint8_t id) {
    // Load the template
    int p = finger.loadModel(id);
    if (p != FINGERPRINT_OK) {
        Serial.println("Error loading model");
     toreMode   return;
    }

    p = finger.getModel();
    if (p != FINGERPRINT_OK) {
        Serial.println("Error getting model");
        return;
    }

    // One data packet is 267 bytes. In one data packet, 11 bytes are 'useless'
    uint8_t bytesReceived[534]; // 2 data packets
    memset(bytesReceived, 0xff, 534);

    uint32_t starttime = millis();
    int i = 0;
    while (i < 534 && (millis() - starttime) < 20000) {
        if (mySerial.available()) {
            bytesReceived[i++] = mySerial.read();
     toreMode   }
    }
    Serial.print(i);
    Serial.println(" bytes read.");
    Serial.println("Decoding packet...");

    uint8_t fingerTemplate[512]; // The real template
    memset(fingerTemplate, 0xff, 512);

    // Filtering only the data packets
    int uindx = 9, index = 0;
    while (index < 534) {
        while (index < uindx)
            ++index;
        uindx += 256;
        while (index < uindx) {
            fingerTemplate[index++] = bytesReceived[index];
        }
        uindx += 2;
        while (index < uindx)
            ++index;
        uindx = index + 9;
    }

    // Convert the template to Base64
    String base64Template = base64::encode(fingerTemplate, 512);
toreMode
    Serial.println("Base64-encoded template:");
    Serial.println(base64Template); // Print the Base64-encoded template

    // Store or send the Base64 template as needed to a server for storing in a database



}
*/

/*
String ValidationChecker(int fingerid) {
    bool foundMatch = false;
    if (SPIFFS.begin(true)) {
        String Empid;
        File readstrings = SPIFFS.open("/BioRegs.csv", "r");
        if (!readstrings) {
            Serial.println("Failed to open file for reading");
            return "File Not Found";
        }
        while (readstrings.available()) {
            String line;
            line = readstrings.readStringUntil('\n');

            int commaIndex = line.indexOf(',');
            if (commaIndex == -1) {
                continue; // Skip lines that don't have a comma
            }

            String idfile = line.substring(0, commaIndex);
            Empid = line.substring(commaIndex + 1); // Assuming empId is the second value
            Serial.println("Empid check: " + Empid);
            RegEmpId = Empid;
            int idmatchCheck = idfile.toInt();
            if (fingerid == idmatchCheck) {
                Serial.println("ID Found");
                foundMatch = true;
                readstrings.close();
                return Empid;
            }
        }
        readstrings.close();
        if (!foundMatch) {
            return "Could Not Find Match";
        }
    } else {
        return "Storage is not working";
    }

    // Ensure a return value is provided even if SPIFFS.begin(true) fails
    return "Unknown Error";
}
*/

// changed string

String ValidationChecker(int fingerid)
{
  bool foundMatch = false;

  // Initialize SPIFFS
  if (SPIFFS.begin(true))
  {
    Serial.println("mount SPIFFS Correctly");
  }
  else
  {
    return "Failed to Initiate Spiffs";
  }

  // Open the file for reading
  File readstrings = SPIFFS.open("/BioRegs.csv", "r");
  if (!readstrings)
  {
    Serial.println("Failed to open file for reading");
    return "File Not Found";
  }

  String Empid;
  while (readstrings.available())
  {
    String line = readstrings.readStringUntil('\n');

    int commaIndex = line.indexOf(',');
    if (commaIndex == -1)
    {
      continue; // Skip lines that don't have a comma
    }

    String idfile = line.substring(0, commaIndex);
    Empid = line.substring(commaIndex + 1); // Assuming empId is the second value
    Serial.println("Empid check: " + Empid);
    int idmatchCheck = idfile.toInt();

    if (fingerid == idmatchCheck)
    {
      Serial.println("ID Found");
      foundMatch = true;
      readstrings.close(); // Close file when a match is found
      return Empid;
    }
  }

  readstrings.close();                                // Ensure file is closed after reading
  return foundMatch ? Empid : "Could Not Find Match"; // Return Empid if found, else error message
}

// AsyncWebServer code for webpage
void WebServerRoutes()
{
  if (SPIFFS.begin(true))
  {
    Serial.println("SPIFFS Beginned");
  }

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("Main page requested");
    // Debug Information 
    Serial.print("Rfid Register: ");
    Serial.println(RfidRegister);
    RfidRegister = false;
    RegistrationFinger = true;
    WebsocketConnected = true;
    OtpVerifiy = TokenVerifications();
    Serial.print("Otp Verifiy: ");
    Serial.println(OtpVerifiy);
    ResetWebserverPages();
    Serial.println("Reset Webserver pages called");
    registerSendtoBios();

    // checking if Mainpage is properly intialized
    request->send(SPIFFS, "/Mainpage.html", "text/html"); 
    Serial.println("Response sent"); });

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
  // server.on("/BiometricImg", HTTP_GET, [](AsyncWebServerRequest *request)
  //           { request->send(SPIFFS, "/Biometric.svg", "image/svg+xml"); });

  server.on("/BiometricImg", HTTP_GET, [](AsyncWebServerRequest *request)
            { 
              esp_task_wdt_reset();
              request->send(SPIFFS, "/Biometric.png", "image/png"); });

  server.on("/quickSetting.js", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("quick setup called");
    esp_task_wdt_reset();
    if (SPIFFS.exists("/quickSetting.js")) {
      esp_task_wdt_reset();
        request->send(SPIFFS, "/quickSetting.js", "application/javascript");
    } else {
        request->send(404, "text/plain", "File not found");
        } });

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
        esp_task_wdt_reset();
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

  /*
  server.on("/quickSetting.js", HTTP_GET, [](AsyncWebServerRequest *request)
            {
        Serial.print("quick setup called");
        if (SPIFFS.exists("/quickSetting.js")) {
            request->send(SPIFFS, "/quickSetting.js", "text/plain");
        } else {
            request->send(404, "text/plain", "File not found");
        } });

        */
  // RFID Page

  server.on("/upload", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/upload.html", "text/html"); });
  server.on("/File Test", HTTP_GET, [](AsyncWebServerRequest *request)
            { fileReadAndWrite();
              request->send(200, "Done", "text/plain"); });
  server.on("/Biometric", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              RegistrationFinger = false;
              fileReadAndWrite();
              // get the page

              // bool orgStatus = OrganizationStatus();
              // if(orgStatus){

              // }
              if (WiFi.status() != WL_CONNECTED)
              {
                request->send(200, "text/plane", "Connect Device to Network");
              }
              else
              {
                if (!(client.connect("www.google.com", 80)))
                {
                  request->send(200, "text/plane", "Check the Internet");
                }
              }
              if (!SPIFFS.exists("/Rfid.csv"))
              {
                // SPIFFS.remove("/Rfid.csv");
                request->send(200, "text/plane", "Data is not Available please check internet and Sync for update .");
                esp_restart();
              }
              if (!SPIFFS.exists("/BioRegs.csv"))
              {
                File datas = SPIFFS.open("/BioRegs.csv", "w");
                datas.close();
              }
              RfidRegisterPage = true;
              if (RfidRegisterPage)
              {
                RfidRegister = true;
                // rfidInitialCheck();
                request->send(SPIFFS, "/Biometric.html", "text/html");
              }
              else
              {
                request->send(200, "text/plane", "UnAuthorised Access");
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
              // mountinSD();
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
              if (SPIFFS.exists("/EmpRfid.csv"))
              {
                if (SPIFFS.remove("/EmpRfid.csv"))
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
              if (SPIFFS.exists("/Rfid.csv"))
              {
                if (SPIFFS.remove("/Rfid.csv"))
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
              delay(1000);
              RestartEsp(); });
  server.on("/Reset", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("Inside reset page ");
    EEPROM.begin(512);
    // mountinSD();
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
    if (SPIFFS.exists("/EmpRfid.csv")) {
      if (SPIFFS.remove("/EmpRfid.csv")) {
        Serial.println("EmpRfid.csv Deleted Successfully");
      } else {
        Serial.println("Failed to Delete EmpRfid.csv");
      }
    } else {
      Serial.println("EmpRfid.csv does not exist");
    }
    if (SPIFFS.exists("/Rfid.csv")) {
      if (SPIFFS.remove("/Rfid.csv")) {
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
              SPIFFS.remove("/EmpRfid.csv");
              SPIFFS.remove("/Rfid.csv");
              request->send(200, "text/json", "Update");
              delay(1000);
              RestartEsp();
      
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
      // Wire.begin();
      if (rtc.begin()) {
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
      else {
        Serial.println("RTC is NOT RUNNING");
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
          JsonEncoder["deviceId"] = deviceId;
          char JsonBufferMessage[500];
           JsonEncoder.printTo(JsonBufferMessage, sizeof(JsonBufferMessage));
          Serial.println(JsonBufferMessage);
          http.begin("https://wildfly.letzcheckin.com:443/EmployeeAttendenceAPI/BiometricAPI/BioMetricDoorMapping");
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
     // Set the request flag
     String data = json;
     // xTaskNotify(WiFiscanTaskHandle, 1, eSetValueWithOverwrite); // Notify the task      wifiScantaskDelete();
      request->send(200, "text/plain", data); });
  // server.on("/EmployeeLists", HTTP_GET, [](AsyncWebServerRequest *request)
  //           {
  //   File csv_File = SPIFFS.open("/Rfid.csv", "r");
  //   if (csv_File) {
  //     Serial.print("Opend csv File");
  //     request->send(SPIFFS, "/Rfid.csv", "text/csv");
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
        // SPIFFS.remove("/Rfid.csv");
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
  //   File csv_File = SPIFFS.open("/EmpRfid.csv", "r");
  //   if (csv_File) {
  //     Serial.print("Opend csv File");
  //     request->send(SPIFFS, "/EmpRfid.csv", "text/csv");
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
  /// BioRegs.csv
  server.on("/EmployeeBioLists", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    File csv_File = SPIFFS.open("/BioRegs.csv", "r");
    if (csv_File) {
      Serial.print("Opend csv File");
      request->send(SPIFFS, "/BioRegs.csv", "text/csv");
      csv_File.close();
    }
    else {
      Serial.print("File not Found");
      request->send(404, "text/plain", "FIle Not Found");
    } });
  server.on("/OfflineEmpLists", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    File csv_File = SPIFFS.open("/OfflineData.csv", "r");
    if (csv_File) {
      Serial.print("Opend csv File");
      request->send(SPIFFS, "/OfflineData.csv", "text/csv");
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
              SPIFFS.remove("/OfflineData.csv");
              // SPIFFS.remove("/Rfid.csv");
              request->send(200, "text/plane", "Deleted"); });

  server.on("/DeleteBioRegsFiles", HTTP_GET, [](AsyncWebServerRequest *request)
            {
            SPIFFS.remove("/BioRegs.csv");
            // SPIFFS.remove("/Rfid.csv");
            request->send(200, "text/plane", "Deleted"); });

  server.on("/Wifi_submit", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
            {
              EEPROM.begin(512);
              String ssid = "";
              String pass = "";
              DynamicJsonBuffer jsonBuffer;
              JsonObject &json = jsonBuffer.parseObject(data);
              if (json.success())
              {
                ssid = json["SSID"].as<char *>();
                pass = json["PASSWORD"].as<char *>();
                Serial.println(ssid);
                Serial.println(pass);
                if (ssid.length() > 0)
                {
                  Serial.println("Clearing EEPROM");
                  for (int i = 0; i < 94; ++i)
                  {
                    EEPROM.write(i, 0);
                  }
                  Serial.println("Writting SSID and Password to eeprom");
                  for (int i = 0; i < ssid.length(); ++i)
                  {
                    EEPROM.write(i, ssid[i]);
                  }
                  for (int i = 0; i < pass.length(); ++i)
                  {
                    EEPROM.write(i + 32, pass[i]);
                  }
                }
                EEPROM.commit();
                request->send(200, "text/plain", "WiFi submitted and connected successfully.");
                delay(1000);
                if (WiFi.status() != WL_CONNECTED)
                {
                  RestartEsp();
                }
              }
              else
              {
                request->send(400, "text/plain", "Invalid SSID provided.");
              } });

  // WiFi Page Start ..

  /*

  server.on("/Wifi_submit", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
{
    EEPROM.begin(512);  // Initialize EEPROM with 512 bytes.
    String ssid = "";
    String pass = "";
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.parseObject(data);

    if (json.success()) {
        ssid = json["SSID"].as<String>();  // Extract SSID
        pass = json["PASSWORD"].as<String>();  // Extract Password

        Serial.println("Received WiFi credentials:");
        Serial.println("SSID: " + ssid);
        Serial.println("Password: " + pass);

        if (ssid.length() > 0) {
            Serial.println("Clearing EEPROM...");
            for (int i = 0; i < 94; ++i) {  // Clear EEPROM where SSID and password will be stored.
                EEPROM.write(i, 0);
            }

            Serial.println("Writing SSID and Password to EEPROM...");
            for (int i = 0; i < ssid.length() && i < 32; ++i) {  // Write SSID to EEPROM, up to 32 characters.
                EEPROM.write(i, ssid[i]);
            }
            for (int i = 0; i < pass.length() && i < 32; ++i) {  // Write Password to EEPROM, up to 32 characters.
                EEPROM.write(i + 32, pass[i]);
            }

            EEPROM.commit();  // Save changes to EEPROM
            Serial.println("Credentials stored successfully.");

            WiFi.disconnect();  // Disconnect if connected
            WiFi.begin(ssid.c_str(), pass.c_str());  // Start connection with new credentials

            unsigned long startAttemptTime = millis();
            int count = 0;
            // Attempt to connect to WiFi for up to 10 seconds
            while (WiFi.status() != WL_CONNECTED && count ) {
                delay(500);
                Serial.print(".");
            }

            if (WiFi.status() == WL_CONNECTED) {
                Serial.println("\nWiFi connected successfully.");
                request->send(200, "text/plain", "WiFi submitted and connected successfully.");
            } else {
                Serial.println("\nFailed to connect to WiFi.");
                request->send(404, "text/plain", "WiFi submitted but could not connect.");
            }
        } else {
            request->send(400, "text/plain", "Invalid SSID provided.");
        }
    } else {
        request->send(400, "text/plain", "Failed to parse JSON.");
    }
});
*/

  // Updated Rfid
  server.on("/RfiduidUpdate", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
            { Serial.println("Rfid Update called"); });
  // Finger validation check MAM

  /*
   server.on("/fingerValidCheck", HTTP_GET, [](AsyncWebServerRequest *request){
      Serial.println("Finger is check ");
      // FingerValidCheck = false;
      RegistrationFinger = false;
      int fingerCountsensor = 0;

      while(fingerCountsensor < 20){
      esp_task_wdt_reset();
      Serial.print("Finger sensor id: ");

      uint8_t p = finger.getImage();
      if (p != FINGERPRINT_OK)
      {
        fingerCountsensor ++;
        // Serial.println("Failed to take image");
          // Use -1 to indicate failure
      }

      p = finger.image2Tz();
      if (p != FINGERPRINT_OK)
      {
        Serial.println("Failed to convert image");
          // Use -1 to indicate failure
      }

      p = finger.fingerFastSearch();
      if (p == FINGERPRINT_OK)
      {
        Serial.println("Finger Already Registered");
        request->send(502,"Finger is Registered");
        break;
      }
      else if (p!= FINGERPRINT_OK){
          Serial.println("Print Not Matched");
          request->send(200,"Finger is Not Registered");
          if(fingerCountsensor > 10){
            break;
          }
        }

    }
    // request->send(502,"Finger Time Out");
  });


  server.on("/fingerValidCheck", HTTP_GET, [](AsyncWebServerRequest *request) {
      Serial.println("Finger is being checked");
      RegistrationFinger = false;
      int fingerCountsensor = 0;

      bool fingerFound = false;  // Flag to track if fingerprint is found

      while (fingerCountsensor < 20) {
          esp_task_wdt_reset();  // Reset watchdog timer

          Serial.print("Finger sensor id: ");
          uint8_t p = finger.getImage();
          if (p != FINGERPRINT_OK) {
              fingerCountsensor++;
              delay(100);  // Add a small delay to avoid rapid retries
              continue;
          }

          p = finger.image2Tz();
          if (p != FINGERPRINT_OK) {
              Serial.println("Failed to convert image");
              fingerCountsensor++;
              delay(100);
              continue;
          }

          p = finger.fingerFastSearch();
          if (p == FINGERPRINT_OK) {
              Serial.println("Finger Already Registered");
              fingerFound = true;  // Fingerprint is registered
              break;
          } else if (p != FINGERPRINT_OK) {
              Serial.println("Print Not Matched");
              if (fingerCountsensor > 10) {
                  break;  // Exit if too many failed attempts
              }
              delay(100);
          }
      }

      if (fingerFound) {
          request->send(200, "application/json", "{\"status\":\"Finger is Registered\"}");
      } else {
          request->send(200, "application/json", "{\"status\":\"Finger is Not Registered\"}");
      }
  });
  */

 
  server.on("/FingerChecker", HTTP_POST, [](AsyncWebServerRequest *request)
            {
        Serial.println("Finger Checker Inside");
        uint8_t p;
        uint8_t storevar ;
        int count = 0;
        bool fingerprintDetected = false;

        while (count < 10) {
            p = finger.getImage();
            Serial.print("P:");
            Serial.println(p);

            if (p == FINGERPRINT_OK) {
                Serial.println("Got Finger");
                fingerprintDetected = true;
                break;
            }
            count++;
            delay(500);
            esp_task_wdt_reset(); // Reset watchdog timer
        }

        if (fingerprintDetected) {
            p = finger.image2Tz();
            storevar = finger.image2Tz();
            if (p == FINGERPRINT_OK) {
                p = finger.fingerFastSearch();
                if (p == FINGERPRINT_OK) {
                  storevar = finger.storeModel(12);
                    Serial.println("Fingerprint matched.");
                    request->send(200, "text/html", "Fingerprint matched.");
                    return;
                } else {
                    Serial.println("Failed to find a match");
                    request->send(200, "text/html", "No match found.");
                    return;
                }
            } else {
                Serial.println("Failed to convert image to template.");
                request->send(500, "text/html", "Error processing fingerprint.");
                return;
            }
        } else {
            request->send(500, "text/html", "Fingerprint not detected.");
            return;
        } });

  server.on("/FingerCheckingP", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
            {
    Serial.println("clicked finger checking");
    uint8_t p;
    String EmployeeId;
    uint8_t storevar;
    DynamicJsonBuffer jsonBuffer;
      JsonObject& json = jsonBuffer.parseObject(data);
      if (json.success()) {
          String empIdStr = json["empId"].as<String>();
          Serial.println("Received empId: " + empIdStr);
          RegEmpId = empIdStr;
          RegistrationFinger = false;
          int count = 0;
          bool fingerprintDetected = false;

          while (count < 10) {
              p = finger.getImage();
              Serial.print("P:");
              Serial.println(p);

              if (p == FINGERPRINT_OK) {
                  Serial.println("Got Finger");
                  fingerprintDetected = true;
                  break;
              }
              count++;
              delay(500);
              esp_task_wdt_reset(); // Reset watchdog timer
          }

              if (fingerprintDetected) {
              p = finger.image2Tz();
              // storevar = finger.image2Tz();
              if (p == FINGERPRINT_OK) {
                  p = finger.fingerFastSearch();
                  if (p == FINGERPRINT_OK) {
                    // storevar = finger.storeModel(12);
                    FingerPrintId = (int)finger.fingerID;
                    EmployeeId = ValidationChecker(FingerPrintId);
                    Serial.print("Finger print id:");
                    Serial.println(FingerPrintId);
                    Serial.print("Employee Id");
                    Serial.println(EmployeeId);
                    Serial.print("Reg Emp Id");
                    Serial.println(RegEmpId);
                    int EmplIntId = EmployeeId.toInt();
                    int RegsEmpIntId = RegEmpId.toInt();
                    if(EmployeeId == "Could Not Find Match"){
                      Serial.print("Inside Validation it found id is not maped to any finger");
                      request->send(414, "text/plain", "Finger is Not Associated with any Registered Finger in the organization Try Registering Again");

                    }
                    if(EmplIntId == RegsEmpIntId){
                      Serial.println("Fingerprint matched.success");
                      request->send(200, "text/plain", "Finger is Mapped with the Selected Employee");
                    }
                    else{
                      Serial.println("Finger is already mapped wrong finger");
                      request->send(404, "text/html", "Fingerprint matched.");
                      return;
                    }
                  } else {
                      Serial.println("Failed to find a match");
                      request->send(408, "text/html", "No match found.");
                      return;
                  }
              } else {
                  Serial.println("Failed to convert image to template.");
                  request->send(500, "text/html", "Error processing fingerprint.");
                  return;
              }
          } else {
              request->send(500, "text/html", "Fingerprint not detected.");
              return;
          }


          // RegistrationFinger = true;
    request->send(200, "text/plain", "File Not Found");
      }
      else{
        request->send(505, "text/plain","Invalid Json");
      } });

  /*

  Serial.println("Image converted");
                    p = finger.fingerSearch();
                    Serial.println("Finding Finger ");
                    FingerPrintId = (int)finger.fingerID;
                    EmployeeId = ValidationChecker(FingerPrintId);
                    Serial.println(FingerPrintId);

                    if(strcmp(EmployeeId.c_str(), RegEmpId.c_str())==0){
                      request->send(404, "text/plain", "Finger is Mapped with the Selected Employee");
                    }
                    else{
  */

  /*
  server.on("/FingerChecker", HTTP_POST, [](AsyncWebServerRequest *request) {
      Serial.println("Finger Checker Test");

      uint8_t p = finger.getImage();
      Serial.print("Sensor Response Code: ");
      Serial.println(p);

      if (p == FINGERPRINT_OK) {
          Serial.println("Fingerprint image successfully captured.");
          request->send(200, "text/html", "Fingerprint detected.");
      } else {
          Serial.print("Error Code: ");
          Serial.println(p);
          request->send(500, "text/html", "Error processing fingerprint.");
      }
  });
  */

  /*
    server.on("/FingerChecker", HTTP_POST, [](AsyncWebServerRequest *request)
   {
        Serial.println("Finger Checker Inside");
        uint8_t storevar; // Storage
        uint8_t p;
        int count = 0;
        bool fingerprintDetected = false;
        RegistrationFinger = false;
        // Fingerprint checking loop
        while (count < 10) {
            p = finger.getImage();
            Serial.print("P:");
            Serial.println(p);

            if (p == FINGERPRINT_OK) {
                Serial.println("Got Finger");
                fingerprintDetected = true;
                break;
            }

            // If there's an error, log it and continue
            if (p != FINGERPRINT_NOFINGER && p != FINGERPRINT_PACKETRECIEVEERR && p != FINGERPRINT_IMAGEFAIL) {
                Serial.print("Error: ");
                Serial.println(p);
            }
            count++;
            delay(500); // Small delay to prevent rapid polling
            esp_task_wdt_reset(); // Reset watchdog timer
        }

        storevar = p; // Aug 9 2024

        if (fingerprintDetected) {
            p = finger.image2Tz();
            storevar = finger.image2Tz();
            if (p == FINGERPRINT_OK) {
              esp_task_wdt_reset();
                p = finger.fingerFastSearch();
                if (p == FINGERPRINT_OK) {
                  esp_task_wdt_reset();
                  FingerPrintId = (int)finger.fingerID;
                  Serial.println(FingerPrintId);
                  String Empid = ValidationChecker(FingerPrintId);
                  storevar = finger.storeModel(12); // Here
                  if (storevar == FINGERPRINT_OK) {
                    Serial.println("Stored New Variable");
                  }
                  delay(100);
                  Serial.println("Fingerprint matched.");
                  request->send(404, "text/html", "Fingerprint detected to Employee :"+Empid+" Do you want to Re-Register");
                  return;
                } else {
                    Serial.println("Failed to find a match");
                    request->send(200, "text/html", "No match found.");
                    return;
                }
            } else {
                Serial.println("Failed to convert image to template.");
                esp_task_wdt_reset();
                request->send(500, "text/html", "Error processing fingerprint.");
                return;
            }
        } else {
          request->send(500, "text/html", "Fingerprint is Not Detected");
          return;
      }

            Serial.println("Timeout: No fingerprint dFingerCheckeretected.");
            request->send(408, "text/html", "Timeout: No fingerprint detected.");

        RegistrationFinger = true;
        return;
        });
  */

  // server.on("/EmployeeRfidLists", HTTP_GET, [](AsyncWebServerRequest *request)

  /*

    server.on("/re-registering", HTTP_POST, [](AsyncWebServerRequest *request) {
      Serial.println("Re-registering fingerprint");
      SendRegisterData("Place The Same Finger again");
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
          SendRegisterData("Place Your Same Finger Again");
      }
      if (p != FINGERPRINT_OK) {
          Serial.println("Failed to capture fingerprint image");
          SendRegisterData("Failed to capture fingerprint image");
          request->send(500, "text/plain", "Failed to capture fingerprint image");
          return;
      }
      count = 0;
      Serial.println("Place same finger again");
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
          SendRegisterData("Place your Same Finger Again");
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
          SendRegisterData("Processing...");

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
              SendRegisterData("SuccessFully Registered");

               if (SPIFFS.begin(true)) {
                Serial.println("mounting SPIFFS");
               }
              // Append data

              File empidFile = SPIFFS.open("/BioRegs.csv", "a");
              if (!empidFile) {
                Serial.println("Failed to open file for appending");
                return;
              }

              String data = String(id) + String(",") + RegEmpId + String("\n");
              empidFile.print(data);
              empidFile.close();
              // Read and print file contents
              Serial.println("Reading Files");
              // updating it in main file
              updated_Activity("",0,RegEmpId,"","",1);

                  File empidFile = SPIFFS.open("/BioRegs.csv", "a");
      if (!empidFile) {
          Serial.println("Failed to open file for appending");
          return;
      }

      // Prepare the data to be appended
      String data = String(id) + "," + RegEmpId + "\n";
      empidFile.print(data);

      // Close the file after writing
      empidFile.close();

      // Optionally, read and print file contents for debugging
      Serial.println("Reading Files");

      File readFile = SPIFFS.open("/BioRegs.csv", "r");
      if (!readFile) {
          Serial.println("Failed to open file for reading");
          return;
      }

      // Read and print file contents
      while (readFile.available()) {
          String line = readFile.readStringUntil('\n');
          Serial.println(line);
      }





      // Close the file after reading
      readFil

e.close();


      // Call the updated_Activity function
      updated_Activity("", 0, RegEmpId, "", "", 1);
              request->send(200, "text/plain", "Re-registration completed");
          } else {
              Serial.println("Failed to store model");
              SendRegisterData("Failed to store model");
              request->send(500, "text/plain", "Failed to store model");
          }
      } else {
          Serial.println("Re-registration failed");
          SendRegisterData("Re-registration failed");
          request->send(500, "text/plain", "Re-registration failed");
      }
    });

  */

  // Bio metric Scan process
  /*
  server.on("/start-registration", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
            {
      int status=0;

      Serial.println("Start Registration");
      RegistrationFinger = false;
      DynamicJsonBuffer jsonBuffer;
      JsonObject& json = jsonBuffer.parseObject(data);
      if (json.success()) {
          String empIdStr = json["empId"].as<String>();
          Serial.println("Received empId: " + empIdStr);
          RegEmpId = empIdStr;
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
            esp_task_wdt_reset();
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
                    // request->send(408, "text/plain", "Finger is Not Mapped With the Selected Employee");
                  // if(FingerPrintId > 0){
                  //   Serial.println("Finger Found");
                  //   if(strcmp(EmployeeId.c_str(),"Could Not Find Match")==0){
                  //     EmployeeId = "But Failed To Get Employee Data You can Delete the data from the Tictoks Ui";
                  //     request->send(404, "text/plain", "Finger Already Registered"+ EmployeeId);
                  //   }
                    //"Could Not Find Match"
                    // request->send(404, "text/plain", "Finger Already Registered: "+ EmployeeId +"<br> Do You want to Re-Register Select 'Yes' or 'No' ");
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
    }
      else {
        request->send(400, "text/plain", "Invalid JSON");
    } });
*/

  /*
    server.on("/re-registering", HTTP_POST, [](AsyncWebServerRequest *request)
              {
      Serial.println("Re-registering fingerprint");
      SendRegisterData("Place The Same Finger again");
      int status = 0;
      int p = 0;
      int count = 0;
      const int maxAttempts = 10;
      const int imageSlot = 2; // Slot for the second image
      uint8_t storevar; // Variable to store additional data
      delay(2000);
      // Wait until the finger is removed
      while (count < maxAttempts) {
        esp_task_wdt_reset();  // Reset watchdog at the start of the loop
        p = finger.getImage();
        if (p == FINGERPRINT_OK) {
            Serial.println("Image taken");
            break;
        }
        delay(200);
        Serial.println("Getting image Re-registering");
        SendRegisterData("Place Your Same Finger Again");
      }

      if (p != FINGERPRINT_OK) {
          Serial.println("Failed to capture fingerprint image re");
          SendRegisterData("Failed to capture fingerprint image re");
          request->send(500, "text/plain", "Failed to capture fingerprint image");
          return;
      }
      count = 0;
      Serial.println("Place same finger again");
      // Wait for the same finger to be placed again
      while (count < maxAttempts) {
          p = finger.getImage();
          Serial.println("Re-register Finger");
          count++;
          esp_task_wdt_reset();
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
          SendRegisterData("Place your Same Finger Again");
          delay(200);
      }
      if (status > 0) {
          // Convert image to template
          p = finger.image2Tz(1);
          // storevar = finger.image2Tz(imageSlot);
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
          SendRegisterData("Processing...");

          // Create model
          p = finger.createModel();

          p = finger.storeModel(id);
          if (p == FINGERPRINT_OK) {
              Serial.println("Fingerprint model stored successfully");
              SendRegisterData("Successfully Registered");

              if (SPIFFS.begin(true)) {
                  Serial.println("Mounting SPIFFS");
              }

              // Append data including storevar to the file

              File empidFile = SPIFFS.open("/BioRegs.csv", "a");
              if (!empidFile) {
                  Serial.println("Failed to open file for appending");
                  return;
              }

              // Prepare the data to be appended
              String data = String(id) + "," + RegEmpId+ "\n";
              empidFile.print(data);

              // Close the file after writing
              empidFile.close();

              // Optionally, read and print file contents for debugging
              Serial.println("Reading Files");


              File readFile = SPIFFS.open("/BioRegs.csv", "r");
              if (!readFile) {
                  Serial.println("Failed to open file for reading");
                  return;
              }

              // Read and print file contents
              while (readFile.available()) {
                  String line = readFile.readStringUntil('\n');
                  Serial.println(line);
              }

              // Close the file after reading
              readFile.close();

              // Call the updated_Activity function
              esp_task_wdt_reset();
              updated_ActivityBio("", 0, RegEmpId, "", "",int(id), 1);
              File rfidregisterStatus = SPIFFS.open("/RfidRegisterStatus","a");
              rfidregisterStatus.print(RegEmpId+","+int(id)+'\n');
              rfidregisterStatus.close();
              request->send(200, "text/plain", "Re-registration completed");




              if(WiFi.status()==WL_CONNECTED){
                if(client.connect("www.google.com",80)){
                  HTTPClient http;
                  esp_task_wdt_reset(); // Reset watchdog before starting the HTTP request

              StaticJsonBuffer<200> jsonBuffer; // Adjust size as needed
              JsonObject &JSONEncoder = jsonBuffer.createObject();
              JSONEncoder["companyId"] = CompanyId;
              JSONEncoder["employeeId"] = RegEmpId;
              JSONEncoder["biometricTemplate1"]="";
              JSONEncoder["biometricTemplate2"]="";
              JSONEncoder["biometricTemplateId1"]= String(id);
              JSONEncoder["biometricTemplateId2"]="";
              JSONEncoder["biometricFingerPrintStatus"] = 1 ;

              String JSONmessageBuffer;
              JSONEncoder.printTo(JSONmessageBuffer); // Serialize to String
              // JSONEncoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
              Serial.print("Bio list");
              esp_task_wdt_reset();
              Serial.println(JSONmessageBuffer);
              http.begin("https://wildfly.tictoks.in:443/EmployeeAttendenceAPI/BiometricAPI/RegisterBiometricInfo");
              esp_task_wdt_reset();
              http.addHeader("Content-type", "application/json");
              int response = http.POST(JSONmessageBuffer);

              Serial.print("HTTP code");
              Serial.println(response);
              String payload = http.getString();
              if (response == 200)
              {
                Serial.println("Pay Load");
                Serial.println(payload);
                esp_task_wdt_reset();
                request->send(200, "text/plain", "Re-registration completed");
                delay(100);
                updated_ActivityBio("", 0, RegEmpId, "", "",int(id), 1);
                esp_task_wdt_reset();
              }
                }
              }
          } else {
              Serial.println("Failed to store model");
              SendRegisterData("Failed to store model");
              request->send(500, "text/plain", "Failed to store model");
          }


      }
       else {
          Serial.println("Re-registration failed");
          SendRegisterData("Re-registration failed");
          request->send(500, "text/plain", "Re-registration failed");
      }
  });
  */

  server.on("/start-registration", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
            {
    int status = 0;
    Serial.println("Start Registration");
    RegistrationFinger = false;
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.parseObject(data);

    if (json.success()) {
        String empIdStr = json["empId"].as<String>();
        Serial.println("Received empId: " + empIdStr);
        RegEmpId = empIdStr;
        id = getNextFreeID(); // Get the next free ID from the fingerprint sensor

        if (id == 0xFF) {
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
            esp_task_wdt_reset();

            switch (p) {
                case FINGERPRINT_OK:
                    SendRegisterData("Image taken");
                    Serial.println("Image taken");
                    count = maxAttempts; // Exit the loop
                    status = 1;
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

            vTaskDelay(pdMS_TO_TICKS(delayMs)); // Yield to other tasks
        }

        if (p != FINGERPRINT_OK) {
            Serial.println("Failed to get a valid fingerprint image after max attempts");
            SendRegisterData("Failed to get a valid fingerprint image");
            request->send(500, "text/plain", "Failed to get a valid fingerprint image");
            return;
        }

        // Convert image to template
        p = finger.image2Tz(1);
        if (p != FINGERPRINT_OK) {
            SendRegisterData("Failed to create template");
            request->send(500, "text/plain", "Failed to create template");
            return;
        }

        SendRegisterData("Remove finger");
        Serial.println("Remove finger");
        request->send(200, "text/plain", "Registration started");

    } else {
        request->send(400, "text/plain", "Invalid JSON");
    } });

  server.on("/re-registering", HTTP_POST, [](AsyncWebServerRequest *request)
            {
    Serial.println("Re-registering fingerprint");
    SendRegisterData("Place The Same Finger again");

    int status = 0;
    int p = 0;
    int count = 0;
    const int maxAttempts = 10;
    uint8_t storevar;
    delay(2000);

    // Capture fingerprint image
    while (count < maxAttempts) {
        esp_task_wdt_reset();
        p = finger.getImage();
        if (p == FINGERPRINT_OK) {
            Serial.println("Image taken");
            break;
        }
        delay(200);
        Serial.println("Getting image Re-registering");
        SendRegisterData("Place Your Same Finger Again");
        count++;
    }

    if (p != FINGERPRINT_OK) {
        Serial.println("Failed to capture fingerprint image");
        request->send(500, "text/plain", "Failed to capture fingerprint image");
        return;
    }

    // Convert image to template
    p = finger.image2Tz(2);
    if (p != FINGERPRINT_OK) {
        Serial.println("Failed to convert image");
        request->send(500, "text/plain", "Failed to convert fingerprint image");
        return;
    }

    // Create and store model
    p = finger.createModel();
    if (p == FINGERPRINT_OK) {
        p = finger.storeModel(id);
        if (p == FINGERPRINT_OK) {
            Serial.println("Fingerprint model stored successfully");
            SendRegisterData("Successfully Registered");

            // Ensure SPIFFS is mounted
            if (!SPIFFS.begin(true)) {
                Serial.println("Failed to mount SPIFFS");
                request->send(500, "text/plain", "SPIFFS mount failed");
                return;
            }

            // Open file in append mode
            File rfidregisterStatus = SPIFFS.open("/RfidRegisterStatus", FILE_APPEND);
            if (!rfidregisterStatus) {
                Serial.println("Failed to open file for appending");
                request->send(500, "text/plain", "Failed to open file");
                return;
            }

            // Write data to file
            String data = RegEmpId + "," + String(id) + "\n";
            if (rfidregisterStatus.print(data)) {
                Serial.println("Data written to file: " + data);
            } else {
                Serial.println("Failed to write data to file");
            }

            rfidregisterStatus.close();  // Ensure file is closed properly

            request->send(200, "text/plain", "Re-registration completed");
            updated_ActivityBio("", 0, RegEmpId, "", "", id, 1);

        } else {
            Serial.println("Failed to store model");
            SendRegisterData("Failed to store model");
            request->send(500, "text/plain", "Failed to store model");
        }
    } else {
        Serial.println("Failed to create model");
        SendRegisterData("Failed to create model");
        request->send(500, "text/plain", "Failed to create fingerprint model");
    } });

  /*
    server.on("/re-registering", HTTP_POST, [](AsyncWebServerRequest *request)
              {
      Serial.println("Re-registering fingerprint");
      SendRegisterData("Place The Same Finger again");
      int status = 0;
      int p = 0;
      int count = 0;
      const int maxAttempts = 10;
      const int imageSlot = 2; // Slot for the second image
      uint8_t storevar; // Variable to store additional data
      delay(2000);
      // Wait until the finger is removed
      while (count < maxAttempts) {
        esp_task_wdt_reset();  // Reset watchdog at the start of the loop
        p = finger.getImage();
        if (p == FINGERPRINT_OK) {
            Serial.println("Image taken");
            break;
        }
        delay(200);
        Serial.println("Getting image Re-registering");
        SendRegisterData("Place Your Same Finger Again");
      }

      if (p != FINGERPRINT_OK) {
          Serial.println("Failed to capture fingerprint image re");
          SendRegisterData("Failed to capture fingerprint image re");
          request->send(500, "text/plain", "Failed to capture fingerprint image");
          return;
      }
      count = 0;
      Serial.println("Place same finger again");
      // Wait for the same finger to be placed again
      while (count < maxAttempts) {
          p = finger.getImage();
          Serial.println("Re-register Finger");
          count++;
          esp_task_wdt_reset();
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
          SendRegisterData("Place your Same Finger Again");
          delay(200);
      }
      if (status > 0) {
          // Convert image to template
          p = finger.image2Tz(2);
          // storevar = finger.image2Tz(imageSlot);
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
          SendRegisterData("Processing...");

          // Create model
          p = finger.createModel();

          // Store model
          p = finger.storeModel(id);
          if (p == FINGERPRINT_OK) {
              Serial.println("Fingerprint model stored successfully");
              SendRegisterData("Successfully Registered");

              if (SPIFFS.begin(true)) {
                  Serial.println("Mounting SPIFFS");
              }

              // Append data including storevar to the file
              /*
              File empidFile = SPIFFS.open("/BioRegs.csv", "a");
              if (!empidFile) {
                  Serial.println("Failed to open file for appending");
                  return;
              }

              // Prepare the data to be appended
              String data = String(id) + "," + RegEmpId+ "\n";
              empidFile.print(data);

              // Close the file after writing
              empidFile.close();

              // Optionally, read and print file contents for debugging
              Serial.println("Reading Files");
              */
  /*
  File readFile = SPIFFS.open("/BioRegs.csv", "r");
  if (!readFile) {
      Serial.println("Failed to open file for reading");
      return;
  }

  // Read and print file contents
  while (readFile.available()) {
      String line = readFile.readStringUntil('\n');
      Serial.println(line);
  }

  // Close the file after reading
  readFile.close();

  // Call the updated_Activity function

  esp_task_wdt_reset();

request->send(200, "text/plain", "Re-registration completed");
delay(100);
updated_ActivityBio("", 0, RegEmpId, "", "",int(id), 1);
File rfidregisterStatus = SPIFFS.open("/RfidRegisterStatus","a");
Serial.print("Lines: ");
Serial.println(RegEmpId+","+int(id)+'\n');
rfidregisterStatus.print(RegEmpId+","+int(id)+'\n');
rfidregisterStatus.close();
esp_task_wdt_reset();



} else {
  Serial.println("Failed to store model");
  SendRegisterData("Failed to store model");
  request->send(500, "text/plain", "Failed to store model");
}
} else {
Serial.println("Re-registration failed");
SendRegisterData("Re-registration failed");
request->send(500, "text/plain", "Re-registration failed");
} });

*/

  server.on("/delete-all", HTTP_GET, [](AsyncWebServerRequest *request)
            {
        for (int finger = 1; finger < 100; finger++)
        {
          esp_task_wdt_reset();
          deleteFingerprint(finger);
          // downloadFingerprintTemplate(finger);
        }
        Serial.println("Deleting all fingerprints");
         
  request->send(200, "Deleting all finger "); });
  // updated_Activity
  server.on("/updateBioTest", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    updated_ActivityBio("",0,"0198","","",17,1);
    request->send(200, "Deleting all finger "); });

  server.begin();
}



bool CheckNullonServerRfidInitialFetch()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    if (client.connect("www.google.com", 80))
    {
      if (SPIFFS.begin(true))
      {
        Serial.println("Update Activity");
        delay(2000);
        DeviceIdInitialize();

        int dataCount = 0;
        int startCount = 0;
        const int endCount = 20;
        int httpCode = 0;
        DateTime now = rtc.now();
        int minutes_start = now.minute();
        Serial.print("Start Time :");
        Serial.println(minutes_start);

        ledFetchingon();
        HTTPClient http; 
        DynamicJsonBuffer jsonBuffers;
        JsonObject &JSONEncoder = jsonBuffers.createObject();
        JSONEncoder["companyId"] = CompanyId;
        JSONEncoder["deviceId"] = DeviceId;
        JSONEncoder["startCount"] = startCount;
        JSONEncoder["endCount"] = endCount;

        char JSONmessageBuffer[500]; 
        JSONEncoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
        Serial.println(JSONmessageBuffer);
// Mark
        http.begin("https://wildfly.letzcheckin.com:443/EmployeeAttendenceAPI/RFIDAPI/AutoSyncEmployeeData");
        http.addHeader("Content-Type", "application/json");

        httpCode = http.POST(JSONmessageBuffer); 
        Serial.print("HTTP Code: ");
        Serial.println(httpCode);

        String payload = http.getString();
        Serial.print("Payload: ");
        Serial.println(payload);
        
        http.end(); // Close the connection

        if (httpCode == 200)
        {
          JsonObject &root = jsonBuffers.parseObject(payload);

          if (!root.success())
          {
            Serial.println("JSON Parsing failed!");
            return false;
          }

          if (root.size() > 0)
          {
            Serial.println("Root success and has value Update");
            root.printTo(Serial);
          }

          JsonArray &empRfidList = root["employeeInfoList"];
          Serial.print("Data Count: ");
          Serial.println(empRfidList.size());

          if (empRfidList.size() <= 0)
          {
            return false;
          }
          else
          {

            // Only way to update..
            return true;
          }
        }
        else
        {
          Serial.println("HTTP request failed");
          return false;
        }
      }
      else
      {
        Serial.println("SPIFFS initialization failed");
        return false;
      }
    }
    else
    {
      Serial.println("Client connection to Google failed");
      return false;
    }
  }
  else
  {
    Serial.println("WiFi not connected");
    return false;
  }
}


void softAp()
{
  WiFi.softAP(ASSID, APASS);
  Serial.println("AP Started");
  Serial.print("IP Address:");
  Serial.println(WiFi.softAPIP());
  MDNSServer();
}

// Define the getNextFreeID function
uint8_t getNextFreeID()
{
  for (uint8_t id = 2; id <= 255; id++)
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
  if (SPIFFS.begin(true))
  {
    Serial.println("Spiffs Beginned Correctly");
  }
  else
  {
    Serial.println("Spiffs Not Beginned Properly");
  }
}

void registerSendtoBio()
{
  Serial.println("Reading Bios from file");
  if (WiFi.status() != WL_CONNECTED)
  {
    if (client.connect("www.google.com", 80))
    {
      HTTPClient http; // HTTP client instance

      File regsFilesQs = SPIFFS.open("/RfidRegisterStatus", "r");
      if (!regsFilesQs)
      {
        Serial.println("Failed to open file for reading");
        return;
      }
      // Read and print each line of the file
      while (regsFilesQs.available())
      {
        String line = regsFilesQs.readStringUntil('\n');
        Serial.print("Line: ");
        Serial.println(line);
        String RegEmpId = ""; // Placeholder for the employee ID
        String id = "";       // Placeholder for the biometric template ID
        // Parse the line, assuming CSV format
        int commaIndex = line.indexOf(',');
        if (commaIndex > 0)
        {
          RegEmpId = line.substring(0, commaIndex);
          id = line.substring(commaIndex + 1);
        }
        else
        {
          Serial.println("Invalid line format");
          continue; // Skip invalid lines
        }
        Serial.print("Register to bio");
        Serial.println("Lines " + RegEmpId + id);

        // Create the JSON object
        /*
        DynamicJsonBuffer jsonBuffers;  // Adjust size as needed
        JsonObject &jsonBuffer = jsonBuffers.createObject();
        jsonBuffer["companyId"] = CompanyId;  // Assuming CompanyId is globally defined
        jsonBuffer["employeeId"] = RegEmpId;
        jsonBuffer["biometricTemplate1"] = "";
        jsonBuffer["biometricTemplate2"] = "";
        jsonBuffer["biometricTemplateId1"] = id;
        jsonBuffer["biometricTemplateId2"] = "";
        jsonBuffer["biometricFingerPrintStatus"] = 1;

        // Serialize JSON to string
        String JSONmessageBuffer;
        jsonBuffer.printTo(JSONmessageBuffer);//(jsonBuffer, JSONmessageBuffer);
        Serial.println("JSON message: ");
        Serial.println(JSONmessageBuffer);

        // Send HTTP POST request
        http.begin("https://wildfly.tictoks.in:443/EmployeeAttendenceAPI/BiometricAPI/RegisterBiometricInfo");
        http.addHeader("Content-Type", "application/json");

        int responseCode = http.POST(JSONmessageBuffer);  // Send the request
        String responseBody = http.getString();           // Get the response

        // Print the response from the server
        Serial.print("HTTP Response code: ");
        Serial.println(responseCode);
        Serial.print("Response body: ");
        Serial.println(responseBody);

        // Check if the POST was successful
        if (responseCode == 200) {
            Serial.println("Successfully sent data");
            // Here, you can add any logic to handle successful submission
            // like deleting the line from the file or marking it as sent.
        } else {
            Serial.println("Failed to send data");
        }

        // Close the HTTP connection
        http.end();
        */
      }
      regsFilesQs.close();
    }

    // Close the file after reading
  }
}

// Open the RfidRegisterStatus file for reading

// Function to check internet connectivity
bool isInternetAvailable()
{
  WiFiClient client;
  return client.connect("www.google.com", 80); // Check by trying to connect to Google
}

void registerSendtoBios()
{
  Serial.println("Reading Bios from file");

  // Mount SPIFFS if not mounted already
  if (!SPIFFS.begin(true))
  { // true to format if failed to mount
    Serial.println("Failed to mount SPIFFS");
    return;
  }

  // Check Wi-Fi connection status and internet availability
  if (WiFi.status() == WL_CONNECTED && isInternetAvailable())
  {
    HTTPClient http; // HTTP client instance

    // Open the RfidRegisterStatus file for reading
    File regsFilesQs = SPIFFS.open("/RfidRegisterStatus", "r");
    if (!regsFilesQs)
    {
      Serial.println("Failed to open file for reading");
      return;
    }

    bool allDataSent = true; // To track if all data was sent successfully

    // Read and print each line of the file
    while (regsFilesQs.available())
    {
      String line = regsFilesQs.readStringUntil('\n');
      Serial.print("Line: ");
      Serial.println(line);

      String RegEmpId = ""; // Placeholder for the employee ID
      String id = "";       // Placeholder for the biometric template ID

      // Parse the line, assuming CSV format
      int commaIndex = line.indexOf(',');
      if (commaIndex > 0)
      {
        RegEmpId = line.substring(0, commaIndex);
        id = line.substring(commaIndex + 1);
      }
      else
      {
        Serial.println("Invalid line format");
        continue; // Skip invalid lines
      }

      Serial.println("Register to bio:");
      Serial.println("Employee ID: " + RegEmpId + ", Template ID: " + id);

      // Create the JSON object
      StaticJsonBuffer<200> jsonBuffer; // Adjust size as needed
      JsonObject &json = jsonBuffer.createObject();
      json["companyId"] = CompanyId; // Assuming CompanyId is globally defined
      json["employeeId"] = RegEmpId;
      json["biometricTemplate1"] = "";
      json["biometricTemplate2"] = "";
      json["biometricTemplateId1"] = id;
      json["biometricTemplateId2"] = "";
      json["biometricFingerPrintStatus"] = 1;

      // Serialize JSON to string
      String JSONmessageBuffer;
      json.printTo(JSONmessageBuffer);
      Serial.println("JSON message: ");
      Serial.println(JSONmessageBuffer);

      // Send HTTP POST request
      http.begin("https://wildfly.letzcheckin.com:443/EmployeeAttendenceAPI/BiometricAPI/RegisterBiometricInfo");
      http.addHeader("Content-Type", "application/json");

      int responseCode = http.POST(JSONmessageBuffer); // Send the request
      String responseBody = http.getString();          // Get the response

      // Print the response from the server
      Serial.print("HTTP Response code: ");
      Serial.println(responseCode);
      Serial.print("Response body: ");
      Serial.println(responseBody);

      // Check if the POST was successful
      if (responseCode != 200)
      {
        Serial.println("Failed to send data");
        allDataSent = false; // Mark as false if any request fails
        break;               // Stop further processing on error
      }
    }

    regsFilesQs.close(); // Close the file after reading

    // If all data was successfully sent, delete the file
    if (allDataSent)
    {
      if (SPIFFS.remove("/RfidRegisterStatus"))
      {
        Serial.println("File successfully deleted after sending all data.");
      }
      else
      {
        Serial.println("Failed to delete the file.");
      }
    }
  }
  else
  {
    Serial.println("Wi-Fi not connected or no internet access.");
  }
}

/*
void registerSendtoBio(){
  Serial.println("Register Bios");
  if(WiFi.status()!= WL_CONNECTED){
    if(client.connect("www.google.com",80)){
      // HTTPClient http;
      File regsFilesQs = SPIFFS.open("/RfidRegisterStatus","r");
      while(regsFilesQs.available()){
        String line = regsFilesQs.readStringUntil('\n');
        Serial.print("line: ");
        Serial.println(line);
        StaticJsonBuffer<200> jsonBuffer; // Adjust size as needed
        JsonObject &JSONEncoder = jsonBuffer.createObject();
        JSONEncoder["companyId"] = CompanyId;
        JSONEncoder["employeeId"] = RegEmpId;
        JSONEncoder["biometricTemplate1"]="";
        JSONEncoder["biometricTemplate2"]="";
        JSONEncoder["biometricTemplateId1"]= String(id);
        JSONEncoder["biometricTemplateId2"]="";
        JSONEncoder["biometricFingerPrintStatus"] = 1 ;

        String JSONmessageBuffer;
        JSONEncoder.printTo(JSONmessageBuffer); // Serialize to String
        // JSONEncoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
        Serial.print("Bio list");
        esp_task_wdt_reset();
        Serial.println(JSONmessageBuffer);
        http.begin("https://wildfly.tictoks.in:443/EmployeeAttendenceAPI/BiometricAPI/RegisterBiometricInfo");
        esp_task_wdt_reset();
        http.addHeader("Content-type", "application/json");
        int response = http.POST(JSONmessageBuffer);

        Serial.print("HTTP code");
        Serial.println(response);
        String payload = http.getString();
        if (response == 200)
        {
          Serial.println("Pay Load");
          Serial.println(payload);
          esp_task_wdt_reset();
          delay(100);
          // updated_ActivityBio("", 0, RegEmpId, "", "",int(id), 1);
          esp_task_wdt_reset();


      }
      regsFilesQs.close();
    }
  }
}

*/

void initialSetupFun()
{
  Serial.begin(115200);
  EEPROM.read(512);
  esp_task_wdt_init(10, true);
  initialavailableWifi();
  mountinSpiffs();

  WiFi.mode(WIFI_AP_STA);
  softAp();
  initializePinMode();
  WifiConnectCheck();
  CompanyIdCheck();
  // mountinSD();
  DeviceIdInitialize();
  InitializeRTC();
  SensorFingerBegin();
  // delay(2000);
  rfidInitialCheck();
  fileReadAndWrite();
  // checkAllTemplates();
  WebSocketRegister();
  // registerSendtoBio();
  registerSendtoBios();
  WebServerRoutes();
  delay(500);
  initializeCoreWork();
  Serial.print("Build Version:");
  Serial.println(Build_Version);
}

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
    UnauthorizedAccess();
    return -1; // Use -1 to indicate failure
  }

  p = finger.fingerSearch();
  Serial.println("start l");
  // delay(10000);
  Serial.println("end l");
  Serial.println("Finger Inside ");
  if (p != FINGERPRINT_OK)
  {
    p = 0;
    Serial.println("Failed to find a match");
    UnauthorizedAccess();
    return -1; // Use -1 to indicate failure
  }
  else
  {
    FingerPrintId = (int)finger.fingerID; // Convert uint8_t to int
    Serial.print("SSSSS");
    Serial.print(FingerPrintId); // Print as int
    Serial.print(" with confidence of ");
    Serial.println(finger.confidence);
    Serial.println();
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

// changed text

bool matchRfid(int fingerid)
{
  bool validFinger = false;
  String Empid;

  Serial.println("Inside matchRfid function");
  Serial.print("Finger ID: ");
  Serial.println(fingerid);

  // Open the file from SPIFFS
  File datafilesBio = SPIFFS.open("/BioRegs.csv", "r");
  if (!datafilesBio)
  {
    Serial.println("Failed to open file for reading");
    return false;
  }

  // Read the file line by line
  while (datafilesBio.available())
  {
    String line = datafilesBio.readStringUntil('\n');
    Serial.print("Line: ");
    Serial.println(line);

    // Find the comma separating ID and Empid
    int commaIndex = line.indexOf(',');
    if (commaIndex == -1)
    {
      continue; // Skip lines that don't have a comma
    }

    // Extract ID and Empid from the line
    String idfile = line.substring(0, commaIndex);
    Empid = line.substring(commaIndex + 1); // Assuming Empid is the second value
    Serial.println("Empid check: " + Empid);
    int idmatchCheck = idfile.toInt();

    // Check if the fingerprint ID matches
    if (fingerid == idmatchCheck)
    {
      Serial.println("ID Found");
      validFinger = true;
      break; // Exit the loop if ID is found
    }
  }

  // Close the file
  datafilesBio.close();

  // Handle the result
  Serial.println("Checking conditions for door open");
  Serial.print("rfidFound: ");
  Serial.println(validFinger);
  esp_task_wdt_reset();
  if (validFinger)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      if (client.connect("www.google.com", 80))
      {
        esp_task_wdt_reset();
        Serial.println("Connected to server, sending data");
        ServerSend(Empid, CompanyId); // Implement this function based on your needs
      }
      else
      {
        Serial.println("Failed to connect to server");
        Serial.println("OFFLINE Preparation");
        Serial.println("DOOR OPEN");
        OfflineDataWrite(Empid); // Implement this function based on your needs
        OpenDoors = true;
      }
    }
    else
    {
      Serial.println("WiFi not connected");
      Serial.println("OFFLINE Preparation");
      Serial.println("DOOR OPEN");
      OfflineDataWrite(Empid); // Implement this function based on your needs
      OpenDoors = true;
    }
  }
  else
  {
    Serial.println("Unauthorized access here");
    digitalWrite(REDLED, HIGH);
    digitalWrite(REDLED1, HIGH);
    digitalWrite(GREENLED1, LOW);
    digitalWrite(GREENLED, LOW);
    UnauthorizedAccess(); // Implement this function based on your needs
    vTaskDelay(pdMS_TO_TICKS(300));
    digitalWrite(REDLED, LOW);
    digitalWrite(REDLED1, LOW);
  }

  vTaskDelay(pdMS_TO_TICKS(2000));
  return validFinger;
}

/*
bool matchRfid(int fingerid)
{
    bool validFinger = false;
    String Empid;
        Serial.println("inside else part Match Rfid");
        Serial.println(fingerid);  // /EmpRfid.csv

        File datafiles = SPIFFS.open("/BioRegs.csv", "r");
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
                validFinger = true;
                datafiles.close();
                break;
            }
        }
        datafiles.close();
    Serial.println("Checking conditions for door open");
    Serial.print("rfidFound: ");
    Serial.println(validFinger);
    Serial.print("nemo: ");
    if(validFinger){
      if (WiFi.status() == WL_CONNECTED) {
            if (client.connect("www.google.com", 80)) {
                Serial.println("Connected to server, sending data");
                ServerSend(Empid, CompanyId);
            } else {
                Serial.println("Failed to connect to server");
                Serial.println("OFFLINE Preparation");
                Serial.println("DOOR OPEN");
                OfflineDataWrite(Empid);
                OpenDoors = true;
            }
        } else {
            Serial.println("WiFi not connected");
            Serial.println("OFFLINE Preparation");
            Serial.println("DOOR OPEN");
            OfflineDataWrite(Empid);
            OpenDoors = true;
        }
    }
    else
    {
      Serial.println("Last");
      digitalWrite(REDLED, HIGH);
      digitalWrite(REDLED1, HIGH);
      UnauthorizedAccess();
      vTaskDelay(pdMS_TO_TICKS(300));
      digitalWrite(REDLED, LOW);
      digitalWrite(REDLED1, LOW);
    }

    vTaskDelay(pdMS_TO_TICKS(2000));
    return validFinger;
}
*/

void SensorFingerBegin()
{
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

void UpdateActivity()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    if (client.connect("www.google.com", 80))
    {
      if (SPIFFS.begin(true))
      {
        Serial.println("Update Activity");
        delay(2000);
        DeviceIdInitialize();

        int dataCount = 0;
        int startCount = 0;
        const int endCount = 20;
        int loop_count = 0;
        int httpCode = 0;
        int falseReturn = 0;
        DateTime now = rtc.now();
        int minutes_start = now.minute();
        Serial.print("Start Time :");
        Serial.print(minutes_start);

        do
        {
          ledFetchingon();
          HTTPClient http; // Declare object of class HTTPClient
          DynamicJsonBuffer jsonBuffers;
          JsonObject &JSONEncoder = jsonBuffers.createObject();
          JSONEncoder["companyId"] = CompanyId;
          JSONEncoder["deviceId"] = DeviceId;
          JSONEncoder["startCount"] = startCount;
          JSONEncoder["endCount"] = endCount;
          char JSONmessageBuffer[500]; // Adjust the buffer size as needed
          JSONEncoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
          Serial.println(JSONmessageBuffer);
          Serial.println("Hello");

          http.begin("https://wildfly.letzcheckin.com:443/EmployeeAttendenceAPI/RFIDAPI/AutoSyncEmployeeData");
          Serial.println("http begin");
          http.addHeader("Content-Type", "application/json"); // Specify content-type header
          Serial.println("Post Main");

          // Sending requests starts here ..
          httpCode = http.POST(JSONmessageBuffer); // Send the request
          Serial.print("HTTP Code ");
          Serial.println(httpCode);
          yield();

          String payload = http.getString();
          Serial.print("Payload :");
          Serial.println(payload);

          if (httpCode == 200)
          {
            // Convert the JSON object to a string and print it
            Serial.println("RFID response");
            JsonObject &root = jsonBuffers.parseObject(payload); // Parse the JSON payload dynamically

            if (!root.success())
            {
              Serial.println("Parsing failed!");
              continue; // Continue with the next iteration
            }

            if (root.size() > 0)
            {
              Serial.println("Root success and has value Update");
            }

            Serial.print("Json string : ");
            root.printTo(Serial);
            Serial.print("root size :");
            Serial.println(root.size());

            JsonArray &empRfidList = root["employeeInfoList"];
            Serial.print("Data Count :");
            Serial.print(dataCount);
            Serial.print("emp rfid size:");
            Serial.print(empRfidList.size());
            Serial.println();
            Serial.print("JSON NODES");

            String jsonString;
            empRfidList.printTo(jsonString);
            JsonArray &nodes = jsonBuffers.parseArray(jsonString);

            if (!nodes.success())
            {
              Serial.println("parseObject() failed");
              jsonBuffers.clear();
              continue; // Continue with the next iteration
            }
            else
            {
              String dataCountStr = root["dataCount"].as<char *>();
              dataCount = dataCountStr.toInt();
              startCount += endCount;
              int node_length = nodes.size();

              for (int i = 0; i < node_length; i++)
              {
                Serial.printf("node-%i\nEmployee : ", i);
                String empId = nodes[i]["employeeId"].as<const char *>();
                String Name = nodes[i]["name"].as<const char *>();
                String Status = nodes[i]["status"].as<const char *>();
                String Activity = nodes[i]["activity"].as<const char *>();
                String Department = nodes[i]["department"].as<const char *>();
                String rfid = nodes[i]["rfidNo"].as<const char *>();
                int Matched = Status.toInt();

                if (rfid == "" || rfid == " ")
                {
                  rfid = '-';
                  Serial.println("Rfid is not empty");
                }

                if (Activity == "Add_Employee" || Activity == "Update_Employee" || Activity == "UnBlocked_Employee" || Activity == "UnLocked_Employee")
                {
                  updated_Activity(rfid, Matched, empId, Name, Department);
                }
                else if (Activity == "Deleted_Employee" || Activity == "Blocked_Employee" || Activity == "Locked_Employee")
                {
                  delete_activity(rfid, Matched, empId, Name, Department);
                }
              }
              Serial.println("Forloop end ");
            }
            Serial.println("ELSE end ");
            yield();
            Serial.println(httpCode);
            Serial.println("Successfully Stored RFID Values");
          }
          else
          {
            falseReturn += 1;
            if (falseReturn > 2)
            {
              Serial.println("Http response Failed");
              ledFetchingoff();
              http.end();
              break;
            }
          }
          http.end();
        } while (dataCount >= startCount);

        int minute_end = now.minute();
        int total_minutes = minutes_start - minute_end;
        Serial.print("Total Time Took : ");
        Serial.println(total_minutes);
        EEPROM.write(510, 1);
        EEPROM.commit(); // Save data to EEPROM
      }
    }
  }
  fileReadAndWrite();
  Serial.println("Update worked fine");
  vTaskDelay(2000);
  ledFetchingoff();
  // BioRegsCheck();
}

/*
void updated_Activity(String rfid, int Matched, String empId, String Name, String Department, int BioRegster)
{
  // Open the file for reading
  File readFile = SPIFFS.open("/Rfid.csv", FILE_READ);
  if (!readFile)
  {
    Serial.println("Failed to open file for reading");
    return;
  }

  // Open the write file
  File writeFile = SPIFFS.open("/EmpRfid.csv", FILE_WRITE);
  if (!writeFile)
  {
    Serial.println("Failed to open file for writing");
    readFile.close();
    return;
  }
  bool lineFound = false;
  // Read and write line by line
  while (readFile.available())
  {
    String line = readFile.readStringUntil('\n');
    //
    int commaIndex        = line.indexOf(',');
    if (commaIndex == -1) {
      continue; // Skip lines that don't have a comma
    }
    int secondCommaIndex  = line.indexOf(',', commaIndex + 1);
    int thirdCommaIndex   = line.indexOf(',', secondCommaIndex + 1);
    int fourthCommanIndex = line.indexOf(',', thirdCommaIndex+1);
    int fifthCommaIndex   = line.indexOf(",", fourthCommanIndex);
    // Serial.println(empId);
    String currentEmpId = line.substring(secondCommaIndex + 1, thirdCommaIndex);
    int currentEmpidint = currentEmpId.toInt();
    int empIdint = empId.toInt();
    Serial.print("empid: ");
    Serial.println(empIdint);
    Serial.print("Current Empid: ");
    Serial.println(currentEmpId);
    if (currentEmpidint == empIdint)
    {
      Serial.print("Bio Register V :");
      Serial.println(BioRegster);
      if(BioRegster > 0){
        Serial.print("line ");
        Serial.println(line);
        String Rfid = line.substring(0, commaIndex);
        String matchedStatus = line.substring(commaIndex+1,secondCommaIndex);
        int Matchede = matchedStatus.toInt();
        String Names = line.substring(thirdCommaIndex+1, fourthCommanIndex);//line.substring(secondCommaIndex+1, thirdCommaIndex);
        String Departments = line.substring(fourthCommanIndex+1);
        Serial.print("Rfid");
        Serial.println(Rfid);
        Serial.print("Names");
        Serial.println(Names);
        Serial.print("Matched");
        Serial.println(Matchede);
        Serial.print("Departments ss:");
        Serial.println(Departments);
        String force = Rfid + String(",") + Matchede + String(",") + empId + String(",") + Names + String(",") + Departments + String(",")+ BioRegster + String("\n") ;
        Serial.print("Data Bio example ");
        Serial.println(force);
        Serial.println("End");
        writeFile.print(force);
        lineFound = true;
      }
      else{
        Serial.print("Current Empid: ");
        Serial.println(empId);
        line = rfid + String(",") + Matched + String(",") + empId + String(",") + Name + String(",") + Department + String(",Updated\n");
        writeFile.print(line);
        lineFound = true;
      }
    }

    else
    {
      writeFile.print(line + "\n");
    }

  }

  readFile.close();
  writeFile.close();

  if (!lineFound)
  {
    Serial.print("Line Found :");
    File readFile = SPIFFS.open("/Rfid.csv", FILE_APPEND);
    Serial.println(lineFound);
    // readFile.print(rfid + String(",") + Matched + String(",") + empId + String(",") + Name + String(",") + Department + String(",") + String("Unblocked Employee") + String('\n'));
     readFile.print(rfid + String(",") + Matched + String(",") + empId + String(",") + Name + String(",") + Department + String(",") + String('\n'));
    readFile.close();
  }

  if (lineFound)
  {
    // Remove the original file
    if (SPIFFS.remove("/Rfid.csv"))
    {
      Serial.println("Original file removed");
    }
    else
    {
      Serial.println("Failed to remove the original file");
    }

    // Rename the new file to the original file name
    if (SPIFFS.rename("/EmpRfid.csv", "/Rfid.csv"))
    {
      Serial.println("File renamed successfully");
    }
    else
    {
      Serial.println("Failed to rename the file");
    }
    Serial.println("Line updated successfully");
  }
  else
  {
    Serial.println("Line not found; new line added");
  }
  // Unblocked is not working in logic mistake...
}
*/

void updated_ActivityBio(String rfid, int Matched, String empId, String Name, String Department, int TemplateId, int BioRegster)
{
  // Open the file for reading
  File readFile = SPIFFS.open("/Rfid.csv", FILE_READ);
  if (!readFile)
  {
    Serial.println("Failed to open file for reading");
    return;
  }

  // Open a temporary file for writing
  File tempFile = SPIFFS.open("/TempRfid.csv", FILE_WRITE);
  if (!tempFile)
  {
    Serial.println("Failed to open temporary file for writing");
    readFile.close();
    return;
  }

  bool lineFound = false;

  // Read and write line by line
  while (readFile.available())
  {
    esp_task_wdt_reset();
    String line = readFile.readStringUntil('\n');
    Serial.print("Line:");
    Serial.println(line);
    int commaIndex = line.indexOf(',');
    if (commaIndex == -1)
    {
      continue; // Skip lines that don't have a comma
    }

    int secondCommaIndex = line.indexOf(',', commaIndex + 1);
    int thirdCommaIndex = line.indexOf(',', secondCommaIndex + 1);
    int fourthCommaIndex = line.indexOf(',', thirdCommaIndex + 1);
    int fifthCommaIndex = line.indexOf(',', fourthCommaIndex + 1);

    String currentEmpId = line.substring(secondCommaIndex + 1, thirdCommaIndex);
    int currentEmpidint = currentEmpId.toInt();
    int empIdint = empId.toInt();

    if (currentEmpidint == empIdint)
    {
      if (BioRegster > 0)
      {
        String Rfid = line.substring(0, commaIndex);
        String matchedStatus = line.substring(commaIndex + 1, secondCommaIndex);
        int Matchede = matchedStatus.toInt();
        String Names = line.substring(thirdCommaIndex + 1, fourthCommaIndex);
        String Departments = line.substring(fourthCommaIndex + 1, fifthCommaIndex);
        String force = Rfid + "," + Matchede + "," + empId + "," + Names + "," + Departments + "," + String(TemplateId) + "," + String(BioRegster) + "\n";
        Serial.print("Data:");
        Serial.println(force);
        tempFile.print(force);
      }
      else
      {
        String lineToWrite = rfid + "," + Matched + "," + empId + "," + Name + "," + Department + ",Updated\n";
        tempFile.print(lineToWrite);
      }
      lineFound = true;
    }
    else
    {
      tempFile.print(line + "\n");
    }
  }

  readFile.close();
  tempFile.close();

  if (!lineFound)
  {
    // Append the new line if not found
    File appendFile = SPIFFS.open("/Rfid.csv", FILE_APPEND);
    if (!appendFile)
    {
      Serial.println("Failed to open file for appending");
      return;
    }
    appendFile.print(rfid + "," + Matched + "," + empId + "," + Name + "," + Department + ",\n");
    appendFile.close();
  }

  // Replace the original file with the updated file
  if (SPIFFS.remove("/Rfid.csv"))
  {
    esp_task_wdt_reset();
    if (SPIFFS.rename("/TempRfid.csv", "/Rfid.csv"))
    {
      Serial.println("File updated successfully");
    }
    else
    {
      Serial.println("Failed to rename the file");
    }
  }
  else
  {
    Serial.println("Failed to remove the original file");
  }
  esp_task_wdt_reset();
}

/*

void updated_Activity(String rfid, int Matched, String empId, String Name, String Department, int BioRegster)
{
  // Open the file for reading
  File readFile = SPIFFS.open("/Rfid.csv", FILE_READ);
  if (!readFile)
  {
    Serial.println("Failed to open file for reading");
    return;
  }

  // Open a temporary file for writing
  File tempFile = SPIFFS.open("/TempRfid.csv", FILE_WRITE);
  if (!tempFile)
  {
    Serial.println("Failed to open temporary file for writing");
    readFile.close();
    return;
  }

  bool lineFound = false;

  // Read and write line by line
  while (readFile.available())
  {
    String line = readFile.readStringUntil('\n');
    Serial.print("Line:");
    Serial.println(line);
    int commaIndex = line.indexOf(',');
    if (commaIndex == -1)
    {
      continue; // Skip lines that don't have a comma
    }

    int secondCommaIndex = line.indexOf(',', commaIndex + 1);
    int thirdCommaIndex = line.indexOf(',', secondCommaIndex + 1);
    int fourthCommaIndex = line.indexOf(',', thirdCommaIndex + 1);
    int fifthCommaIndex = line.indexOf(',', fourthCommaIndex + 1);

    String currentEmpId = line.substring(secondCommaIndex + 1, thirdCommaIndex);
    int currentEmpidint = currentEmpId.toInt();
    int empIdint = empId.toInt();

    if (currentEmpidint == empIdint)
    {
      lineFound = true;
      if (BioRegster > 0)
      {
        String Rfid = line.substring(0, commaIndex);
        String matchedStatus = line.substring(commaIndex + 1, secondCommaIndex);
        int Matchede = matchedStatus.toInt();
        String Names = line.substring(thirdCommaIndex + 1, fourthCommaIndex);
        String Departments = line.substring(fourthCommaIndex + 1);
        String force = Rfid + "," + Matchede + "," + empId + "," + Names + "," + Departments + "," + BioRegster + "\n";
        Serial.print("Data:");
        Serial.println(force);
        tempFile.print(force);
      }
      else
      {
        String lineToWrite = rfid + "," + Matched + "," + empId + "," + Name + "," + Department + ",Updated\n";
        Serial.print("Updated Data:");
        Serial.println(lineToWrite);
        tempFile.print(lineToWrite);
      }
    }
    else
    {
      tempFile.print(line + "\n");
    }
  }

  readFile.close();
  tempFile.close();

  if (!lineFound)
  {
    // Append the new line if not found
    File appendFile = SPIFFS.open("/Rfid.csv", FILE_APPEND);
    if (!appendFile)
    {
      Serial.println("Failed to open file for appending");
      return;
    }
    appendFile.print(rfid + "," + Matched + "," + empId + "," + Name + "," + Department + ",\n");
    appendFile.close();
  }

  // Replace the original file with the updated file
  if (SPIFFS.remove("/Rfid.csv"))
  {
    if (SPIFFS.rename("/TempRfid.csv", "/Rfid.csv"))
    {
      Serial.println("File updated successfully");
    }
    else
    {
      Serial.println("Failed to rename the file");
    }
  }
  else
  {
    Serial.println("Failed to remove the original file");
  }
}

*/

void updated_Activity(String rfid, int Matched, String empId, String Name, String Department)
{
  // Open the file for reading
  File readFile = SPIFFS.open("/Rfid.csv", "r");
  if (!readFile)
  {
    Serial.println("Failed to open file for reading");
    return;
  }

  // Open the write file
  File writeFile = SPIFFS.open("/EmpRfid.csv", "w");
  if (!writeFile)
  {
    Serial.println("Failed to open file for writing");
    readFile.close();
    return;
  }
  bool lineFound = false;
  // Read and write line by line
  while (readFile.available())
  {
    String line = readFile.readStringUntil('\n');
    int commaIndex = line.indexOf(',');
    int secondCommaIndex = line.indexOf(',', commaIndex + 1);
    int thirdCommaIndex = line.indexOf(',', secondCommaIndex + 1);
    // Serial.println(empId);
    String currentEmpId = line.substring(secondCommaIndex + 1, thirdCommaIndex);
    int currentEmpidint = currentEmpId.toInt();
    int empIdint = empId.toInt();
    if (currentEmpidint == empIdint)
    {
      Serial.print("Current Empid: ");
      Serial.println(empId);
      line = rfid + String(",") + Matched + String(",") + empId + String(",") + Name + String(",") + Department + String(",Updated\n");
      writeFile.print(line);
      lineFound = true;
    }
    else
    {
      writeFile.print(line + "\n");
    }
  }

  readFile.close();
  writeFile.close();

  if (!lineFound)
  {
    Serial.print("Line Found :");
    File readFile = SPIFFS.open("/Rfid.csv", "a");
    Serial.println(lineFound);
    readFile.print(rfid + String(",") + Matched + String(",") + empId + String(",") + Name + String(",") + Department + String(",") + String("Unblocked Employee") + String('\n'));
    readFile.close();
  }

  if (lineFound)
  {
    // Remove the original file
    if (SPIFFS.remove("/Rfid.csv"))
    {
      Serial.println("Original file removed");
    }
    else
    {
      Serial.println("Failed to remove the original file");
    }

    // Rename the new file to the original file name
    if (SPIFFS.rename("/EmpRfid.csv", "/Rfid.csv"))
    {
      Serial.println("File renamed successfully");
    }
    else
    {
      Serial.println("Failed to rename the file");
    }
    Serial.println("Line updated successfully");
  }
  else
  {
    Serial.println("Line not found; new line added");
  }
  // Unblocked is not working in logic mistake...
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
  JSONencoder["deviceType"] = DeviceType;
  JSONencoder["date"] = String(now.year()) + String("-") + String(now.month(), DEC) + String("-") + String(now.day());
  JSONencoder["time"] = String(now.hour()) + String(":") + String(now.minute()) + String(":") + String(now.second());
  Serial.println(String(now.hour()) + String(":") + String(now.minute()) + String(":") + String(now.second()));
  char JSONmessageBuffer[300];
  JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  Serial.print("JSON MESSAGE server send");
  Serial.println(JSONmessageBuffer);
  http.begin("http://letzcheckin.com:8080/EmployeeAttendenceAPI/employee/EmployeeCheckInOut"); // Specify request destination
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
    const char *OrganizationStatusF = root["organizationStatus"];
    Serial.print("Code return element = ");
    Serial.println("EmployeeId ");
    Serial.println(code);
    Serial.println("Employee name ");
    Serial.println(userName);
    Serial.println("Status name ");
    Serial.println(printStatus);
    Serial.print("Org Status");
    Serial.print(OrganizationStatusF);
    if (strcmp(OrganizationStatusF, "Active") == 0)
    {
      Serial.println("inside the organization status");
      if ((strcmp(retStatus, "CHECKIN") == 0))
      {
        Serial.println("Inside Open Door send Server");
        OpenDoors = true;
        return -1;
        // Fetching = true;
      }
      else if ((strcmp(retStatus, "CHECKOUT") == 0))
      {
        // OpenDoor();
        OpenDoors = true;
        return -1;
        // Fetching = true;
      }
      else if (strcmp(retStatus, "SAME_TIME") == 0)
      {
        OpenDoors = true;
        // Fetching = true;
        return -1;
      }
      else if ((strcmp(retStatus, "BLOCKED") == 0))
      {
        UnauthorizedAccess();
        CloseDoors = true;
        digitalWrite(REDLED, HIGH); // turn the LED off.
        digitalWrite(REDLED1, HIGH);
        return -1;
        // Fetching = true;
      } //                          NOT_VAILD
      else if ((strcmp(retStatus, "NOT_VAILD") == 0))
      {
        UnauthorizedAccess();
        digitalWrite(REDLED, HIGH); // turn the LED off.
        digitalWrite(REDLED1, HIGH);
        CloseDoors = true;
        return -1;
        // Fetching = true;
      }
      else if ((strcmp(retStatus, "Employee_Not_Assigned_To_The_Device") == 0))
      {
        UnauthorizedAccess();
        CloseDoors = true;
        digitalWrite(REDLED, HIGH); // turn the LED off.
        digitalWrite(REDLED1, HIGH);
        // Fetching = true;
        return -1;
      }
      else if ((strcmp(retStatus, "RFID_NO_Is_Not_Mapped_To_Any_Employee") == 0))
      {
        UnauthorizedAccess();
        CloseDoors = true;
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
    else
    {
      // Organization is not Active
    }
  }
  // NOT_VAILD
  else
  {
    Serial.println("could not send back to server ");
    OfflineDataWrite(Empid); // if response failed but valid employee
    OpenDoors = true;
  }

  http.end(); // Close connection
  Serial.println("Succesfully Send Data To BackEnd");
  // delay(1000);
  return 1;
}

void deleteFingerprint(uint8_t id)
{
  // uint8_t p = -1;
  // delay(100); // Small delay before sending the command
  finger.deleteModel(id);
  // delay(100); // Small delay after sending the command
}

void fingerSensor()
{
  if (RegistrationFinger)
  {
    int id = getFingerprintID();
    if (id == 1)
    {
      Serial.print("ID: ");
      Serial.println(id);
      if (FingerPrintId > 1)
      {
        digitalWrite(GREENLED, HIGH);
        digitalWrite(GREENLED1, HIGH);
        matchRfid(FingerPrintId);
        FingerPrintId = 0;
      }
    }
    else if (id == -2)
    {
      UnauthorizedAccess();
      // delay(2000);
    }
    else if (id == 0)
    {
      UnauthorizedAccess();
    }
    else
    {
      // Leave it empty
    }

    vTaskDelay(pdMS_TO_TICKS(1)); // Minimal delay
  }
}

void wifiConnectedCheckerMin()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    int stationConnect = WiFi.softAPgetStationNum();
    if (stationConnect > 0)
    {
      WiFi.mode(WIFI_AP);
    }
  }
}

void setup()
{
  // Try to get the templates for fingers 1 through 10
  initialSetupFun();

  // char sup = char(EEPROM.read(512));
  // String temp = String(sup);
  // Serial.print(temp);
  // Serial.println(temp);
  // if(temp.length()>0){

  // Serial.begin(115200);
  // SensorFingerBegin();
  /*
  for (int finger = 1; finger < 100; finger++)
  {
    // deleteFingerprint(finger);
    downloadFingerprintTemplate(finger);
  }

  */

  // EEPROM.write(512,0);
  // EEPROM.commit();
  // }
}

void loop()
{
  ws.cleanupClients();
  fingerSensor();
  wifiConnectedCheckerMin();
}