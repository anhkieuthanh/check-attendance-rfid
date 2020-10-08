#include <Wire.h>
#include <MFRC522.h>
#include <SPI.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <NTPtimeESP.h>
#include <RTClib.h>
#include "display_lcd.h"
#include "buzz.h"
#include "handle.h"
#include "config.h"
#include "customKeyPad.h"

#define SS_PIN 21
#define RST_PIN 22
#define SIZE_BUFFER 18
#define MAX_SIZE_BLOCK 16

String stdCode = "";
String userPhone = "";
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

void readingData();
void scrollSingleLine(String fixedString, String scrolledString, int *flag);
void writingData();
void setup_wifi();
void callback(char *topic, byte *payload, unsigned int length);
void reconnect();

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
  if (!client.connected())
  {
    reconnect();
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
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32", mqtt_user, mqtt_pwd))
    {
      Serial.println("connected");
      client.subscribe(mqtt_topic_sub);
    }
    delay(500);
    Serial.print(".");
  }
}
void setup_wifi()
{
  int countTime = 0;
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
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
    while (1)
    {
      scrollSingleLine("Message:", message, &flag);
      if (flag == 1)
        break;
    }
    turnBackDefault();
    break;
  case 0:
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
        break;
      }
      if (choice == 'B')
      {
        turnBackDefault();
        break;
      }
    };
    client.publish(mqtt_topic_reg, dataCombineReg(string2char(userIDBuffer), string2char(stdCode), string2char(userPhone)));
    Serial.println(userIDBuffer.c_str());
    Serial.println(stdCode.c_str());
    Serial.println(userPhone.c_str());
    stdCode = "";
    userPhone = "";
    break;
  default:
    wrongBuzz();
    oneLineBack("Undefined Error", 1000);
    break;
  }
}
