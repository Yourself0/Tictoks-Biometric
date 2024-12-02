// Library FOr Wifi
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

//Library For SD CARD CSV
#include <SdFat.h>
#include <CSVFile.h>

//Library for LCD
#include <LiquidCrystal_I2C.h>

//For Biometric
#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>

//For RTC DS3231
#include <RTClib.h>
#include <Wire.h>

//For Comparing Strings in C
#include <stdio.h>
#include <string.h>

//Timer for Automatic data Sync
#include <timer.h>

//Html Pages
//#include "WifiPage.h"
#include "RegisterPage.h"
#include "CompanyPage.h"
#include "CommonPage.h"
#include "WifiSettingPage.h"
auto timer = timer_create_default(); // create a timer with default settings

SoftwareSerial mySerial(2, 15);

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

//For Wifi Connection
const char *ssid = "Tictoks_V1";
const char *passphrase = "12345678";
String st;
String content;
int statusCode;
String esid;
String epass = "";
String fingerprintstatus = "";

ESP8266WebServer server(80);
WiFiClient client;
char servername[] = "google.com";

//For Sending Data via Wifi

const char *host = "192.168.43.128"; //https://circuits4you.com website or IP address of server

//For SD card and CSV
// SPI pinout

#define PIN_SPI_CLK 14
#define PIN_SPI_MOSI 13
#define PIN_SPI_MISO 12
#define PIN_SD_CS 15

#define PIN_KEY_PAD 10

#define PIN_OTHER_DEVICE_CS -1
#define SD_CARD_SPEED SPI_QUARTER_SPEED

#define FILENAME "TictoksV10.csv"
#define EMPDETAILS "EmpDetails.csv"

SdFat sd;
CSVFile csv;

//uint8_t id;
int id;

int i = 0;
int firstvariable;
String companyId = "0";
String companyName;

//Initialize LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

//RTC Variable
RTC_DS3231 rtc;

#define Lock 16 //D0 pin 

void setup()
{
  pinMode(Lock, OUTPUT);
  Serial.begin(9600);
  //For Wifi to get Data from EEPROM
  EEPROM.begin(512);
  //Initializing RTC
  InitializeRTC();
  lcd.begin();     // initializing the LCD
  lcd.backlight(); // Enable or Turn On the backlight
  String cmpid;
  String cmpName;
  for (int i = 96; i < 100; ++i)
  {
    if (EEPROM.read(i) != 255)
    {
      cmpid += char(EEPROM.read(i));
      Serial.println("Reading");
    }
    Serial.println(EEPROM.read(i));
  }
  Serial.println("CompanyId");
  Serial.println(cmpid);
  Serial.println(cmpid.length());
  if (cmpid.length() == 0)
  {
    // Getting CompanyId and company Name from Web
    InitializeCompany();
    Serial.println(companyId);
  }
  else
  {
    for (int i = 100; i < 164; ++i)
    {
      if (EEPROM.read(i) != 0)
      {
        cmpName += char(EEPROM.read(i));
      }
    }
    companyName = cmpName;
    companyId = cmpid;
    Serial.println("Company Name" + companyName);
    Serial.println("Company Id" + companyId);
    welcomeNote();
    CompanyWebServer(1);
    //Initializing Wifi Function
    InitialWiFi();
    //Initialize Fingerprint
    // InitialFingerPrint();
    //Initializing SD card
    //InitialSDcard();
    //Initializing Csv File
    //InitialCSV();
    //Initializing RTC
    //InitializeRTC();
    //Call webserver for Registeration
    RegisterFinger();
    //Timer Function will be invoked Every 3Mins
    timer.every(180000, SendOfflineData);
  }
}
void loop()
{
  digitalWrite(Lock, LOW);
  server.handleClient();
  //  lcd.clear();
  //  lcd.setCursor(0, 0);
  //  lcd.print("Place Registered");
  //  lcd.setCursor(0, 1);
  //  lcd.print("Finger...");
  int empId = -1;
  empId = 26;

  if (empId != -1)
  {
    Serial.println(" Id :");
    Serial.print("Finger Matched so Send Data");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("FingerPrint ");
    lcd.setCursor(0, 1);
    lcd.print("Matched..");
    //Function For Sending Data
    SendData(empId);
    digitalWrite(Lock, HIGH);
    Serial.println("Door UNLOCKED");
    delay(2000);
  }
  delay(30000);
  //Calling Timer
  timer.tick(); // tick the timer
  /*
    DateTime now = rtc.now();
    char t[32];
    sprintf(t, "%02d:%02d:%02d %02d/%02d/%02d",  now.hour(), now.minute(), now.second(), now.day(), now.month(), now.year());
    Serial.print(F("Date/Time: "));
    //Serial.print(String(now));
    Serial.println(t);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(t);
    delay(1000);
  */
}
//Function For Initializing Company
void InitializeCompany()
{
  Serial.println("Initializing Company");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Initializing ");
  lcd.setCursor(0, 1);
  lcd.print("Company..");
  WiFi.softAP(ssid, passphrase, 6);
  Serial.println("softap");
  CompanyWebServer(1);
  // Start the server
  server.begin();
  Serial.println("WiFi not Connected");
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("SoftAP IP: ");
  Serial.println(WiFi.softAPIP());
  Serial.println("Server started...");
}
//Function for Getting CompanyID and Name
void CompanyWebServer(int webtype)
{
  webtype = 2;
  Serial.println("Inside CreateWebserver" + webtype);

  if (webtype == 2)
  {
    Serial.println("WebType " + webtype);
    server.on("/Company", CompanyPage);
    server.on("/Configuration", []() {
      String cmpid = server.arg("companyId");
      String cmpName = server.arg("companyName");
      String date = server.arg("date");
      char mydate[20] ;
      strcpy(mydate, date.c_str());
      char *token = strtok(mydate, ",");
      int j = 0;
      int dyear;
      int dmnth;
      int ddt;
      int dhr;
      int dmin;
      int dsec;
      // Keep printing tokens while one of the
      // delimiters present in str[].
      while (token != NULL)
      {
        switch (j) {
          case 0:
            dyear = atoi(token);
            break;
          case 1:
            dmnth = atoi(token);
            break;
          case 2:
            ddt = atoi(token);
            break;
          case 3:
            dhr = atoi(token);
            break;
          case 4:
            dmin = atoi(token);
            break;
          case 5:
            dsec = atoi(token);
            break;
        }
        j++;
        token = strtok(NULL, ",");
      }
      //  rtc.adjust(DateTime(mydate));
      Wire.begin();
      rtc.begin();
      rtc.adjust(DateTime(dyear, dmnth, ddt, dhr, dmin, dsec));
      if (rtc.begin()) {
        Serial.println("RTC Runing");
      } else {
        Serial.println("RTC not Runing");
      }
      if (cmpid.length() > 0 && cmpName.length() > 0)
      {
        Serial.println("clearing eeprom");
        for (int i = 0; i < 164; ++i)
        {
          EEPROM.write(i, 0);
        }
        Serial.println(cmpid);
        Serial.println("");
        Serial.println(cmpName);
        Serial.println("");
        Serial.println("writing eeprom Company Id: ");
        for (int i = 0; i < cmpid.length(); ++i)
        {
          EEPROM.write(96 + i, cmpid[i]);
          Serial.print("Wrote: ");
          Serial.println(cmpid[i]);
        }
        companyId = cmpid;
        Serial.println("writing eeprom CompanyName: ");
        for (int i = 0; i < cmpName.length(); ++i)
        {
          EEPROM.write(100 + i, cmpName[i]);
          Serial.print("Wrote: ");
          Serial.println(cmpName[i]);
        }
        companyName = cmpName;
        EEPROM.commit();
        lcd.clear();
        lcd.print("Saved Company");
        lcd.setCursor(0, 2);
        lcd.print("Details");
        content = "<body>";
        content += "<h2 align='center'> SuccessFully Saved Company Details !!!!</h2>";
        content += "<h3 align='center'><a href='Wifi'>Configure Wifi</a></h3>";
        content += "</body>";
        statusCode = 200;
        String s = Common_page;
        server.send(statusCode, "text/html", s + "<br/>" + content);
        delay(1000);
        //Initializing Wifi Function
        InitialWiFi();
        //Initialize Fingerprint
        //InitialFingerPrint();
        //Initializing SD card
        //InitialSDcard();
        //Initializing Csv File
        // InitialCSV();
        //Starting Register finger Server
        RegisterFinger();
        //Timer Function will be invoked Every 3Mins
        timer.every(180000, SendOfflineData);
      }
      else
      {
        content = "<body>";
        content = "<h2 align='center'>Error\":\"404 not found\"<h2>";
        content += "<h3 align='center'><a href='Company'>Try Again</a></h3>";
        content += "</body>";
        statusCode = 404;
        Serial.println("Sending 404");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Error: Try ");
        lcd.setCursor(0, 1);
        lcd.print("Again");
        delay(1000);
        welcomeNote();
      }
      String s = Common_page;
      server.send(statusCode, "text/html", s + "<br/>" + content);

    });
  }
}

