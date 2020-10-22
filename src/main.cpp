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

#define SS_PIN 21
#define RST_PIN 22
#define SIZE_BUFFER 18
#define MAX_SIZE_BLOCK 16

//int i = 0;
int statusCode;
boolean check;
String st;
String content;
char resp[30];
byte rowPins[KEYPADROW] = {0, 1, 2, 3};
byte colPins[KEYPADCOL] = {4, 5, 6, 7};
char keys[KEYPADROW][KEYPADCOL] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};
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

//Establishing Local server at port 80 whenever required
WebServer server(80);

void readingData();
void scrollSingleLine(String fixedString, String scrolledString, int *flag);
void writingData();
void setup_wifi();
void callback(char *topic, byte *payload, unsigned int length);
void reconnect();
bool testWifi(void);
void launchWeb(void);
void setupAP(void);
void createWebServer();

void setup()
{
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  SPI.begin();
  client.setCallback(callback);
  Wire.begin(5, 17);
  customKeypad.begin();
  customKeypad.setHoldTime(2000);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(1, 0);
  lcd.print("Mandevices Lab");
  pinMode(BUZZ_PIN, OUTPUT);
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  Serial.begin(9600);
  mfrc522.PCD_Init();
  Serial.println("Approach your reader card...");
}

void loop()
{
  // if (WiFi.status() != WL_CONNECTED)
  // check=client.connected();
  if (!client.connected())
  {
    //check==0;
     for (int i = 0; i < 512; i++) 
    {
      EEPROM.write(i, 0);
      delay(5);
    }
    reconnect();
    // if (testWifi())
    //   client.connected();
  }
  client.loop();
  if (!mfrc522.PICC_IsNewCardPresent())
  {
    return;
  }
  // Select a card
  if (!mfrc522.PICC_ReadCardSerial())
  {
    return;
  }
  readingData();
  //instructs the PICC when in the ACTIVE state to go to a "STOP" state
  mfrc522.PICC_HaltA();
  // "stop" the encryption of the PCD, it must be called after communication with authentication, otherwise new communications can not be initiated
  mfrc522.PCD_StopCrypto1();
}
void reconnect()
{
  int countTime = 0;
    while (!client.connected())
    {
      Serial.print("Attempting MQTT connection...");
      EEPROM.begin(512); //Initialasing EEPROM
      delay(10);
      pinMode(LED_BUILTIN, OUTPUT);
      Serial.println();
      Serial.println();
      Serial.println("Startup");
      //---------------------------------------- Read eeprom for ssid and pass
      Serial.println("Reading EEPROM ssid");
      String esid;
      for (int i = 0; i < 32; ++i)
      {
        esid += char(EEPROM.read(i));
      }
      Serial.println();
      Serial.print("SSID: ");
      Serial.println(esid);
      Serial.println("Reading EEPROM pass");
      String epass = "";
      for (int i = 32; i < 96; ++i)
      {
        epass += char(EEPROM.read(i));
      }
      Serial.print("PASS: ");
      Serial.println(epass);
      WiFi.begin(esid.c_str(), epass.c_str());
      if (testWifi())
      {
        Serial.println("Succesfully Connected!!!");
        check=true;
        return;
      }
      else
      {
        Serial.println("Turning the HotSpot On");
        launchWeb();
        setupAP(); 
      }
      Serial.println();
      Serial.println("Waiting.");
      while ((WiFi.status() != WL_CONNECTED))
      {
        Serial.print(".");
        delay(100);
        server.handleClient();
      }
        if (client.connect("ESP32", mqtt_user, mqtt_pwd))
        {
          Serial.println("connected");
          client.subscribe(mqtt_topic_sub);
        }
        delay(500);
        Serial.print(".");
        countTime++;
        if (countTime == 10)
        {
          break;
        }
    }
  }
void setup_wifi()
{
  // digitalWrite(RED_PIN, 1);
  // int countTime = 0;
  // Serial.print("Connecting to ");
  // Serial.println(ssid);
  // WiFi.begin(ssid, password);
  // while (WiFi.status() != WL_CONNECTED)
  // {
  //   delay(500);
  //   Serial.print(".");
  //   countTime++;
  //   if (countTime == 10)
  //     break;
  // }
  // if (WiFi.status() == WL_CONNECTED)
  // {
  //   Serial.println("");
  //   Serial.println("WiFi connected");
  //   digitalWrite(RED_PIN, 0);
  //   digitalWrite(GREEN_PIN, 1);
  //   delay(3000);
  //   digitalWrite(GREEN_PIN, 0);
  // }
//   
//   }
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
  client.publish(mqtt_topic_pub, dataCombine(string2char(userid)));
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
  char *stdCode;
  char *userPhone;
  while (stdCode == NULL || userPhone == NULL)
  {
    stdCode = (char *)malloc(9);
    userPhone = (char *)malloc(10);
  }
  const char *data1;
  const char *data2;
  int flag = 0;
  int commonCase;
  if (code == 2 || code == 6 || code == 8 || code == 9 || code == 10)
    commonCase = 1;
  else if (code == 7)
    commonCase = 0;
  else
    commonCase = 3;
  switch (commonCase)
  {
  case 1:
    correctBuzz();
    while (!mfrc522.PICC_IsNewCardPresent())
    {
      scrollSingleLine("Message:", message, &flag);
      if (flag == 1)
        break;
    }
    delay(2000);
    turnBackDefault();
    oneLineBack(message, 2000);
    break;
  case 0:
    correctBuzz();
    while (!mfrc522.PICC_IsNewCardPresent())
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
        while (1)
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
            break;
          }
        }
        data1 = stdCode;
        count = 0;
        while (1)
        {
          readKeyPad(9, userPhone);
          if (stateInput == 0)
          {
            oneLineFix("Nhap sdt");
            stateInput = 1;
          }
          if (stateData == 2)
          {
            oneLineFix("Done!");
            oneLineBack("Sent to server", 1000);
            stateData = 1;
            delay(2000);
            break;
          }
        }
        data2 = userPhone;
        break;
      }
      if (choice == 'B')
      {
        turnBackDefault();
        break;
      }
    }
    client.publish(mqtt_topic_reg, dataCombineReg(userIDBuffer.c_str(), data1, data2));
    free(userPhone);
    free(stdCode);
    Serial.println();
    Serial.println(data1);
    Serial.println(data2);

    break;
  default:
    wrongBuzz();
    oneLineBack("Undefined Error", 1000);
  break;
  }

}
//connecting to wifi
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
void setupAP(void)
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

