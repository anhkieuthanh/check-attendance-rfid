#include <Wire.h>
#include <MFRC522.h>
#include <SPI.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <string.h>
#include <ArduinoJson.h>
#include <NTPtimeESP.h>
#include <RTClib.h>
#include "display_lcd.h"
#include "buzz.h"

#define SS_PIN 21
#define RST_PIN 22
#define SIZE_BUFFER 18
#define MAX_SIZE_BLOCK 16

#define ssid "realme 5i"
#define password "nhoden3210"
#define mqtt_server "mandevices.com"
#define mqtt_topic_pub "attendance/card-register"
#define mqtt_topic_sub "attendance/response"
#define mqtt_user "nhodennn"
#define mqtt_pwd "Abccbdewn"

char data[50];
const uint16_t mqtt_port = 1883;
char resp[30];

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
const char *dataCombine(const char *uid, const char *state);
char *string2char(String command);
//void scrollSingleLine(String str1, String str2, int *flag);

void setup()
{
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  Wire.begin(5, 17);
  RTC.begin();
  dateTime = NTPch.getNTPtime(7.0, 0);
  while (!dateTime.valid)
  {
    dateTime = NTPch.getNTPtime(7.0, 0);
  }
  RTC.adjust(DateTime(dateTime.year, dateTime.month, dateTime.day, dateTime.hour, dateTime.minute, dateTime.second));
  DateTime now = RTC.now();
  Serial.print(now.year(), DEC); // Năm
  Serial.print('/');
  Serial.print(now.month(), DEC); // Tháng
  Serial.print('/');
  Serial.print(now.day(), DEC); // Ngày
  Serial.print(' ');
  Serial.print(now.hour(), DEC); // Giờ
  Serial.print(':');
  Serial.print(now.minute(), DEC); // Phút
  Serial.print(':');
  Serial.print(now.second(), DEC); // Giây
  Serial.println();
  delay(1000); // Delay
  lcd.init();
  lcd.backlight();
  lcd.setCursor(1, 0);
  lcd.print("Mandevices Lab");
  pinMode(BUZZ_PIN,OUTPUT);

  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();

  Serial.println("Approach your reader card...");
  Serial.println();
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();
  //waiting the card approach
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
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
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
    client.publish(mqtt_topic_pub, dataCombine(string2char(userid), "In"));
    writingData(buffer2);
    oneLineBack("Welcome!!", 1000);
  }
  else
  {
    byte buffer2[MAX_SIZE_BLOCK] = "Out";
    client.publish(mqtt_topic_pub, dataCombine(string2char(userid), "Out"));
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
  const char *message = "Nguyen Dang Nho Den";
  int flag = 0;
  switch (code)
  {
  case 0:
    oneLineBack("No connection!!", 1000);
    break;
  case 1:
    wrongBuzz();
    oneLineBack("Not available!", 1000);
    break;
  case 2:
    correctBuzz();
    oneLineBack("Assign Successfull", 1000);
    break;
  case 3:
    wrongBuzz();
    lcd.clear();
    lcd.setCursor(0, 0);
    while (!mfrc522.PICC_IsNewCardPresent())
    {
      scrollSingleLine("Message:", "You are not in today list", &flag);
      if (flag == 1)
        break;
    }
    delay(2000);
    turnBackDefault();
    break;
  case 4:
    wrongBuzz();
    lcd.clear();
    lcd.setCursor(0, 0);
    while (!mfrc522.PICC_IsNewCardPresent())
    {
      scrollSingleLine("Message:", "Invalid Card!!", &flag);
      if (flag == 1)
        break;
    }
    delay(2000);
    turnBackDefault();
    break;
  case 5:
  {
    correctBuzz();
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print(fullName);
    lcd.setCursor(3, 1);
    lcd.print(stdCode);
    delay(1000);
    lcd.clear();
    while (!mfrc522.PICC_IsNewCardPresent())
    {
      scrollSingleLine("Message: ", message, &flag);
      if (flag == 1)
        break;
    }
    turnBackDefault();
    break;
  }
  case 6:
    wrongBuzz();
    lcd.clear();
    lcd.setCursor(1, 0);
    while (!mfrc522.PICC_IsNewCardPresent())
    {
      scrollSingleLine("Error", "Invalid package format!", &flag);
      if (flag == 1)
        break;
    }
    break;
  default:
    wrongBuzz();
    oneLineBack("Undefined Error", 1000);
    break;
  }
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
  }
}

const char *dataCombine(const char *uid, const char *state)
{
  data[0] = '\0';
  DynamicJsonDocument doc(200);
  // Add values in the document
  doc["code"] = uid;
  doc["state"] = state;

  // Generate the minified JSON and send it to the Serial port.
  //
  serializeJson(doc, data);
  // The above line prints:
  // {"code":"uid","state":state}
  Serial.println("test code: ");
  Serial.print(data);
  Serial.println("---------------");
  return data;
}

char *string2char(String command)
{
  if (command.length() != 0)
  {
    char *p = const_cast<char *>(command.c_str());
    return p;
  }
}