//Function For Initializing RTC DS3231
void InitializeRTC()
{
  Wire.begin();
  rtc.begin();
  /*if (!rtc.begin())
    {
     rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
     //rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
     Serial.println("RTC is NOT RUNNING");
    }
    else
    {
     Serial.println("RTC is RUNNING");
     Serial.println(__DATE__);
     Serial.println(__TIME__);
     //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
  */
}

//Function For Initializing Wifi
void InitialWiFi()
{

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Initializing ");
  lcd.setCursor(0, 1);
  lcd.print("WIFI....");
  Serial.println("Initialzing Wifi Connection");
  // read eeprom for ssid and pass
  Serial.println("Reading ssid");
  for (int i = 0; i < 32; ++i)
  {
    esid += char(EEPROM.read(i));
  }
  Serial.print("SSID: ");
  Serial.println(esid);
  Serial.println("Reading  password");
  for (int i = 32; i < 96; ++i)
  {
    epass += char(EEPROM.read(i));
  }
  Serial.print("PASS: ");
  Serial.println(epass);
  launchWeb(1);
  if (esid.length() > 1)
  {
    WiFi.begin(esid.c_str(), epass.c_str());
  }

  if (!testWifi())
  {
    Serial.println(" LAUNCH WEB");
    // launchWeb(0);
    setupAP();
    return;
  }
}
//Function For Testing Wifi Connection
bool testWifi(void)
{
  int c = 0;
  Serial.println("Waiting for Wifi to connect to ESP8266");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting Wifi...");
  while (c < 20)
  {

    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println(" WIFI Status Connected " + WiFi.isConnected());
      lcd.clear();
      lcd.print("Wi-Fi Connected....");
      delay(1000);
      // int status = EmployeeDetails();
      Serial.println("Status Employee Details");
      // Serial.println(status);
      welcomeNote();
      //Send Offline Data After Connecing to Wifi
      //SendOfflineData(&c);
      c = 20;
      return true;
    }
    delay(500);
    Serial.println("WIFI Status Not Connected " + WiFi.status());
    c++;
  }
  Serial.println("");
  Serial.println("Connect timed out, opening AP");
  lcd.clear();
  lcd.print("Wi-Fi Not ");
  lcd.setCursor(0, 1);
  lcd.print("Connected....");
  delay(1000);
  welcomeNote();
  return false;
}
//Function For Launching Web to reset Password
void launchWeb(int webtype)
{
  Serial.println("WiFi not Connected");
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("SoftAP IP: ");
  Serial.println(WiFi.softAPIP());
  server.begin();
  createWebServer(webtype);
  // Start the server
  server.begin();
  Serial.println("Server started");
}
//Function for Browser page launching
void createWebServer(int webtype)
{
  webtype = 1;
  Serial.println("Inside CreateWebserver" + webtype);
  if (webtype == 1)
  {
    Serial.println("WebType " + webtype);
    server.on("/Wifi", WifiPage);
    server.on("/Setting", []() {
      String qsid = server.arg("ssid");
      String qpass = server.arg("pass");
      if (qsid.length() > 0 && qpass.length() > 0)
      {
        Serial.println("clearing eeprom");
        for (int i = 0; i < 96; ++i)
        {
          EEPROM.write(i, 0);
        }
        Serial.println(qsid);
        Serial.println("");
        Serial.println(qpass);
        Serial.println("");

        Serial.println("writing eeprom ssid:");
        for (int i = 0; i < qsid.length(); ++i)
        {
          EEPROM.write(i, qsid[i]);
          Serial.print("Wrote: ");
          Serial.println(qsid[i]);
        }
        Serial.println("writing eeprom pass:");
        for (int i = 0; i < qpass.length(); ++i)
        {
          EEPROM.write(32 + i, qpass[i]);
          Serial.print("Wrote: ");
          Serial.println(qpass[i]);
        }
        EEPROM.commit();
        content = "<body>";
        content += "<h2 align='center'>Successfully Stored Wi-Fi Details</h2>";
        content += "<h3 align='center'><a href='Register'>Register Finger Print</a></h3>";
        content += "</body>";
        statusCode = 200;
        String esid;
        for (int i = 0; i < 32; ++i)
        {
          esid += char(EEPROM.read(i));
        }
        Serial.print("SSID: ");
        Serial.println(esid);
        Serial.println("Reading  password");
        String epass = "";
        for (int i = 32; i < 96; ++i)
        {
          epass += char(EEPROM.read(i));
        }
        Serial.print("PASS: ");
        Serial.println(epass);
        /*
            if (esid.length() > 1)
          {
          WiFi.begin(esid.c_str(), epass.c_str());
          // Serial.println("Wifi  " + testWifi());
          Serial.println("wifi name " + esid);
          Serial.println("wifi password " + epass);
          if (!testWifi())
          {
           //launchWeb(0);
           return;
          }
          delay(1000);
          return ;
        */
        launchWeb(1);
        if (esid.length() > 1)
        {
          WiFi.begin(esid.c_str(), epass.c_str());
          statusCode = 200;

          String s = Common_page;
          server.send(statusCode, "text/html", s + "<br/>" + content);

        }

        if (!testWifi())
        {
          Serial.println(" LAUNCH WEB");
          launchWeb(0);
          //setupAP();
          statusCode = 200;

          String s = Common_page;
          server.send(statusCode, "text/html", s + "<br/>" + content);


        }
        String s = Common_page;
        server.send(statusCode, "text/html", s + "<br/>" + content);

        return ;
      }
      else
      {
        content = "<h2>Error\":\"404 not found\"</h2>";
        statusCode = 404;
        Serial.println("Sending 404");

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Error: Try ");
        lcd.setCursor(0, 1);
        lcd.print("Again");
        statusCode = 200;
        String s = Common_page;
        server.send(statusCode, "text/html", s + "<br/>" + content);

        delay(1000);
        welcomeNote();
      }

      String s = Common_page;
      server.send(statusCode, "text/html", s + "<br/>" + content);
    });
  }
}
//Function to on NodeMCU Wifi
void setupAP(void)
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0)
    Serial.println("No networks found");
  else
  {
    Serial.print(n);
    Serial.println(" Networks found");
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
      delay(10);
    }
  }
  Serial.println("");
  st = "<ol>";
  for (int i = 0; i < n; ++i)
  {
    // Print SSID and RSSI for each network found
    st += "<li>";
    st += WiFi.SSID(i);
    st += " (";
    st += WiFi.RSSI(i);
    st += ")";
    st += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*";
    st += "</li>";
  }
  st += "</ol>";
  delay(100);
  WiFi.softAP(ssid, passphrase, 6);
  Serial.println("softap");
  server.begin();
  launchWeb(1);
  Serial.println("over");
}

