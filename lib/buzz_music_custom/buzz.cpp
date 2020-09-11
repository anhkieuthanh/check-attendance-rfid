void correctBuzz()
{
    digitalWrite(buzz, 1);
    delay(500);
    digitalWrite(buzz, 0);
}
void wrongBuzz()
{
    digitalWrite(buzz, 1);
    delay(300);
    digitalWrite(buzz, 0);
    delay(60);
    digitalWrite(buzz, 1);
    delay(100);
    digitalWrite(buzz, 0);
    delay(60);
    digitalWrite(buzz, 1);
    delay(100);
    digitalWrite(buzz, 0);
    delay(60);
    digitalWrite(buzz, 1);
    delay(300);
    digitalWrite(buzz, 0);
}