#include <ESP8266WiFi.h>
#include<ESP8266WebServer.h>
#include<WiFiClient.h>
#include<EEPROM.h>
//Variables
const char *ssid = "Viet Quy";
const char *passphrase = "vietquy160591";
//Macro
void light_sleep();
void setup()
{
  Serial.begin(9600);
  WiFi.begin(ssid,passphrase);
  Serial.println("Connecting");
  while ((WiFi.status() != WL_CONNECTED))
  {
    Serial.printf(".");
    delay(100);
  }
  Serial.println("Connected");
}
void loop()
{
   if ((WiFi.status() == WL_CONNECTED))
  {
    for (int i = 0; i <3 ; i++)
    {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(1000);
      digitalWrite(LED_BUILTIN, LOW);
      delay(1000);
    }
  }
  Serial.println("Go to sleep now");
  light_sleep();
  delay(200);
  Serial.println("Wake up");
  WiFi.begin(ssid,passphrase);
  while ((WiFi.status() != WL_CONNECTED))
  {
    Serial.printf(".");
    delay(100);
  }
  Serial.println("Connected");
  
}
void light_sleep()
{
  wifi_station_disconnect();
  wifi_set_opmode_current(NULL_MODE);
  wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
  wifi_fpm_open();
  gpio_pin_wakeup_enable(GPIO_ID_PIN(4),GPIO_PIN_INTR_HILEVEL);
  wifi_fpm_do_sleep(0xFFFFFFF);

}