//Function for Initializing SdCard
void InitialSDcard()
{
  // Initialize Sd card Pins
  pinMode(PIN_SPI_MOSI, OUTPUT);
  pinMode(PIN_SPI_MISO, INPUT);
  pinMode(PIN_SPI_CLK, OUTPUT);
  Serial.println("Initializing SD card.... ");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Initializing ");
  lcd.setCursor(0, 1);
  lcd.print("SD Card....");
  // Setup SD card
  if (sd.begin(PIN_SD_CS, SD_CARD_SPEED))
  {
    Serial.println("SD card Initialization is Success");
    lcd.clear();
    lcd.print("SD Card Success....");
    return;
  }
  else
  {

    Serial.println("SD card begin error");
    lcd.clear();
    lcd.print("SD Card Failed....");
    delay(10000);
    InitialSDcard();
  }
}
//Function For Initializing Csv File
void InitialCSV()
{
  Serial.println("Initializing Csv File");
  csv.close();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Initializing ");
  lcd.setCursor(0, 1);
  lcd.print("Csv File....");
  if (!csv.open(FILENAME, O_RDWR | O_CREAT))
  {
    Serial.println("Failed open file");
    lcd.clear();
    lcd.print(" Csv open Failed ....");
    delay(10000);
    InitialCSV();
  }
  else
  {
    lcd.clear();
    lcd.print(" Csv open Success ....");
    Serial.println("Open Success");
    welcomeNote();
  }
  /* if (csv.open(FILENAME, O_RDWR | O_CREAT)) {
     Serial.println("File open is Success");

    } else {
     Serial.println("Failed To Open File");
     delay(10000);
     InitialCSV();

    }*/
}
//Function For Sending Data to Back end
void SendData(int empId)
{
  Serial.println(WiFi.status());
  if (WiFi.status() == WL_CONNECTED)
  {
    if (client.connect(servername, 80))
    {
      Serial.println("connected");
      bool flag = true;
      if (flag)
      {
        ServerSend(empId, companyId);
      }
      else
      {
        OfflineDataWrite(empId);
        lcd.clear();
        lcd.print("Attendance Recorded");
        lcd.setCursor(0, 2);
        lcd.print("Successfully");
        delay(1000);
        delay(1000);
        welcomeNote();
      }
      return;
    }

    else
    {
      Serial.println("Wifi Is Not Connected Offline");
      // Writing Data in CSV Because WiFi is not Connected
      OfflineDataWrite(empId);
      //delay(10000);
      //Re-initialize to connect to Wifi
      // InitialWiFi();
      lcd.clear();

      lcd.print("Attendance Recorded");
      lcd.setCursor(0, 2);
      lcd.print("Successfully");
      delay(1000);
      delay(1000);
      welcomeNote();
      return;
    }
  }
  else
  {
    Serial.println("Wifi Is Not Connected Offline");
    // Writing Data in CSV Because WiFi is not Connected
    OfflineDataWrite(empId);
    //delay(10000);
    //Re-initialize to connect to Wifi
    // InitialWiFi();
    lcd.clear();
    lcd.print("Attendance Recorded");
    lcd.setCursor(0, 2);
    lcd.print("Successfully");
    delay(1000);
    delay(1000);
    welcomeNote();
    return;
  }
}

