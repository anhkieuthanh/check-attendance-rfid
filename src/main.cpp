#include <Wire.h>
#include <MFRC522.h>
#include <SPI.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <NTPtimeESP.h>
#include <RTClib.h>
#include <WebServer.h>
#include <WiFiClient.h>
#include <EEPROM.h>
#include "display_lcd.h"
#include "buzz.h"
#include "handle.h"
#include "config.h"
#include "customKeyPad.h"
#include "esp32/ulp.h"
#include "driver/rtc_io.h"
#include "time.h"

#define SS_PIN 21
#define RST_PIN 22
#define SIZE_BUFFER 18
#define MAX_SIZE_BLOCK 16
#define Threshold 40

String stdCode = "";
String userPhone = "";
int statusCode;
String st;
String content;
bool isMessageReceived = false;
bool checkSleep=false;
char resp[30];
byte rowPins[KEYPADROW] = {0, 1, 2, 3};
byte colPins[KEYPADCOL] = {4, 5, 6, 7};
char keys[KEYPADROW][KEYPADCOL] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};
char monthOfTheYear[12][10]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
const char* ntpServer="pool.ntp.org";
const long gmtOffset_sec=25200;
const int   daylightOffset_sec = 3600;
WiFiClient espClient;
PubSubClient client(espClient);
NTPtime NTPch("ch.pool.ntp.org");
strDateTime dateTime;
RTC_DS1307 RTC;

String userIDBuffer;
//used in authentication
MFRC522::MIFARE_Key key;
//authentication return status code
MFRC522::StatusCode status;
// Defined pins to module RC522
MFRC522 mfrc522(SS_PIN, RST_PIN);

LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMN, LCD_ROW);
Keypad_I2C customKeypad(makeKeymap(keys), rowPins, colPins, KEYPADROW, KEYPADCOL, I2CADDR);

WebServer server(80);

hw_timer_t *timer=NULL;

void setup_wifi();
void readingData();
void scrollSingleLine(String fixedString, String scrolledString, int *flag);
void writingData();
void callback(char *topic, byte *payload, unsigned int length);
void reconnect();
bool testWifi(void);
void launchWeb(void);
void setupAP(void);
void createWebServer();
void sleepMode();
void rtc_setPins();
void setTimer();
void printLocalTime();

void setup()
{
  
  //Connect to server
  setup_wifi();
  client.setKeepAlive(60);
  client.setServer(mqtt_server, mqtt_port);
  //Initialize SPI protocol
  SPI.begin();

  client.setCallback(callback);
  Wire.begin(5, 17);
  //Initialize Keypad 4x4
  customKeypad.begin();
  //Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(1, 0);
  lcd.print("Mandevices Lab");
  //set up for ntp time
  configTime(gmtOffset_sec,daylightOffset_sec,ntpServer);
  printLocalTime();
  //set pins for ULP
  rtc_setPins();
  rtc_gpio_set_level(GPIO_NUM_15,0x00);
  rtc_gpio_set_level(GPIO_NUM_0,0x01);
  //set pin for horn
  pinMode(BUZZ_PIN, OUTPUT);
  //Initialize RFID module
  Serial.begin(9600);
  mfrc522.PCD_Init();
  Serial.println("Approach your reader card...");
}

void loop()
{
  if (isMessageReceived)
  {
    client.publish(mqtt_topic_reg, dataCombineReg(string2char(userIDBuffer), string2char(stdCode), string2char(userPhone)), true);
    stdCode = "";
    userPhone = "";
    isMessageReceived = false;
  }
  if (!client.connected())
  {
    reconnect();
    //scrollSingleLine("","Connecting...");
  }
  client.loop();

  printLocalTime();

  if (!mfrc522.PICC_IsNewCardPresent())
  {
  //if no card is recognized, start Timer for Deep-sleep Mode
    if (!checkSleep)
    {
      setTimer();
      timerAlarmEnable(timer);
      Serial.println("Start timer");
      checkSleep=true;
    }
    return;
  }
  
  // Select a card
  if (!mfrc522.PICC_ReadCardSerial())
  {
    return;
  }
  readingData();
  correctBuzz();
  //instructs the PICC when in the ACTIVE state to go to a "STOP" state
  mfrc522.PICC_HaltA();
  // "stop" the encryption of the PCD, it must be called after communication with authentication, otherwise new communications can not be initiated
  mfrc522.PCD_StopCrypto1();
  checkSleep=false;
}
void reconnect()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    setup_wifi();
    if (!testWifi())
    {
      launchWeb();
      setupAP();
      while ((WiFi.status() != WL_CONNECTED))
      {
        Serial.print(".");
        delay(100);
        server.handleClient();
      }
    }
  }
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32", mqtt_user, mqtt_pwd, mqtt_device_status_topic, 1, true, "lost"))
    {
      Serial.println("connected");
      client.subscribe(mqtt_topic_sub);
      client.publish(mqtt_device_status_topic, "ready", true);
      client.setCallback(callback);
    }
    delay(500);
  }
}
void setup_wifi()
{
  int countTime = 0;
  EEPROM.begin(512); //Initialasing EEPROM
  delay(10);
  Serial.println("Reading EEPROM ssid");
  String esid = "";
  for (int i = 0; i < 32; ++i)
  {
    esid += char(EEPROM.read(i));
  }
  String epass = "";
  for (int i = 32; i < 96; ++i)
  {
    epass += char(EEPROM.read(i));
  }
  WiFi.begin(esid.c_str(), epass.c_str());
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    countTime++;
    if (countTime == 10)
      break;
  }
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("");
    Serial.println("WiFi connected");
  }
}

