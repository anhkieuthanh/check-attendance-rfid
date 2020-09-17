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
#include <OnewireKeypad.h>

//#include <EEPROM.h>
bool isConnectWiFi = false;

#define SS_PIN 21
#define RST_PIN 22
#define SIZE_BUFFER 18
#define MAX_SIZE_BLOCK 16
#define keypad 35
//extern char data[50];

char resp[30];
int flaseeAddress = 0;

// just some reference flags for scroll lcd funtion
// int stringStart = 0, stringStop = 0;
// int scrollCursor = 16;

WiFiClient espClient;
PubSubClient client(espClient);

NTPtime NTPch("ch.pool.ntp.org");
strDateTime dateTime;
RTC_DS1307 RTC;

//used in authentication
MFRC522::MIFARE_Key key;
//authentication return status code
MFRC522::StatusCode status;
// Defined pins to module RC522
MFRC522 mfrc522(SS_PIN, RST_PIN);

extern LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMN, LCD_ROW);

void readingData();

void writingData(byte buffer[MAX_SIZE_BLOCK]);
void setup_wifi();
void callback(char *topic, byte *payload, unsigned int length);
void reconnect();

void setup()
{
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  Wire.begin(5, 17);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(1, 0);
  lcd.print("Mandevices Lab");
  lcd.setCursor(0, 1);
  lcd.print("A:Me B:St C:Re ");
  pinMode(BUZZ_PIN, OUTPUT);
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  Serial.begin(9600);
  SPI.begin();
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

void setup_wifi()
{
  digitalWrite(RED_PIN, 1);
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
    isConnectWiFi = true;
    Serial.println("");
    Serial.println("WiFi connected");
    digitalWrite(RED_PIN, 0);
    digitalWrite(GREEN_PIN, 1);
    delay(3000);
    digitalWrite(GREEN_PIN, 0);
  }
}

void readingData()
{
  Serial.println();
  //prints the technical details of the card/tag
  mfrc522.PICC_DumpDetailsToSerialUid(&(mfrc522.uid));

  //Convert UID into String
  String userid;

  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    userid += String(mfrc522.uid.uidByte[i], HEX);
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
  Serial.print("Last state: ");
  //prints read data
  for (uint8_t i = 0; i < MAX_SIZE_BLOCK; i++)
  {
    Serial.write(buffer[i]);
  }
  Serial.print(" ");
  int count = 0, i = 0;
  while (buffer[i] != '\0')
  {
    i++;
    count++;
  }
  if (count != 2)
  {
    byte buffer2[MAX_SIZE_BLOCK] = "In";
    client.publish(mqtt_topic_pub, dataCombine(string2char(userid)));
    writingData(buffer2);
    oneLineBack("Welcome!!", 1000);
  }
  else
  {
    byte buffer2[MAX_SIZE_BLOCK] = "Out";
    client.publish(mqtt_topic_pub, dataCombine(string2char(userid)));
    writingData(buffer2);
    oneLineBack("See you again!", 1000);
  }
}

void writingData(byte buffer[MAX_SIZE_BLOCK])
{
  Serial.println();
  //prints technical details from of the card/tag
  mfrc522.PICC_DumpDetailsToSerialUid(&(mfrc522.uid));

  //prepare the key - all keys are set to FFFFFFFFFFFFh
  for (byte i = 0; i < 6; i++)
    key.keyByte[i] = 0xFF;
  byte block;                  //the block to operate
  block = 1;                   //the block to operate
  String str = (char *)buffer; //transforms the buffer data in String
  Serial.print("Current state: ");
  Serial.println(str);

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
  status = mfrc522.MIFARE_Write(block, buffer, MAX_SIZE_BLOCK);
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
  const char *stdCode = doc["payload"]["stdCode"];
  const char *fullName = doc["payload"]["fullName"];
  const char *message = doc["message"];
  int flag = 0;
  int commonCase;
  if (code == 2 || code == 6 || code == 8 || code == 9 || code == 10)
    commonCase = 1;
  else if (code == 7)
    commonCase = 0;
  else
    commonCase = 3;
  switch (code)
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
    break;
  case 0:
    lcd.clear();
    lcd.setCursor(0, 0);
    while (!mfrc522.PICC_IsNewCardPresent())
    {
      scrollSingleLine("Message:", message, &flag);
      if (flag == 1)
        break;
    }
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("Dang ki the ?");
    lcd.setCursor(1, 0);
    lcd.print("A: Yes");
    lcd.setCursor(1, 7);
    lcd.print("B: No");
    break;
  default:
    wrongBuzz();
    oneLineBack("Undefined Error", 1000);
    break;
  }
}

void reconnect()
{
  int countTime = 0;
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32", mqtt_user, mqtt_pwd))
    {
      Serial.println("connected");
      client.subscribe(mqtt_topic_sub);
      client.subscribe(boardStatusTopic);
    }
    delay(500);
    Serial.print(".");
    countTime++;
    if (countTime == 10)
    {
      isConnectWiFi = false;
      break;
    }
  }
}