//Read CSV And check where existing data to send
bool readCSV()
{
  bool flag = true;
  int num;
  InitialSDcard();
  InitialCSV();
  //To move to beginning of file

  csv.gotoBeginOfFile();
  /*const byte BUFFER_SIZE = 50;
    char buffer[BUFFER_SIZE + 1];
    buffer[BUFFER_SIZE] = '\0';
  */
  Serial.println("GOing to Read Lines");

  do
  {
    const byte BUFFER_SIZE = 200;
    char buffer[BUFFER_SIZE + 1];
    buffer[BUFFER_SIZE] = '\0';
    Serial.println("Line Number :");
    int num = csv.getNumberOfLine();
    Serial.println(num);
    buffer[BUFFER_SIZE] = '\0';
    csv.readField(buffer, BUFFER_SIZE);
    Serial.println("Reading    :");
    // sprintf ("buffer %s", buffer);
    Serial.print(buffer);
    int len = strlen(buffer);
    Serial.println("SIze");
    Serial.println(len);
    if (len > 0)
    {
      Serial.println("File is Not Empty Contain Records");
      flag = false;
      break;
    }
    csv.nextField();
  } while (csv.nextLine());
  return flag;
}

//Function for Writing Data in CSV
void OfflineDataWrite(int empId)
{
  //Initialize and Open CSV File
  //csv.close();
  /* InitialSDcard();
    InitialCSV();
    //To move to beginning of file
    csv.gotoBeginOfFile();
    /*const byte BUFFER_SIZE = 50;
     char buffer[BUFFER_SIZE + 1];
     buffer[BUFFER_SIZE] = '\0';

    const byte BUFFER_SIZE = 200;
    char buffer[BUFFER_SIZE + 1];

    do
    {

     buffer[BUFFER_SIZE] = '\0';
     Serial.println("Line Number :");
     Serial.print(csv.getNumberOfLine());
     Serial.println(csv.isLineMarkedAsDelete() ? F("True") : F("False"));
     buffer[BUFFER_SIZE] = '\0';
     csv.readField(buffer, BUFFER_SIZE);
     Serial.println("Buffer 1     :");
     // sprintf ("buffer %s", buffer);
     Serial.print(buffer);

     Serial.print("\n");
     char *ptr = strtok(buffer, ",");
     while (ptr != NULL)
     {
       // printf("%02d", ptr);
       Serial.println(ptr);
       ptr = strtok(NULL, ",");
     }

     csv.nextField();
    } while (csv.nextLine());
    /*while (!csv.isEndOfField()) {
     const byte BUFFER_SIZE = 50;
     char buffer[BUFFER_SIZE + 1];
     buffer[BUFFER_SIZE] = '\0';
     Serial.println(csv.getNumberOfLine());
     buffer[BUFFER_SIZE] = '\0';
     csv.readField(buffer, BUFFER_SIZE);
     //sprintf ("buffer %s", buffer);
     Serial.println("Buffer     :");
     // Serial.print(buffer);

     Serial.print("\n");
     csv.nextField();
     Serial.println("writing");
     }*
    csv.nextField();
    Serial.println("Total No of Line ");
    Serial.print(csv.getNumberOfLine());
    int len = strlen(buffer);
    if (len != 0)
    {
     csv.addLine();
     Serial.println("New Line Added");
    }*/
  DateTime now = rtc.now();
  //DateTime now = rtc.now();
  char t[32];
  sprintf(t, "%02d:%02d:%02d %02d/%02d/%02d",  now.hour(), now.minute(), now.second(), now.day(), now.month(), now.year());
  Serial.print(F("Date/Time: "));
  //Serial.print(String(now));
  Serial.println(t);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(t);
  StaticJsonBuffer<300> JSONbuffer;
  JsonObject& JSONencoder = JSONbuffer.createObject();

  JSONencoder["employeeId"] = "027";
  JSONencoder["companyId"] = "001";
  //JSONencoder["date"] = Date;
  //JSONencoder["time"] = Time;
  JSONencoder["date"] = String(now.year()) + String("-") + String(now.month(), DEC) + String("-") + String(now.day());
  JSONencoder["time"] = String(now.hour()) + String(":") + String(now.minute()) + String(":") + String(now.second());
  char JSONmessageBuffer[300];
  JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));

  Serial.println(JSONmessageBuffer);
  String senddata = "";
  senddata += F(String("001,"));                                                                                          //companyId
  senddata += empId;                                                                                                      //employeeid
  senddata += String(",") + String(now.year()) + String("-") + String(now.month()) + String("-") + String(now.day());     // date
  senddata += String(",") + String(now.hour()) + String(":") + String(now.minute()) + String(":") + String(now.second()); // time
  Serial.println("String  :");
  Serial.print(F(senddata.c_str()));
  //Serial.println("String  1 :");
  //Serial.print(F(senddata));
  /*csv.addField((unsigned int)empId);
    csv.addField(F(";001;"));
    csv.addField(now.year());
    csv.addField("-");
    csv.addField(now.month());
    csv.addField("-");
    csv.addField(now.day());
    csv.addField(",");
    csv.addField(now.hour());
    csv.addField(":");
    csv.addField(now.minute());
    csv.addField(":");
    csv.addField(now.second());

    csv.addField(F(senddata.c_str()));
    csv.addLine();
    csv.addField("");
    Serial.println("Finish writing");
    csv.close();*/
}

