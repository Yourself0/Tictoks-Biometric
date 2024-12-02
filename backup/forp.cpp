#define Build_Version "RFSD_V2_JUL_2_2024"
#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <WiFi.h>
#include <Wire.h>
#include <EEPROM.h>
#include <SPIFFS.h>
#include <MFRC522.h>
#include <ESPmDNS.h>
#include <RTClib.h>
#include <HTTPClient.h>
#include "soc/rtc_wdt.h"
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include <ESPAsyncWebServer.h>

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

long OtpVerifiy;
bool WifiPage = false;
bool quicksetupCld = false;
bool RfidRegisterPage = false;
bool CompanyPage = false;
bool DeviceidPage = false;
bool EmployeeListsHigh = false;
bool wifiScanRequested = false;
bool UpdateEmployee = true;
bool Network_status = true;
bool SpiffsTimerStart = false;
bool Fetching = true;
bool RfidRegister = false;
bool OpenDoors = false;
bool CloseDoors = false;
bool WebsocketConnected = false;

String CompanyId = "";
String CompanyName = "";
String DeviceType = "RFID";
String DeviceId = "";
String json;
String DeviceiList;
String hostName = "tictoksrfid";

byte ssPins[] = {SS_1_PIN, SS_2_PIN};

RTC_DS3231 rtc;
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

MFRC522 mfrc522[NO_OF_READERS];
WiFiClient client;

int serverUpdateCount = 0;
int i = 0;

const char *ASSID = "Tictoks RF V2";
const char *APASS = "123456789";
char EMPDETAILS[] = "/EmpRfid.csv";
char servername[] = "www.google.com";

void ServerSend(String RfId, String companyId);
bool matchRfid(String Rfid, int nemo);
void fileReadAndWrite();

void printFreeHeap(const char *position)
{
  Serial.print(position);
  Serial.print(" Free Heap: ");
  Serial.println(ESP.getFreeHeap());
}

void setupWatchdogTimer()
{
  // Configure the Watchdog Timer timeout
  esp_task_wdt_init(WATCHDOG_TIMEOUT_SECONDS, true); // Set true to enable panic (reset) when timeout is reached
  esp_task_wdt_add(NULL);                            // NULL means the main task, add other tasks if needed
}

