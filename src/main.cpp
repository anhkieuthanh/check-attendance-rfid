// #include <ESP8266WiFi.h>
// #include<ESP8266WebServer.h>
// #include<WiFiClient.h>
// #include<EEPROM.h>
// //Variables
// const char *ssid = "Viet Quy";
// const char *passphrase = "vietquy160591";
// //Macro
// void light_sleep();
// void setup()
// {
//   Serial.begin(9600);
//   WiFi.begin(ssid,passphrase);
//   Serial.println("Connecting");
//   while ((WiFi.status() != WL_CONNECTED))
//   {
//     Serial.printf(".");
//     delay(100);
//   }
//   Serial.println("Connected");
// }
// void loop()
// {
//    if ((WiFi.status() == WL_CONNECTED))
//   {
//     for (int i = 0; i <3 ; i++)
//     {
//       digitalWrite(LED_BUILTIN, HIGH);
//       delay(1000);
//       digitalWrite(LED_BUILTIN, LOW);
//       delay(1000);
//     }
//   }
//   Serial.println("Go to sleep now");
//   light_sleep();
//   delay(200);
//   Serial.println("Wake up");
//   WiFi.begin(ssid,passphrase);
//   while ((WiFi.status() != WL_CONNECTED))
//   {
//     Serial.printf(".");
//     delay(100);
//   }
//   Serial.println("Connected");
  
// }
// void light_sleep()
// {
//   wifi_station_disconnect();
//   wifi_set_opmode_current(NULL_MODE);
//   wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
//   wifi_fpm_open();
//   gpio_pin_wakeup_enable(GPIO_ID_PIN(4),GPIO_PIN_INTR_HILEVEL);
//   wifi_fpm_do_sleep(0xFFFFFFF);

// }
#include <Arduino.h>
hw_timer_t * timer = NULL;

void onTimer();
void endTimer();
void startTimer();

void onTimer(){
  static unsigned int counter = 1;
  
  Serial.print("onTimer ");
  Serial.print(counter);
  Serial.print(" at ");
  Serial.print(millis());
  Serial.println(" ms");

  if (counter == 10)
    endTimer();

  counter++;
}

void startTimer() {
  timer = timerBegin(0, 80, true); // timer_id = 0; divider=80; countUp = true;
  timerAttachInterrupt(timer, &onTimer, true); // edge = true
  timerAlarmWrite(timer, 1000000, true);  //1000 ms
  timerAlarmEnable(timer);
}

void endTimer() {
  timerEnd(timer);
  timer = NULL; 
}

void setup() {
  Serial.begin(115200);
  startTimer();
}

void loop() {}