//Function For Sending data to Backend
void ServerSend(int empId, String companyId)
{
  DateTime now = rtc.now();
  //DateTime now = rtc.now();
  char t[32];
  sprintf(t, "%02d:%02d:%02d %02d/%02d/%02d",  now.hour(), now.minute(), now.second(), now.day(), now.month(), now.year());
  Serial.print(F("Date/Time: "));
  //Serial.print(String(now));
  Serial.println(t);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(t);

  Serial.println("Sending data TO Backend");
  HTTPClient http; //Declare object of class HTTPClient
  StaticJsonBuffer<300> JSONbuffer;
  JsonObject& JSONencoder = JSONbuffer.createObject();
  JSONencoder["employeeId"] = empId;
  JSONencoder["companyId"] = companyId;
  JSONencoder["date"] = String(now.year()) + String("-") + String(now.month(), DEC) + String("-") + String(now.day());
  JSONencoder["time"] = String(now.hour()) + String(":") + String(now.minute()) + String(":") + String(now.second());
  //lcd.clear();
  //lcd.print(String(now.hour()) + String(":") + String(now.minute()) + String(":") + String(now.second()));
  char JSONmessageBuffer[300];
  JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));

  Serial.println(JSONmessageBuffer);
  http.begin("http://13.126.195.214:8080/EmployeeAttendenceAPI/employee/EmployeeCheckInOut"); //Specify request destination
  http.addHeader("Content-Type", "application/json");                                         //Specify content-type header

  int httpCode = http.POST(JSONmessageBuffer); //Send the request
  /*  int httpCode = http.GET(JSONmessageBuffer);*/
  if (httpCode > 0)
  {
    const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(2) + 600;
    DynamicJsonBuffer jsonBuffer(capacity);
    // Parse JSON object
    JsonObject &root = jsonBuffer.parseObject(http.getString());
    const char *code = root["employeeId"];
    //const char*  department = root["department"];
    const char *retStatus = root["status"];
    const char *printStatus = "Checked In";
    const char *userName = root["employeeName"];
    Serial.println("resturn" + http.getString());
    Serial.print("Code return element = ");
    Serial.println("EmployeeId ");
    Serial.println(code);
    Serial.println("Employee name ");
    Serial.println(userName);
    Serial.println("Status name ");
    Serial.println(retStatus);
    if ((strcmp(retStatus, "CHECKIN") == 0))
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print( String("Welcome ") );
      lcd.setCursor(0, 1);
      lcd.print(String(userName) + String("!!"));
    } else if (strcmp(retStatus, "SAME_TIME") == 0) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print( String("Now Only Attenda ") );
      lcd.setCursor(0, 1);
      lcd.print( String("nce Recorded ") );

    } else if ((strcmp(retStatus, "BLOCKED") == 0)) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print( String("Your Id is ") );
      lcd.setCursor(0, 1);
      lcd.print(String("Blocked !!"));
    } else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(String("Thank You ") );
      lcd.setCursor(0, 1);
      lcd.print(String(userName) + String("!!"));
    }
    Serial.println(httpCode); //Print HTTP return code
  }
  http.end(); //Close connection
  Serial.println(httpCode);
  Serial.println("SuucessFully Send Data To BackEnd");
  i = 0;
  firstvariable = 0;
  delay(1000);
  delay(1000);
  welcomeNote();
}

