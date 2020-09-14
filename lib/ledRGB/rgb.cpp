#include "rgb.h"
#include<Arduino.h>
void red()
{
    digitalWrite(red_pin, 0);
    digitalWrite(green_pin, 1);
    digitalWrite(blue_pin, 1);
    delay(1000);
    digitalWrite(red_pin, 1);
    digitalWrite(green_pin, 1);
    digitalWrite(blue_pin, 1);
}
void green()
{
    digitalWrite(red_pin, 1);
    digitalWrite(green_pin, 0);
    digitalWrite(blue_pin, 1);
    delay(1000);
    digitalWrite(red_pin, 1);
    digitalWrite(green_pin, 1);
    digitalWrite(blue_pin, 1);
}
void yellow()
{
    digitalWrite(red_pin, 0);
    digitalWrite(green_pin, 0);
    digitalWrite(blue_pin, 1);
    delay(1000);
    digitalWrite(red_pin, 1);
    digitalWrite(green_pin, 1);
    digitalWrite(blue_pin, 1);
}