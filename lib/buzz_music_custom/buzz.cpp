#include"buzz.h"
void correctBuzz()
{
    digitalWrite(BUZZ_PIN, 1);
    delay(500);
    digitalWrite(BUZZ_PIN, 0);
}
void wrongBuzz()
{
    digitalWrite(BUZZ_PIN, 1);
    delay(300);
    digitalWrite(BUZZ_PIN, 0);
    delay(60);
    digitalWrite(BUZZ_PIN, 1);
    delay(100);
    digitalWrite(BUZZ_PIN, 0);
    delay(60);
    digitalWrite(BUZZ_PIN, 1);
    delay(100);
    digitalWrite(BUZZ_PIN, 0);
    delay(60);
    digitalWrite(BUZZ_PIN, 1);
    delay(300);
    digitalWrite(BUZZ_PIN, 0);
}