void InitialFingerPrint()
{
  Serial.println("Initialize Finger Print");
  finger.begin(57600);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Initializing Finger ");
  lcd.setCursor(0, 1);
  lcd.print("Print....");
  if (finger.verifyPassword())
  {
    Serial.println("Found fingerprint sensor!");
    return;
  }
  else
  {
    Serial.println("Did not find fingerprint sensor :(");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Fingerprint Sensor");
    lcd.setCursor(0, 1);
    lcd.print("Not Found....");
    delay(10000);
    InitialFingerPrint();
  }
}
//FingerPrint Module Function
uint8_t getFingerprintEnroll()
{
  int p = -1;
  Serial.print("Waiting for valid finger to enroll as #");
  Serial.println(id);
  int countMatch = 0;
  do {
    //while (p != FINGERPRINT_OK){
    p = finger.getImage();
    Serial.print("p Status");
    Serial.print(String(p));
    countMatch = countMatch + 1;
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
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Unable to");
        lcd.setCursor(0, 1);
        lcd.print("Register");
        break;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error");
        countMatch = 100;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Unable to");
        lcd.setCursor(0, 1);
        lcd.print("Register");
        break;
      default:
        Serial.println("Unknown error");
        break;
    }
  }  while (countMatch < 100);

  // OK success!
  if (p == FINGERPRINT_OK) {
    p = finger.image2Tz(1);
    Serial.print("p Status 2nd ");
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
        Serial.println("Could not find fingerprint features");
        return p;
      case FINGERPRINT_INVALIDIMAGE:
        Serial.println("Could not find fingerprint features");
        return p;
      default:
        Serial.println("Unknown error");
        return p;
    }
    Serial.println("Remove finger");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Remove Finger ");
    lcd.setCursor(0, 1);
    lcd.print("and Place Again");
    delay(1000);
    p = 0;
    while (p != FINGERPRINT_NOFINGER)
    {
      p = finger.getImage();
    }
    Serial.print("ID ");
    Serial.println(id);
    p = -1;
    Serial.println("Place same finger again");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Place Finger");
    lcd.setCursor(0, 1);
    lcd.print("Again");
    countMatch = 0;
    do {
      //while (p != FINGERPRINT_OK){
      p = finger.getImage();
      countMatch = countMatch + 1;
      switch (p)
      {
        case FINGERPRINT_OK:
          Serial.println("Image taken");
          countMatch = 100;
          //          lcd.clear();
          //          lcd.setCursor(0, 0);
          //          lcd.print("Unable to");
          //          lcd.setCursor(0, 1);
          //          lcd.print("Register");
          break;
        case FINGERPRINT_NOFINGER:
          Serial.print(".");
          break;
        case FINGERPRINT_PACKETRECIEVEERR:
          Serial.println("Communication error");
          countMatch = 100;
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Unable to");
          lcd.setCursor(0, 1);
          lcd.print("Register");
          break;
        case FINGERPRINT_IMAGEFAIL:
          Serial.println("Imaging error");
          countMatch = 100;
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Unable to");
          lcd.setCursor(0, 1);
          lcd.print("Register");
          break;
        default:
          Serial.println("Unknown error");
          break;
      }
    } while (countMatch < 100);

    // OK success!
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
          Serial.println("Could not find fingerprint features");
          return p;
        case FINGERPRINT_INVALIDIMAGE:
          Serial.println("Could not find fingerprint features");
          return p;
        default:
          Serial.println("Unknown error");
          return p;
      }mySerial
      // OK converted!
      Serial.print("Creating model for #");
      Serial.println(id);

      p = finger.createModel();
      if (p == FINGERPRINT_OK)
      {
        Serial.println("Prints matched!");
        fingerprintstatus = "Match";
      }
      else if (p == FINGERPRINT_PACKETRECIEVEERR)
      {
        Serial.println("Communication error");
        return p;
      }
      else if (p == FINGERPRINT_ENROLLMISMATCH)
      {
        Serial.println("Fingerprints did not match");
        fingerprintstatus = "NotMatch";
        countMatch = 100;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Fingerprint Does ");
        lcd.setCursor(0, 1);
        lcd.print("not Match");
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

      p = finger.loadModel(id);
      switch (p)
      {
        case FINGERPRINT_OK:
          Serial.print("Template AFEER ");
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
        if (mySerial.available())
        {
          bytesReceived[i++] = mySerial.read();mySerial
        }
      }
      Serial.print(i);
      Serial.println(" bytes read.");
      Serial.println("Decoding packet...");

      uint8_t fingerTemplate[512]; // the real template
      memset(fingerTemplate, 0xff, 512);

      // filtering only the data packets
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
      }
      for (int i = 0; i < 512; ++i)
      {
        //Serial.print("0x");
        printHex(fingerTemplate[i], 2);
        //Serial.print(", ");
      }
      Serial.println("\ndone.");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Successfully ");
      lcd.setCursor(0, 1);
      lcd.print("Stored..");
      delay(1000);
    } else {
      Serial.println("Place FingerPrint  Correctly because finger prints does not matched");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Fingerprint Does ");
      lcd.setCursor(0, 1);
      lcd.print("not Matched");
    }
  } else {
    Serial.println("Place FingerPrint  Correctly");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Place FingerPrint ");
    lcd.setCursor(0, 1);
    lcd.print("Correctly. TimeOut");
  }
  delay(1000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Welcome");
  lcd.setCursor(0, 1);
  lcd.print(companyName);
  //Function for sending data o backend
  //SendData();
}
void printHex(int num, int precision)
{
  char tmp[16];
  char format[128];

  sprintf(format, "%%.%dX", precision);

  sprintf(tmp, format, num);
  Serial.print(tmp);
}
//Function for Matching FingerPrint
// returns -1 if failed, otherwise returns ID #
int getFingerprintIDez()
{
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)
    return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)
    return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)
    return -1;

  // found a match!
  Serial.print("Found ID #");
  Serial.print(finger.fingerID);
  Serial.print(" with confidence of ");
  Serial.println(finger.confidence);
  return finger.fingerID;
}
//Function For Reading Offline Data to be Send to Backendda
bool SendOfflineData(void *)
{
  if (WiFi.status() == WL_CONNECTED)
  {
    if (client.connect(servername, 80))
    {

      Serial.println("Sending Data online");
      /*InitialSDcard();
        InitialCSV();
        //To move to beginning of file
        csv.gotoBeginOfFile();
        const byte BUFFER_SIZE = 200;
        char buffer[BUFFER_SIZE + 1];
        buffer[BUFFER_SIZE] = '\0';
        do
        {

        Serial.println("Line Number :");
        Serial.print(csv.getNumberOfLine());
        buffer[BUFFER_SIZE] = '\0';
        csv.readField(buffer, BUFFER_SIZE);
        Serial.println("Buffer 1     :");
        // sprintf ("buffer %s", buffer);
        Serial.print(buffer);
        int len = strlen(buffer);
        if (len != 0)
        {
          Serial.print("\n");
          char *ptr = strtok(buffer, ",");
          Serial.println(ptr);
          int j = 0;
          String CmpId;
          String EmpId;
          String Date;
          String Time;
          while (ptr != NULL)
          {
            // printf("%02d", ptr);
            if (j == 0)
            {
              CmpId = ptr;
            }
            else if (j == 1)
            {
              EmpId = ptr;
            }
            else if (j == 2)
            {
              Date = ptr;
            }
            else if (j == 3)
            {
              Time = ptr;
            }
            j++;
            Serial.println(ptr);
            ptr = strtok(NULL, ",");
          }
          Serial.println("CompanyId : " + String(CmpId) + " EmployeeID : " + String(EmpId) + " Date :" + String(Date) + " Time :" + String(Time));
          int result = OfflineSend(CmpId, EmpId, Date, Time);
          if (result == 200)
          {
            Serial.println("Data Send SuccessFully");
            csv.gotoBeginOfField();
            csv.markLineAsDelete();
            Serial.println("Deleting Current Record");
          }
        }
        csv.nextField();
        } while (csv.nextLine());
        welcomeNote();
      */
    } else
    {
      Serial.println("No Internet");
      ReconnectWiFi();

    }
  } else {
    Serial.println("Offline");
    ReconnectWiFi();

  }

  return true; // repeat? true
}

