#include <WiFi.h>
#include<WebServer.h>
#include <Arduino.h>
//Variables
hw_timer_t * timer = NULL;
#define Threshold 40 /* Greater the value, more the sensitivity */
RTC_DATA_ATTR int bootCount = 0;
touch_pad_t touchPin;
 char *ssid = "Viet Quy";
 char *passphrase = "vietquy160591";
//Functions
void setTimer();
void sleepMode();
void callback();
void print_wakeup_reason();
void print_wakeup_touchpad();
void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN,OUTPUT);
  Serial.printf("Blink Led");
  for (int i=0;i<3;i++)
  {
    digitalWrite(LED_BUILTIN,HIGH);
    delay(2000);
    digitalWrite(LED_BUILTIN,LOW);
    delay(2000);
  }
  // WiFi.begin(ssid,passphrase);
  // while (WiFi.status() != WL_CONNECTED)
  // {
  //   Serial.printf(".");
  //   delay(1000);
  // }

  //Serial.println("Connected");
  setTimer();
  timerAlarmEnable(timer);
  Serial.println("start timer");
}

void loop() {
}

void setTimer()
{
  timer = timerBegin(0,80,true);
  timerAttachInterrupt(timer,&sleepMode,true);
  timerAlarmWrite(timer,10000000,true);
}
void sleepMode()
{
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));

  //Print the wakeup reason for ESP32 and touchpad too
  print_wakeup_reason();
  print_wakeup_touchpad();

  //Setup interrupt on Touch Pad 3 (GPIO15)
  touchAttachInterrupt(T3, callback, Threshold);

  //Configure Touchpad as wakeup source
  esp_sleep_enable_touchpad_wakeup();

  //Go to sleep now
  Serial.println("Going to sleep now");
  esp_deep_sleep_start();
}
void print_wakeup_touchpad(){
  touchPin = esp_sleep_get_touchpad_wakeup_status();

  switch(touchPin)
  {
    case 0  : Serial.println("Touch detected on GPIO 4"); break;
    case 1  : Serial.println("Touch detected on GPIO 0"); break;
    case 2  : Serial.println("Touch detected on GPIO 2"); break;
    case 3  : Serial.println("Touch detected on GPIO 15"); break;
    case 4  : Serial.println("Touch detected on GPIO 13"); break;
    case 5  : Serial.println("Touch detected on GPIO 12"); break;
    case 6  : Serial.println("Touch detected on GPIO 14"); break;
    case 7  : Serial.println("Touch detected on GPIO 27"); break;
    case 8  : Serial.println("Touch detected on GPIO 33"); break;
    case 9  : Serial.println("Touch detected on GPIO 32"); break;
    default : Serial.println("Wakeup not by touchpad"); break;
  }
}
void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}
void callback(){
  //placeholder callback function
}