void WifiStatusNotConnected()
{
  if (WebsocketConnected)
  {
    Serial.print("Sent not connected");
    ws1.textAll("Not Connected");
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
  else{
    Serial.println("it was false man ");
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

// Initiliaze RFID Reader
void InitializeRfid()
{
  SPI.begin();
  for (uint8_t reader = 0; reader < NO_OF_READERS; reader++)
  {
    mfrc522[reader].PCD_Init(ssPins[reader], RST_PIN);
    Serial.println(F("RFID "));
    Serial.println(reader);
    Serial.println(F(": "));
    mfrc522[reader].PCD_DumpVersionToSerial();
    mfrc522[reader].PICC_HaltA();
    mfrc522[reader].PCD_SoftPowerDown();
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
  digitalWrite(GREENLED, HIGH);
  digitalWrite(REDLED1, LOW);
  digitalWrite(GREENLED1, HIGH);
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

void delete_activity(String rfid, int Matched, String empId, String Name, String Department)
{
  File readFile = SD.open("/Rfid.csv", FILE_READ);
  if (!readFile)
  {
    Serial.println("Failed to open file for reading");
    return;
  }
  File writeFile = SD.open("/EmpRfid.csv", FILE_WRITE);
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
    if (SD.remove("/Rfid.csv"))
    {
      Serial.println("Original file removed");
    }
    else
    {
      Serial.println("Failed to remove the original file");
    }

    // Rename the new file to the original file name
    if (SD.rename("/EmpRfid.csv", "/Rfid.csv"))
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

void updated_Activity(String rfid, int Matched, String empId, String Name, String Department)
{
  // Open the file for reading
  File readFile = SD.open("/Rfid.csv", FILE_READ);
  if (!readFile)
  {
    Serial.println("Failed to open file for reading");
    return;
  }

  // Open the write file
  File writeFile = SD.open("/EmpRfid.csv", FILE_WRITE);
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
    File readFile = SD.open("/Rfid.csv", FILE_APPEND);
    Serial.println(lineFound);
    readFile.print(rfid + String(",") + Matched + String(",") + empId + String(",") + Name + String(",") + Department + String(",") + String("Unblocked Employee") + String('\n'));
    readFile.close();
  }

  if (lineFound)
  {
    // Remove the original file
    if (SD.remove("/Rfid.csv"))
    {
      Serial.println("Original file removed");
    }
    else
    {
      Serial.println("Failed to remove the original file");
    }

    // Rename the new file to the original file name
    if (SD.rename("/EmpRfid.csv", "/Rfid.csv"))
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

void UpdateActivity()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    if (client.connect(servername, 80))
    {
      // DebuggingUpdate("Start");
      mountinSD();
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
        http.begin("https://wildfly.tictoks.in:443/EmployeeAttendenceAPI/RFIDAPI/AutoSyncEmployeeData");
        Serial.println("http begin");
        http.addHeader("Content-Type", "application/json"); // Specify content-type header
        Serial.println("Post Main");
        // Sending requests startes here ..
        httpCode = http.POST(JSONmessageBuffer); // Send the request
        Serial.print("HTTP Code ");
        Serial.println(httpCode);
        yield();
        String payload = http.getString();
        char *payloadBuffer; // Adjust the size based on your requirement
        payload.toCharArray(payloadBuffer, sizeof(payloadBuffer));
        Serial.print("pay load :");
        Serial.println(payload);
        if (httpCode == 200)
        {
          // Convert the JSON object to a string and print it
          Serial.println("RFID response");
          JsonObject &root = jsonBuffers.parseObject(payload); // Parse the JSON payload dynamically
          payload = "";
          if (!root.success())
          {
            Serial.println("Parsing failed!");
          }
          if (root.success())
          {
            if (root.size() > 0)
            {
              Serial.println("Root success ahd have value Update");
            }
          }
          Serial.print("Json string : ");
          root.printTo(Serial);
          Serial.print("root size :");
          Serial.println(root.size());
          // JsonArray& empRfidList = root["empRfidList"]; to this employeeInfoList make sure u change ...
          JsonArray &empRfidList = root["employeeInfoList"];
          Serial.print("Data Count :");
          Serial.print(dataCount);
          Serial.print("emp rfid size:");
          Serial.print(empRfidList.size());
          Serial.println();
          Serial.print("JSON NODES");
          // root.remove("employeeInfoList");
          String jsonString;
          empRfidList.printTo(jsonString);
          JsonArray &nodes = jsonBuffers.parseArray(jsonString);
          // new writting code here
          if (!nodes.success())
          {
            Serial.println("parseObject() failed");
            jsonBuffers.clear();
          }
          else
          {
            String dataCountStr = root["dataCount"].as<char *>();
            dataCount = dataCountStr.toInt();
            startCount += endCount;
            int node_length = nodes.size();
            for (int i = 0; i < node_length; i++)
            {
              // File dataFile = SD.open("/Rfid.csv", "a");
              Serial.printf("node-%i\nEmployee : ", i);
              String empId = nodes[i]["employeeId"].as<const char *>();
              String Name = nodes[i]["name"].as<const char *>();
              // String rfid = nodes[i]["rfidNo"].as
              String Status = nodes[i]["status"].as<const char *>();
              String Activity = nodes[i]["activity"].as<const char *>();
              // String DeviceIdM = nodes[i]["deviceId"].as<const char *>();
              String Department = nodes[i]["department"].as<const char *>();
              String rfid = nodes[i]["rfidNo"].as<const char *>();
              // dataFile.print("Add Function "+String("\n"));
              int Matched = Status.toInt();
              if (rfid == "" || rfid == " ")
              {
                rfid = '-';
                Serial.println("Rfid is not empty");
              }
              // DeviceIdM.toCharArray(buf, sizeof(buf));
              if (Activity == "Add_Employee")
              {
                updated_Activity(rfid, Matched, empId, Name, Department);
                // Serial.println("EmpID :");
                // Serial.println(empId);
                // Serial.print("RFID : ");
                // Serial.println(rfid);
                // String empData = "";
                // Serial.println("RFID:");
                // Serial.println(rfid.c_str());
                // empData = rfid + String(",") + Matched + String(",") + empId + String(",") + Name + String(",") + Department + String(",") + String("Added") + String("\n");
                // Serial.print("Empid data: ");
                // Serial.println(empData.c_str());
                // dataFile.print(empData);
                // Serial.println("Written");
                // dataFile.close();
              }
              else if (Activity == "Update_Employee")
              {
                Serial.println(rfid + String(",") + Matched + String(",") + empId + String(",") + Name + String(",") + Department + String("\n"));
                updated_Activity(rfid, Matched, empId, Name, Department);
              }
              else if (Activity == "Deleted_Employee")
              {
                delete_activity(rfid, Matched, empId, Name, Department);
                ;
              }
              else if (Activity == "UnBlocked_Employee")
              {
                updated_Activity(rfid, Matched, empId, Name, Department);
              }
              else if (Activity == "Blocked_Employee")
              {
                delete_activity(rfid, Matched, empId, Name, Department);
                ;
              }
              else if (Activity == "UnLocked_Employee")
              {
                Serial.println(rfid + String(",") + Matched + String(",") + empId + String(",") + Name + String(",") + Department + String("\n"));
                updated_Activity(rfid, Matched, empId, Name, Department);
              }
              else if (Activity == "Locked_Employee")
              {
                delete_activity(rfid, Matched, empId, Name, Department);
                ;
              }
            }
            Serial.println("Forloop end ");
          }
          Serial.println("ELSe end ");
          // Close connection
          yield();
          Serial.println(httpCode);
          Serial.println("SucessFully Stored RFID Values");
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
      Serial.print("total Time Took : ");
      Serial.println(total_minutes);
      EEPROM.write(510, 1);
      EEPROM.commit(); // Save data to EEPROM
      // return httpCode;
    }
  }
  fileReadAndWrite();
  Serial.println("Updated worked fine");
  vTaskDelay(2000);
  // DebuggingUpdate("End");
  ledFetchingoff();
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

// Scanning wifi in core
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
    Serial.println("Spiffs Start :" + SpiffsTimerStart);
    if (SpiffsTimerStart)
    {
      bool datasent = true;
      HTTPClient http; // Declare object of class HTTPClient
      Serial.println("Sending offline data to server");
      Serial.println("Wifi status connected :"+WebsocketConnected);
            
      mountinSD();
      if (WiFi.status() == WL_CONNECTED)
      {
        digitalWrite(ORANGELED, LOW);
        Network_status = false;
        if (client.connect(servername, 80))
        {
          Network_status = true;
          if (SD.exists("/OfflineData.csv"))
          {
            File csv = SD.open("/OfflineData.csv", "r");
            Serial.println("Connected to Internet ./");
            Serial.println("Wifi status connected "+WebsocketConnected);
            if (csv.available())
            {
              Serial.println("Sending Offline data to server");
              const byte BUFFER_SIZE = 200;
              char buffer[BUFFER_SIZE + 1];
              buffer[BUFFER_SIZE] = '\0';
              int j = 0; // Initialize j outside the loop
              int file_count = 0;
              int success_count = 0;
              Serial.print("Inside csv");
              while (csv.available())
              {
                file_count += 1;
                Serial.println("file read fun");
                Serial.print(csv.read());
                String line = csv.readStringUntil('\n');
                Serial.println("line");
                Serial.print(line);
                const byte BUFFER_SIZE = 200;
                char buffer[BUFFER_SIZE + 1];
                buffer[BUFFER_SIZE] = '\0';
                // Copy the line content to the buffer
                strncpy(buffer, line.c_str(), sizeof(buffer));
                char *ptr = strtok(buffer, ",");
                int j = 0;
                String smp_String;
                String EmpId;
                String Date;
                String Temp_Time;
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
                  JSONencoder["deviceType"] = "RFID";
                  JSONencoder["date"] = Date; // String(now.year()) + String("-") + String(now.month(), DEC) + String("-") + String(now.day());
                  JSONencoder["time"] = Time; // String(now.hour()) + String(":") + String(now.minute()) + String(":") + String(now.second());
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
                    Serial.println("Data Send Successfully");
                    success_count += 1;
                  }
                  http.end();
                }
              }
              csv.close();
              SD.remove("/OfflineData.csv");
            }
          }
          else
          {
            Serial.println("File Does Not exists");
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
        Serial.println("Websocket status: " + WebsocketConnected);
        Network_status = false;
        digitalWrite(ORANGELED, HIGH);
        int stationCount = WiFi.softAPgetStationNum();
        if (stationCount <= 0)
        {
          WifiConnectCheck();
        }
      }
      Serial.println("End of send server ");
    }
    vTaskDelay(pdMS_TO_TICKS(60000));
  }
}

void updateEmployeeDetails(void *pvParameters)
{
  (void)pvParameters; // Unused parameter
  while (true)
  {
    Serial.println("Update Function Updates 1");
    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("Update Function 3");
      UpdateActivity();
      fileReadAndWrite();
    }
    else
    {
      Serial.println("Wi-Fi not connected");
    }
    vTaskDelay(pdMS_TO_TICKS(1200000));
  }
}

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
      digitalWrite(GREENLED, LOW);
      digitalWrite(GREENLED1, LOW);
      vTaskDelay(pdMS_TO_TICKS(200));
      digitalWrite(GREENLED, HIGH);
      digitalWrite(GREENLED1, HIGH);
      digitalWrite(Buzzer, LOW);
      vTaskDelay(pdMS_TO_TICKS(4000));
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

  pinMode(GREENLED, OUTPUT);
  pinMode(GREENLED1, OUTPUT);
  pinMode(Relay, OUTPUT);
  pinMode(Buzzer, OUTPUT);

  while (true)
  {
    if (CloseDoors)
    {
      Serial.println("Inside Close Door");
      CloseDoors = false;
      digitalWrite(GREENLED, HIGH);
      digitalWrite(GREENLED1, HIGH);
      digitalWrite(Relay, HIGH);
      digitalWrite(Buzzer, HIGH);
      vTaskDelay(pdMS_TO_TICKS(300));
      digitalWrite(GREENLED, LOW);
      digitalWrite(GREENLED1, LOW);
      digitalWrite(Buzzer, LOW);
      vTaskDelay(pdMS_TO_TICKS(4000));
      digitalWrite(Relay, LOW);
    }
    else
    {
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }
}

void wifiStatusData(void *pvParameters) {
    (void) pvParameters;
    while (true) {
        if (WiFi.status() == WL_CONNECTED) {
            if (client.connect("google.com", 80)) {
                WifiStatusConnected();
                client.stop(); // Close the connection
            } else {
                ws1.textAll("Internet Not Available");
            }
        } else {
            WifiStatusNotConnected();
        }

        vTaskDelay(pdMS_TO_TICKS(10000)); // Delay for 10 seconds
    }
}

void initializeCoreWork()
{
  xTaskCreate(
      wifiStatusData,  // Task function
      "OfflineData",  // Task name
      8192,           // Stack size (bytes)
      NULL,           // Task input parameter
      5,              // Priority
      &WifiStatusDataC // Task handle
  );
  // updateEmployeeDetails
  xTaskCreate(
      updateEmployeeDetails,  // Task function
      "OfflineData",  // Task name
      8192,           // Stack size (bytes)
      NULL,           // Task input parameter
      2,              // Priority
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
}

// WebSocket event Handle
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
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

// wifi Status
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

void rfidInitialList()
{
  rtc_wdt_protect_off();
  String rfidList = "";
  Serial.print("Rfid Initial Lists ");
  if (WiFi.status() == WL_CONNECTED)
  {
    if (client.connect(servername, 80))
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

// restart the esp32
void RestartEsp()
{
  ESP.restart();
}

void DeviceidFetch()
{
  HTTPClient http;
  int httpCode = 0;
  if (WiFi.status() == WL_CONNECTED)
  {
    if (client.connect("www.google.com", 80))
    {
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
    Serial.println("Response sent");
  });

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
server.on("/quickSetting.js", HTTP_GET, [](AsyncWebServerRequest *request)
{
    Serial.println("quick setup called");
    if (SPIFFS.exists("/quickSetting.js")) {
        request->send(SPIFFS, "/quickSetting.js", "application/javascript");
    } else {
        request->send(404, "text/plain", "File not found");
    }
});

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
  server.on("/RFID", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              if(RfidRegisterPage){
                RfidRegister = true;
                rfidInitialCheck();
                request->send(SPIFFS, "/RFID.html", "text/html");}
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
  server.on("/EmployeeLists", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    File csv_File = SD.open("/Rfid.csv", "r");
    if (csv_File) {
      Serial.print("Opend csv File");
      request->send(SD, "/Rfid.csv", "text/csv");
      csv_File.close();
    }
    else {
      Serial.print("File not Found");
      request->send(404, "text/plain", "File Not Found");
    } });
  server.on("/EmployeeRfidLists", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    File csv_File = SD.open("/EmpRfid.csv", "r");
    if (csv_File) {
      Serial.print("Opend csv File");
      request->send(SD, "/EmpRfid.csv", "text/csv");
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
              SD.remove("/EmpRfid.csv");
              SD.remove("/Rfid.csv");
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
{
    HTTPClient http;
    int httpCode = 0;
    RfidRegister = false;
    Serial.println("button Pressed");
    Serial.print("RFID reg ");
    Serial.println(RfidRegister);
    String jsonString;
    DynamicJsonBuffer jsonBuffer;
    JsonArray& jsonArray = jsonBuffer.parseArray(data);
    jsonArray.printTo(Serial);
    JsonArray& jsonArrays = jsonBuffer.createArray();  // Replace jsonArrays with the variable/data you want to print
    int count = 0;
    if (jsonArray.success()) {
        for (JsonObject& json : jsonArray) {
            JsonObject &JSONencoder = jsonBuffer.createObject();
            String employeeId = json["employeeId"];
            String rfid = json["rfid"];
            Serial.println(employeeId);
            JSONencoder["rfid"] = rfid;
            JSONencoder["employeeId"] = employeeId;
            Serial.println(rfid);
            jsonArrays.add(JSONencoder);
        }
    } else {
        Serial.println("JSON parsing failed");
    }
    Serial.print("Json Array");
    jsonArrays.printTo(Serial);
    char JSONmessageBuffer[500];
    String rfidArray;
    if (client.connect("www.google.com", 80)) {
        jsonArrays.printTo(rfidArray);
        JsonObject &JSONencoder = jsonBuffer.createObject();
        JSONencoder["companyId"] = CompanyId;
        JSONencoder["employeeId"] = "001"; // For admin Registering
        JSONencoder["registrationList"] = rfidArray;
        JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
        Serial.print("Encoded message");
        Serial.println(JSONmessageBuffer);
        yield();
        http.begin("https://wildfly.tictoks.in:443/EmployeeAttendenceAPI/RFIDAPI/RegisterEmployeeRFIDInfo");
        http.addHeader("Content-Type", "application/json");
        esp_task_wdt_reset();
        httpCode = http.POST(JSONmessageBuffer);
        String payload = http.getString();
        esp_task_wdt_reset();
        Serial.println(payload);
        DynamicJsonBuffer jsonBuffers;
        // Parsing the received payload
        Serial.print("Pay load size:");
        Serial.println(payload.length());
        request->send(706, "application/json", payload);
        return;
        JsonObject& jsonObject = jsonBuffers.parseObject(payload);
        JsonArray& employeeInfoList = jsonObject["employeeInfoList"];
        if (!employeeInfoList.success()) {
            Serial.println("employeeInfoList parsing failed");
            request->send(400, "text/plain", "employeeInfoList parsing failed");
            return;
        }
        else {
            if (employeeInfoList.size() > 0) {
                JsonArray& rfidArray = jsonBuffers.createArray();
                for (int i = 0; i < employeeInfoList.size(); i++) {
                    JsonObject &rfidObject = jsonBuffers.createObject();
                    rfidObject["rfid"] = employeeInfoList[i]["rfidNo"].as<const char *>();
                    rfidObject["employeeId"] = employeeInfoList[i]["employeeId"].as<const char *>();
                    rfidArray.add(rfidObject);
                }
                char JSONmessageBuffers[500];
                rfidArray.printTo(JSONmessageBuffers, sizeof(JSONmessageBuffers));
                request->send(706, "application/json", JSONmessageBuffers);
                return; // Exit after sending the special response
            }
            Serial.println("Employeeinfo list came to backend");
        }
        Serial.print("httpCode");
        Serial.println(httpCode);
        http.end();
        request->send(200, "text/plain", "Rfid Updated");
    }
});
  server.begin();
}

// Function for writting data in SD in offline data
void OfflineDataWrite(String empId)
{
  mountinSD();
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

// Sending message to Rfid Page
void sendMessageToWsClient(String rfid)
{
  String message = "{\"rfid\":\"" + rfid + "\"}";
  ws.textAll(message);
}

//  Function For Sending data to Backend
void ServerSend(String RfId, String companyId)
{
  Serial.print("RFID ");
  Serial.println(RfId);
  Serial.print("COMPANY ID ");
  Serial.println(companyId);
  DynamicJsonBuffer JSONbuffer(300);
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
  JSONencoder["employeeId"] = "-";
  JSONencoder["rfid"] = RfId;
  JSONencoder["deviceId"] = DeviceId;
  JSONencoder["companyId"] = companyId;
  JSONencoder["deviceType"] = "RFID";
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
      OpenDoors = true;
      Fetching = true;
    }
    else if ((strcmp(retStatus, "CHECKOUT") == 0))
    {
      // OpenDoor();
      OpenDoors = true;
      Fetching = true;
    }
    else if (strcmp(retStatus, "SAME_TIME") == 0)
    {
      CloseDoors = true;
      Fetching = true;
    }
    else if ((strcmp(retStatus, "BLOCKED") == 0))
    {
      UnauthorizedAccess();
      CloseDoors = true;
      digitalWrite(REDLED, HIGH); // turn the LED off.
      digitalWrite(REDLED1, HIGH);
      Fetching = true;
    }
    else if ((strcmp(retStatus, "NOT_VAILD") == 0))
    {
      UnauthorizedAccess();
      digitalWrite(REDLED, HIGH); // turn the LED off.
      digitalWrite(REDLED1, HIGH);
      CloseDoors = true;
      Fetching = true;
    }
    else if ((strcmp(retStatus, "Employee_Not_Assigned_To_The_Device") == 0))
    {
      UnauthorizedAccess();
      CloseDoors = true;
      digitalWrite(REDLED, HIGH); // turn the LED off.
      digitalWrite(REDLED1, HIGH);
      Fetching = true;
    }
    else if ((strcmp(retStatus, "RFID_NO_Is_Not_Mapped_To_Any_Employee") == 0))
    {
      UnauthorizedAccess();
      CloseDoors = true;
      digitalWrite(REDLED, HIGH); // turn the LED off.
      digitalWrite(REDLED1, HIGH);
      Fetching = true;
    }

    else
    {
      Serial.print("Wrong method followed");
      Fetching = true;
    }
    Serial.println(httpCode); // Print HTTP return code
  }
  // NOT_VAILD
  else
  {
    Serial.println("could not send back to server ");
    matchRfid(RfId, 1);
    Fetching = true;
    // OfflineDataWrite(RfId, companyId);
  }
  http.end(); // Close connection
  // Serial.println("Succesfully Send Data To BackEnd");
  i = 0;
  digitalWrite(GREENLED, LOW);
  digitalWrite(GREENLED1, LOW);
  digitalWrite(REDLED, LOW);
  digitalWrite(REDLED1, LOW);
  // added green led
  digitalWrite(GREENLED, HIGH); // turn the LED off.
  digitalWrite(GREENLED1, HIGH);
  delay(1000);
}

// Function for checking rfid with employee rfid and getting employee's info
void fileReadAndWrite()
{
  File data = SD.open("/EmpRfid.csv", "w");
  data.close();
  File datafile = SD.open("/Rfid.csv", FILE_READ);
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
    if (commaIndex == -1)
    {
      continue;
    }
    String Rfid = line.substring(0, commaIndex);
    String DeviceIds = line.substring(commaIndex + 1, secondCommaIndex);
    String Empid = line.substring(secondCommaIndex + 1, thirdCommaIndex);
    // Serial.println("Empid : "+Empid);
    // Serial.println("Rfid and Deviceids "+Rfid+" "+DeviceIds);
    // Process the first two fields (e.g., print them)
    if (Rfid != "-" && DeviceIds.toInt() > 0)
    {
      File data = SD.open("/EmpRfid.csv", "a");
      String empdata = Rfid + String(",") + Empid + String('\n');
      data.print(empdata);
      Serial.print("Field 1: ");
      Serial.print(Rfid);
      Serial.print(" Field 2: ");
      Serial.println(Empid);
      data.close();
    }
    else
    {
      continue;
    }
  }
  datafile.close();
}

bool matchRfid(String Rfid, int nemo)
{
  bool rfidFound = false;
  Fetching = false;
  String Empid;
  if (client.connect(servername, 80) && nemo != 1)
  {
    ServerSend(Rfid, CompanyId);
    return true;
  }
  else
  {
    Serial.println("inside else part Match Rfid");
    File datafile = SD.open("/EmpRfid.csv", "r");
    while (datafile.available())
    {
      String line = datafile.readStringUntil('\n');
      Serial.println(line);
      int commaIndex = line.indexOf(',');
      int secondCommaIndex = line.indexOf(',', commaIndex + 1);
      int thirdCommaIndex = line.indexOf(',', secondCommaIndex + 1);
      if (commaIndex == -1)
      {
        continue;
      }
      String rfid = line.substring(0, commaIndex);
      // deviceId = line.substring(commaIndex + 1, secondCommaIndex);
      Empid = line.substring(commaIndex + 1, secondCommaIndex); // line.substring(secondCommaIndex + 1, thirdCommaIndex);
      Serial.println("Empid check" + Empid);
      bool Matched = true;
      if (rfid == Rfid && Matched)
      {
        rfidFound = true;
        break;
      }
    }
    if (rfidFound)
    {
      Serial.print("RFID: ");
      Serial.println(Rfid);
      Serial.print("EmployeeID: ");
      Serial.println(Empid);
      Serial.println("OFFLINE Preparation");
      Serial.println("DOOR OPEN");
      OpenDoors = true;
      OfflineDataWrite(Empid);
    }
    else
    {
      Serial.println("RFID not found in CSV");
      digitalWrite(REDLED, HIGH);
      digitalWrite(REDLED1, HIGH);
      UnauthorizedAccess();
      vTaskDelay(pdMS_TO_TICKS(300));
      digitalWrite(REDLED, LOW);
      digitalWrite(REDLED1, LOW);
    }
  }
  vTaskDelay(pdMS_TO_TICKS(2000));

  Fetching = true;
  return rfidFound;
}

void softAp()
{
  WiFi.softAP(ASSID, APASS);
  Serial.println("AP Started");
  Serial.print("IP Address:");
  Serial.println(WiFi.softAPIP());
}

// spiffs file check
void SdFileCheck()
{
  mountinSD();
  if (SD.exists("/EmpRfid.csv"))
  {
    File DataFile = SD.open("/EmpRfid.csv");
    size_t fileSize = DataFile.size();
    Serial.print("Rfid File Exists");
    Serial.println(fileSize);
    if (fileSize == 0)
    {
      DataFile.close();
      SD.remove("/EmpRfid.csv");
      Serial.println("File Removed");
    }
    else
    {
      Serial.print("File Already There");
      DataFile.close();
    }
  }
  else
  {
    Serial.println("File does not exist!");
    Serial.println("Fetching Started");
  }
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

void RfidScanReader()
{
  ws.cleanupClients();
  if (Fetching)
  {
    // Reset the Watchdog Timer
    esp_task_wdt_reset();
    for (uint8_t reader = 0; reader < NO_OF_READERS; reader++)
    {
      String content = "";
      if (mfrc522[reader].PICC_IsNewCardPresent() && mfrc522[reader].PICC_ReadCardSerial())
      {
        Serial.print(F("RFID "));
        Serial.print(reader);
        Serial.println();
        Serial.print(F("Card UID:"));
        for (byte i = 0; i < mfrc522[reader].uid.size; i++)
        {
          Serial.print(mfrc522[reader].uid.uidByte[i] < 0x10 ? "0" : " ");
          Serial.print(mfrc522[reader].uid.uidByte[i], HEX);
          content.concat(String(mfrc522[reader].uid.uidByte[i] < 0x10 ? "0" : " "));
          content.concat(String(mfrc522[reader].uid.uidByte[i], HEX));
        }
        Serial.println();
        content.toUpperCase();
        Serial.println("String:");
        Serial.println(content.substring(1));
        mfrc522[reader].PICC_HaltA();
        mfrc522[reader].PCD_StopCrypto1();
        mfrc522[reader].PCD_SoftPowerDown();
        Serial.print("RFID true ");
        Serial.println(RfidRegister);
        if (RfidRegister == true)
        {

          sendMessageToWsClient(content.substring(1));
        }
        else
        {
          if (content.length() > 0)
          {
            matchRfid(content.substring(1), 0);
          }
          else
          {
            Serial.print("Invalid Card");
          }
        }
      }
    }
  }
}

void WebSocketRegister()
{
  ws.onEvent(onWsEvent);
  ws1.onEvent(wifiStatusEvent);
  server.addHandler(&ws);
  server.addHandler(&ws1);
}

void digitalRegister()
{
  digitalWrite(GREENLED1, HIGH);
  digitalWrite(GREENLED, HIGH);
  digitalWrite(REDLED1, LOW);
  digitalWrite(REDLED, LOW);
}

void initialSetupFun()
{
  Serial.begin(115200);
  EEPROM.read(512);
  // SerialBT.begin("Tictoks Facial");
  printFreeHeap("initialiazation");
  initialavailableWifi();
  mountinSpiffs();
  printFreeHeap("After Core Work");
  WiFi.mode(WIFI_AP_STA);
  softAp();
  initializePinMode();
  WifiConnectCheck();
  CompanyIdCheck();
  InitializeRfid();
  mountinSD();
  DeviceIdInitialize();
  rfidInitialCheck();
  SdFileCheck();
  fileReadAndWrite();
  InitializeRTC();
  WebSocketRegister();
  WebServerRoutes();
  delay(500);
  initializeCoreWork();
  printFreeHeap("After initiliazation");
  Serial.print("Build Version:");
  Serial.println(Build_Version);

}

void MDNSServer(){
  if (!MDNS.begin(hostName))
  {
    Serial.println("Error setting up MDNS responder!");
    esp_restart();
  }
  Serial.println("mDNS responder started");
}

// *Setup
void setup()
{
  initialSetupFun();
  MDNSServer();
  // UpdateActivity();
}

void loop()
{
  // BluetoothProcesses();
  RfidScanReader();
}