//Function for Sending Offline Data to Back end
int OfflineSend(String CmpId, String EmpId, String Date, String Time)
{
  lcd.clear();
  Serial.println("Sending Offline data TO Backend");
  HTTPClient http; //Declare object of class HTTPClient
  StaticJsonBuffer<300> JSONbuffer;
  JsonObject& JSONencoder = JSONbuffer.createObject();

  JSONencoder["employeeId"] = EmpId;
  JSONencoder["companyId"] = CmpId;
  JSONencoder["date"] = Date;
  JSONencoder["time"] = Time;

  char JSONmessageBuffer[500];
  JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  Serial.println(JSONmessageBuffer);

  http.begin("http://13.126.195.214:8080/EmployeeAttendenceAPI/employee/EmployeeCheckInOut"); //Specify request destination
  http.addHeader("Content-Type", "application/json");                                         //Specify content-type header

  int httpCode = http.POST(JSONmessageBuffer); //Send the request
  /*  int httpCode = http.GET(JSONmessageBuffer);*/
  if (httpCode > 0)
  {
    const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(2) + 600;
    DynamicJsonBuffer jsonBuffer(capacity);
    // Parse JSON object
    JsonObject &root = jsonBuffer.parseObject(http.getString());
    const char *code = root["employeeId"];
    //const char*  department = root["department"];
    const char *retStatus = root["status"];
    const char *printStatus = "Checked In";
    const char *userName = root["employeeName"];
    Serial.println("resturn" + http.getString());
    Serial.print("Code return element = ");
    Serial.println("EmployeeId ");
    Serial.println(code);
    Serial.println("Employee name ");
    Serial.println(userName);
    if ((strcmp(retStatus, "CHECKIN") == 0))
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print( String("Welcome ") );
      lcd.setCursor(0, 1);
      lcd.print(String(userName) + String("!!"));
    } else if (strcmp(retStatus, "SAME_TIME") == 0) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print( String("Now Only Attendance Recorded ") );
    } else if ((strcmp(retStatus, "BLOCKED") == 0)) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print( String("Your Id is ") );
      lcd.setCursor(0, 1);
      lcd.print(String("Blocked !!"));
    } else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(String("Thank You ") );
      lcd.setCursor(0, 1);
      lcd.print(String(userName) + String("!!"));
    }
    Serial.println(httpCode); //Print HTTP return code
  }
  http.end(); //Close connection
  Serial.println(httpCode);
  Serial.println("SucessFully Send Data To BackEnd");
  return httpCode;
}
void welcomeNote()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Welcome");
  lcd.setCursor(0, 1);
  lcd.print(companyName);
}
void WifiPage()
{
  String s = Wifi_Setting_page;
  String wifilists = "<h2>Wi-Fi Connection</h2><p>" + st + "</p>";
  server.send(200, "text/html", wifilists + s);
}
void RegisterPage()
{
  String s = Register_page;
  String select = DynamicSelect();
  String Content = "<body>" ;
  Content +=          "<h2 align='center'>Registration </h2>" ;
  Content +=          "<div class=\"row\">" ;
  Content +=          "<div class=\"col-75\">" ;
  Content +=          "<div class=\"container\">" ;
  Content +=         "<form action=\"/Success\">" ;
  Content +=         "<div class=\"row\">" ;
  Content +=         "<div class=\"col-50\">" ;
  Content +=        " <label for=\"employeeId\">Employee Id </label>" ;
  Content +=      select;
  //Content +=       //  "<input type='number' name='employeeId' length=4 placeholder=\"Employee Id\">";
  Content +=      "<div  align=\"center\" class=\"col-50\">" ;
  Content +=      " <input type=\"submit\" value=\"Submit\" class=\"btn\">" ;
  Content +=     "</div>" ;
  Content +=   "</div>" ;
  Content +=   "</div>" ;
  Content +=   " </form>" ;
  Content +=   " </div>" ;
  Content +=    " </div>" ;
  Content +=    "</div>" ;
  Content +=   "</body>";
  server.send(200, "text/html", s + "<br/>" + Content);
}
void CompanyPage()
{
  String s = Company_page;
  server.send(200, "text/html", s);
}
void RegisterFinger()
{
  server.on("/Register", RegisterPage);

  server.on("/Success", []() {
    String empId = server.arg("employeeId");
    if (empId != 0)
    {
      Serial.println("Entered iID " + empId);
      Serial.println("Registering");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Place the Finger ");
      id = atoi(empId.c_str());
      Serial.println("Id  :" + id);
      finger.getImage();


      if (!getFingerprintEnroll())
      {
        Serial.println("FingerPrint ");
      }
      if (strcmp(fingerprintstatus.c_str(), "Match") == 0) {
        content = "<body>" ;
        content += "<h2 align='center'>Successfully Registered FingerPrint </h2>" ;
        content += "<h3 align='center'><a href='Register'>Register Again</a></h3>";
        content +=  "</body>";
        statusCode = 200;
      } else {
        content = "<body>" ;
        content = "<h2 align='center'>Not Registered</h2>";
        content += "<h4><p align='center'>FingerPrints Are not Matched Try again.</p></h4>";
        content += "<h3 align='center'><a href='Register'>Register Again</a></h3>";
        content += "</body>" ;
        statusCode = 200;
      }
    }
    else
    {
      content = "<h2 align='center'>Error : 404 not found </h2>";
      statusCode = 404;
      Serial.println("Sending 404");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Error: Try ");
      lcd.setCursor(0, 1);
      lcd.print("Again");
      delay(1000);
      welcomeNote();
    }
    String s = Common_page;
    server.send(statusCode, "text/html", s + "<br/>" + content);
  });
}