void readingData()
{
  Serial.println();
  //prints the technical details of the card/tag
  mfrc522.PICC_DumpDetailsToSerialUid(&(mfrc522.uid));

  //Convert UID into String
  String userid;
  userIDBuffer = "";
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    userid += String(mfrc522.uid.uidByte[i], HEX);
    userIDBuffer += String(mfrc522.uid.uidByte[i], HEX);
  }
  //prepare the key - all keys are set to FFFFFFFFFFFFh

  for (byte i = 0; i < 6; i++)
    key.keyByte[i] = 0xFF;

  //buffer for read data
  byte buffer[SIZE_BUFFER] = {0};

  //the block to operate
  byte block = 1;
  byte size = SIZE_BUFFER;
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK)
  {
    Serial.print(F("Authentication failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  //read data from block
  status = mfrc522.MIFARE_Read(block, buffer, &size);
  if (status != MFRC522::STATUS_OK)
  {
    Serial.print(F("Reading failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  client.publish(mqtt_topic_pub, dataCombine(string2char(userid)), true);
  Serial.println(userid);
}

void writingData()
{
  Serial.println();
  //prints technical details from of the card/tag
  mfrc522.PICC_DumpDetailsToSerialUid(&(mfrc522.uid));

  //prepare the key - all keys are set to FFFFFFFFFFFFh
  for (byte i = 0; i < 6; i++)
    key.keyByte[i] = 0xFF;
  byte block; //the block to operate
  block = 1;  //the block to operate
  // String str = (char *)buffer; //transforms the buffer data in String
  // Serial.print("Current state: ");
  // Serial.println(str);

  //authenticates the block to operate
  //Authenticate is a command to hability a secure communication
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A,
                                    block, &key, &(mfrc522.uid));

  if (status != MFRC522::STATUS_OK)
  {
    Serial.println(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  //Writes in the block
  //status = mfrc522.MIFARE_Write(block, buffer, MAX_SIZE_BLOCK);
  if (status != MFRC522::STATUS_OK)
  {
    Serial.println(F("MIFARE_Write() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
}

void callback(char *topic, byte *payload, unsigned int length)
{

  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++)
  {
    resp[i] = (char)payload[i];
    Serial.print((char)payload[i]);
  }
  DynamicJsonDocument doc(200);
  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, resp);
  // Test if parsing succeeds.
  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }
  // Fetch values.
  uint8_t code = doc["code"];
  const char *message = doc["message"];

  int flag = 0;
  int commonCase;
  bool inputState = false;
  if (code == 2 || code == 6 || code == 8 || code == 9 || code == 10)
    commonCase = 1;
  else if (code == 7)
    commonCase = 0;
  else
    commonCase = 3;
  switch (commonCase)
  {
  case 1:
  {
    correctBuzz();
    while (1)
    {
      scrollSingleLine("Message:", message, &flag);
      if (flag == 1)
        break;
    }
    turnBackDefault();
    break;
  }
  case 0:
  {
    correctBuzz();
    while (1)
    {
      scrollSingleLine("Message:", message, &flag);
      if (flag == 1)
        break;
    }
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Dang ki the ?");
    lcd.setCursor(0, 1);
    lcd.print("A: Yes");
    lcd.setCursor(7, 1);
    lcd.print("B: No");
    while (1)
    {
      char choice = customKeypad.waitForKey();
      if (choice == 'A')
      {
        oneLineFix("Nhap MSSV:");
        stateInput = 1;
        stateData = 1;
        count = 0;
        while (!inputState)
        {
          readKeyPad(8, stdCode);
          if (stateInput == 0)
          {
            oneLineFix("Nhap MSSV");
            stateInput = 1;
          }
          if (stateData == 2)
          {
            oneLineFix("Done");
            delay(1000);

            oneLineFix("Nhap sdt:");
            stateData = 1;
            inputState = true;
          }
        }
        count = 0;
        inputState = false;
        while (!inputState)
        {
          readKeyPad(9, userPhone);
          if (stateInput == 0)
          {
            oneLineFix("Nhap sdt:");
            stateInput = 1;
          }
          if (stateData == 2)
          {
            oneLineFix("Done!");
            oneLineBack("Sent to server", 1000);
            isMessageReceived = true;
            stateData = 1;
            delay(2000);
            inputState = true;
          }
        }
        if (!inputState)
        break;
      break;
      }
      if (choice == 'B')
      {
        turnBackDefault();
        break;
      }
    };
    break;
  }
  default:
  {
    wrongBuzz();
    oneLineBack("Undefined Error", 1000);
    break;
  }
  }
}
bool testWifi(void)
{
  int c = 0;
  Serial.println("Waiting for Wifi to connect");
  while (c < 20)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      return true;
    }
    delay(500);
    Serial.print("*");
    c++;
  }
  Serial.println("");
  Serial.println("Connect timed out, opening AP");
  return false;
}
void launchWeb()
{
  Serial.println("");
  if (WiFi.status() == WL_CONNECTED)
  Serial.println("WiFi connected");
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("SoftAP IP: ");
  Serial.println(WiFi.softAPIP());
  createWebServer();
  // Start the server
  server.begin();
  Serial.println("Server started");
}
void setupAP()
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0)
    Serial.println("no networks found");
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == 7) ? " " : "*");
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
    st += (WiFi.encryptionType(i) == 7) ? " " : "*";
    st += "</li>";
  }
  st += "</ol>";
  delay(100);
  WiFi.softAP("MandevicesESP", "");
  Serial.println("softap");
  launchWeb();
  Serial.println("over");
}
void createWebServer()
{
  {
    server.on("/", []() {
      IPAddress ip = WiFi.softAPIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      content = "<!DOCTYPE HTML>\r\n<html>Hello from ESP8266 at ";
      content += "<form action=\"/scan\" method=\"POST\"><input type=\"submit\" value=\"scan\"></form>";
      content += ipStr;
      content += "<p>";
      content += st;
      content += "</p><form method='get' action='setting'><label>SSID: </label><input name='ssid' length=32><input name='pass' length=64><input type='submit'></form>";
      content += "</html>";
      server.send(200, "text/html", content);
    });
    server.on("/scan", []() {
      //setupAP();
      IPAddress ip = WiFi.softAPIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      content = "<!DOCTYPE HTML>\r\n<html>go back";
      server.send(200, "text/html", content);
    });
    server.on("/setting", []() {
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
        content = "{\"Success\"\"saved to eeprom... reset to boot into new wifi\"}";
        statusCode = 200;
        ESP.restart();
      }
      else
      {
        content = "{\"Error:404 not found\"}";
        statusCode = 404;
        Serial.println("Sending 404");
      }
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(statusCode, "application/json", content);
    });
  }
}
void callback1()
{
}
//when esp32 is in deep-sleep mode, we use ULP coprocessor
void rtc_setPins()
{
//set pins used by ULP
  rtc_gpio_init(GPIO_NUM_0);
  rtc_gpio_init(GPIO_NUM_15);
//set mode for pins  
  rtc_gpio_set_direction(GPIO_NUM_15, RTC_GPIO_MODE_OUTPUT_ONLY);
  rtc_gpio_set_direction(GPIO_NUM_0, RTC_GPIO_MODE_OUTPUT_ONLY);

 
}
void setTimer()
{
  timer = timerBegin(0,80,true);
  timerAttachInterrupt(timer,&sleepMode,true);
  timerAlarmWrite(timer,300000000,true);
}
void sleepMode()
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Sleeping");
  //Setup interrupt on Touch Pad 3 (GPIO15)
  touchAttachInterrupt(T5, callback1, Threshold);

  //Configure Touchpad as wakeup source
  esp_sleep_enable_touchpad_wakeup();

  //Go to sleep now
  //turn on the LED red, turn off the Blue LED
  rtc_gpio_set_level(GPIO_NUM_15,0x01);
  rtc_gpio_set_level(GPIO_NUM_0,0x00);

  Serial.println("Going to sleep now");
  esp_deep_sleep_start();
}
void printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  lcd.setCursor(0,1);
  // lcd.print(monthOfTheYear[timeinfo.tm_mon]);
  // lcd.print(" ");
  // lcd.print(timeinfo.tm_mday);
  // lcd.print(" ");
  // lcd.print(timeinfo.tm_hour);
  // lcd.print(":");
  // lcd.print(timeinfo.tm_min);
  // lcd.print(":");
  // if (timeinfo.tm_sec<10)
  // {
  // lcd.print("0");
  // lcd.print(timeinfo.tm_sec);
  lcd.print(monthOfTheYear[timeinfo.tm_mon]);
  lcd.print(" ");
  lcd.print(&timeinfo, "%d %H:%M:%S");
}