//Function for Getting EmployeeId  Details
int EmployeeDetails() {
  lcd.clear();
  Serial.println("Going to get EmployeeId Details");
  HTTPClient http; //Declare object of class HTTPClient
  int httpCode = 0;
  if (WiFi.status() == WL_CONNECTED)
  {
    if (client.connect(servername, 80))
    {
      Serial.println("connected");
      StaticJsonBuffer<300> JSONbuffer;
      JsonObject& JSONencoder = JSONbuffer.createObject();

      JSONencoder["companyId"] = companyId;
      char JSONmessageBuffer[500];
      JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
      Serial.println(JSONmessageBuffer);

      http.begin("http://13.126.195.214:8080/EmployeeAttendenceAPI/device/EmpRFIDList"); //Specify request destination
      http.addHeader("Content-Type", "application/json");                                //Specify content-type header
      httpCode = http.POST(JSONmessageBuffer); //Send the request
      /*  int httpCode = http.GET(JSONmessageBuffer);*/
      if (httpCode > 0)
      {
        const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(2) + 600;
        DynamicJsonBuffer jsonBuffer(capacity);
        // Parse JSON object
        JsonObject &root = jsonBuffer.parseObject(http.getString());
        const char *code = root["employeeId"];
        const char *role = root["role"];
        const char *uname = root["name"];
        const char *dept = root["department"];

        Serial.println("return" );
        String payload = http.getString();
        Serial.println( payload.length());
        if (payload.length() > 0) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Storing Employee");
          lcd.setCursor(0, 1);
          lcd.print("Details");
          WriteEmployeeDataSD(payload);
          Serial.println(httpCode); //Print HTTP return code
        } else {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("No Employee");
          lcd.setCursor(0, 1);
          lcd.print("Details");
        }
        http.end(); //Close connection
      }
    }
    Serial.println(httpCode);
    Serial.println("SucessFully Stored Employee Details");
    return httpCode;
  }
}
void WriteEmployeeDataSD(String data) {
  const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(2) + 600;
  DynamicJsonBuffer jsonBuffer(capacity);
  JsonArray& nodes = jsonBuffer.parseArray(data);
  sd.begin();
  if (sd.exists(EMPDETAILS))
  {
    sd.remove(EMPDETAILS);
    Serial.println("Deleted the Existing File");
    //return;
  }
  Serial.println("Opening File");
  InitialSDcard();
  //InitialCSV();
  csv.close();
  if (!csv.open(EMPDETAILS, O_RDWR | O_CREAT)) {
    Serial.println("Failed open file");
  } else {
    Serial.println("Open Success");
  }
  if (!nodes.success()) {
    Serial.println("parseObject() failed");
    jsonBuffer.clear();
  } else {
    int node_length = nodes.size();
    for (int i = 0; i < node_length; i++) {
      Serial.printf("node-%i\nEmployee : ", i );
      String empId = nodes[i]["employeeId"].as<const char*>();
      String uname = nodes[i]["name"].as<const char*>();
      String role = nodes[i]["role"].as<const char*>();
      String dept = nodes[i]["department"].as<const char*>();
      Serial.println(empId);
      Serial.print("Uname : ");
      Serial.println(uname);
      Serial.print("Department : ");
      Serial.println(dept);
      Serial.print("Role : ");
      Serial.println(role);

      String empData = "";
      empData =  empId.c_str() + String(",") + uname.c_str() + String(",") + role.c_str() + String(",") + dept.c_str();
      csv.addField(empData.c_str());
      csv.addLine();
    }
    welcomeNote();
  }
}
//Function For Reading and Setting Dynmaic value in Select Option
String DynamicSelect()
{
  Serial.println("Function For Setting Dynamic Select Value");
  InitialSDcard();
  //To move to beginning of file
  csv.close();
  if (!csv.open(EMPDETAILS, O_RDWR | O_CREAT)) {
    Serial.println("Failed open file");
  } else {
    Serial.println("Open Success");
  }
  welcomeNote();
  //To move to beginning of file
  csv.gotoBeginOfFile();
  const byte BUFFER_SIZE = 200;
  char buffer[BUFFER_SIZE + 1];
  buffer[BUFFER_SIZE] = '\0';
  String select = "<select name=employeeId>";
  do
  {

    Serial.println("Line Number :");
    Serial.print(csv.getNumberOfLine());
    buffer[BUFFER_SIZE] = '\0';
    csv.readField(buffer, BUFFER_SIZE);
    Serial.println("Buffer 1     :");
    // sprintf ("buffer %s", buffer);
    Serial.print(buffer);
    int len = strlen(buffer);
    if (len != 0)
    {
      Serial.print("\n");
      char *ptr = strtok(buffer, ",");
      int j = 0;
      String EmpId;
      String Name;
      String Role;
      String Department;
      // select += "<option  value='' disabled='disabled' disabled selected hidden > Select Employee</option>";

      while (ptr != NULL)
      {
        if (j == 0)
        {
          EmpId = ptr;
          select += "<option value=" + EmpId + ">";
          select += EmpId;
        }
        else if (j == 1)
        {
          Name = ptr;
          select += " " + Name;
        }
        else if (j == 2)
        {
          Role = ptr;
          select += " " + Role;
        }
        else if (j == 3)
        {
          Department = ptr;
          select += " " + Department;
          select += "</option>";
        }
        j++;
        Serial.println(ptr);
        ptr = strtok(NULL, ",");
      }
    }
    csv.nextField();
  } while (csv.nextLine());
  //select += "</select>";
  return select; // repeat? true
}

//Function For Reconnecting Wifi
void ReconnectWiFi()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Reconnecting ");
  lcd.setCursor(0, 1);
  lcd.print("WIFI....");
  Serial.println("Reconnectiing Wifi Connection");
  esid = "";
  epass = "";
  // read eeprom for ssid and pass
  Serial.println("Reading ssid");
  for (int i = 0; i < 32; ++i)
  {
    esid += char(EEPROM.read(i));
  }
  Serial.print("SSID: ");
  Serial.println(esid);
  Serial.println("Reading  password");
  for (int i = 32; i < 96; ++i)
  {
    epass += char(EEPROM.read(i));
  }
  Serial.print("PASS: ");
  Serial.println(epass);
  launchWeb(1);
  if (esid.length() > 1)
  {
    WiFi.begin(esid.c_str(), epass.c_str());
  }

  bool stat = WifiTest();
  Serial.print("Connection Status");
  Serial.println(stat);
}
//Function For Testing Wifi Connection
bool WifiTest(void)
{
  int c = 0;
  Serial.println("Waiting for Wifi to connect to ESP8266");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting Wifi...");
  while (c < 20)
  {

    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println(" WIFI Status Connected " + WiFi.isConnected());
      lcd.clear();
      lcd.print("Wi-Fi Connected....");
      delay(1000);
      //int status = EmployeeDetails();
      Serial.println("Status Employee Details");
      //Serial.println(status);
      welcomeNote();
      c = 20;
      return true;
    } else {
      Serial.println("WIFI Status Not Connected " + WiFi.status());
    }
    delay(500);
    c++;
  }
  Serial.println("");
  Serial.println("Connect timed out, opening AP");
  lcd.clear();
  lcd.print("Wi-Fi Not ");
  lcd.setCursor(0, 1);
  lcd.print("Connected....");
  delay(1000);
  welcomeNote();
  return false;
}